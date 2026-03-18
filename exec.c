#include "exec.h"

#include "bc.h"
#include "fn.h"
#include "rwhelp.h"
#include "var.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BINOP(l, r, op)                                                                                                                                                       \
  (l.type == E_VARTYPE_FLOAT || r.type == E_VARTYPE_FLOAT) ? (e_var){ .type = E_VARTYPE_FLOAT, .val.f = (double)l.val.f op(double) r.val.f } : (e_var)                        \
  {                                                                                                                                                                           \
    .type = E_VARTYPE_INT, .val.i = l.val.i op r.val.i                                                                                                                        \
  }
#define BOOLEAN_BINOP(l, r, op)                                                                                                                                               \
  (l.type == E_VARTYPE_FLOAT || r.type == E_VARTYPE_FLOAT) ? (e_var){ .type = E_VARTYPE_BOOL, .val.b = (double)l.val.f op(double) r.val.f } : (e_var)                         \
  {                                                                                                                                                                           \
    .type = E_VARTYPE_BOOL, .val.b = l.val.i op r.val.i                                                                                                                       \
  }

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

static inline bool
is_float(e_var v)
{
  return v.type == E_VARTYPE_FLOAT;
}

#define COERCE_BINOP(l, r, op)                                                                                                                                                \
  (is_float(l) || is_float(r)) ? (e_var){ .type = E_VARTYPE_FLOAT, .refc = e_refc_init(), .val.f = to_float(l) op to_float(r) } : (e_var)                                     \
  {                                                                                                                                                                           \
    .type = E_VARTYPE_INT, .refc = e_refc_init(), .val.i = to_int(l) op to_int(r)                                                                                             \
  }

#define COERCE_BOOLEAN_BINOP(l, r, op)                                                                                                                                        \
  (is_float(l) || is_float(r)) ? (e_var){ .type = E_VARTYPE_BOOL, .refc = e_refc_init(), .val.b = to_float(l) op to_float(r) } : (e_var)                                      \
  {                                                                                                                                                                           \
    .type = E_VARTYPE_BOOL, .refc = e_refc_init(), .val.b = to_int(l) op to_int(r)                                                                                            \
  }

static inline e_var
operate(e_var l, e_var r, e_opcode op)
{
  if (op == E_OPCODE_NOT) return (e_var){ .type = E_VARTYPE_BOOL, .refc = e_refc_init(), .val.b = !to_int(r) };

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
    case E_OPCODE_BNOT: return (e_var){ .type = E_VARTYPE_INT, .refc = e_refc_init(), .val.i = ~to_int(r) };
    case E_OPCODE_BAND: return (e_var){ .type = E_VARTYPE_INT, .refc = e_refc_init(), .val.i = to_int(l) & to_int(r) };
    case E_OPCODE_BOR: return (e_var){ .type = E_VARTYPE_INT, .refc = e_refc_init(), .val.i = to_int(l) | to_int(r) };
    case E_OPCODE_XOR: return (e_var){ .type = E_VARTYPE_INT, .refc = e_refc_init(), .val.i = to_int(l) ^ to_int(r) };
    default: return (e_var){ 0 };
  }
}
struct stack {
  size_t size;
  size_t capacity;
  e_var* stack;
};

static inline int
stack_init(struct stack* st)
{
  const size_t stack_capacity = 256;

  st->size     = 0;
  st->capacity = stack_capacity;
  st->stack    = malloc(sizeof(e_var) * stack_capacity);
  if (st->stack == nullptr) return -1;

  return 0;
}

static inline void
stack_free(struct stack* st)
{
  free(st->stack);
}

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
{
  return &st->stack[st->size - 1];
}

// static inline e_var*
// stack_find_rev(const struct stack* s, u16 id)
// {
//   for (int i = 0; i < s->size; i++)
//   {
//     // if (id == s->stack[i].) { stack_push(&stack, &stack.stack[variables[i].offset]); }
//   }
// }

static e_var
call(const e_exec_info* info, struct stack* stack, u32 hash, u32 nargs)
{
  // printf("DEBUG: entering call\n");

  const u32 println = (u32)e_hash_fnv("println", strlen("println"));
  const u32 print   = (u32)e_hash_fnv("print", strlen("print"));

  e_var return_value = { .type = E_VARTYPE_VOID };
  bool  found        = false;

  if (hash == println) {
    // printf("%s\n", stack[stack.size - j - 1].val.s->s);

    // Pushed in correct order, must be popped in reverse.
    for (int64_t j = nargs - 1; j >= 0; j--) { e_var_print(&stack->stack[stack->size - j - 1], stdout); }
    fputc('\n', stdout);

    found = true;
  } else if (hash == print) {
    for (int64_t j = nargs - 1; j >= 0; j--) { e_var_print(&stack->stack[stack->size - j - 1], stdout); }
    found = true;
  } else {
    for (int f = 0; f < info->nfuncs; f++) {
      const e_function* func = &info->funcs[f];
      if (info->funcs[f].hash == hash) {
        e_var* args_ptr = &stack->stack[stack->size - info->funcs[f].nargs];

        e_exec_info exec_func = {
          .code      = func->code,
          .args      = args_ptr,
          .slots     = func->arg_slots,
          .literals  = info->literals,
          .funcs     = info->funcs,
          .code_size = func->code_size,
          .nargs     = func->nargs,
          .nliterals = info->nliterals,
          .nfuncs    = info->nfuncs,
        };

        return_value = e_exec(&exec_func);

        found = true;

        // printf("Got return value: ");
        // e_var_print(&v, stderr);
        // fputc('\n', stderr);
        break;
      }
    }
  }

  if (!found) {
#if defined(__has_builtin) && __has_builtin(__builtin_unreachable)
    __builtin_unreachable();
#endif
    fprintf(stderr, "Function %u not defined\n", hash);
    exit(-1);
  }

  // Pop all arguments off
  for (size_t i = 0; i < nargs; i++) { stack_pop(stack); }

  return return_value;
}

e_var
e_exec(const e_exec_info* info)
{
  struct stack stack;

  int e = stack_init(&stack);
  if (e) return (e_var){ .type = E_VARTYPE_INT, .refc = e_refc_init(), .val.i = -1 };

  struct pair {
    size_t offset_index;
    u32    id;
  };

  size_t       cvariables = 64;
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

  const u8* ip  = info->code;
  const u8* end = info->code + info->code_size;

  size_t save_sp = 0;

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
        u32 hash       = e_read_u32(&ip);
        u16 func_nargs = e_read_u16(&ip);

        e_var r = call(info, &stack, hash, func_nargs);
        /* Push the return value only after popping the arguments. */
        if (r.type != E_VARTYPE_VOID) stack_push(&stack, r);
        break;
      }

      case E_OPCODE_LITERAL: {
        u16 id = e_read_u16(&ip);
        assert(id < info->nliterals);

        e_var v;
        e_var_deep_cpy(&info->literals[id], &v);

        stack_push(&stack, v);

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
      case E_OPCODE_GTE: {
        // Since we compile left first, right next
        // right will be at the top of the stack and left will be below it
        e_var r = operate(stack.stack[stack.size - 2], stack.stack[stack.size - 1], opcode);
        // if (attrs & E_ATTR_CLEAN)
        // {
        stack_pop(&stack);
        stack_pop(&stack);
        // } // remove L & R

        /* Dont acquire the r. it's new. */
        stack_push(&stack, r);
        break;
      }

      case E_OPCODE_INC:
      case E_OPCODE_DEC: {
        e_var* v  = NULL;
        u32    id = e_read_u32(&ip);
        for (size_t i = 0; i < nvariables; i++) {
          size_t idx = nvariables - i - 1;
          if (variables[idx].id == id) {
            v = &stack.stack[variables[idx].offset_index];
            break;
          }
        }

        if (!v) { exit(-1); }

        if (v->type == E_VARTYPE_INT) {
          if (opcode == E_OPCODE_INC) {
            v->val.i++;
          } else {
            v->val.i--;
          }
        } else {
          if (opcode == E_OPCODE_INC) {
            v->val.f++;
          } else {
            v->val.f--;
          }
        }

        break;
      }

      case E_OPCODE_NOT:
      case E_OPCODE_BNOT: {
        // Provide an empty variable for the LHS
        e_var r = operate((e_var){ 0 }, stack.stack[stack.size - 1], opcode);
        // if (attrs & E_ATTR_CLEAN) stack.size--; // remove R
        stack_pop(&stack);

        stack_push(&stack, r);

        break;
      }

      case E_OPCODE_JZ: {
        u32 target = e_read_u32(&ip); // always read the operand
        if (!stack.stack[stack.size - 1].val.i) ip = info->code + target;

        if (attrs & E_ATTR_CLEAN) stack_pop(&stack);
        break;
      }

      case E_OPCODE_JNZ: {
        u32 target = e_read_u32(&ip); // always read the operand
        if (stack.stack[stack.size - 1].val.i) ip = info->code + target;

        if (attrs & E_ATTR_CLEAN) stack_pop(&stack);
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

      case E_OPCODE_ASSIGN: {
        u32 id = e_read_u32(&ip);
        for (int i = 0; i < nvariables; i++) {
          if (id == variables[i].id) {
            e_var* slot = &stack.stack[variables[i].offset_index];
            e_var_release(slot); // free old value in slot

            *slot = *stack_top(&stack); // move ownership from stack top to slot

            if (attrs & E_ATTR_CLEAN) {
              stack.size--; // just drop the stack entry without freeing,
                            // ownership is now slot
            } else {
              e_var_acquire(slot);
            }
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
          // Don't free the arguments!
          for (size_t i = 0; i < stack.size; i++) { e_var_release(&stack.stack[i]); }
          stack_free(&stack);
          free(variables);
          return (e_var){ .type = E_VARTYPE_VOID };
        }
        break;
      }

      case E_OPCODE_PUSH_VARIABLES: save_sp = stack.size; break;

      case E_OPCODE_POP_VARIABLES:
        for (size_t i = save_sp; i < stack.size; i++) e_var_release(&stack.stack[i]); // Release the popped variables
        stack.size = save_sp;
        break;

      // case E_OPCODE_LOAD_REFERENCE:
      default: printf("Unknown instruction\n"); exit(-1);

      // Non fatal return
      case E_OPCODE_HALT:
        // Don't free the arguments!
        for (size_t i = 0; i < stack.size; i++) { e_var_release(&stack.stack[i]); }
        stack_free(&stack);
        free(variables);
        return (e_var){ .type = E_VARTYPE_INT, .refc = e_refc_init(), .val.i = (int)e_read_u32(&ip) };

      case E_OPCODE_COUNT: printf("Illegal instruction"); exit(-1);
    }
  }

  // Don't free the arguments!
  for (size_t i = 0; i < stack.size; i++) { e_var_release(&stack.stack[i]); }
  free(variables);
  stack_free(&stack);
  return (e_var){ .type = E_VARTYPE_VOID, .refc = e_refc_init() };
}
