/**
 * MIT License
 *
 * Copyright (c) 2026 Tellcode1
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "exec.h"

#include "bc.h"
#include "bfunc.h"
#include "fn.h"
#include "list.h"
#include "map.h"
#include "operate.h"
#include "perr.h"
#include "pool.h"
#include "rwhelp.h"
#include "script.h"
#include "stack.h"
#include "stdafx.h"
#include "string.h"
#include "var.h"

#include <assert.h>
#include <float.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PASTE(x, y) x##y

// clang-format off
#define TRY(expr) do { int PASTE(__e__, __LINE__) = 0; if ((PASTE(__e__, __LINE__) = (expr))) { return PASTE(__e__, __LINE__); } } while (0)
#define TRY_V(expr) do {   int PASTE(__e__, __LINE__) = (expr);   if (PASTE(__e__, __LINE__)) { return (e_var){ .type = E_VARTYPE_ERROR, .val.errcode = PASTE(__e__, __LINE__) }; } } while (0)
// clang-format on

// static inline e_var*
// stack_find_rev(const struct stack* s, u16 id)
// {
//   for (int i = 0; i < s->size; i++)
//   {
//     // if (id == s->stack[i].) {TRY_V( e_stack_push(info->stack, info->stack.stack[variables[i].offset]); )}
//   }
// }

static inline const e_builtin_func*
get_builtin_func_hashed(u32 hash)
{
  for (u32 i = 0; i < E_ARRLEN(eb_funcs); i++) {
    if (e_hash_fnv(eb_funcs[i].name, strlen(eb_funcs[i].name)) == hash) return &eb_funcs[i];
  }
  return nullptr;
}

static e_var
call(const e_exec_info* info, u32 hash, u32 nargs)
{
  e_var return_value = { .type = E_VARTYPE_NULL };

  // builtins
  const e_builtin_func* builtin = get_builtin_func_hashed(hash);
  if (builtin != nullptr) {
    e_var* args  = &info->stack->stack[info->stack->size - nargs];
    return_value = builtin->func(args, nargs);
    goto pop_and_ret;
  }

  // extern
  for (u32 i = 0; i < info->nextern_funcs; i++) {
    const char* name = info->extern_funcs[i].name;
    if (e_hash_fnv(name, strlen(name)) != hash) continue;
    if (info->extern_funcs[i].func) { return_value = info->extern_funcs[i].func(&info->stack->stack[info->stack->size - nargs], nargs); }
    goto pop_and_ret;
  }

  // user defined
  for (u32 f = 0; f < info->nfuncs; f++) {
    if (info->funcs[f].name_hash != hash) continue;

    /**
     * The depth that we'll need to
     * restore to when the function returns.
     */
    u32 depth_restore = info->stack->depth;
    TRY_V(e_stack_push_frame(info->stack));

    e_exec_info fi = {
      .code          = info->funcs[f].code,
      .args          = &info->stack->stack[info->stack->size - info->funcs[f].nargs],
      .slots         = info->funcs[f].arg_slots,
      .literals      = info->literals,
      .funcs         = info->funcs,
      .extern_funcs  = info->extern_funcs,
      .code_size     = info->funcs[f].code_size,
      .nargs         = info->funcs[f].nargs,
      .nliterals     = info->nliterals,
      .nfuncs        = info->nfuncs,
      .nextern_funcs = info->nextern_funcs,
      .extern_vars   = info->extern_vars,
      .nextern_vars  = info->nextern_vars,
      .stack         = info->stack,
    };
    return_value = e_exec(&fi);

    /**
     * Restore to the depth that we
     * were in
     */
    while (info->stack->depth > depth_restore) e_stack_pop_frame(info->stack);

    goto pop_and_ret;
  }

  fprintf(stderr, "Function %u not defined\n", hash);

  e_var null_var = { .type = E_VARTYPE_NULL };
  return null_var;

pop_and_ret:
  // #ifdef DEBUG_PRINT_STACK
  // printf("Function %u returned: ", hash);
  // eb_println(&return_value, 1);
  // #endif

  for (u32 i = 0; i < nargs; i++) e_stack_pop(info->stack);
  return return_value;
}

static inline u32
get_variable_slot(e_var_entry* variables, u32 nvariables, u32 hash)
{
  for (u32 i = 0; i < nvariables; i++) {
    if (variables[i].id == hash) { return i; }
  }

  return -1;
}

static inline e_var*
get_variable_from_id(e_stack* stack, u32 hash)
{
  for (u32 i = 0; i < stack->nvariables; i++) {
    u32 idx = stack->nvariables - i - 1;
    if (hash == stack->variables[idx].id) return &stack->stack[stack->variables[idx].offset_index];
  }
  return NULL;
}

static inline void
assign(const e_exec_info* info, u32 slot, e_var value)
{
  e_var* variable_slot = &info->stack->stack[info->stack->variables[slot].offset_index];
  e_var_release(variable_slot); // free old value in slot

  *variable_slot = value; // move ownership from stack top to slot
}

e_var
e_exec(const e_exec_info* info)
{
  /* Initial state check. */
  const e_var nullvar = { .type = E_VARTYPE_NULL };
  if (!info->stack) { return nullvar; }
  if (info->nargs > 0 && (info->nargs == nullptr || info->slots == nullptr)) { return nullvar; }
  if (info->code == nullptr && info->code_size != 0) { return nullvar; }
  if (info->extern_funcs == nullptr && info->nextern_funcs > 0) { return nullvar; }
  if (info->extern_vars == nullptr && info->nextern_vars > 0) { return nullvar; }
  if (info->funcs == nullptr && info->nfuncs > 0) { return nullvar; }
  if (info->literals == nullptr && info->nliterals > 0) { return nullvar; }

  for (u32 i = 0; i < info->nargs; i++) {
    e_var v = { 0 };

    int e = e_var_deep_cpy(&info->args[i], &v);
    if (e) return nullvar;

    e_var* slot = e_stack_push_variable(info->slots[i], info->stack);
    if (!slot) return (e_var){ .type = E_VARTYPE_ERROR, .val.errcode = E_EUNKNOWN };
    e_var_release(slot);

    *slot = v;
  }

  e_var retval = { .type = E_VARTYPE_NULL };

  const u8* ip  = info->code;
  const u8* end = info->code + info->code_size;

  while (true) {
    if (ip >= end) { goto _RETURN; }

    e_opcode_bck opcode = 0;
    memcpy(&opcode, ip, sizeof(e_opcode)); // faster for some reason?

    ip += sizeof(e_opcode);

    // fputs("STACK[ ", stdout);
    // for (u32 i = 0; i < info->stack->size; i++) {
    //   eb_print(&info->stack->stack[i], 1);
    //   fputs(", ", stdout);
    // }
    // fputs(" ]\n\n", stdout);

    switch (opcode) {
      /* NOOPs */
      case E_OPCODE_LABEL: ip += 4; break; // move over Label ID
      case E_OPCODE_COUNT:
      case E_OPCODE_NOOP: break;

      case E_OPCODE_POP: e_stack_pop(info->stack); break;
      case E_OPCODE_DUP: {
        e_var v;
        // e_var_acquire(e_stack_top(info->stack)); // Don't acquire! Stack_push already does that
        TRY_V(e_var_shallow_cpy(e_stack_top(info->stack), &v));
        e_stack_push(info->stack, &v);
        break;
      }

      case E_OPCODE_CALL: {
        u32 hash       = e_read_u32(&ip);
        u16 func_nargs = e_read_u16(&ip);

        e_var r = call(info, hash, func_nargs);
        if (r.type == E_VARTYPE_ERROR) { return r; }

        TRY_V(e_stack_push(info->stack, &r));
        e_var_release(&r);
        break;
      }

      case E_OPCODE_LOAD_ARG_LIST: {
        e_var args = eb_get_command_line_args(NULL, 0);
        e_stack_push(info->stack, &args);
        e_var_release(&args); // release our copy
        break;
      }

      case E_OPCODE_LITERAL: {
        u16 id = e_read_u16(&ip);
        assert(id < info->nliterals);

        e_var v;

        int e = e_var_deep_cpy(&info->literals[id], &v); // Deep copy the literal.
        if (e) return (e_var){ .type = E_VARTYPE_NULL };

        TRY_V(e_stack_push(info->stack, &v));
        e_var_release(&v); // Hand over v to the stack.
        break;
      }

      case E_OPCODE_MK_LIST: {
        u32 nelems = e_read_u32(&ip);

        e_refdobj* obj = e_refdobj_pool_acquire(&ge_pool);
        if (!obj) return nullvar;

        // Convert the object
        e_list* list = E_OBJ_AS_LIST(obj);

        e_var new_list = {
          .type = E_VARTYPE_LIST,
          // e_list_init initializes refc too
          .val.list = obj,
        };

        e_var* stack      = info->stack->stack;
        size_t stack_size = info->stack->size;

        e_var* elems = &stack[stack_size - nelems];
        TRY_V(e_list_init(elems, nelems, list)); // acquires the elements.

        // Release variables from the stack.
        for (u32 i = 0; i < nelems; i++) { e_stack_pop(info->stack); }

        TRY_V(e_stack_push(info->stack, &new_list));
        e_var_release(&new_list);

        break;
      }

      case E_OPCODE_MK_MAP: {
        u32 npairs = e_read_u32(&ip);

        e_refdobj* obj = e_refdobj_pool_acquire(&ge_pool);
        if (!obj) return nullvar;

        // Convert the object
        e_map* map = E_OBJ_AS_MAP(obj);

        e_var new_map = {
          .type    = E_VARTYPE_MAP,
          .val.map = obj,
        };

        e_var* stack      = info->stack->stack;
        size_t stack_size = info->stack->size;

        e_var* elems = &stack[stack_size - (npairs * 2)];
        TRY_V(e_map_init(elems, npairs, map)); // acquires the elements.

        // Release variables from the stack.
        for (u32 i = 0; i < npairs * 2; i++) { e_stack_pop(info->stack); }

        TRY_V(e_stack_push(info->stack, &new_map));
        e_var_release(&new_map);

        break;
      }

      case E_OPCODE_MK_STRUCT: {
        u32  member_count = e_read_u32(&ip);
        u32* fields       = calloc(member_count, sizeof(u32));
        for (u32 i = 0; i < member_count; i++) { fields[i] = e_read_u32(&ip); }

        e_var st = {
          .type      = E_VARTYPE_STRUCT,
          .val.struc = e_refdobj_pool_acquire(&ge_pool),
        };
        if (!st.val.struc) return nullvar;

        E_VAR_AS_STRUCT(&st)->member_hashes = fields;
        E_VAR_AS_STRUCT(&st)->members       = (e_var*)calloc(member_count, sizeof(e_var));
        E_VAR_AS_STRUCT(&st)->member_count  = member_count;

        for (u32 i = 0; i < member_count; i++) { E_VAR_AS_STRUCT(&st)->members[i] = (e_var){ .type = E_VARTYPE_NULL }; }

        TRY_V(e_stack_push(info->stack, &st));
        e_var_release(&st); // Stack owns it now

        break;
      }

      case E_OPCODE_MEMBER_ACCESS: {
        u32 member = e_read_u32(&ip);

        if (!info->stack) return nullvar;

        e_var push = { .type = E_VARTYPE_NULL };

        e_vartype type = e_stack_top(info->stack)->type;
        if (type == E_VARTYPE_VEC2 || type == E_VARTYPE_VEC3 || type == E_VARTYPE_VEC4) {
          e_var v = evector_zero_extend(e_stack_top(info->stack));

          double d = DBL_MAX;
          if (member == e_hash_fnv("x", 1)) { d = v.val.vec4.x; }
          if (member == e_hash_fnv("y", 1)) { d = v.val.vec4.y; }
          if (member == e_hash_fnv("z", 1)) { d = v.val.vec4.z; }
          if (member == e_hash_fnv("w", 1)) { d = v.val.vec4.w; }

          // Vectors are not ref counted. This is safe.
          push = (e_var){
            .type  = d == DBL_MAX ? E_VARTYPE_NULL : E_VARTYPE_FLOAT, // if member not in vec2, return a null var
            .val.f = d,
          };
        } else if (type == E_VARTYPE_STRUCT) {
          e_struct* st = E_VAR_AS_STRUCT(e_stack_top(info->stack));
          for (u32 i = 0; i < st->member_count; i++) {
            if (st->member_hashes[i] == member) {
              TRY_V(e_var_shallow_cpy(&st->members[i], &push));
              e_var_acquire(&push); // acquire tmp
              break;
            }
          }
        } else {
          push = nullvar;
        }

        // pop off struct
        e_stack_pop(info->stack);

        // Push member
        TRY_V(e_stack_push(info->stack, &push));
        e_var_release(&push); // release tmp hold
        break;
      }

      case E_OPCODE_MEMBER_ASSIGN: {
        u32 member = e_read_u32(&ip);

        e_var* value = e_stack_top(info->stack);
        e_var* struc = e_stack_top(info->stack) - 1;

        e_vartype type = struc->type;

        if (type == E_VARTYPE_VEC2 || type == E_VARTYPE_VEC3 || type == E_VARTYPE_VEC4) {
          const u32 x = e_hash_fnv("x", 1);
          const u32 y = e_hash_fnv("y", 1);
          const u32 z = e_hash_fnv("z", 1);
          const u32 w = e_hash_fnv("w", 1);
          // printf("Vector assign\n");
          if (member == x) {
            struc->val.vec4.x = value->val.f;
          } else if (member == y) {
            struc->val.vec4.y = value->val.f;
          } else if (type != E_VARTYPE_VEC2 && member == z) {
            struc->val.vec4.z = value->val.f;
          } else if (type == E_VARTYPE_VEC4 && member == w) {
            struc->val.vec4.w = value->val.f;
          }
        } else if (struc->type == E_VARTYPE_STRUCT) {
          e_struct* st = E_VAR_AS_STRUCT(struc);
          for (u32 i = 0; i < st->member_count; i++) {
            if (st->member_hashes[i] == member) {
              e_var_release(&st->members[i]);
              TRY_V(e_var_shallow_cpy(value, &st->members[i]));
              e_var_acquire(&st->members[i]);
              break;
            }
          }
        }

        // e_var value_copy;
        // e_var_shallow_cpy(value, &value_copy);
        // e_var_acquire(&value_copy);

        /* remove value, we have a copy of it.  */
        e_stack_pop(info->stack);

        // /* release old struct object */
        // e_var_release(struc);

        // /* replace struct slot with value copy */
        // *struc = value_copy;

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
        e_var r = operate(info->stack->stack[info->stack->size - 2], info->stack->stack[info->stack->size - 1], opcode);

        e_stack_pop(info->stack); // remove L & R
        e_stack_pop(info->stack);

        /* No need to acquire r. */
        TRY_V(e_stack_push(info->stack, &r));
        break;
      }

      case E_OPCODE_INC:
      case E_OPCODE_DEC: {
        e_var* top = e_stack_top(info->stack);

        /* These two are always compound, but still emit the is_compound flag */
        (void)e_read_u8(&ip);

        if (top->type == E_VARTYPE_INT) {
          top->val.i += (opcode == E_OPCODE_INC) ? 1 : -1;
        } else {
          top->val.f += (opcode == E_OPCODE_INC) ? 1.F : -1.F;
        }

        break;
      }

      case E_OPCODE_NEG:
      case E_OPCODE_NOT:
      case E_OPCODE_BNOT: {
        bool is_compound = (bool)e_read_u8(&ip);

        // Provide an empty variable for the LHS
        e_var r = operate((e_var){ 0 }, *e_stack_top(info->stack), opcode);

        // printf("in goes ");
        // eb_print(e_stack_top(info->stack), 1);
        // printf(", and out comes ");
        // eb_println(&r, 1);

        if (is_compound) {
          e_var_release(e_stack_top(info->stack));
          *e_stack_top(info->stack) = r;
        } else {
          e_stack_pop(info->stack); // remove RHS
          TRY_V(e_stack_push(info->stack, &r));
        }

        break;
      }

      case E_OPCODE_JZ: {
        u32 target = e_read_u32(&ip); // always read the operand

        const e_var* cnd  = e_stack_top(info->stack);
        bool         eval = evar_to_bool(*cnd);
        if (!eval) ip = info->code + target;

        e_stack_pop(info->stack); // remove condition
        break;
      }

      case E_OPCODE_JNZ: {
        u32 target = e_read_u32(&ip); // always read the operand

        const e_var* cnd  = e_stack_top(info->stack);
        bool         eval = evar_to_bool(*cnd);
        if (eval) ip = info->code + target;

        e_stack_pop(info->stack); // remove condition
        break;
      }

      case E_OPCODE_JE: {
        u32 target = e_read_u32(&ip); // always read the operand

        e_var* top = e_stack_top(info->stack);
        if (e_var_equal(top, top - 1)) ip = info->code + target;

        e_stack_pop(info->stack); // remove conditions
        e_stack_pop(info->stack);
        break;
      }

      case E_OPCODE_JNE: {
        u32 target = e_read_u32(&ip); // always read the operand

        e_var* top = e_stack_top(info->stack);
        if (!e_var_equal(top, top - 1)) ip = info->code + target;

        e_stack_pop(info->stack); // remove conditions
        e_stack_pop(info->stack);
        break;
      }

      case E_OPCODE_JMP:
        ip = info->code + e_read_u32(&ip);
        break;

        // case E_OPCODE_JE: ip = inss + e_read_u32(&ip); break;
        // case E_OPCODE_JNE: ip = inss + e_read_u32(&ip); break;

      case E_OPCODE_INIT: {
        u32  hash        = e_read_u32(&ip);
        bool is_compound = (bool)e_read_u8(&ip);

        if (is_compound) {
          e_var  top_take;
          e_var* top = e_stack_top(info->stack);

          e_var_acquire(top);
          TRY_V(e_var_shallow_cpy(top, &top_take));

          e_stack_pop(info->stack);

          e_var* v = e_stack_push_variable(hash, info->stack);
          TRY_V(e_var_shallow_cpy(&top_take, v));
        } else {
          e_stack_push_variable(hash, info->stack);
        }
        break;
      }

      case E_OPCODE_LOAD: {
        u32 id = e_read_u32(&ip);

        bool was_external = false;
        for (u32 i = 0; i < info->nextern_vars; i++) {
          const char* name = info->extern_vars[i].name;
          if (e_hash_fnv(name, strlen(name)) == id) {
            e_var v = {
              .type = info->extern_vars[i].type,
              .val  = info->extern_vars[i].value,
            };

            TRY_V(e_stack_push(info->stack, &v));

            was_external = true;
            break;
          }
        }

        if (was_external) break;

        e_var* slot = get_variable_from_id(info->stack, id);
        if (!slot) return nullvar;

        e_var v;
        TRY_V(e_var_shallow_cpy(slot, &v));

        TRY_V(e_stack_push(info->stack, &v));
        break;
      }

      case E_OPCODE_INDEX: {
        e_var push = { .type = E_VARTYPE_NULL };

        e_var* stack      = info->stack->stack;
        size_t stack_size = info->stack->size;

        e_vartype left_type = stack[stack_size - 2].type;
        if (left_type == E_VARTYPE_LIST) {
          e_list* list = stack[stack_size - 2].type == E_VARTYPE_LIST ? E_VAR_AS_LIST(&stack[stack_size - 2]) : NULL;
          if (!list) { return (e_var){ .type = E_VARTYPE_NULL }; }

          int idx = evar_to_int(stack[stack_size - 1]);

          if (list && idx >= 0 && (u64)idx < list->size) {
            push = list->vars[idx];
            e_var_acquire(&push);
          }
        } else if (left_type == E_VARTYPE_MAP) {
          e_var* map_var = &stack[stack_size - 2];
          e_map* map     = E_VAR_AS_MAP(map_var);
          e_var  key     = stack[stack_size - 1];

          if (!map) { return (e_var){ .type = E_VARTYPE_NULL }; }

          e_var* find = e_map_find(map, &key);
          if (find) {
            push = *find;
            e_var_acquire(&push);
          } // else push is null var
        } else if (left_type == E_VARTYPE_VEC2) {
          e_vec2 v2  = stack[stack_size - 2].val.vec2;
          int    idx = evar_to_int(stack[stack_size - 1]);
          if (idx < 2) { push = (e_var){ .type = E_VARTYPE_FLOAT, .val.f = idx == 0 ? v2.x : v2.y }; }
        } else if (left_type == E_VARTYPE_VEC3) {
          e_vec3 v3  = stack[stack_size - 2].val.vec3;
          int    idx = evar_to_int(stack[stack_size - 1]);
          if (idx < 3) { push = (e_var){ .type = E_VARTYPE_FLOAT, .val.f = idx == 0 ? v3.x : idx == 1 ? v3.y : v3.z }; }
        } else if (left_type == E_VARTYPE_VEC4) {
          e_vec4 v4  = stack[stack_size - 2].val.vec4;
          int    idx = evar_to_int(stack[stack_size - 1]);
          if (idx < 4) { push = (e_var){ .type = E_VARTYPE_FLOAT, .val.f = idx == 0 ? v4.x : idx == 1 ? v4.y : idx == 2 ? v4.z : v4.w }; }
        }

        e_stack_pop(info->stack); // pop index
        e_stack_pop(info->stack); // pop base

        e_stack_push(info->stack, &push);

        e_var_release(&push);
        break;
      }

      case E_OPCODE_INDEX_ASSIGN: {
        e_var* stack      = info->stack->stack;
        u32    stack_size = info->stack->size;

        e_var* base  = &stack[stack_size - 3];
        e_var* index = &stack[stack_size - 2];
        e_var* value = &stack[stack_size - 1];

        if (base->type == E_VARTYPE_LIST) {
          e_list* list = E_VAR_AS_LIST(base);
          int     idx  = evar_to_int(*index);

          e_var* slot = &list->vars[idx];
          e_var_release(slot);
          TRY_V(e_var_shallow_cpy(value, slot));
          e_var_acquire(slot);
        } else if (base->type == E_VARTYPE_MAP) {
          e_map* map  = E_VAR_AS_MAP(base);
          e_var* slot = e_map_find_or_insert(map, index);

          if (!map || !slot) { return (e_var){ .type = E_VARTYPE_NULL }; }

          e_var_release(slot);
          TRY_V(e_var_shallow_cpy(value, slot));
          e_var_acquire(slot);
        } else if (base->type == E_VARTYPE_VEC2 || base->type == E_VARTYPE_VEC3 || base->type == E_VARTYPE_VEC4) {
          int    idx = evar_to_int(*index);
          double f   = evar_to_float(*value);

          if (idx == 0) {
            base->val.vec4.x = f;
          } else if (idx == 1) {
            base->val.vec4.y = f;
          } else if (idx == 2) {
            base->val.vec4.z = f;
          } else if (idx == 3) {
            base->val.vec4.w = f;
          }
        }

        e_stack_pop(info->stack);
        e_stack_pop(info->stack);
        /* base remains on stack */

        break;
      }

      case E_OPCODE_ASSIGN: {
        u32    id   = e_read_u32(&ip);
        e_var* slot = get_variable_from_id(info->stack, id);

        if (!slot) { return (e_var){ .type = E_VARTYPE_NULL }; }

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
        TRY_V(e_var_shallow_cpy(e_stack_top(info->stack), slot));

        e_var_acquire(slot);

        break;
      }

      case E_OPCODE_RETURN: {
        u8 has_return_value = e_read_u8(&ip);

        if (has_return_value) {
          TRY_V(e_var_shallow_cpy(e_stack_top(info->stack), &retval));
          e_var_acquire(&retval);
        }

        goto _RETURN;
      }

      case E_OPCODE_PUSH_VARIABLES: TRY_V(e_stack_push_frame(info->stack)); break;
      case E_OPCODE_POP_VARIABLES: e_stack_pop_frame(info->stack); break;

      // Non fatal return
      case E_OPCODE_HALT: goto _RETURN;
    }
  }

  printf("Illegal instruction\n");
  exit(-1);

_RETURN: {
  /**
   * Everything is externally managed. Don't need to free anything.
   */
  return retval;
}
}

e_var
e_script_call(e_script* s, const char* func_name, e_var* args, u32 nargs)
{
  u32 hash = e_hash_fnv(func_name, strlen(func_name));

  for (u32 i = 0; i < s->compiled.nfunctions; i++) {
    if (hash == s->compiled.functions[i].name_hash) {
      e_exec_info info = {
        .code          = s->compiled.functions[i].code,
        .args          = args,
        .slots         = s->compiled.functions[i].arg_slots,
        .literals      = s->compiled.literals,
        .funcs         = s->compiled.functions,
        .extern_funcs  = s->extern_funcs,
        .stack         = &s->stack,
        .code_size     = s->compiled.functions[i].code_size,
        .nargs         = nargs,
        .nliterals     = s->compiled.nliterals,
        .nfuncs        = s->compiled.nfunctions,
        .nextern_funcs = s->nxtern_funcs,
        .extern_vars   = s->extern_vars,
        .nextern_vars  = s->nextern_vars,
      };

      e_stack_push_frame(info.stack);
      e_var r = e_exec(&info);
      e_stack_pop_frame(info.stack);

      return r;
    }
  }

  return (e_var){ .type = E_VARTYPE_NULL };
}
