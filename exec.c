#include "exec.h"

#include "bc.h"
#include "bfunc.h"
#include "fn.h"
#include "perr.h"
#include "refcount.h"
#include "rwhelp.h"
#include "stdafx.h"
#include "var.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct pair {
  size_t offset_index;
  u32    id;
};

#define BINOP(l, r, op)                                                                                                                                                       \
  (l.type == E_VARTYPE_FLOAT || r.type == E_VARTYPE_FLOAT) ? (e_var){ .type = E_VARTYPE_FLOAT, .val.f = (double)l.val.f op(double) r.val.f } : (e_var)                        \
  { .type = E_VARTYPE_INT, .val.i = l.val.i op r.val.i }
#define BOOLEAN_BINOP(l, r, op)                                                                                                                                               \
  (l.type == E_VARTYPE_FLOAT || r.type == E_VARTYPE_FLOAT) ? (e_var){ .type = E_VARTYPE_BOOL, .val.b = (double)l.val.f op(double) r.val.f } : (e_var)                         \
  { .type = E_VARTYPE_BOOL, .val.b = l.val.i op r.val.i }

static inline double
to_float(e_var v)
{
  switch (v.type) {
    case E_VARTYPE_FLOAT: return v.val.f;
    case E_VARTYPE_INT: return (double)v.val.i;
    case E_VARTYPE_CHAR: return (double)v.val.c;
    case E_VARTYPE_BOOL: return (double)v.val.b;
    default: return 0.0;
  }
}

static inline int
to_int(e_var v)
{
  switch (v.type) {
    case E_VARTYPE_INT: return v.val.i;
    case E_VARTYPE_FLOAT: return (int)v.val.f;
    case E_VARTYPE_CHAR: return (int)v.val.c;
    case E_VARTYPE_BOOL: return (int)v.val.b;
    default: return 0;
  }
}

static inline char
to_char(e_var v)
{
  switch (v.type) {
    case E_VARTYPE_INT: return (char)v.val.i;
    case E_VARTYPE_FLOAT: return (char)v.val.f;
    case E_VARTYPE_CHAR: return (char)v.val.c;
    case E_VARTYPE_BOOL: return (char)v.val.b;
    default: return 0;
  }
}

static inline bool
to_bool(e_var v)
{
  switch (v.type) {
    case E_VARTYPE_INT: return (bool)v.val.i;
    case E_VARTYPE_FLOAT: return (bool)v.val.f;
    case E_VARTYPE_CHAR: return (bool)v.val.c;
    case E_VARTYPE_BOOL: return (bool)v.val.b;
    default: return false;
  }
}

static inline bool
is_float(e_var v)
{ return v.type == E_VARTYPE_FLOAT; }

#define COERCE_BINOP(l, r, op)                                                                                                                                                \
  (is_float(l) || is_float(r)) ? (e_var){ .type = E_VARTYPE_FLOAT, .refc = e_refc_init(), .val.f = to_float(l) op to_float(r) } : (e_var)                                     \
  { .type = E_VARTYPE_INT, .refc = e_refc_init(), .val.i = to_int(l) op to_int(r) }

#define COERCE_BOOLEAN_BINOP(l, r, op)                                                                                                                                        \
  (is_float(l) || is_float(r)) ? (e_var){ .type = E_VARTYPE_BOOL, .refc = e_refc_init(), .val.b = to_float(l) op to_float(r) } : (e_var)                                      \
  { .type = E_VARTYPE_BOOL, .refc = e_refc_init(), .val.b = to_int(l) op to_int(r) }

static inline e_var
operate(e_var l, e_var r, e_opcode op)
{
  if (op == E_OPCODE_NOT) return (e_var){ .type = E_VARTYPE_BOOL, .refc = e_refc_init(), .val.b = (bool)!to_int(r) };

  switch (op) {
    case E_OPCODE_ADD: return COERCE_BINOP(l, r, +);
    case E_OPCODE_SUB: return COERCE_BINOP(l, r, -);
    case E_OPCODE_MUL: return COERCE_BINOP(l, r, *);
    case E_OPCODE_DIV: return COERCE_BINOP(l, r, /);
    case E_OPCODE_MOD: return (e_var){ .type = E_VARTYPE_INT, .refc = e_refc_init(), .val.i = to_int(l) % to_int(r) };
    case E_OPCODE_EQL: return COERCE_BOOLEAN_BINOP(l, r, ==);
    case E_OPCODE_NEQ: return COERCE_BOOLEAN_BINOP(l, r, !=);
    case E_OPCODE_LT: return COERCE_BOOLEAN_BINOP(l, r, <);
    case E_OPCODE_LTE: return COERCE_BOOLEAN_BINOP(l, r, <=);
    case E_OPCODE_GT: return COERCE_BOOLEAN_BINOP(l, r, >);
    case E_OPCODE_GTE: return COERCE_BOOLEAN_BINOP(l, r, >=);
    case E_OPCODE_AND: return COERCE_BOOLEAN_BINOP(l, r, &&);
    case E_OPCODE_OR: return COERCE_BOOLEAN_BINOP(l, r, ||);
    case E_OPCODE_NEG:
      switch (r.type) {
        case E_VARTYPE_INT: return (e_var){ .type = E_VARTYPE_INT, .refc = e_refc_init(), .val.i = -r.val.i };
        case E_VARTYPE_CHAR: return (e_var){ .type = E_VARTYPE_CHAR, .refc = e_refc_init(), .val.c = (char)-(int)r.val.c };
        case E_VARTYPE_FLOAT: return (e_var){ .type = E_VARTYPE_FLOAT, .refc = e_refc_init(), .val.f = -r.val.f };
        default: break;
      }
      if (r.type == E_VARTYPE_INT) return (e_var){ .type = E_VARTYPE_INT, .refc = e_refc_init(), .val.i = -r.val.i };
    case E_OPCODE_BNOT: return (e_var){ .type = E_VARTYPE_INT, .refc = e_refc_init(), .val.i = ~to_int(r) };
    case E_OPCODE_BAND: return (e_var){ .type = E_VARTYPE_INT, .refc = e_refc_init(), .val.i = to_int(l) & to_int(r) };
    case E_OPCODE_BOR: return (e_var){ .type = E_VARTYPE_INT, .refc = e_refc_init(), .val.i = to_int(l) | to_int(r) };
    case E_OPCODE_XOR: return (e_var){ .type = E_VARTYPE_INT, .refc = e_refc_init(), .val.i = to_int(l) ^ to_int(r) };
    default: return (e_var){ 0 };
  }
}

typedef struct stack_frame {
  size_t variable_offset;
  size_t stack_size;
} stack_frame;

struct stack {
  stack_frame frames[64];
  size_t      depth;
  size_t      size;
  size_t      capacity;
  e_var*      stack;
};

static inline int
stack_init(size_t init_capacity, struct stack* st)
{
  st->size     = 0;
  st->depth    = 0;
  st->capacity = init_capacity;
  st->stack    = malloc(sizeof(e_var) * init_capacity);
  if (st->stack == nullptr) return -1;

  return 0;
}

static inline void
stack_free(struct stack* st)
{ free(st->stack); }

static inline void
stack_free_variables(struct stack* st)
{
  for (size_t i = 0; i < st->size; i++) { e_var_release(&st->stack[i]); }
}

static inline void
stack_push(struct stack* st, e_var v)
{
  if (st->size >= st->capacity) {
    size_t new_cap   = MAX(st->capacity * 2, 1);
    e_var* new_stack = (e_var*)realloc(st->stack, sizeof(e_var) * new_cap);

    if (!new_stack) return;

    st->stack    = new_stack;
    st->capacity = new_cap;
  }

  st->stack[st->size] = v;
  st->size++;
}

static inline void
stack_pop(struct stack* st)
{
  e_var_release(&st->stack[st->size - 1]);
  st->size--;
}

static inline e_var*
stack_top(struct stack* st)
{ return &st->stack[st->size - 1]; }

// static inline e_var*
// stack_find_rev(const struct stack* s, u16 id)
// {
//   for (int i = 0; i < s->size; i++)
//   {
//     // if (id == s->stack[i].) { stack_push(&stack, &stack.stack[variables[i].offset]); }
//   }
// }

static inline int
len(e_var* var)
{
  if (var->type == E_VARTYPE_LIST) {
    return (int)var->val.list->size;
  } else if (var->type == E_VARTYPE_MAP) {
    return (int)var->val.map->size;
  }
  return E_ENONEXISTENT;
}

static inline bool
is_builtin_func(const char* name)
{
  for (size_t i = 0; i < E_ARRLEN(eb_funcs); i++) {
    if (strcmp(eb_funcs[i].name, name) == 0) return true;
  }
  return false;
}

static inline const e_builtin_func*
get_builtin_func_hashed(u32 hash)
{
  for (size_t i = 0; i < E_ARRLEN(eb_funcs); i++) {
    if (e_hash_fnv(eb_funcs[i].name, strlen(eb_funcs[i].name)) == hash) return &eb_funcs[i];
  }
  return nullptr;
}

static e_var
call(const e_exec_info* info, struct stack* stack, u32 hash, u32 nargs)
{
  e_var return_value = { .type = E_VARTYPE_VOID };

  // builtins
  const e_builtin_func* builtin = get_builtin_func_hashed(hash);
  if (builtin != nullptr) {
    e_var* args  = &stack->stack[stack->size - nargs];
    return_value = builtin->func(args, nargs);
    goto pop_and_ret;
  }

  // user defined
  for (u32 f = 0; f < info->nfuncs; f++) {
    if (info->funcs[f].name_hash != hash) continue;
    e_exec_info fi = {
      .code          = info->funcs[f].code,
      .args          = &stack->stack[stack->size - info->funcs[f].nargs],
      .slots         = info->funcs[f].arg_slots,
      .literals      = info->literals,
      .funcs         = info->funcs,
      .extern_funcs  = info->extern_funcs,
      .code_size     = info->funcs[f].code_size,
      .nargs         = info->funcs[f].nargs,
      .nliterals     = info->nliterals,
      .nfuncs        = info->nfuncs,
      .nextern_funcs = info->nextern_funcs,
    };
    return_value = e_exec(&fi);
    goto pop_and_ret;
  }

  // extern
  for (u32 i = 0; i < info->nextern_funcs; i++) {
    if (info->extern_funcs[i].hash != hash) continue;
    if (info->extern_funcs[i].func) return_value = info->extern_funcs[i].func(&stack->stack[stack->size - nargs]);
    goto pop_and_ret;
  }

  fprintf(stderr, "Function %u not defined\n", hash);
  exit(-1);

pop_and_ret:
  for (size_t i = 0; i < nargs; i++) stack_pop(stack);
  return return_value;
}

u32
get_variable_slot(struct pair* variables, u32 nvariables, u32 hash)
{
  for (u32 i = 0; i < nvariables; i++) {
    if (variables[i].id == hash) { return i; }
  }

  return -1;
}

static inline void
assign(u32 slot, struct stack* stack, struct pair* variables, e_var value)
{
  e_var* variable_slot = &stack->stack[variables[slot].offset_index];
  e_var_release(variable_slot); // free old value in slot

  *variable_slot = value; // move ownership from stack top to slot
}

e_var
e_exec(const e_exec_info* info)
{
  struct stack stack;

  int e = stack_init(16, &stack);
  if (e) return (e_var){ .type = E_VARTYPE_INT, .refc = e_refc_init(), .val.i = -1 };

  size_t       cvariables = 4;
  size_t       nvariables = 0;
  struct pair* variables  = (struct pair*)calloc(cvariables, sizeof(struct pair));

  for (size_t i = 0; i < info->nargs; i++) {
    if (nvariables >= cvariables) {
      size_t       new_c         = cvariables * 2;
      struct pair* new_variables = (struct pair*)realloc(variables, new_c * sizeof(struct pair));
      if (!new_variables) exit(-1);

      // memset new region to 0
      memset(new_variables + nvariables, 0, (new_c - nvariables) * sizeof(struct pair));

      variables  = new_variables;
      cvariables = new_c;
    }

    variables[nvariables].id           = info->slots[i];
    variables[nvariables].offset_index = i;
    nvariables++;

    e_var v;
    e_var_shallow_cpy(&info->args[i], &v);

    // make new refc
    v.refc = e_refc_init();

    stack_push(&stack, v);
  }

  e_ecode retcode = E_OK;

  const u8* ip  = info->code;
  const u8* end = info->code + info->code_size;

  while (ip < end) {
    e_opcode opcode = *(e_opcode*)ip;
    ip += sizeof(e_opcode);

    e_attr attrs = *(e_attr*)ip;
    ip += sizeof(e_attr);

    switch ((e_opcode_bck)opcode) {
      /* NOOPs */
      case E_OPCODE_LABEL: ip += 4; break; // move over Label ID
      case E_OPCODE_NOOP: break;

      case E_OPCODE_CALL: {
        u16 func_nargs = e_read_u16(&ip);
        u32 hash       = e_read_u32(&ip);

        e_var r = call(info, &stack, hash, func_nargs);

        /* Push the return value only after popping the arguments. */
        if (r.type != E_VARTYPE_VOID) { stack_push(&stack, r); }
        break;
      }

      case E_OPCODE_LITERAL: {
        u16 id = e_read_u16(&ip);
        assert(id < info->nliterals);

        e_var v;
        e_var_deep_cpy(&info->literals[id], &v); // Deep copy the literal.

        stack_push(&stack, v);

        break;
      }

      case E_OPCODE_MK_LIST: {
        u32 nelems = e_read_u32(&ip);

        e_var new_list = {
          .type     = E_VARTYPE_LIST,
          .refc     = e_refc_init(),
          .val.list = calloc(1, sizeof(e_list)),
        };

        e_var* elems = &stack.stack[stack.size - nelems];
        e_list_init(elems, nelems, new_list.val.list); // acquires the elements.

        // Release variables from the stack.
        for (u32 i = 0; i < nelems; i++) { e_var_release(&stack.stack[stack.size - nelems + i]); }
        stack.size -= nelems;

        stack_push(&stack, new_list);

        break;
      }

      case E_OPCODE_ADD:
      case E_OPCODE_SUB:
      case E_OPCODE_MUL:
      case E_OPCODE_DIV:
      case E_OPCODE_MOD:
      case E_OPCODE_EXP:
      case E_OPCODE_AND:
      case E_OPCODE_OR:
      case E_OPCODE_BAND:
      case E_OPCODE_BOR:
      case E_OPCODE_XOR:
      case E_OPCODE_EQL:
      case E_OPCODE_NEQ:
      case E_OPCODE_LT:
      case E_OPCODE_LTE:
      case E_OPCODE_GT:
      case E_OPCODE_GTE:
      case E_OPCODE_NEG: {
        // Since we compile left first, right next
        // right will be at the top of the stack and left will be below it
        e_var r = operate(stack.stack[stack.size - 2], stack.stack[stack.size - 1], opcode);

        stack_pop(&stack); // remove L & R
        stack_pop(&stack);

        /* No need to acquire r. */
        stack_push(&stack, r);
        break;
      }

      case E_OPCODE_INC:
      case E_OPCODE_DEC: {
        u32    id = (u32)-1;
        e_var* v  = stack_top(&stack);

        if (attrs & E_ATTR_COMPOUND) { id = e_read_u32(&ip); }

        if (v->type == E_VARTYPE_INT) {
          v->val.i += (opcode == E_OPCODE_INC) ? 1 : -1;
        } else {
          v->val.f += (opcode == E_OPCODE_INC) ? 1.F : -1.F;
        }

        if (attrs & E_ATTR_COMPOUND) {
          assign(get_variable_slot(variables, nvariables, id), &stack, variables, *v);
          stack.size--; // Variable slot owns v now!
        }

        break;
      }

      case E_OPCODE_NOT:
      case E_OPCODE_BNOT: {
        // Provide an empty variable for the LHS
        e_var r = operate((e_var){ 0 }, *stack_top(&stack), opcode);

        stack_pop(&stack); // remove R

        stack_push(&stack, r);

        break;
      }

      case E_OPCODE_JZ: {
        u32 target = e_read_u32(&ip); // always read the operand
        if (!stack_top(&stack)->val.i) ip = info->code + target;

        stack_pop(&stack); // remove condition
        break;
      }

      case E_OPCODE_JNZ: {
        u32 target = e_read_u32(&ip); // always read the operand
        if (stack_top(&stack)->val.i) ip = info->code + target;

        stack_pop(&stack); // remove condition
        break;
      }

      case E_OPCODE_JMP:
        ip = info->code + e_read_u32(&ip);
        break;

        // case E_OPCODE_JE: ip = inss + e_read_u32(&ip); break;
        // case E_OPCODE_JNE: ip = inss + e_read_u32(&ip); break;

      case E_OPCODE_INIT: {
        if (nvariables >= cvariables) {
          size_t       new_c         = cvariables * 2;
          struct pair* new_variables = (struct pair*)realloc(variables, new_c * sizeof(struct pair));
          if (!new_variables) exit(-1);

          // memset new region to 0
          memset(new_variables + nvariables, 0, (new_c - nvariables) * sizeof(struct pair));

          variables  = new_variables;
          cvariables = new_c;
        }

        u32 id = e_read_u32(&ip);

        variables[nvariables].offset_index = stack.size;
        variables[nvariables].id           = id;
        nvariables++;
        if (attrs & E_ATTR_COMPOUND) {
          e_var v = stack.stack[stack.size - 1];
          e_var_shallow_cpy(&stack.stack[stack.size - 1], &v);
          e_var_acquire(&v);
          stack_push(&stack, v);
        } else {
          stack_push(&stack, (e_var){ .refc = e_refc_init() });
        }
        break;
      }

      case E_OPCODE_LOAD: {
        u32 id = e_read_u32(&ip);
        for (size_t i = 0; i < nvariables; i++) {
          size_t idx = nvariables - i - 1;
          if (id == variables[idx].id) {
            e_var v;
            e_var_shallow_cpy(&stack.stack[variables[idx].offset_index], &v);
            e_var_acquire(&v);
            stack_push(&stack, v);
            break;
          }
        }
        break;
      }

      case E_OPCODE_INDEX: {
        int     idx  = to_int(stack.stack[stack.size - 1]);
        e_list* list = stack.stack[stack.size - 2].type == E_VARTYPE_LIST ? stack.stack[stack.size - 2].val.list : NULL;

        stack_pop(&stack); // pop index
        stack_pop(&stack); // pop base

        if (list && idx >= 0 && (u64)idx < list->size) {
          e_var_acquire(&list->vars[idx]);
          stack_push(&stack, list->vars[idx]);
        }
        break;
      }

      case E_OPCODE_INDEX_ASSIGN: {
        u32     idx  = stack.stack[stack.size - 2].val.i;
        e_list* list = stack.stack[stack.size - 3].type == E_VARTYPE_LIST ? stack.stack[stack.size - 3].val.list : NULL;

        // Copy value and acquire it temporarily
        e_var value = stack.stack[stack.size - 1];
        e_var_acquire(&value);

        stack_pop(&stack); // pop value
        stack_pop(&stack); // pop index
        stack_pop(&stack); // pop base

        if (!list || idx >= list->size) {
          e_var_release(&value);
          fprintf(stderr, "List has size %zu, but index is %zu\n", list->size, (size_t)idx);
          retcode = E_EOUTOFRANGE;
          goto _RETURN;
        }

        e_var_release(&list->vars[idx]);
        e_var_shallow_cpy(&value, &list->vars[idx]);
        e_var_acquire(&list->vars[idx]);
        e_var_release(&value); // release our temporary hold
        break;
      }

      case E_OPCODE_ASSIGN: {
        u32 id = e_read_u32(&ip);
        for (u32 i = 0; i < nvariables; i++) {
          if (id == variables[i].id) {
            e_var* slot = &stack.stack[variables[i].offset_index];
            e_var_release(slot); // free old value in slot

            /**
             * Copy it. We need the variable on the stack to support.
             * chained assignments
             * let x = 16;
             * let y = 32;
             * let z = 64;
             * x = y = z = 68;
             *             ^ value needs to be on the stack!
             */
            e_var_shallow_cpy(stack_top(&stack), slot);

            e_var_acquire(slot);

            break;
          }
        }
        break;
      }

      case E_OPCODE_RETURN: {
        u8 has_return_value = e_read_u8(&ip);

        if (has_return_value) {
          e_var r;
          e_var_deep_cpy(&stack.stack[stack.size - 1], &r);
          // now safe to release the stack slot
          // YASS NO LEAKS!!!!
          for (size_t i = 0; i < stack.size; i++) { e_var_release(&stack.stack[i]); }

          stack_free(&stack);
          free(variables);

          return r;
        } else {
          goto _RETURN;
        }
        break;
      }

      case E_OPCODE_PUSH_VARIABLES:
        stack.frames[stack.depth++] = (stack_frame){
          .stack_size      = stack.size,
          .variable_offset = nvariables,
        };

        if (stack.depth >= (sizeof(stack.frames) / sizeof(stack.frames[0]))) {
          _UNREACHABLE();
          fprintf(stderr, "Stack frame depth exceeded %zu\n", sizeof(stack.frames) / sizeof(stack.frames[0]));
          retcode = E_EOUTOFRANGE;
          goto _RETURN;
        }
        break;

      case E_OPCODE_POP_VARIABLES: {
        const stack_frame* frame = &stack.frames[--stack.depth];
        for (size_t i = frame->stack_size; i < stack.size; i++) e_var_release(&stack.stack[i]);
        stack.size = frame->stack_size;
        nvariables = frame->variable_offset;
        break;
      }

      // case E_OPCODE_LOAD_REFERENCE:
      default: printf("Unknown instruction\n"); exit(-1);

      // Non fatal return
      case E_OPCODE_HALT: retcode = E_OK; goto _RETURN;

      case E_OPCODE_COUNT: printf("Illegal instruction"); exit(-1);
    }
  }

_RETURN:
  // Don't free the arguments!
  for (size_t i = 0; i < stack.size; i++) { e_var_release(&stack.stack[i]); }
  free(variables);
  stack_free(&stack);
  if (retcode == E_OK) {
    return (e_var){ .type = E_VARTYPE_VOID };
  } else {
    return (e_var){
      .type  = E_VARTYPE_INT,
      .refc  = e_refc_init(),
      .val.i = retcode,
    };
  }
}
