#include "../../inc/kit.cc.h"
#include "compile_routines.h"
#include "lvalue.h"
#include "scope.h"

kit_vreg_t
compile_assign(kit_compiler* cc, int node)
{
  int right = KIT_GET_NODE(cc->ast, node)->assign.right;
  int left  = KIT_GET_NODE(cc->ast, node)->assign.left;

  if (!can_make_value(cc->ast, left)) {
    kit_filespan left_span = KIT_GET_NODE(cc->ast, left)->common.span;
    cerror(left_span, "Can not assign to left: Failed to lower to lvalue\n");
    return -1;
  }

  val_t lv;
  int   e = value_init(cc, left, &lv);
  if (e < 0) return e;

  kitc_builtin_variables_table* builtin_vars_table = cc->builtin_var_table;

  kitc_var* exists = scope_lookup_info(cc, lv.val.var.id);

  if (!exists && KIT_GET_NODE(cc->ast, left)->type == KIT_AST_NODE_VARIABLE) {
    // Doesn't exist and node is supposed to be a variable (Not member access or index)

    /* Check if the user is trying to modify a builtin variable. */
    for (u32 i = 0; i < builtin_vars_table->builtin_vars_count; i++) {
      if (lv.val.var.id == builtin_vars_table->builtin_var_hashes[i]) {
        cerror(KIT_GET_NODE(cc->ast, left)->common.span, "Attempting to modify builtin constant '%s'\n", lv.val.var.name);
        value_free(&lv);
        return -1;
      }
    }

    cerror(KIT_GET_NODE(cc->ast, left)->common.span, "Undeclared variable '%s'\n", lv.val.var.name);
    value_free(&lv);
    return -1;
  }

  if (exists && exists->is_const) {
    cerror(KIT_GET_NODE(cc->ast, left)->common.span, "Can not assign to const qualified variable '%s'\n", lv.val.var.name);
    value_free(&lv);
    return -1;
  }

  kit_vreg_t rreg = compile(cc, right);
  if (rreg < 0) return rreg;

  e = emit_lvalue_assign(cc, rreg, &lv);
  value_free(&lv);
  if (e < 0) return e;

  /* Propogate the RHS' register */
  return rreg;
}

kit_vreg_t
compile_member_assign(kit_compiler* cc, int node)
{
  int value = KIT_GET_NODE(cc->ast, node)->member_assign.value;

  if (!can_make_value(cc->ast, node)) {
    cerror(KIT_GET_NODE(cc->ast, node)->common.span, "Can not assign to member access: Failed to lower to lvalue\n");
    return -1;
  }

  kit_vreg_t rhs = compile(cc, value);
  if (rhs < 0) { return rhs; }

  val_t lv;
  int   e = value_init(cc, node, &lv);
  if (e < 0) return e;

  kit_vreg_t propogate_rhs = emit_lvalue_assign(cc, rhs, &lv);
  value_free(&lv);

  return propogate_rhs;
}

kit_vreg_t
compile_index_assign(kit_compiler* cc, int node)
{
  int value = KIT_GET_NODE(cc->ast, node)->index_assign.value;

  if (!can_make_value(cc->ast, node)) {
    kit_filespan left_span = KIT_GET_NODE(cc->ast, node)->index_assign.span;
    cerror(left_span, "Can not assign to indexed expression: Failed to lower to lvalue\n");
    return -1;
  }

  val_t v = { 0 };

  int e = value_init(cc, node, &v);
  if (e < 0) return e;

  kit_vreg_t eval_value = compile(cc, value);
  kit_vreg_t dst        = emit_lvalue_assign(cc, eval_value, &v);
  value_free(&v);

  return dst;
}
