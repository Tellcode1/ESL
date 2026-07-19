#include "../../inc/kit.ast.h"
#include "../../inc/kit.cc.h"
#include "../../inc/kit.rwhelp.h"
#include "compile_routines.h"
#include "defer.h"

kit_vreg_t
compile_statement_list(kit_compiler* cc, int node)
{
  kit_ast_node* nodep  = KIT_GET_NODE(cc->ast, node);
  const int*    stmts  = nodep->stmts.stmts;
  u32           nstmts = nodep->stmts.nstmts;

  for (u32 i = 0; i < nstmts; i++) {
    int e = compile(cc, stmts[i]);
    if (e < 0) {
      cerror(KIT_GET_NODE(cc->ast, stmts[i])->common.span, "Failed to compile statement [statement list]\n");
      return e;
    }
  }

  return 0;
}

kit_vreg_t
compile_return(kit_compiler* cc, int node)
{
  int r = defer_emit_all_scopes(cc);
  if (r < 0) return r;

  if (KIT_GET_NODE(cc->ast, node)->ret.has_return_value) {
    int ret_node = KIT_GET_NODE(cc->ast, node)->ret.expr_id;
    /* Compile the return value */
    kit_vreg_t rv = compile(cc, ret_node);
    if (rv < 0) {
      cerror(KIT_GET_NODE(cc->ast, node)->common.span, "Failed to compile return value [return]\n");
      return rv;
    }

    kit_emit_ins(cc, (kit_ins){ .ret = { .opcode = KIT_IR_OPCODE_RET, .return_value = rv } });
  } else {
    kit_var    nil    = KIT_NULLVAR;
    kit_vreg_t nilvar = compile_and_push_literal_variable(cc, &nil);
    if (nilvar < 0) return nilvar;

    kit_emit_ins(cc, (kit_ins){ .ret = { .opcode = KIT_IR_OPCODE_RET, .return_value = nilvar } });
  }
  return r;
}