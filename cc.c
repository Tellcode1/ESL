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

#include "cc.h"

#include "ast.h"
#include "bc.h"
#include "bfunc.h"
#include "cerr.h"
#include "fn.h"
#include "label.h"
#include "lval.h"
#include "pool.h"
#include "rwhelp.h"
#include "stack.h"
#include "stdafx.h"
#include "var.h"

#include <error.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Operators like SUB (-) can be used
 * as unary operators and that would add confusion
 * to the parser.
 * Only supports binary operators.
 * Unary operators must be handled seperately.
 */
static inline e_opcode
e_binary_operator_to_opcode(e_operator op)
{
  switch (op) {
    case E_OPERATOR_ADD: return E_OPCODE_ADD;
    case E_OPERATOR_SUB: return E_OPCODE_SUB;
    case E_OPERATOR_MUL: return E_OPCODE_MUL;
    case E_OPERATOR_DIV: return E_OPCODE_DIV;
    case E_OPERATOR_MOD: return E_OPCODE_MOD;
    case E_OPERATOR_EXP: return E_OPCODE_EXP;
    case E_OPERATOR_AND: return E_OPCODE_AND;
    case E_OPERATOR_OR: return E_OPCODE_OR;
    case E_OPERATOR_BAND: return E_OPCODE_BAND;
    case E_OPERATOR_BOR: return E_OPCODE_BOR;
    case E_OPERATOR_XOR: return E_OPCODE_XOR;
    case E_OPERATOR_ISEQL: return E_OPCODE_EQL;
    case E_OPERATOR_ISNEQ: return E_OPCODE_NEQ;
    case E_OPERATOR_LT: return E_OPCODE_LT;
    case E_OPERATOR_LTE: return E_OPCODE_LTE;
    case E_OPERATOR_GT: return E_OPCODE_GT;
    case E_OPERATOR_GTE: return E_OPCODE_GTE;
    case E_OPERATOR_NOT: return E_OPCODE_NOT;
    case E_OPERATOR_BNOT: return E_OPCODE_BNOT;
    case E_OPERATOR_DEC: return E_OPCODE_DEC;
    case E_OPERATOR_INC: return E_OPCODE_INC;
  }
  return -1;
}

static inline int compile_literal(e_compiler* cc, int node);
static int        compile_function_definition(struct e_compiler* cc, int node);
static int        compile_function_call(struct e_compiler* cc, int node);
static int        compile_if_statement(struct e_compiler* cc, int node);
static int        compile_while_statement(struct e_compiler* cc, int node);
static int        compile_binary_op(struct e_compiler* cc, int node);
static int        compile_unary_op(struct e_compiler* cc, int node);
static int        compile(struct e_compiler* cc, int node);

static inline u32
make_label_id(e_compiler* cc)
{ return cc->next_label++; }

static inline int
emit_lvalue_assign_prologue(e_compiler* cc, e_lval lv)
{
  if (lv.type == E_LVAL_VAR) {
    return 0;
  }
  /* LVAL_INDEX handles all three of INDEX, INDEX_ASSIGN and INDEX_COMPOUND */
  else if (lv.type == E_LVAL_INDEX) {
    int e = compile(cc, lv.val.index.left_node);
    if (e < 0) return e;

    e = compile(cc, lv.val.index.index_node);
    if (e < 0) return e;

    return 0;
  }
  return -1;
}

static inline int
emit_lvalue_assign_epilogue(e_compiler* cc, e_lval lv)
{
  if (lv.type == E_LVAL_VAR) {
    e_emit_instruction(cc, E_OPCODE_ASSIGN, E_ATTR_NONE);
    e_emit_u32(cc, lv.val.var.id);
    return 0;
  }
  /* LVAL_INDEX handles all three of INDEX, INDEX_ASSIGN and INDEX_COMPOUND */
  else if (lv.type == E_LVAL_INDEX) {
    e_emit_instruction(cc, E_OPCODE_INDEX_ASSIGN, E_ATTR_NONE);
    return 0;
  }
  return -1;
}

int
e_emit_lvalue_assign(e_compiler* cc, int value, e_lval lv)
{
  int e = emit_lvalue_assign_prologue(cc, lv);
  if (e) return e;

  e = compile(cc, value);
  if (e) return e;

  e = emit_lvalue_assign_epilogue(cc, lv);
  if (e) return e;

  return 0;
}

static inline int
lower_node_to_literal(const e_ast* p, int node, e_var* o)
{
  switch (E_GET_NODE(p, node)->type) {
    case E_AST_NODE_INT: *o = (e_var){ .type = E_VARTYPE_INT, .val.i = E_GET_NODE(p, node)->i.i }; return 0;
    case E_AST_NODE_FLOAT: *o = (e_var){ .type = E_VARTYPE_FLOAT, .val.f = E_GET_NODE(p, node)->f.f }; return 0;
    case E_AST_NODE_CHAR: *o = (e_var){ .type = E_VARTYPE_CHAR, .val.c = E_GET_NODE(p, node)->c.c }; return 0;
    case E_AST_NODE_BOOL: *o = (e_var){ .type = E_VARTYPE_BOOL, .val.b = E_GET_NODE(p, node)->b.b }; return 0;
    case E_AST_NODE_STRING: {
      e_refdobj* obj = e_refdobj_pool_acquire(&ge_pool);
      if (!obj) return -1;

      E_OBJ_AS_STRING(obj)->s = strdup(E_GET_NODE(p, node)->s.s);

      *o = (e_var){ .type = E_VARTYPE_STRING, .val.s = obj };
      return 0;
    }

    case E_AST_NODE_LIST: {
      // u32 nelems = E_GET_NODE(p, node)->val.list.nelems;

      // o->type     = E_VARTYPE_LIST;
      // o->val.list = (e_list*)malloc(sizeof(e_list));

      // e_var* vars_tmp = (e_var*)malloc(sizeof(e_var) * nelems);
      // for (u32 i = 0; i < nelems; i++) {
      //   int elem_node = E_GET_NODE(p, node)->val.list.elems[i];
      //   if (lower_node_to_literal(p, elem_node, &vars_tmp[i])) { cerror(E_GET_NODE(p, elem_node)->common.span, "Failed to compile literal for list"); }
      // }

      // e_list_init(vars_tmp, nelems, o->val.list);

      // free(vars_tmp);
      // return 0;
    }

    case E_AST_NODE_MAP: /* TODO: Implement */ abort(); break;

    default: return 1;
  }
  return 1;
}

static void
ns_push(e_compiler* cc, const char* name)
{
  if (cc->ns->nnamespaces >= cc->ns->capacity) {
    u32    new_cnamespaces = cc->ns->capacity * 2;
    char** new_namespaces  = realloc(cc->ns->namespaces, new_cnamespaces * sizeof(char*));

    cc->ns->namespaces = new_namespaces;
    cc->ns->capacity   = new_cnamespaces;
  }

  cc->ns->namespaces[cc->ns->nnamespaces++] = strdup(name);
}

static void
ns_pop(e_compiler* cc)
{ free(cc->ns->namespaces[--cc->ns->nnamespaces]); }

/** */
static char*
mk_name(const e_compiler* cc, const char* name)
{
  size_t len = strlen(name) + 1;

  for (u32 i = 0; i < cc->ns->nnamespaces; i++) {
    len += strlen(cc->ns->namespaces[i]) + 2; // ::
  }

  char* out = malloc(len);
  out[0]    = '\0';

  for (u32 i = 0; i < cc->ns->nnamespaces; i++) {
    strcat(out, cc->ns->namespaces[i]);
    strcat(out, "::");
  }

  strcat(out, name);
  return out;
}

static inline int
compile_literal_variable(e_compiler* cc, e_var v)
{
  if (cc->nliterals >= cc->cliterals) {
    size_t new_c              = cc->cliterals * 2;
    e_var* new_literals       = realloc(cc->literals, sizeof(e_var) * new_c);
    u16*   new_literal_hashes = realloc(cc->literal_hashes, sizeof(u16) * new_c);
    if (!new_literals || !new_literal_hashes) {
      free(new_literals); // free(NULL) = noop
      free(new_literal_hashes);
      return -1;
    }

    cc->literals       = new_literals;
    cc->literal_hashes = new_literal_hashes;
    cc->cliterals      = new_c;
  }

  bool found = false;
  u16  id    = cc->nliterals;

  /* Search for the literal in our table. */
  u16 hash = (u16)e_var_hash(&v);
  for (u32 i = 0; i < cc->nliterals; i++) {
    if (cc->literal_hashes[i] == hash && e_var_equal(&v, &cc->literals[i])) {
      id    = i;
      found = true;
      break;
    }
  }

  if (!found) {
    // Add it to our list
    id = cc->nliterals;

    cc->literals[id]       = v;
    cc->literal_hashes[id] = e_var_hash(&v);
    cc->nliterals++;
  } else {
    // e_var_free(pool, &v); // free discarded variable
    if (v.type == E_VARTYPE_STRING) {
      free(E_VAR_AS_STRING(&v)->s);
      e_refdobj_pool_return(&ge_pool, v.val.s);
    }

    /**
     * Since the variables all have the same lifetime, using the refcounter
     * here would just complicate things (need to store more literals
     * on the compilers literal table, then free them).
     * What we do instead is:
     * Create all the literals during compilation.
     * Free all the literals after.
     * Done!
     */
    // e_var_acquire(&cc->literals[id]); // refcounter++ for the reused variable
  }

  // printf("LOADK[%u], ATTR_NONE[%u], IDX[%u]\n", E_OPCODE_LITERAL, E_ATTR_NONE, cc->nliterals);
  e_emit_instruction(cc, E_OPCODE_LITERAL, E_ATTR_NONE);
  e_emit_u16(cc, id);

  return 0;
}

static inline int
compile_literal(e_compiler* cc, int node)
{
  e_var v = { 0 };
  if (lower_node_to_literal(cc->ast, node, &v)) return -1;

  return compile_literal_variable(cc, v);
}

static int
compile_function_definition(struct e_compiler* cc, int node)
{
  const char* function_name = E_GET_NODE(cc->ast, node)->func.name;
  char*       full          = mk_name(cc, function_name);

  const u32 hash = e_hash_fnv(full, strlen(full));

  free(full);

  /**
   * Push frame for the stack
   */
  e_stack_push_frame(cc->stack);

  /* Ensure it doesn't already exist */
  for (u32 i = 0; i < cc->functions_count; i++) {
    if (cc->functions[i].name_hash == hash) {
      _UNREACHABLE();
      cerror(E_GET_NODE(cc->ast, node)->common.span, "Multiple definitions of function \"%s\"\n", function_name);
      return -1;
    }
  }

  u32 nargs = E_GET_NODE(cc->ast, node)->func.nargs;

  u32* arg_slots = nullptr;
  if (nargs > 0) {
    arg_slots = (u32*)malloc(sizeof(u32) * nargs);
    for (u32 i = 0; i < nargs; i++) {
      const char* arg_name = E_GET_NODE(cc->ast, node)->func.args[i];
      u32         arg_hash = e_hash_fnv(arg_name, strlen(arg_name));

      arg_slots[i] = arg_hash;

      /* Add variable entry to stack */
      e_var* r = e_stack_push_variable(arg_hash, cc->stack);

      /* Acquire a refdobj */
      r->val.compinfo                 = e_refdobj_pool_acquire(&ge_pool);
      E_VAR_AS_INFO(r)->initializer   = -1; // Arguments aren't initialized.
      E_VAR_AS_INFO(r)->current_value = -1; // Or initialized to void if you think about it.
      E_VAR_AS_INFO(r)->hash          = arg_hash;
      E_VAR_AS_INFO(r)->span          = E_GET_NODE(cc->ast, node)->common.span;
      E_VAR_AS_INFO(r)->is_const      = false; // User can override the argument any time.
    }
  }

  const u32 init_code_capacity = 256;

  struct e_compiler copy = {
    .ast                = cc->ast,
    .loop               = nullptr, // reset loop on function.
    .literals           = cc->literals,
    .literal_hashes     = cc->literal_hashes,
    .nliterals          = cc->nliterals,
    .cliterals          = cc->cliterals,
    .ns                 = cc->ns,
    .stack              = cc->stack,
    .emit               = (u8*)malloc(init_code_capacity),
    .emitted            = 0,
    .code_capacity      = init_code_capacity,
    .functions          = cc->functions,
    .functions_capacity = cc->functions_capacity,
    .functions_count    = cc->functions_count,
  };

  int e = e_compile_function(&copy, node);

  /* Always return void if no other return value was specified */
  e_emit_instruction(&copy, E_OPCODE_RETURN, E_ATTR_NONE);
  e_emit_u8(&copy, false);

  cc->literals           = copy.literals;
  cc->literal_hashes     = copy.literal_hashes;
  cc->nliterals          = copy.nliterals;
  cc->cliterals          = copy.cliterals;
  cc->functions          = copy.functions;
  cc->ns                 = copy.ns;
  cc->functions_count    = copy.functions_count;
  cc->functions_capacity = copy.functions_capacity;

  e_function f = {
    .code      = copy.emit,
    .code_size = copy.emitted,
    .arg_slots = arg_slots,
    .name_hash = hash,
    .nargs     = nargs,
  };

  if (cc->functions_count >= cc->functions_capacity) {
    u32         new_capacity  = cc->functions_capacity * 2;
    e_function* new_functions = realloc(cc->functions, sizeof(e_function) * new_capacity);
    if (!new_functions) return -1;

    cc->functions          = new_functions;
    cc->functions_capacity = new_capacity;
  }

  cc->functions[cc->functions_count] = f;
  cc->functions_count++;

  e_stack_pop_frame(cc->stack);

  return e;
}

static int
compile_binary_op(struct e_compiler* cc, int node)
{
  int e = 0;

  bool is_compound = E_GET_NODE(cc->ast, node)->binaryop.is_compound;
  int  left        = E_GET_NODE(cc->ast, node)->binaryop.left;
  int  right       = E_GET_NODE(cc->ast, node)->binaryop.right;

  e_opcode opcode = e_binary_operator_to_opcode(E_GET_NODE(cc->ast, node)->binaryop.op);
  if (opcode < 0) {
    cerror(E_GET_NODE(cc->ast, node)->common.span, "Operator %u can not be used as a binary operator\n", E_GET_NODE(cc->ast, node)->binaryop.op);
    return -1;
  }

  if (is_compound && !e_can_make_value(cc->ast, left)) {
    cerror(E_GET_NODE(cc->ast, left)->common.span, "Can not assign to left\n");
    return -1;
  }

  e_lval lv = { 0 };
  if (is_compound) {
    // Verified  earlier that we can make it into an lvalue
    lv = e_make_value(cc->ast, left);

    /* Load data to point to where we want to assign it (Not stack information!) */
    e = emit_lvalue_assign_prologue(cc, lv);
    if (e) return e;

    /* Load left */
    e = e_emit_lvalue_load(cc, lv);
    if (e) return e;

    /* Load right */
    e = compile(cc, right);
    if (e) return e;

    /* Emit operator */
    e_emit_instruction(cc, opcode, E_ATTR_NONE);

    /* Emit actual assign instruction (takes value produced earlier and assigns it) */
    emit_lvalue_assign_epilogue(cc, lv);
  } else {
    e = compile(cc, left);
    if (e) return e;

    e = compile(cc, right);
    if (e) return e;

    e_emit_instruction(cc, opcode, E_ATTR_NONE);
  }

  return e;
}

static int
compile_unary_op(struct e_compiler* cc, int node)
{
  int e = compile(cc, E_GET_NODE(cc->ast, node)->unaryop.right);
  if (e) return e;

  e_opcode opcode = -1;

  // clang-format off
      switch (E_GET_NODE(cc->ast, node)->unaryop.op)
      {
        case E_OPERATOR_NOT: opcode = E_OPCODE_NOT; break;
        case E_OPERATOR_BNOT: opcode = E_OPCODE_BNOT; break;
        case E_OPERATOR_INC: opcode = E_OPCODE_INC; break;
        case E_OPERATOR_DEC: opcode = E_OPCODE_DEC; break;
        case E_OPERATOR_SUB: opcode = E_OPCODE_NEG; break;
        default: cerror(E_GET_NODE(cc->ast, node)->common.span, "Operator %u can not be used as a unary operator\n", E_GET_NODE(cc->ast, node)->unaryop.op); return -1;
      }
  // clang-format on

  e_attr attrs = E_ATTR_NONE;
  if (E_GET_NODE(cc->ast, node)->unaryop.is_compound) { attrs |= E_ATTR_COMPOUND; }

  e_emit_instruction(cc, opcode, attrs);

  if (E_GET_NODE(cc->ast, node)->unaryop.is_compound) {
    int        right_node = E_GET_NODE(cc->ast, node)->unaryop.right;
    e_filespan span       = E_GET_NODE(cc->ast, right_node)->common.span;

    if (!e_can_make_value(cc->ast, right_node)) {
      cerror(span, "Can not lower right to lvalue. Are you sure it's a modifiable variable?\n");
      return -1;
    }

    e_emit_u32(cc, e_make_value(cc->ast, right_node).val.var.id);
  }

  return 0;
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
get_builtin_func(const char* name)
{
  for (size_t i = 0; i < E_ARRLEN(eb_funcs); i++) {
    if (strcmp(eb_funcs[i].name, name) == 0) return &eb_funcs[i];
  }
  return nullptr;
}

static int
compile_function_call(struct e_compiler* cc, int node)
{
  e_filespan function_span = E_GET_NODE(cc->ast, node)->common.span;
  u32        nargs         = E_GET_NODE(cc->ast, node)->call.nargs;
  int*       args          = E_GET_NODE(cc->ast, node)->call.args;

  const char* function_name = E_GET_NODE(cc->ast, node)->call.function;

  char* full = mk_name(cc, function_name);
  // printf("%s\n", full);
  u32 hash = e_hash_fnv(full, strlen(full));
  free(full);

  int e = 0;
  for (u32 i = 0; i < nargs; i++) {
    e = compile(cc, args[i]);
    if (e) return e;
  }

  if (is_builtin_func(function_name)) {
    /* Validate arguments */
    const e_builtin_func* func = get_builtin_func(function_name);

    // for (u32 i = 0; i < nargs; i++)
    // {

    // }
    if (nargs > func->max_args || nargs < func->min_args) {
      cerror(function_span, "Builtin function '%s' expects between [%u-%u] arguments, but [%u] were given\n", func->name, func->min_args, func->max_args, nargs);
      return -1;
    }
  }
  // Find the function (user defined) and check if the argument count matches
  else {
    e_function* func = nullptr;
    for (u32 i = 0; i < cc->functions_count; i++) {
      if (cc->functions[i].name_hash == hash) {
        func = &cc->functions[i];
        break;
      }
    }

    if (func && func->nargs != nargs) {
      cerror(E_GET_NODE(cc->ast, node)->common.span, "User defined function '%s' expects [%u] arguments, but [%u] were given\n", function_name, func->nargs, nargs);
      return -1;
    }
  }

  e_emit_instruction(cc, E_OPCODE_CALL, E_ATTR_NONE);    // 2 bytes
  e_emit_u16(cc, E_GET_NODE(cc->ast, node)->call.nargs); // 2 bytes, number of arguments
  e_emit_u32(cc, hash);                                  // 4 bytes, function ID

  return 0;
}

// This is the dirtiest of them all...
static int
compile_if_statement(struct e_compiler* cc, int node)
{
  /* Label after if statements body */
  u32 end_label = make_label_id(cc);

  /**
   * Label of the next else if / else to jump to. Still used if no branches are present, there's
   * just a jmp instruction directly after the JMP to where the branch would be.
   */
  u32 next_in_chain_label = make_label_id(cc);

  /* Push a new scope before everything. */
  e_emit_instruction(cc, E_OPCODE_PUSH_VARIABLES, E_ATTR_NONE);
  /* For the compiler too */
  e_stack_push_frame(cc->stack);

  // condition
  int e = compile(cc, E_GET_NODE(cc->ast, node)->if_stmt.condition);
  if (e) return e;

  // condition failed :<
  e_emit_instruction(cc, E_OPCODE_JZ, E_ATTR_NONE); // Jump to the next in chain
                                                    // Possibly else if or else
  e_emit_u32(cc, next_in_chain_label);

  // BODY OF ROOT IF STATEMENT
  for (u32 i = 0; i < E_GET_NODE(cc->ast, node)->if_stmt.nstmts; i++) {
    e = compile(cc, E_GET_NODE(cc->ast, node)->if_stmt.body[i]);
    if (e) return e;
  }

  // Still inside the body, JMP over all other branches
  // since we're done executing the body of the if statement
  e_emit_instruction(cc, E_OPCODE_JMP, E_ATTR_NONE); // JUMP!
  e_emit_u32(cc, end_label);

  // ELSE IFS
  for (u32 else_if = 0; else_if < E_GET_NODE(cc->ast, node)->if_stmt.nelse_ifs; else_if++) {
    // Emit the next in chain label for instructions to jump to.
    e_emit_label(cc, next_in_chain_label);
    next_in_chain_label = make_label_id(cc);

    // dont worry about it dont worry about it dont worry about it dont worry about it
    struct e_if_stmt* if_stmt = &E_GET_NODE(cc->ast, node)->if_stmt.else_ifs[else_if];

    // CONDITION
    e = compile(cc, if_stmt->condition);
    if (e) return e;

    /* Failed. Jump to the next in chain. */
    e_emit_instruction(cc, E_OPCODE_JZ, E_ATTR_NONE);
    e_emit_u32(cc, next_in_chain_label);

    /* Condition true! Execute the body */
    for (u32 i = 0; i < if_stmt->nstmts; i++) {
      e = compile(cc, if_stmt->body[i]);
      if (e) return e;
    }

    /* JMP over all other branches. */
    e_emit_instruction(cc, E_OPCODE_JMP, E_ATTR_NONE); // skip remaining elseifs and else
    e_emit_u32(cc, end_label);
  }

  /* Emit the final next in chain label for the else statement. */
  e_emit_label(cc, next_in_chain_label); // BAM!
  /* ELSE BODY */
  for (u32 i = 0; i < E_GET_NODE(cc->ast, node)->if_stmt.nelse_stmts; i++) {
    e = compile(cc, E_GET_NODE(cc->ast, node)->if_stmt.else_body[i]);
    if (e) return e;

    /* No need to jump! we're already at the end :> */
  }

  /* END LABEL. There's still one instruction after this and it's to ensure we always pop our variables. */
  e_emit_label(cc, end_label);

  /* Pop scope. */
  e_emit_instruction(cc, E_OPCODE_POP_VARIABLES, E_ATTR_NONE);

  /* for the compieler too */
  e_stack_pop_frame(cc->stack);

  return 0;
}

static int
compile_while_statement(struct e_compiler* cc, int node)
{
  /* Push a new scope */
  e_emit_instruction(cc, E_OPCODE_PUSH_VARIABLES, E_ATTR_NONE);

  /* Computes the condition and jumps to the end label (breaks) if condition is false */
  const u32 pre_condition_label = make_label_id(cc);

  /* After the while loop, with one POP_VARIABLES to ensure we always pop our variables. */
  const u32 end_label = make_label_id(cc);

  /* Append a loop entry to our compiler. */
  ecc_loop_location loop = {
    .continue_label = pre_condition_label,
    .break_label    = end_label,
  };

  /* Ensure we don't overwrite it... */
  ecc_loop_location* last = cc->loop;
  cc->loop                = &loop;

  //
  e_emit_label(cc, pre_condition_label);
  //

  /* CONDITION */
  int e = compile(cc, E_GET_NODE(cc->ast, node)->while_stmt.condition);
  if (e) return e;

  // Break out of loop if condition is false.
  e_emit_instruction(cc, E_OPCODE_JZ, E_ATTR_NONE);
  e_emit_u32(cc, end_label);

  // WHILE BODY
  for (u32 i = 0; i < E_GET_NODE(cc->ast, node)->while_stmt.nstmts; i++) {
    e = compile(cc, E_GET_NODE(cc->ast, node)->while_stmt.stmts[i]);
    if (e) return e;
  }

  /* Jump to condition, body is done executing */
  e_emit_instruction(cc, E_OPCODE_JMP, E_ATTR_NONE);
  e_emit_u32(cc, pre_condition_label);

  // End label.
  e_emit_label(cc, end_label);

  // Pop the scope
  e_emit_instruction(cc, E_OPCODE_POP_VARIABLES, E_ATTR_NONE);

  // swap the old loop metadata back in
  cc->loop = last;

  return e;
}

static int
compile_for_statement(struct e_compiler* cc, int node)
{
  int initializers = -1;
  int condition    = -1;

  /* Push a new scope */
  e_emit_instruction(cc, E_OPCODE_PUSH_VARIABLES, E_ATTR_NONE);
  /* For the compiler too */
  e_stack_push_frame(cc->stack);

  /**
   * The for statement is compiled as:
   * Initializers (inlined)
   * TOP_LABEL:
   * Condition
   * JZ END_LABEL
   * BODY
   * ITERATOR_LABEL:
   * Iterators (inlined)
   * JMP TOP_LABEL
   * END_LABEL:
   *  (next instruction)
   */

  const u32 top_label      = make_label_id(cc);
  const u32 end_label      = make_label_id(cc);
  const u32 iterator_label = make_label_id(cc); // Needed so continue can jmp directly to iterators

  /* Append a loop entry to our compiler. */
  ecc_loop_location loop = {
    .continue_label = iterator_label,
    .break_label    = end_label,
  };

  /* Ensure we don't overwrite it... */
  ecc_loop_location* last = cc->loop;
  cc->loop                = &loop;

  // INITIALIZERS
  initializers = E_GET_NODE(cc->ast, node)->for_stmt.initializers;
  if (initializers >= 0 && compile(cc, initializers) < 0) {
    cerror(E_GET_NODE(cc->ast, initializers)->common.span, "Failed to compile initializers [for statement]\n");
    goto err;
  }

  // TOP_LABEL
  e_emit_label(cc, top_label);

  // CONDITION
  condition = E_GET_NODE(cc->ast, node)->for_stmt.condition;
  if (condition < 0) {
    // Can't use condition's span!
    cerror(E_GET_NODE(cc->ast, initializers)->common.span, "Empty for statement conditions are not currently supported [for statement]\n");
    goto err;
  }

  if (condition >= 0 && compile(cc, condition) < 0) {
    cerror(E_GET_NODE(cc->ast, condition)->common.span, "Failed to compile condition [for statement]");
    goto err;
  }

  // JZ END_LABEL
  e_emit_instruction(cc, E_OPCODE_JZ, E_ATTR_NONE);
  e_emit_u32(cc, end_label);

  // BODY
  u32 nstmts = E_GET_NODE(cc->ast, node)->for_stmt.nstmts;
  for (u32 i = 0; i < nstmts; i++) {
    int stmt = E_GET_NODE(cc->ast, node)->for_stmt.stmts[i];
    if (compile(cc, stmt) < 0) {
      cerror(E_GET_NODE(cc->ast, stmt)->common.span, "Failed to compile statement in body [for statement]");
      goto err;
    }
  }

  // ITERATOR_LABEL
  e_emit_label(cc, iterator_label);

  // ITERATORS
  u32  niterators = E_GET_NODE(cc->ast, node)->for_stmt.niterators;
  int* iterators  = E_GET_NODE(cc->ast, node)->for_stmt.iterators;
  for (u32 i = 0; i < niterators; i++) {
    if (compile(cc, iterators[i]) < 0) {
      cerror(E_GET_NODE(cc->ast, iterators[i])->common.span, "Failed to compile iterators [for statement]");
      goto err;
    }
  }

  // JMP TOP_LABEL
  e_emit_instruction(cc, E_OPCODE_JMP, E_ATTR_NONE);
  e_emit_u32(cc, top_label);

  // END_LABEL
  e_emit_label(cc, end_label);

  // Pop scope
  e_emit_instruction(cc, E_OPCODE_POP_VARIABLES, E_ATTR_NONE);
  // For the compiler too
  e_stack_pop_frame(cc->stack);

  cc->loop = last;

  return 0;

err:
  return -1;
}

static int
e_compile_member_access(e_compiler* cc, int node)
{
  int   left  = E_GET_NODE(cc->ast, node)->member_access.left;
  char* right = E_GET_NODE(cc->ast, node)->member_access.right;

  compile(cc, left);

  e_emit_instruction(cc, E_OPCODE_MEMBER_ACCESS, E_ATTR_NONE);
  e_emit_u32(cc, e_hash_fnv(right, strlen(right)));

  return 0;
}

static int
compile(struct e_compiler* cc, int node)
{
  if (node < 0) return -1;

  switch (E_GET_NODE(cc->ast, node)->type) {
    case E_AST_NODE_NOP: return 0;

    case E_AST_NODE_ROOT: {
      /**
       * Initializes all global variables and namespaced
       * variables.
       *
       * Shouldn't take lots of time if the user hasn't
       * done something extremely stupid.
       * AKA. initializing a global variable to the
       * value of a non trivial function call.
       */
      e_ast_node* root = E_GET_NODE(cc->ast, node);
      for (u32 i = 0; i < root->root.nstmts; i++) {
        int e = compile(cc, root->root.stmts[i]);
        if (e) { return e; }
      }

      const u32 main_id = e_hash_fnv("main", strlen("main"));

      /* Find main and ensure it doesn't ask for any arguments. */
      bool found = false;
      for (u32 i = 0; i < cc->functions_count; i++) {
        if (cc->functions[i].name_hash == main_id) {
          found = true;

          if (cc->functions[i].nargs != 0) {
            cerror(root->common.span, "main can not accept any arguments!\n");
            return -1;
          }

          break;
        }
      }

      if (!found) {
        cerror(root->common.span, "main undefined\n");
        return -1;
      }

      /* CALL to main */
      e_emit_instruction(cc, E_OPCODE_CALL, E_ATTR_NONE);
      e_emit_u16(cc, 0); // no arguments to main
      e_emit_u32(cc, main_id);

      return 0;
    }

    case E_AST_NODE_EXPRESSION_LIST: {
      /* Don't push a sccope */
      int e = 0;
      for (u32 i = 0; i < E_GET_NODE(cc->ast, node)->stmts.nstmts; i++) {
        e = compile(cc, E_GET_NODE(cc->ast, node)->stmts.stmts[i]);
        if (e) return e;
      }
      return 0;
    }

    case E_AST_NODE_NAMESPACE_DECL: {
      const char* name = E_GET_NODE(cc->ast, node)->namespace_decl.name;

      /* PUSH namespace */
      ns_push(cc, name);

      int* stmts  = E_GET_NODE(cc->ast, node)->namespace_decl.stmts;
      u32  nstmts = E_GET_NODE(cc->ast, node)->namespace_decl.nstmts;

      /* Initialize variables and load functions. */
      for (u32 i = 0; i < nstmts; i++) {
        int e = compile(cc, stmts[i]);
        if (e) {
          ns_pop(cc);
          return e;
        }
      }

      /* POP namespace */
      ns_pop(cc);
      return 0;
    }

    case E_AST_NODE_INDEX: {
      int e = compile(cc, E_GET_NODE(cc->ast, node)->index.base);
      if (e < 0) return e;

      e = compile(cc, E_GET_NODE(cc->ast, node)->index.index);
      if (e < 0) return e;

      e_emit_instruction(cc, E_OPCODE_INDEX, E_ATTR_NONE);
      return 0;
    }

    case E_AST_NODE_BINARYOP: {
      return compile_binary_op(cc, node);
    }

    case E_AST_NODE_IF: {
      return compile_if_statement(cc, node);
    }

    case E_AST_NODE_UNARYOP: {
      return compile_unary_op(cc, node);
    }

    case E_AST_NODE_WHILE: {
      return compile_while_statement(cc, node);
    }

    case E_AST_NODE_FOR: {
      return compile_for_statement(cc, node);
    }

    case E_AST_NODE_MEMBER_ACCESS: {
      return e_compile_member_access(cc, node);
    }

    case E_AST_NODE_BREAK: {
      if (!cc->loop) {
        cerror(E_GET_NODE(cc->ast, node)->common.span, "break used outside loop\n");
        return -1;
      }
      e_emit_instruction(cc, E_OPCODE_JMP, E_ATTR_NONE);
      e_emit_u32(cc, cc->loop->break_label);
      return 0;
    }
    case E_AST_NODE_CONTINUE: {
      if (!cc->loop) {
        cerror(E_GET_NODE(cc->ast, node)->common.span, "continue used outside loop\n");
        return -1;
      }
      e_emit_instruction(cc, E_OPCODE_JMP, E_ATTR_NONE);
      e_emit_u32(cc, cc->loop->continue_label);
      break;
    }

    case E_AST_NODE_RETURN: {
      if (E_GET_NODE(cc->ast, node)->ret.has_return_value) {
        /* Compile the return value */
        compile(cc, E_GET_NODE(cc->ast, node)->ret.expr_id);

        e_emit_instruction(cc, E_OPCODE_RETURN, E_ATTR_NONE);
        e_emit_u8(cc, true); // Specify that we're returning a value
      } else {
        e_emit_instruction(cc, E_OPCODE_RETURN, E_ATTR_NONE);
        e_emit_u8(cc, false); /* Returning void! */
      }
      return 0;
    }

    case E_AST_NODE_VARIABLE: {
      char* full = mk_name(cc, E_GET_NODE(cc->ast, node)->ident.ident);
      u32   hash = e_hash_fnv(full, strlen(full));

      e_var* exists = e_stack_find(cc->stack, hash);
      if (exists == nullptr) {
        cerror(E_GET_NODE(cc->ast, node)->common.span, "Undeclared variable '%s'\n", full);
        return -1;
      }

      free(full);

      if (e_emit_lvalue_load(cc, e_make_value(cc->ast, node)) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_VARIABLE_DECL: {
      const char* name = E_GET_NODE(cc->ast, node)->let.name;
      char*       full = mk_name(cc, name);

      u32 hash        = e_hash_fnv(full, strlen(full));
      int initializer = E_GET_NODE(cc->ast, node)->let.initializer;

      e_var* exists = e_stack_find_in_current_scope(cc->stack, hash);
      if (exists != nullptr) {
        cerror(
            E_GET_NODE(cc->ast, node)->common.span,
            "Variable %s redeclared in same scope. Earlier occurence at [%s:%i:%i]\n",
            full,
            E_VAR_AS_INFO(exists)->span.file,
            E_VAR_AS_INFO(exists)->span.line,
            E_VAR_AS_INFO(exists)->span.col);
        return -1;
      }

      free(full);

      /* Add variable entry to stack */
      e_var* r = e_stack_push_variable(hash, cc->stack);

      /* Acquire a refdobj */
      r->val.compinfo                 = e_refdobj_pool_acquire(&ge_pool);
      E_VAR_AS_INFO(r)->initializer   = initializer;
      E_VAR_AS_INFO(r)->current_value = initializer; // current value is initializer, -1 if none
      E_VAR_AS_INFO(r)->hash          = hash;
      E_VAR_AS_INFO(r)->span          = E_GET_NODE(cc->ast, node)->common.span;
      E_VAR_AS_INFO(r)->is_const      = E_GET_NODE(cc->ast, node)->let.is_const;

      int    e     = 0;
      e_attr attrs = E_ATTR_NONE;
      if (initializer >= 0) {
        e = compile(cc, initializer);
        attrs |= E_ATTR_COMPOUND; // Compound sets variable value to top of stack.
      }
      if (e) return e;

      e_emit_instruction(cc, E_OPCODE_INIT, attrs);
      e_emit_u32(cc, hash);

      return 0;
    }

    case E_AST_NODE_ASSIGN: {
      int right = E_GET_NODE(cc->ast, node)->assign.right;
      int left  = E_GET_NODE(cc->ast, node)->assign.left;

      if (!e_can_make_value(cc->ast, left)) {
        e_filespan left_span = E_GET_NODE(cc->ast, left)->common.span;
        cerror(left_span, "Can not assign to left: Failed to lower to lvalue\n");
        return -1;
      }

      int e = compile(cc, right);
      if (e) return e;

      e_lval lv = e_make_value(cc->ast, left);

      e_var* exists = e_stack_find(cc->stack, lv.val.var.id);
      if (exists == nullptr) {
        cerror(*lv.span, "Undeclared variable '%s'\n", lv.val.var.name);
        return -1;
      }

      if (E_VAR_AS_INFO(exists)->is_const) {
        cerror(*lv.span, "Can not assign to const qualified variable '%s'\n", lv.val.var.name);
        return -1;
      }

      return e_emit_lvalue_assign(cc, right, lv);
    }

    case E_AST_NODE_INDEX_ASSIGN: {
      int value = E_GET_NODE(cc->ast, node)->index_assign.value;

      if (!e_can_make_value(cc->ast, node)) {
        e_filespan left_span = E_GET_NODE(cc->ast, node)->common.span;
        cerror(left_span, "Can not assign to indexed expression: Failed to lower to lvalue\n");
        return -1;
      }

      e_lval v = e_make_value(cc->ast, node);
      e_emit_lvalue_assign(cc, value, v);
      return 0;
    }

    case E_AST_NODE_INDEX_COMPOUND_OP: {
      int value = E_GET_NODE(cc->ast, node)->index_compound.value;

      if (!e_can_make_value(cc->ast, node)) {
        e_filespan left_span = E_GET_NODE(cc->ast, node)->common.span;
        cerror(left_span, "Can not assign to indexed compound expression: Failed to lower to lvalue\n");
        return -1;
      }

      e_lval v = e_make_value(cc->ast, node);
      e_emit_lvalue_assign(cc, value, v);

      return 0;
    }

    case E_AST_NODE_CALL: {
      return compile_function_call(cc, node);
    }

    case E_AST_NODE_INT:
    case E_AST_NODE_CHAR:
    case E_AST_NODE_BOOL:
    case E_AST_NODE_STRING:
    case E_AST_NODE_FLOAT:
    case E_AST_NODE_MAP: {
      return compile_literal(cc, node);
    }

    case E_AST_NODE_LIST: {
      u32 nelems = E_GET_NODE(cc->ast, node)->list.nelems;
      for (u32 i = 0; i < nelems; i++) {
        int elem_node = E_GET_NODE(cc->ast, node)->list.elems[i];
        compile(cc, elem_node);
      }

      e_emit_instruction(cc, E_OPCODE_MK_LIST, E_ATTR_NONE);
      e_emit_u32(cc, nelems);

      return 0;
    }

    case E_AST_NODE_FUNCTION_DEFINITION: {
      return compile_function_definition(cc, node);
    }
  }

  return -1;
}

int
e_compile_function(e_compiler* cc, int node)
{
  u32  nstmts = E_GET_NODE(cc->ast, node)->func.nstmts;
  int* stmts  = E_GET_NODE(cc->ast, node)->func.stmts;
  for (u32 i = 0; i < nstmts; i++) {
    int e = compile(cc, stmts[i]);
    if (e) return e;
  }
  return 0;
}

int
e_compile(struct e_ast* ast, int root_node, e_compilation_result* result)
{
  const u32 init_code_capacity       = 256;
  const u32 init_literal_capacity    = 64;
  const u32 init_function_capacity   = 64;
  const u32 init_namespaces_capacity = 8;

  ecc_namespace_stack ns = {
    .namespaces  = malloc(sizeof(char*) * init_namespaces_capacity),
    .nnamespaces = 0,
    .capacity    = init_namespaces_capacity,
  };

  e_stack stack;

  e_compiler cc = {
    .ast                = ast,
    .loop               = NULL,
    .literals           = (e_var*)malloc(sizeof(e_var) * init_literal_capacity),
    .literal_hashes     = (u16*)malloc(sizeof(u16) * init_literal_capacity),
    .nliterals          = 0,
    .ns                 = &ns,
    .stack              = &stack,
    .cliterals          = init_literal_capacity,
    .emit               = (u8*)malloc(init_code_capacity),
    .emitted            = 0,
    .code_capacity      = init_code_capacity,
    .functions_capacity = init_function_capacity,
    .functions_count    = 0,
    .functions          = malloc(sizeof(e_function) * init_function_capacity),
  };

  if (e_stack_init(256, 32, 32, &stack) < 0) return -1;

  int e = compile(&cc, root_node);
  if (e) return e;

  /* Resolve all labels after compilation. Ensure this is the last optimization / cleanup function called! */
  e_resolve_labels(cc.emit, cc.emitted);
  for (size_t i = 0; i < cc.functions_count; i++) { e_resolve_labels(cc.functions[i].code, cc.functions[i].code_size); } // resolve all function labels

  if (result) {
    result->literals      = cc.literals;
    result->nliterals     = cc.nliterals;
    result->functions     = cc.functions;
    result->nfunctions    = cc.functions_count;
    result->ninstructions = cc.emitted;
    result->instructions  = cc.emit;
  }

  free(cc.literal_hashes);
  free(ns.namespaces);
  e_stack_free(&stack);

  // e_print_instruction_stream(cc.emit, cc.emitted);

  return e;
}