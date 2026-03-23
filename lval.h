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

#ifndef E_CC_LVALUE_H
#define E_CC_LVALUE_H

#include "ast.h"
#include "bc.h"
#include "cc.h"
#include "rwhelp.h"
#include "stdafx.h"
#include "var.h"

typedef enum e_lval_type {
  E_LVAL_VAR,
  E_LVAL_MEMBER, // struct member
  E_LVAL_INDEX,  // indexed array
} e_lval_type;

typedef union e_lval_value {
  u32 var_id;
  struct {
    char* name;
    u32   struct_hash;
  } member;
  struct {
    int left_node;  // Compile to get LHS of index. For vec[16], it will push vec to stack.
    int index_node; // Compile to get index. For vec[16], it will push 16 to stack.
  } index;
} e_lval_value;

typedef struct e_lval {
  e_lval_type  type;
  e_lval_value val;
} e_lval;

static inline bool
e_can_make_value(const e_ast* ast, int node)
{
  if (ast == nullptr || node < 0) return false;
  return E_GET_NODE(ast, node)->type == E_ASNODE_VARIABLE;
}

static inline e_lval
e_make_value(const e_ast* ast, int node)
{
  switch (E_GET_NODE(ast, node)->type) {
    case E_ASNODE_VARIABLE: {
      const char* name = E_GET_NODE(ast, node)->val.ident;

      e_lval l;
      l.type       = E_LVAL_VAR;
      l.val.var_id = e_hash_fnv(name, strlen(name));
      return l;
    }

    default: printf("%i can not be represented as a value (it is %u)\n", node, E_GET_NODE(ast, node)->type); exit(-1);
  }

  return (e_lval){};
}

static inline void
e_emit_lvalue_load(e_compiler* cc, e_lval lv)
{
  e_emit_instruction(cc, E_OPCODE_LOAD, E_ATTR_NONE);
  e_emit_u32(cc, lv.val.var_id);
}

// static inline void
// e_emit_lvalue_load_address(e_compiler* cc, e_lval lv)
// {
//   // e_emit_instruction(cc, E_OPCODE_LOAD_REFERENCE, E_ATTR_NONE);
//   e_emit_u16(cc, lv.val.var_id);
// }

#endif // E_CC_LVALUE_H