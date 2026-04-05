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
#include "cc.h"
#include "cerr.h"
#include "stdafx.h"

typedef enum e_lval_type {
  E_LVAL_VAR,
  E_LVAL_MEMBER,  // struct member
  E_LVAL_INDEX,   // indexed array
  E_LVAL_UNKNOWN, // Error!
} e_lval_type;

typedef union e_lval_value {
  struct {
    u32   id;
    char* name; // allocated
  } var;
  struct {
    int         base;
    const char* member;
  } member;
  struct {
    int left_node;  // Compile to get LHS of index. For vec[16], it will push vec to stack.
    int index_node; // Compile to get index. For vec[16], it will push 16 to stack.

    bool left_is_var; // Is left a variable?
    u32  left_var_id;

  } index;
} e_lval_value;

typedef struct e_lval {
  e_filespan*  span;
  e_lval_type  type;
  e_lval_value val;
} e_lval;

static inline bool
e_can_make_value(const e_ast* ast, int node)
{
  if (ast == nullptr || node < 0) return false;
  return E_GET_NODE(ast, node)->type == E_AST_NODE_VARIABLE || E_GET_NODE(ast, node)->type == E_AST_NODE_INDEX
      || E_GET_NODE(ast, node)->type == E_AST_NODE_INDEX_ASSIGN || E_GET_NODE(ast, node)->type == E_AST_NODE_INDEX_COMPOUND_OP
      || E_GET_NODE(ast, node)->type == E_AST_NODE_MEMBER_ACCESS;
}

e_lval e_make_value(e_compiler* cc, int node);
void   e_free_value(e_lval* lv);

int e_emit_lvalue_load(e_compiler* cc, e_lval lv);

// cc.c
int e_emit_lvalue_assign(e_compiler* cc, int value, e_lval lv);

// static inline void
// e_emit_lvalue_load_address(e_compiler* cc, e_lval lv)
// {
//   // e_emit_instruction(cc, E_OPCODE_LOAD_REFERENCE, E_ATTR_NONE);
//   e_emit_u16(cc, lv.val.var_id);
// }

#endif // E_CC_LVALUE_H