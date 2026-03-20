#ifndef E_AST_FREE_H
#define E_AST_FREE_H

#include "ast.h"

#include <stdlib.h>

static inline void
e_asnode_free(e_ast* p, int nodeID)
{
  if (nodeID < 0) return;

  free(E_GET_NODE(p, nodeID)->span.file);
  switch (E_GET_NODE(p, nodeID)->type) {
    case E_ASNODE_NOP: break;
    case E_ASNODE_ROOT:
      for (u32 i = 0; i < E_GET_NODE(p, nodeID)->val.root.nstmts; i++) e_asnode_free(p, E_GET_NODE(p, nodeID)->val.root.stmts[i]);
      free(E_GET_NODE(p, nodeID)->val.root.stmts);
      break;
    case E_ASNODE_BINARYOP:
      e_asnode_free(p, E_GET_NODE(p, nodeID)->val.binaryop.left);
      e_asnode_free(p, E_GET_NODE(p, nodeID)->val.binaryop.right);
      break;
    case E_ASNODE_INDEX: {
      e_asnode_free(p, E_GET_NODE(p, nodeID)->val.index.base);
      e_asnode_free(p, E_GET_NODE(p, nodeID)->val.index.offset);
      break;
    }
    case E_ASNODE_UNARYOP: e_asnode_free(p, E_GET_NODE(p, nodeID)->val.unaryop.right); break;
    case E_ASNODE_EXPRESSION_LIST:
      for (u32 i = 0; i < E_GET_NODE(p, nodeID)->val.stmts.nstmts; i++) e_asnode_free(p, E_GET_NODE(p, nodeID)->val.stmts.stmts[i]);
      free(E_GET_NODE(p, nodeID)->val.stmts.stmts);
      break;
    case E_ASNODE_FOR: {
      e_asnode_free(p, E_GET_NODE(p, nodeID)->val.for_stmt.initializers);
      e_asnode_free(p, E_GET_NODE(p, nodeID)->val.for_stmt.condition);
      e_asnode_free(p, E_GET_NODE(p, nodeID)->val.for_stmt.iterators);
      for (u32 i = 0; i < E_GET_NODE(p, nodeID)->val.for_stmt.nstmts; i++) { e_asnode_free(p, E_GET_NODE(p, nodeID)->val.for_stmt.stmts[i]); }
      free(E_GET_NODE(p, nodeID)->val.for_stmt.stmts);
      break;
    }
    case E_ASNODE_WHILE:
      for (u32 i = 0; i < E_GET_NODE(p, nodeID)->val.while_stmt.nstmts; i++) e_asnode_free(p, E_GET_NODE(p, nodeID)->val.while_stmt.stmts[i]);
      e_asnode_free(p, E_GET_NODE(p, nodeID)->val.while_stmt.condition);
      free(E_GET_NODE(p, nodeID)->val.while_stmt.stmts);
      break;
    case E_ASNODE_BREAK:
    case E_ASNODE_CONTINUE: break;
    case E_ASNODE_RETURN:
      if (E_GET_NODE(p, nodeID)->val.ret.has_return_value) e_asnode_free(p, E_GET_NODE(p, nodeID)->val.ret.expr_id);
      break;
    case E_ASNODE_FUNCTION_DEFINITION: {
      for (u32 i = 0; i < E_GET_NODE(p, nodeID)->val.func.nstmts; i++) e_asnode_free(p, E_GET_NODE(p, nodeID)->val.func.stmts[i]);
      for (u32 i = 0; i < E_GET_NODE(p, nodeID)->val.func.nargs; i++) free(E_GET_NODE(p, nodeID)->val.func.args[i]);
      free(E_GET_NODE(p, nodeID)->val.func.args);
      free(E_GET_NODE(p, nodeID)->val.func.name);
      free(E_GET_NODE(p, nodeID)->val.func.stmts);
      break;
    }
    case E_ASNODE_IF: {
      e_asnode_free(p, E_GET_NODE(p, nodeID)->val.if_stmt.condition);
      for (u32 i = 0; i < E_GET_NODE(p, nodeID)->val.if_stmt.nstmts; i++) e_asnode_free(p, E_GET_NODE(p, nodeID)->val.if_stmt.body[i]);
      for (u32 i = 0; i < E_GET_NODE(p, nodeID)->val.if_stmt.nelse_stmts; i++) e_asnode_free(p, E_GET_NODE(p, nodeID)->val.if_stmt.else_body[i]);
      for (u32 i = 0; i < E_GET_NODE(p, nodeID)->val.if_stmt.nelse_ifs; i++) {
        struct e_if_stmt* else_if = &E_GET_NODE(p, nodeID)->val.if_stmt.else_ifs[i];
        e_asnode_free(p, else_if->condition);
        for (u32 j = 0; j < else_if->nstmts; j++) e_asnode_free(p, else_if->body[j]);
        free(else_if->body);
      }
      free(E_GET_NODE(p, nodeID)->val.if_stmt.else_ifs);
      free(E_GET_NODE(p, nodeID)->val.if_stmt.else_body);
      free(E_GET_NODE(p, nodeID)->val.if_stmt.body);

      break;
    }
    case E_ASNODE_VARIABLE: free(E_GET_NODE(p, nodeID)->val.ident); break;

    case E_ASNODE_VARIABLE_DECL:
      free(E_GET_NODE(p, nodeID)->val.let.name);
      e_asnode_free(p, E_GET_NODE(p, nodeID)->val.let.initializer);
      break;

    case E_ASNODE_ASSIGN:
      e_asnode_free(p, E_GET_NODE(p, nodeID)->val.assign.left);
      e_asnode_free(p, E_GET_NODE(p, nodeID)->val.assign.right);
      break;

    case E_ASNODE_INDEX_ASSIGN:
      e_asnode_free(p, E_GET_NODE(p, nodeID)->val.index_assign.base);
      e_asnode_free(p, E_GET_NODE(p, nodeID)->val.index_assign.offset);
      e_asnode_free(p, E_GET_NODE(p, nodeID)->val.index_assign.value);
      break;

    case E_ASNODE_CALL:
      free(E_GET_NODE(p, nodeID)->val.call.function);
      for (u32 i = 0; i < E_GET_NODE(p, nodeID)->val.call.nargs; i++) e_asnode_free(p, E_GET_NODE(p, nodeID)->val.call.args[i]);
      free(E_GET_NODE(p, nodeID)->val.call.args);
      break;

    case E_ASNODE_STRING: free(E_GET_NODE(p, nodeID)->val.s); break;

    case E_ASNODE_LIST:
      for (u32 i = 0; i < E_GET_NODE(p, nodeID)->val.list.nelems; i++) e_asnode_free(p, E_GET_NODE(p, nodeID)->val.list.elems[i]);
      free(E_GET_NODE(p, nodeID)->val.list.elems);
      break;

    case E_ASNODE_INT:
    case E_ASNODE_CHAR:
    case E_ASNODE_BOOL:
    case E_ASNODE_FLOAT:
    case E_ASNODE_MAP: break;
  }
}
#endif // E_AST_FREE_H