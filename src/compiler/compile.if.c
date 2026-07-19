#include "../../inc/kit.cc.h"
#include "compile_routines.h"
#include "defer.h"
#include "scope.h"

// This is the dirtiest of them all...
kit_vreg_t
compile_if_statement(kit_compiler* cc, int node)
{
  /* Label after if statements body */
  u32 end_label = make_label_id(cc);

  const int condition = KIT_GET_NODE(cc->ast, node)->if_stmt.condition;
  int       e         = 0;

  /**
   * Label of the next else if / else to jump to. Still used if no branches are present, there's
   * just a jmp instruction directly after the JMP to where the branch would be.
   */
  u32 next_in_chain_label = make_label_id(cc);

  scope_push(cc);

  // condition
  kit_vreg_t cond = compile(cc, condition);
  if (cond < 0) {
    cerror(KIT_GET_NODE(cc->ast, condition)->common.span, "Failed to compile condition of top if statement [if statement]\n");
    goto ERR;
  }

  // condition failed :<
  e = emit_and_record_jmp(cc, KIT_IR_OPCODE_JZ, cond, next_in_chain_label); // Jump to the next in chain. else if/else/end of if statement
  if (e < 0) goto ERR;

  e = defer_push_scope(cc);
  if (e < 0) goto ERR;

  // BODY OF ROOT IF STATEMENT
  const int* if_body = KIT_GET_NODE(cc->ast, node)->if_stmt.body;
  for (u32 i = 0; i < KIT_GET_NODE(cc->ast, node)->if_stmt.nstmts; i++) {
    e = compile(cc, if_body[i]);
    if (e < 0) {
      cerror(KIT_GET_NODE(cc->ast, if_body[i])->common.span, "Failed to compile if statement body [if statement]\n");
      goto ERR;
    }
  }

  e = defer_emit_current_scope(cc);
  if (e < 0) goto ERR;

  defer_pop_scope(cc);
  scope_pop(cc);

  // Still inside the body, JMP over all other branches
  // since we're done executing the body of the if statement
  e = emit_and_record_jmp(cc, KIT_IR_OPCODE_JMP, -1, end_label); // JUMP!
  if (e < 0) goto ERR;

  // ELSE IFS
  for (u32 else_if_idx = 0; else_if_idx < KIT_GET_NODE(cc->ast, node)->if_stmt.nelse_ifs; else_if_idx++) {
    // Emit the next in chain label for instructions to jump to.
    define_and_emit_label(cc, next_in_chain_label);
    next_in_chain_label = make_label_id(cc);

    scope_push(cc);

    // dont worry about it dont worry about it dont worry about it dont worry about it
    struct kit_if_stmt* elif = &KIT_GET_NODE(cc->ast, node)->if_stmt.else_ifs[else_if_idx];

    // CONDITION
    kit_vreg_t elif_cond = compile(cc, elif->condition);
    if (elif_cond < 0) {
      cerror(KIT_GET_NODE(cc->ast, elif->condition)->common.span, "Failed to compile condition of else if statement [if statement]\n");
      goto ERR;
    }

    /* Failed. Jump to the next in chain. */
    e = emit_and_record_jmp(cc, KIT_IR_OPCODE_JZ, elif_cond, next_in_chain_label);
    if (e < 0) goto ERR;

    // JZ pops condition

    e = defer_push_scope(cc);
    if (e < 0) goto ERR;

    /* Condition true! Execute the body */
    for (u32 i = 0; i < elif->nstmts; i++) {
      e = compile(cc, elif->body[i]);
      if (e < 0) {
        cerror(KIT_GET_NODE(cc->ast, elif->body[i])->common.span, "Failed to compile body of else if statement [if statement]\n");
        goto ERR;
      }
    }

    e = defer_emit_current_scope(cc);
    if (e < 0) goto ERR;

    defer_pop_scope(cc);

    scope_pop(cc);

    /* JMP over all other branches. */
    e = emit_and_record_jmp(cc, KIT_IR_OPCODE_JMP, -1, end_label); // skip remaining elseifs and else
    if (e < 0) goto ERR;
  }

  /* Emit the final next in chain label for the else statement. */
  define_and_emit_label(cc, next_in_chain_label); // BAM!

  scope_push(cc);

  e = defer_push_scope(cc);
  if (e < 0) goto ERR;

  /* ELSE BODY */
  int* else_body = KIT_GET_NODE(cc->ast, node)->if_stmt.else_body;
  for (u32 i = 0; i < KIT_GET_NODE(cc->ast, node)->if_stmt.nelse_stmts; i++) {
    e = compile(cc, else_body[i]);
    if (e < 0) {
      cerror(KIT_GET_NODE(cc->ast, else_body[i])->common.span, "Failed to compile body of else statement [if statement]\n");
      goto ERR;
    }
    /* No need to jump! we're already at the end :> */
  }

  e = defer_emit_current_scope(cc);
  if (e < 0) goto ERR;

  defer_pop_scope(cc);
  scope_pop(cc);

  /* END LABEL. There's still one instruction after this and it's to ensure we always pop our variables. */
  define_and_emit_label(cc, end_label);

  /* Pop scope. */

  return 0;

ERR:
  return e ? e : -1;
}