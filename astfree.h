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

#ifndef E_AST_FREE_H
#define E_AST_FREE_H

#include "ast.h"

#include <stdlib.h>

static inline void
e_ast_node_free(e_ast* p, int nodeID)
{
  if (nodeID < 0) return;

  // static bool test = false;
  // if (nodeID == 0 && test) {
  //   printf("Multiple root nodes!\n");
  //   return;
  // }
  // if (nodeID == 0) { test = true; }

  // free(E_GET_NODE(p, nodeID)->common.span.file);

  // E_GET_NODE(p, nodeID)->common.alive = false;

  // switch (E_GET_NODE(p, nodeID)->type) {
  //   case E_AST_NODE_NOP: break;
  //   case E_AST_NODE_ROOT:
  //     for (u32 i = 0; i < E_GET_NODE(p, nodeID)->root.nstmts; i++) e_ast_node_free(p, E_GET_NODE(p, nodeID)->root.stmts[i]);
  //     free(E_GET_NODE(p, nodeID)->root.stmts);
  //     break;
  //   case E_AST_NODE_BINARYOP:
  //   case E_AST_NODE_ASSIGN:
  //     e_ast_node_free(p, E_GET_NODE(p, nodeID)->binaryop.left);
  //     e_ast_node_free(p, E_GET_NODE(p, nodeID)->binaryop.right);
  //     break;
  //   case E_AST_NODE_INDEX: {
  //     e_ast_node_free(p, E_GET_NODE(p, nodeID)->index.base);
  //     e_ast_node_free(p, E_GET_NODE(p, nodeID)->index.index);
  //     break;
  //   }
  //   case E_AST_NODE_UNARYOP: e_ast_node_free(p, E_GET_NODE(p, nodeID)->unaryop.right); break;
  //   case E_AST_NODE_STATEMENT_LIST:
  //     for (u32 i = 0; i < E_GET_NODE(p, nodeID)->stmts.nstmts; i++) e_ast_node_free(p, E_GET_NODE(p, nodeID)->stmts.stmts[i]);
  //     free(E_GET_NODE(p, nodeID)->stmts.stmts);
  //     break;
  //   case E_AST_NODE_FOR: {
  //     e_ast_node_free(p, E_GET_NODE(p, nodeID)->for_stmt.initializers);
  //     e_ast_node_free(p, E_GET_NODE(p, nodeID)->for_stmt.condition);
  //     for (u32 i = 0; i < E_GET_NODE(p, nodeID)->for_stmt.niterators; i++) e_ast_node_free(p, E_GET_NODE(p, nodeID)->for_stmt.iterators[i]);
  //     for (u32 i = 0; i < E_GET_NODE(p, nodeID)->for_stmt.nstmts; i++) { e_ast_node_free(p, E_GET_NODE(p, nodeID)->for_stmt.stmts[i]); }
  //     free(E_GET_NODE(p, nodeID)->for_stmt.stmts);
  //     free(E_GET_NODE(p, nodeID)->for_stmt.iterators);
  //     break;
  //   }
  //   case E_AST_NODE_WHILE:
  //     for (u32 i = 0; i < E_GET_NODE(p, nodeID)->while_stmt.nstmts; i++) e_ast_node_free(p, E_GET_NODE(p, nodeID)->while_stmt.stmts[i]);
  //     e_ast_node_free(p, E_GET_NODE(p, nodeID)->while_stmt.condition);
  //     free(E_GET_NODE(p, nodeID)->while_stmt.stmts);
  //     break;
  //   case E_AST_NODE_BREAK:
  //   case E_AST_NODE_CONTINUE: break;
  //   case E_AST_NODE_RETURN:
  //     if (E_GET_NODE(p, nodeID)->ret.has_return_value) e_ast_node_free(p, E_GET_NODE(p, nodeID)->ret.expr_id);
  //     break;
  //   case E_AST_NODE_FUNCTION_DEFINITION: {
  //     for (u32 i = 0; i < E_GET_NODE(p, nodeID)->func.nstmts; i++) e_ast_node_free(p, E_GET_NODE(p, nodeID)->func.stmts[i]);
  //     for (u32 i = 0; i < E_GET_NODE(p, nodeID)->func.nargs; i++) free(E_GET_NODE(p, nodeID)->func.args[i]);
  //     free(E_GET_NODE(p, nodeID)->func.args);
  //     free(E_GET_NODE(p, nodeID)->func.name);
  //     free(E_GET_NODE(p, nodeID)->func.stmts);
  //     break;
  //   }
  //   case E_AST_NODE_IF: {
  //     e_ast_node_free(p, E_GET_NODE(p, nodeID)->if_stmt.condition);
  //     for (u32 i = 0; i < E_GET_NODE(p, nodeID)->if_stmt.nstmts; i++) e_ast_node_free(p, E_GET_NODE(p, nodeID)->if_stmt.body[i]);
  //     for (u32 i = 0; i < E_GET_NODE(p, nodeID)->if_stmt.nelse_stmts; i++) e_ast_node_free(p, E_GET_NODE(p, nodeID)->if_stmt.else_body[i]);
  //     for (u32 i = 0; i < E_GET_NODE(p, nodeID)->if_stmt.nelse_ifs; i++) {
  //       struct e_if_stmt* else_if = &E_GET_NODE(p, nodeID)->if_stmt.else_ifs[i];
  //       e_ast_node_free(p, else_if->condition);
  //       for (u32 j = 0; j < else_if->nstmts; j++) e_ast_node_free(p, else_if->body[j]);
  //       free(else_if->body);
  //     }
  //     free(E_GET_NODE(p, nodeID)->if_stmt.else_ifs);
  //     free(E_GET_NODE(p, nodeID)->if_stmt.else_body);
  //     free(E_GET_NODE(p, nodeID)->if_stmt.body);

  //     break;
  //   }
  //   case E_AST_NODE_VARIABLE:
  //     free(E_GET_NODE(p, nodeID)->ident.ident);
  //     help_im_going_to_die--;
  //     break;

  //   case E_AST_NODE_VARIABLE_DECL:
  //     free(E_GET_NODE(p, nodeID)->let.name);
  //     e_ast_node_free(p, E_GET_NODE(p, nodeID)->let.initializer);
  //     break;

  //   case E_AST_NODE_INDEX_ASSIGN:
  //     e_ast_node_free(p, E_GET_NODE(p, nodeID)->index_assign.base);
  //     e_ast_node_free(p, E_GET_NODE(p, nodeID)->index_assign.index);
  //     e_ast_node_free(p, E_GET_NODE(p, nodeID)->index_assign.value);
  //     break;

  //   case E_AST_NODE_CALL:
  //     for (u32 i = 0; i < E_GET_NODE(p, nodeID)->call.nargs; i++) e_ast_node_free(p, E_GET_NODE(p, nodeID)->call.args[i]);
  //     free(E_GET_NODE(p, nodeID)->call.function_name);
  //     free(E_GET_NODE(p, nodeID)->call.args);
  //     break;

  //   case E_AST_NODE_STRING: free(E_GET_NODE(p, nodeID)->s.s); break;

  //   case E_AST_NODE_LIST:
  //     for (u32 i = 0; i < E_GET_NODE(p, nodeID)->list.nelems; i++) e_ast_node_free(p, E_GET_NODE(p, nodeID)->list.elems[i]);
  //     free(E_GET_NODE(p, nodeID)->list.elems);
  //     break;

  //   case E_AST_NODE_INT:
  //   case E_AST_NODE_CHAR:
  //   case E_AST_NODE_BOOL:
  //   case E_AST_NODE_FLOAT:
  //   case E_AST_NODE_MAP: break;

  //   case E_AST_NODE_INDEX_COMPOUND_OP: {
  //     e_ast_node_free(p, E_GET_NODE(p, nodeID)->index_compound.base);
  //     e_ast_node_free(p, E_GET_NODE(p, nodeID)->index_compound.index);
  //     e_ast_node_free(p, E_GET_NODE(p, nodeID)->index_compound.value);
  //     break;
  //   }
  //   case E_AST_NODE_MEMBER_ACCESS: {
  //     e_ast_node_free(p, E_GET_NODE(p, nodeID)->member_access.left);
  //     free(E_GET_NODE(p, nodeID)->member_access.right);
  //     break;
  //   }
  //   case E_AST_NODE_NAMESPACE_DECL: {
  //     for (u32 i = 0; i < E_GET_NODE(p, nodeID)->namespace_decl.nstmts; i++) { e_ast_node_free(p, E_GET_NODE(p, nodeID)->namespace_decl.stmts[i]); }
  //     free(E_GET_NODE(p, nodeID)->namespace_decl.name);
  //     free(E_GET_NODE(p, nodeID)->namespace_decl.stmts);
  //     break;
  //   }
  // }
}
#endif // E_AST_FREE_H