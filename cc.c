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

#include "arena.h"
#include "ast.h"
#include "bc.h"
#include "bfunc.h"
#include "bstructs.h"
#include "bvar.h"
#include "cerr.h"
#include "fn.h"
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

static inline e_opcode e_binary_operator_to_opcode(e_operator op);
static char*           mk_name(const e_compiler* cc, const char* name);

static inline ecc_defer_scope* defer_push_scope(e_compiler* cc);
static inline void             defer_pop_scope(e_compiler* cc);
static inline int              defer_emit_current_scope(e_compiler* cc);
static inline int              defer_emit_all_scopes(e_compiler* cc);
static inline u32              defer_get_current_depth(e_compiler* cc);
static inline int              defer_emit_to_depth(e_compiler* cc, u32 target_depth);

static inline u32  label_find(u32 label_id, const ecc_label_table* table);
static inline void emit_and_record_jmp(e_compiler* cc, e_opcode opcode, u32 label_id);
static inline void define_and_emit_label(e_compiler* cc, u32 label_id);

static inline void                   append_defer_entry(e_compiler* cc, int* exprs, u32 nexprs);
static int                           append_function_entry(e_arena* a, ecc_function_table* funcs, const e_function* func);
static int                           append_struct_decleration(e_arena* a, const char* name, ecc_struct_information* deposit);
static inline ecc_label_jumps_table* append_label_entry(e_arena* a, u32 label_id, ecc_label_table* table);

static int collect_struct_declerations(e_compiler* cc, int* stmts, u32 nstmts, ecc_struct_information* deposit);

static inline int  compiler_make_fork(const e_compiler* old_c, e_compiler* new_c);
static inline void compiler_join_fork(const e_compiler* copy, e_compiler* cc);

static inline int emit_lvalue_assign_prologue(e_compiler* cc, e_lval lv);
static inline int emit_lvalue_assign_epilogue(e_compiler* cc, e_lval lv);

static int compile_literal_variable(e_compiler* cc, e_var v);
static int compile_literal(e_compiler* cc, int node);
static int compile_list(e_compiler* cc, int node);
static int compile_map(e_compiler* cc, int node);
static int compile_function_definition(e_compiler* cc, int node);
static int compile_binary_op(e_compiler* cc, int node);
static int compile_unary_op(e_compiler* cc, int node);
static int compile_index(e_compiler* cc, int node);
static int compile_index_assign(e_compiler* cc, int node);
static int compile_index_compound(e_compiler* cc, int node);
static int compile_function_call(e_compiler* cc, int node);
static int compile_if_statement(e_compiler* cc, int node);
static int compile_while_statement(e_compiler* cc, int node);
static int compile_for_statement(e_compiler* cc, int node);
static int compile_member_access(e_compiler* cc, int node);
static int compile_assign(e_compiler* cc, int node);
static int compile_return(e_compiler* cc, int node);
static int compile_struct_constructor(e_compiler* fork, e_filespan span, const ecc_struct_information* struc);
static int compile_struct_decleration(e_compiler* cc, int node);
static int compile_variable_decleration(e_compiler* cc, int node);
static int compile_variable_load(e_compiler* cc, int node);
static int compile_namespace_decleration(e_compiler* cc, int node);
static int compile_builtin_structure(e_compiler* cc, const e_builtin_struct* b);
static int compile_builtin_structures(e_compiler* cc);
static int compile_root(e_compiler* cc, int node);
static int compile(e_compiler* cc, int node);

static inline ecc_defer_scope*
defer_push_scope(e_compiler* cc)
{
  const u32 init_defer_entry_capacity = 8;

  ecc_defer_scope* scope = malloc(sizeof(ecc_defer_scope));
  scope->entries         = malloc(sizeof(ecc_defer_entry) * init_defer_entry_capacity);
  scope->count           = 0;
  scope->capacity        = init_defer_entry_capacity;
  scope->parent          = cc->defer_stack;
  cc->defer_stack        = scope;
  return scope;
}

static inline void
defer_pop_scope(e_compiler* cc)
{
  ecc_defer_scope* scope = cc->defer_stack;
  cc->defer_stack        = scope->parent;
  free(scope->entries);
  free(scope);
}

static inline void
append_defer_entry(e_compiler* cc, int* exprs, u32 nexprs)
{
  ecc_defer_scope* scope = cc->defer_stack;
  if (scope->count >= scope->capacity) {
    scope->capacity *= 2;
    scope->entries = realloc(scope->entries, sizeof(ecc_defer_entry) * scope->capacity);
  }

  for (u32 i = 0; i < nexprs; i++) {
    if (E_GET_NODE(cc->ast, exprs[i])->type == E_AST_NODE_DEFER) {
      cerror(E_GET_NODE(cc->ast, exprs[i])->common.span, "Defer statement in another defer statement\n");
    }
  }

  scope->entries[scope->count++] = (ecc_defer_entry){ .exprs = exprs, .nexprs = nexprs };
}

// LIFO
static inline int
defer_emit_current_scope(e_compiler* cc)
{
  ecc_defer_scope* scope = cc->defer_stack;
  if (!scope) return 0;
  for (int i = (int)scope->count - 1; i >= 0; i--) {
    for (u32 j = 0; j < scope->entries[i].nexprs; j++) {
      int e = compile(cc, scope->entries[i].exprs[j]);
      if (e) return e;
      e_emit_instruction(cc, E_OPCODE_POP);
    }
  }
  return 0;
}

/**
 * Emit the deferred statements for all scopes up to now.
 */
static inline int
defer_emit_all_scopes(e_compiler* cc)
{
  ecc_defer_scope* scope = cc->defer_stack;
  while (scope) {
    for (int i = (int)scope->count - 1; i >= 0; i--) {
      for (u32 j = 0; j < scope->entries[i].nexprs; j++) {
        int e = compile(cc, scope->entries[i].exprs[j]);
        if (e) return e;
        e_emit_instruction(cc, E_OPCODE_POP);
      }
    }
    scope = scope->parent;
  }
  return 0;
}

static inline u32
defer_get_current_depth(e_compiler* cc)
{
  u32              d     = 0;
  ecc_defer_scope* scope = cc->defer_stack;
  while (scope) {
    d++;
    scope = scope->parent;
  }
  return d;
}

// flush all defers up to (but not including) depth
static inline int
defer_emit_to_depth(e_compiler* cc, u32 target_depth)
{
  ecc_defer_scope* scope = cc->defer_stack;
  u32              depth = defer_get_current_depth(cc);
  while (scope && depth > target_depth) {
    for (int i = (int)scope->count - 1; i >= 0; i--) {
      for (u32 j = 0; j < scope->entries[i].nexprs; j++) {
        int e = compile(cc, scope->entries[i].exprs[j]);
        if (e) return e;
        e_emit_instruction(cc, E_OPCODE_POP);
      }
    }
    scope = scope->parent;
    depth--;
  }
  return 0;
}

static inline void
compiler_push_scope(e_compiler* cc)
{
}

static inline void
compiler_pop_scope(e_compiler* cc)
{
}

/**
 * Returns UINT32_MAX on no find.
 */
static inline u32
label_find(u32 label_id, const ecc_label_table* table)
{
  for (u32 i = 0; i < table->labels_count; i++) {
    if (table->labels[i].label_id == label_id) { return i; }
  }
  return UINT32_MAX;
}

static inline ecc_label_jumps_table*
append_label_entry(e_arena* a, u32 label_id, ecc_label_table* table)
{
  if (table->labels_count >= table->labels_capacity) {
    size_t                 new_c     = table->labels_capacity * 2;
    ecc_label_jumps_table* new_table = e_arnrealloc(a, table->labels, new_c * sizeof(ecc_label_jumps_table));
    if (!new_table) return nullptr;

    table->labels          = new_table;
    table->labels_capacity = new_c;
  }

  u32 index = label_find(label_id, table);

  ecc_label_jumps_table* end = nullptr;
  if (index == UINT32_MAX) {
    // Add the entry at the end
    end = &table->labels[table->labels_count];
    memset(end, 0, sizeof(*end)); // zero it out for safe

    table->labels_count++;
  } else {
    end = &table->labels[index];
  }

  return end;
}

/**
 * Add the jump to the label's stream.
 * opcode is needed because there are multiple
 * jump instructions (JMP,JE,JNE,JZ,JNZ,etc.)
 */
static inline void
emit_and_record_jmp(e_compiler* cc, e_opcode opcode, u32 label_id)
{
  ecc_label_jumps_table* label = append_label_entry(cc->arena, label_id, cc->label_table);

  if (!label->jumps_target_offsets) {
    *label = (ecc_label_jumps_table){
      .label_id             = label_id,
      .defined              = false,
      .label_offset         = 0,
      .jumps_count          = 0,
      .jumps_capacity       = 64,
      .jumps_target_offsets = e_arnalloc(cc->arena, sizeof(u32) * 64),
    };
  }

  e_emit_instruction(cc, opcode);

  u32 patch_offset = cc->num_bytes_emitted;

  if (label->defined) {
    e_emit_u32(cc, label->label_offset);
  } else {
    if (label->jumps_count >= label->jumps_capacity) {
      size_t new_c                = label->jumps_capacity * 2;
      label->jumps_target_offsets = e_arnrealloc(cc->arena, label->jumps_target_offsets, sizeof(u32) * new_c);
      label->jumps_capacity       = new_c;
    }

    label->jumps_target_offsets[label->jumps_count++] = patch_offset;

    e_emit_u32(cc, 0xDEADBEEF);
  }
}

static inline void
define_and_emit_label(e_compiler* cc, u32 label_id)
{
  e_emit_u8(cc, E_OPCODE_LABEL);
  e_emit_u32(cc, label_id);

  u32 destination_offset = cc->num_bytes_emitted;

  ecc_label_jumps_table* label = append_label_entry(cc->arena, label_id, cc->label_table);

  if (!label->jumps_target_offsets) {
    *label = (ecc_label_jumps_table){
      .label_id             = label_id,
      .defined              = true,
      .label_offset         = destination_offset,
      .jumps_count          = 0,
      .jumps_capacity       = 64,
      .jumps_target_offsets = e_arnalloc(cc->arena, sizeof(u32) * 64),
    };
    return;
  }

  label->defined      = true;
  label->label_offset = destination_offset;

  for (u32 i = 0; i < label->jumps_count; i++) {
    u32 patch_offset = label->jumps_target_offsets[i];
    memcpy(cc->emit + patch_offset, &destination_offset, sizeof(u32));
  }

  // patched every jump up.
  label->jumps_count = 0;
}

static inline int
compiler_make_fork(const e_compiler* old_c, e_compiler* new_c)
{
  const u32 init_code_capacity = 512;
  *new_c                       = (e_compiler){
    .arena             = old_c->arena,
    .ast               = old_c->ast,
    .info              = old_c->info,
    .loop              = nullptr, // reset loop on function.
    .lit_table         = old_c->lit_table,
    .builtin_var_table = old_c->builtin_var_table,
    .function_table    = old_c->function_table,
    .label_table       = old_c->label_table,
    .next_label        = old_c->next_label,
    .ns                = old_c->ns,
    .stack             = old_c->stack,
    .emit              = (u8*)malloc(init_code_capacity),
    .num_bytes_emitted = 0,
    .emit_capacity     = init_code_capacity,
  };
  return new_c->emit ? 0 : -1;
}

static inline void
compiler_join_fork(const e_compiler* copy, e_compiler* cc)
{
  /* The tables are stored on the main compile function stack. Their address SHOULD NOT change. */
  if (cc->lit_table != copy->lit_table || cc->builtin_var_table != copy->builtin_var_table || cc->function_table != copy->function_table) {
    puts("Compiler structure corrupted");
    abort();
  }

  cc->lit_table         = copy->lit_table;
  cc->builtin_var_table = copy->builtin_var_table;
  cc->function_table    = copy->function_table;

  cc->next_label = copy->next_label;

  cc->ns = copy->ns;
  /* Can't modify builtin variable count */
}

static inline u32
make_label_id(e_compiler* cc)
{ return cc->next_label++; }

e_lval
e_make_value(e_compiler* cc, int node)
{
  switch (E_GET_NODE(cc->ast, node)->type) {
    case E_AST_NODE_VARIABLE: {
      char* name = mk_name(cc, E_GET_NODE(cc->ast, node)->ident.ident);

      e_lval l;
      l.span         = &E_GET_NODE(cc->ast, node)->common.span;
      l.type         = E_LVAL_VAR;
      l.val.var.id   = e_hash_fnv(name, strlen(name));
      l.val.var.name = name;
      return l;
    }

    case E_AST_NODE_INDEX: {
      e_lval l;
      l.span                 = &E_GET_NODE(cc->ast, node)->common.span;
      l.type                 = E_LVAL_INDEX;
      l.val.index.left_node  = E_GET_NODE(cc->ast, node)->index.base;
      l.val.index.index_node = E_GET_NODE(cc->ast, node)->index.index;
      return l;
    }

    case E_AST_NODE_INDEX_ASSIGN: {
      e_lval l;
      l.span                 = &E_GET_NODE(cc->ast, node)->common.span;
      l.type                 = E_LVAL_INDEX;
      l.val.index.left_node  = E_GET_NODE(cc->ast, node)->index_assign.base;
      l.val.index.index_node = E_GET_NODE(cc->ast, node)->index_assign.index;
      return l;
    }

    case E_AST_NODE_INDEX_COMPOUND_OP: {
      e_lval l;
      l.span                 = &E_GET_NODE(cc->ast, node)->common.span;
      l.type                 = E_LVAL_INDEX;
      l.val.index.left_node  = E_GET_NODE(cc->ast, node)->index_compound.base;
      l.val.index.index_node = E_GET_NODE(cc->ast, node)->index_compound.index;
      return l;
    }

    case E_AST_NODE_MEMBER_ACCESS: {
      e_lval l;
      l.span              = &E_GET_NODE(cc->ast, node)->common.span;
      l.type              = E_LVAL_MEMBER;
      l.val.member.base   = E_GET_NODE(cc->ast, node)->member_access.left;
      l.val.member.member = E_GET_NODE(cc->ast, node)->member_access.right;
      return l;
    }

    case E_AST_NODE_MEMBER_ASSIGN: {
      e_lval l;
      l.span              = &E_GET_NODE(cc->ast, node)->common.span;
      l.type              = E_LVAL_MEMBER;
      l.val.member.base   = E_GET_NODE(cc->ast, node)->member_assign.left;
      l.val.member.member = E_GET_NODE(cc->ast, node)->member_assign.right;
      return l;
    }

    default:
      cerror(E_GET_NODE(cc->ast, node)->common.span, "%i can not be represented as a value (it is %u)\n", node, E_GET_NODE(cc->ast, node)->type);
      exit(-1);
  }

  return (e_lval){ .type = E_LVAL_UNKNOWN };
}

void
e_free_value(e_lval* lv)
{
  if (lv->type == E_LVAL_VAR) { /* free(lv->val.var.name); */
  }
}

/* Returns 0 on succcess. */
static inline int
emit_lvalue_assign_prologue(e_compiler* cc, e_lval lv)
{
  if (lv.type == E_LVAL_VAR) {
    return 0;
  }
  /* LVAL_INDEX handles all three of INDEX, INDEX_ASSIGN and INDEX_COMPOUND */
  else if (lv.type == E_LVAL_INDEX) {
    if (!e_can_make_value(cc->ast, lv.val.index.left_node)) {
      cerror(*lv.span, "Can not assign back to indexed structure\n");
      return -1;
    }

    e_lval left = e_make_value(cc, lv.val.index.left_node);
    emit_lvalue_assign_prologue(cc, left);

    // int e = compile(cc, lv.val.index.left_node);
    // if (e < 0) return e;
    int e = e_emit_lvalue_load(cc, left);
    if (e < 0) return e;

    e = compile(cc, lv.val.index.index_node);
    if (e < 0) return e;

    return 0;
  } else if (lv.type == E_LVAL_MEMBER) {
    if (!e_can_make_value(cc->ast, lv.val.member.base)) {
      cerror(*lv.span, "Can not assign back to structure member\n");
      return -1;
    }

    e_lval base = e_make_value(cc, lv.val.member.base);
    emit_lvalue_assign_prologue(cc, base);

    int e = e_emit_lvalue_load(cc, base);
    if (e) return e;

    return 0;
  }

  return -1;
}

/* Returns 0 on succcess. */
static inline int
emit_lvalue_assign_epilogue(e_compiler* cc, e_lval lv)
{
  if (lv.type == E_LVAL_VAR) {
    e_emit_instruction(cc, E_OPCODE_ASSIGN);
    e_emit_u32(cc, lv.val.var.id);
    return 0;
  }
  /* LVAL_INDEX handles all three of INDEX, INDEX_ASSIGN and INDEX_COMPOUND */
  else if (lv.type == E_LVAL_INDEX) {
    e_emit_instruction(cc, E_OPCODE_INDEX_ASSIGN);

    e_lval base = e_make_value(cc, lv.val.index.left_node);
    emit_lvalue_assign_epilogue(cc, base);
    return 0;
  } else if (lv.type == E_LVAL_MEMBER) {
    e_emit_instruction(cc, E_OPCODE_MEMBER_ASSIGN);
    e_emit_u32(cc, e_hash_fnv(lv.val.member.member, strlen(lv.val.member.member)));

    e_lval base = e_make_value(cc, lv.val.member.base);
    emit_lvalue_assign_epilogue(cc, base);

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
lower_node_to_literal(const e_compiler* cc, int node, e_var* o)
{
  e_ast* p = cc->ast;
  switch (E_GET_NODE(p, node)->type) {
    case E_AST_NODE_INT: *o = (e_var){ .type = E_VARTYPE_INT, .val.i = E_GET_NODE(p, node)->i.i }; return 0;
    case E_AST_NODE_FLOAT: *o = (e_var){ .type = E_VARTYPE_FLOAT, .val.f = E_GET_NODE(p, node)->f.f }; return 0;
    case E_AST_NODE_CHAR: *o = (e_var){ .type = E_VARTYPE_CHAR, .val.c = E_GET_NODE(p, node)->c.c }; return 0;
    case E_AST_NODE_BOOL: *o = (e_var){ .type = E_VARTYPE_BOOL, .val.b = E_GET_NODE(p, node)->b.b }; return 0;
    case E_AST_NODE_STRING: {
      e_refdobj* obj = e_refdobj_pool_acquire(&ge_pool);
      if (!obj) return -1;

      E_OBJ_AS_STRING(obj)->s = e_arnstrdup(cc->arena, E_GET_NODE(p, node)->s.s);

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
      //   if (lower_node_to_literal(p, elem_node, &vars_tmp[i])) { cerror(E_GET_NODE(p, elem_node)->common.span, "Failed to compile literal for
      //   list"); }
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

int
e_emit_lvalue_load(e_compiler* cc, e_lval lv)
{
  if (lv.type == E_LVAL_VAR) {
    // e_var* exists = e_stack_find(cc->stack, lv.val.var.id);
    // if (exists == nullptr) {
    //   cerror(lv.span ? *lv.span : (e_filespan){ 0 }, "Variable %s undeclared [variable value load]\n", lv.val.var.name);
    //   return -1;
    // }

    // This breaks for arguments. TODO: Add fix.
    // if (E_VAR_AS_INFO(exists)->initializer < 0 && E_VAR_AS_INFO(exists)->current_value < 0) {
    //   cerror(*lv.span, "Variable %s possibly uninitialized at time of use [variable value load]\n", lv.val.var.name);
    // }

    e_emit_instruction(cc, E_OPCODE_LOAD);
    e_emit_u32(cc, lv.val.var.id);

    return 0;
  } else if (lv.type == E_LVAL_INDEX) {
    int e = compile(cc, lv.val.index.left_node);
    if (e < 0) return e;

    e = compile(cc, lv.val.index.index_node);
    if (e < 0) return e;

    e_emit_instruction(cc, E_OPCODE_INDEX);

    return 0;
  } else if (lv.type == E_LVAL_MEMBER) {
    int         left  = lv.val.member.base;
    const char* right = lv.val.member.member;

    int e = compile(cc, left);
    if (e) return e;

    e_emit_instruction(cc, E_OPCODE_MEMBER_ACCESS);
    e_emit_u32(cc, e_hash_fnv(right, strlen(right)));

    return 0;
  }

  return -1;
}

static void
ns_push(e_compiler* cc, const char* name)
{
  if (cc->ns->nnamespaces >= cc->ns->capacity) {
    u32    new_cnamespaces = cc->ns->capacity * 2;
    char** new_namespaces  = e_arnrealloc(cc->arena, cc->ns->namespaces, new_cnamespaces * sizeof(char*));

    cc->ns->namespaces = new_namespaces;
    cc->ns->capacity   = new_cnamespaces;
  }

  cc->ns->namespaces[cc->ns->nnamespaces++] = e_arnstrdup(cc->arena, name);
}

static void
ns_pop(e_compiler* cc)
{
  cc->ns->nnamespaces--;
  /* free(cc->ns->namespaces[--cc->ns->nnamespaces]); */
}

/**
 * Use the namespace stack to build
 * a fully qualified name for the variable.
 */
static char*
mk_name(const e_compiler* cc, const char* name)
{
  size_t len = strlen(name) + 1;

  for (u32 i = 0; i < cc->ns->nnamespaces; i++) {
    len += strlen(cc->ns->namespaces[i]) + 2; // ::
  }

  char* out = e_arnalloc(cc->arena, len);
  out[0]    = '\0';

  char* p = out;
  for (u32 i = 0; i < cc->ns->nnamespaces; i++) {
    p = e_strlpcat(p, cc->ns->namespaces[i], out, len);
    p = e_strlpcat(p, "::", out, len);
  }

  p = e_strlpcat(p, name, out, len);
  return out;
}

static int
compile_literal_variable(e_compiler* cc, e_var v)
{
  ecc_literal_table* literals = cc->lit_table;
  if (literals->literals_count >= literals->literals_capacity) {
    size_t new_c              = literals->literals_capacity * 2;
    e_var* new_literals       = e_arnrealloc(cc->arena, literals->literals, sizeof(e_var) * new_c);
    u16*   new_literal_hashes = e_arnrealloc(cc->arena, literals->literal_hashes, sizeof(u16) * new_c);
    if (!new_literals || !new_literal_hashes) {
      // free(new_literals); // free(NULL) = noop
      // free(new_literal_hashes);
      return -1;
    }

    literals->literals          = new_literals;
    literals->literal_hashes    = new_literal_hashes;
    literals->literals_capacity = new_c;
  }

  bool found = false;
  u16  id    = literals->literals_count;

  /* Search for the literal in our table. */
  u16 hash = (u16)e_var_hash(&v);
  for (u32 i = 0; i < literals->literals_count; i++) {
    if (literals->literal_hashes[i] == hash && e_var_equal(&v, &literals->literals[i])) {
      id    = i;
      found = true;
      break;
    }
  }

  if (!found) {
    // Add it to our list
    id = literals->literals_count;

    literals->literals[id]       = v;
    literals->literal_hashes[id] = e_var_hash(&v);
    literals->literals_count++;
  } else {
    // e_var_free(pool, &v); // free discarded variable
    if (v.type == E_VARTYPE_STRING) {
      // free(E_VAR_AS_STRING(&v)->s);
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

  // printf("LOADK[%u], ATTR_NONE[%u], IDX[%u]\n", E_OPCODE_LITERAL, cc->nliterals);
  e_emit_instruction(cc, E_OPCODE_LITERAL);
  e_emit_u16(cc, id);

  return 0;
}

static int
compile_literal(e_compiler* cc, int node)
{
  e_var v = { 0 };
  if (lower_node_to_literal(cc, node, &v)) return -1;

  return compile_literal_variable(cc, v);
}

static int
compile_list(e_compiler* cc, int node)
{
  u32 nelems = E_GET_NODE(cc->ast, node)->list.nelems;
  for (u32 i = 0; i < nelems; i++) {
    int elem_node = E_GET_NODE(cc->ast, node)->list.elems[i];
    compile(cc, elem_node);
  }

  e_emit_instruction(cc, E_OPCODE_MK_LIST);
  e_emit_u32(cc, nelems);

  return 0;
}

static int
compile_map(e_compiler* cc, int node)
{
  u32 nelems = E_GET_NODE(cc->ast, node)->map.npairs;
  for (u32 i = 0; i < nelems; i++) {
    int key = E_GET_NODE(cc->ast, node)->map.keys[i];
    int val = E_GET_NODE(cc->ast, node)->map.values[i];
    compile(cc, key);
    compile(cc, val);
  }

  e_emit_instruction(cc, E_OPCODE_MK_MAP);
  e_emit_u32(cc, nelems);

  return 0;
}

static int
append_function_entry(e_arena* a, ecc_function_table* funcs, const e_function* func)
{
  if (funcs->functions_count >= funcs->functions_capacity) {
    u32         new_capacity  = funcs->functions_capacity * 2;
    e_function* new_functions = e_arnrealloc(a, funcs->functions, sizeof(e_function) * new_capacity);
    if (!new_functions) return -1;

    funcs->functions          = new_functions;
    funcs->functions_capacity = new_capacity;
  }

  funcs->functions[funcs->functions_count] = *func;
  funcs->functions_count++;

  return 0;
}

static int
compile_function_definition(e_compiler* cc, int node)
{
  const char* function_name = E_GET_NODE(cc->ast, node)->func.name;
  char*       full          = mk_name(cc, function_name);

  const u32 hash = e_hash_fnv(full, strlen(full));

  // free(full);

  /**
   * Push frame for (our/compiler only) stack
   */
  e_stack_push_frame(cc->stack);

  /* Ensure it doesn't already exist */
  const ecc_function_table* func_table = cc->function_table;
  for (u32 i = 0; i < func_table->functions_count; i++) {
    e_function* func = &func_table->functions[i];
    if (func->name_hash == hash) {
      cerror(E_GET_NODE(cc->ast, node)->common.span, "Multiple definitions of function \"%s\"\n", function_name);
      return -1;
    }
  }

  u32 nargs = E_GET_NODE(cc->ast, node)->func.nargs;

  u32* arg_slots = nullptr;
  if (nargs > 0) {
    arg_slots = (u32*)e_arnalloc(cc->arena, sizeof(u32) * nargs);
    for (u32 i = 0; i < nargs; i++) {
      const char* arg_name = E_GET_NODE(cc->ast, node)->func.args[i];

      char* full_arg_name = mk_name(cc, arg_name);
      u32   arg_hash      = e_hash_fnv(full_arg_name, strlen(full_arg_name));
      // free(full_arg_name);

      arg_slots[i] = arg_hash;

      /* Add variable entry to stack */
      e_var* r = e_stack_push_variable(arg_hash, cc->stack);

      /* Acquire a refdobj */
      r->val.var_info                 = e_refdobj_pool_acquire(&ge_pool);
      E_VAR_AS_INFO(r)->initializer   = -1; // Arguments aren't initialized.
      E_VAR_AS_INFO(r)->current_value = -1; // Or initialized to void if you think about it.
      E_VAR_AS_INFO(r)->name_hash     = arg_hash;
      E_VAR_AS_INFO(r)->span          = E_GET_NODE(cc->ast, node)->common.span;
      E_VAR_AS_INFO(r)->is_const      = false; // User can override the argument any time.
    }
  }

  e_compiler copy;
  compiler_make_fork(cc, &copy);

  defer_push_scope(&copy);

  int e = e_compile_function(&copy, node);

  // emit the defer before the return you asshole
  defer_emit_current_scope(&copy);

  /* Always return void if no other return value was specified */
  e_emit_instruction(&copy, E_OPCODE_RETURN);
  e_emit_u8(&copy, false);

  defer_pop_scope(&copy);

  compiler_join_fork(&copy, cc);

  e_function f = {
    .code      = copy.emit,
    .code_size = copy.num_bytes_emitted,
    .arg_slots = arg_slots,
    .name_hash = hash,
    .nargs     = nargs,
  };

  e = append_function_entry(cc->arena, cc->function_table, &f);
  if (e) return e;

  e_stack_pop_frame(cc->stack);

  return e;
}

static int
compile_binary_op(e_compiler* cc, int node)
{
  e_lval lv = { 0 };
  int    e  = 0;

  bool is_compound = E_GET_NODE(cc->ast, node)->binaryop.is_compound;
  int  left        = E_GET_NODE(cc->ast, node)->binaryop.left;
  int  right       = E_GET_NODE(cc->ast, node)->binaryop.right;

  e_opcode opcode = e_binary_operator_to_opcode(E_GET_NODE(cc->ast, node)->binaryop.op);
  if (opcode < 0) {
    cerror(E_GET_NODE(cc->ast, node)->common.span, "Operator %u can not be used as a binary operator\n", E_GET_NODE(cc->ast, node)->binaryop.op);
    goto err;
  }

  if (is_compound && !e_can_make_value(cc->ast, left)) {
    cerror(E_GET_NODE(cc->ast, left)->common.span, "Can not assign to left\n");
    goto err;
  }

  if (is_compound) {
    // Verified  earlier that we can make it into an lvalue
    lv = e_make_value(cc, left);

    /* Load data to point to where we want to assign it (Not stack information!) */
    e = emit_lvalue_assign_prologue(cc, lv);
    if (e) goto err;

    /* Load left */
    e = e_emit_lvalue_load(cc, lv);
    if (e) goto err;

    /* Load right */
    e = compile(cc, right);
    if (e) goto err;

    /* Emit operator */
    e_emit_instruction(cc, opcode);

    /* Emit actual assign instruction (takes value produced earlier and assigns it) */
    emit_lvalue_assign_epilogue(cc, lv);

    e_free_value(&lv);
  } else {
    e = compile(cc, left);
    if (e) goto err;

    e = compile(cc, right);
    if (e) goto err;

    e_emit_instruction(cc, opcode);
  }

  return e;

err:
  e_free_value(&lv);
  return -1;
}

static int
compile_unary_op(e_compiler* cc, int node)
{
  e_opcode opcode = -1;

  // clang-format off
      switch (E_GET_NODE(cc->ast, node)->unaryop.op)
      {
        case E_OPERATOR_NOT: opcode = E_OPCODE_NOT; break;
        case E_OPERATOR_BNOT: opcode = E_OPCODE_BNOT; break;
        case E_OPERATOR_INC: opcode = E_OPCODE_INC; break;
        case E_OPERATOR_DEC: opcode = E_OPCODE_DEC; break;
        case E_OPERATOR_SUB: opcode = E_OPCODE_NEG; break;
        case E_OPERATOR_ADD: opcode = E_OPCODE_NOOP; break;
        default: cerror(E_GET_NODE(cc->ast, node)->common.span, "Operator %u can not be used as a unary operator\n", E_GET_NODE(cc->ast, node)->unaryop.op); return -1;
      }
  // clang-format on

  bool is_compound = E_GET_NODE(cc->ast, node)->unaryop.is_compound;

  int right = E_GET_NODE(cc->ast, node)->unaryop.right;
  int e     = 0;

  if (is_compound) {
    e_lval lv = { 0 };

    if (!e_can_make_value(cc->ast, right)) {
      cerror(E_GET_NODE(cc->ast, right)->common.span, "Can not assign to right\n");
      return -1;
    }

    // Verified  earlier that we can make it into an lvalue
    lv = e_make_value(cc, right);

    /* Load data to point to where we want to assign it (Not stack information!) */
    e = emit_lvalue_assign_prologue(cc, lv);
    if (e) goto err;

    /* Load right */
    e = compile(cc, right);
    if (e) goto err;

    /* Emit operator */
    e_emit_instruction(cc, opcode);
    e_emit_u8(cc, true); // is_compound flag

    /* Emit actual assign instruction (takes value produced earlier and assigns it) */
    emit_lvalue_assign_epilogue(cc, lv);

    e_free_value(&lv);
  } else {
    /* Load right */
    e = compile(cc, right);
    if (e) goto err;

    /* Emit operator */
    e_emit_instruction(cc, opcode);
    e_emit_u8(cc, false); // is_compound flag
  }

  return 0;

err:
  return -1;
}

static int
compile_index(e_compiler* cc, int node)
{
  int e = compile(cc, E_GET_NODE(cc->ast, node)->index.base);
  if (e < 0) return e;

  e = compile(cc, E_GET_NODE(cc->ast, node)->index.index);
  if (e < 0) return e;

  e_emit_instruction(cc, E_OPCODE_INDEX);
  return 0;
}

static int
compile_index_assign(e_compiler* cc, int node)
{
  int value = E_GET_NODE(cc->ast, node)->index_assign.value;

  if (!e_can_make_value(cc->ast, node)) {
    e_filespan left_span = E_GET_NODE(cc->ast, node)->index_assign.span;
    cerror(left_span, "Can not assign to indexed expression: Failed to lower to lvalue\n");
    return -1;
  }

  e_lval v = e_make_value(cc, node);
  int    e = e_emit_lvalue_assign(cc, value, v);
  e_free_value(&v);

  return e;
}

static int
compile_index_compound(e_compiler* cc, int node)
{
  int value = E_GET_NODE(cc->ast, node)->index_compound.value;

  if (!e_can_make_value(cc->ast, node)) {
    e_filespan left_span = E_GET_NODE(cc->ast, node)->common.span;
    cerror(left_span, "Can not assign to indexed compound expression: Failed to lower to lvalue\n");
    return -1;
  }

  e_lval v = e_make_value(cc, node);
  int    e = e_emit_lvalue_assign(cc, value, v);
  e_free_value(&v);

  return e;
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
compile_function_call(e_compiler* cc, int node)
{
  e_filespan function_span = E_GET_NODE(cc->ast, node)->common.span;
  u32        nargs         = E_GET_NODE(cc->ast, node)->call.nargs;
  int*       args          = E_GET_NODE(cc->ast, node)->call.args;

  const char* function_name = E_GET_NODE(cc->ast, node)->call.function_name;

  char* full = mk_name(cc, function_name);
  // printf("%s\n", full);
  u32 hash = e_hash_fnv(full, strlen(full));
  // free(full);

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
      cerror(
          function_span,
          "Builtin function '%s' expects between [%u-%u] arguments, but [%u] were given\n",
          func->name,
          func->min_args,
          func->max_args,
          nargs);
      fprintf(stderr, "[%s:%i] Builtin function is declared as: %s : %s\n", __FILE__, __LINE__, func->signature, func->desc);
      return -1;
    }
  }
  // Find the function (user defined) and check if the argument count matches
  else {
    // bool found = false;

    e_function*         func       = nullptr;
    ecc_function_table* func_table = cc->function_table;
    for (u32 i = 0; i < func_table->functions_count; i++) {
      if (func_table->functions[i].name_hash == hash) {
        func = &func_table->functions[i];
        // found = true;
        break;
      }
    }

    if (func && func->nargs != nargs) {
      cerror(
          E_GET_NODE(cc->ast, node)->common.span,
          "User defined function '%s' expects [%u] arguments, but [%u] were given\n",
          function_name,
          func->nargs,
          nargs);
      return -1;
    }

    // if (!found) { printf("Function %s not defined\n", function_name); }
  }

  e_emit_instruction(cc, E_OPCODE_CALL);                 // 2 bytes
  e_emit_u32(cc, hash);                                  // 4 bytes, function ID
  e_emit_u16(cc, E_GET_NODE(cc->ast, node)->call.nargs); // 2 bytes, number of arguments

  return 0;
}

// This is the dirtiest of them all...
static int
compile_if_statement(e_compiler* cc, int node)
{
  /* Label after if statements body */
  u32 end_label = make_label_id(cc);

  /**
   * Label of the next else if / else to jump to. Still used if no branches are present, there's
   * just a jmp instruction directly after the JMP to where the branch would be.
   */
  u32 next_in_chain_label = make_label_id(cc);

  /* Push a new scope before everything. */
  e_emit_instruction(cc, E_OPCODE_PUSH_VARIABLES);

  // condition
  int e = compile(cc, E_GET_NODE(cc->ast, node)->if_stmt.condition);
  if (e) return e;

  // condition failed :<
  emit_and_record_jmp(cc, E_OPCODE_JZ, next_in_chain_label); // Jump to the next in chain
  // Possibly else if or else

  e_stack_push_frame(cc->stack);
  e_emit_instruction(cc, E_OPCODE_PUSH_VARIABLES);
  defer_push_scope(cc);

  // BODY OF ROOT IF STATEMENT
  for (u32 i = 0; i < E_GET_NODE(cc->ast, node)->if_stmt.nstmts; i++) {
    e = compile(cc, E_GET_NODE(cc->ast, node)->if_stmt.body[i]);
    if (e) return e;
  }

  e_emit_instruction(cc, E_OPCODE_POP_VARIABLES);
  e_stack_pop_frame(cc->stack);
  defer_emit_current_scope(cc);
  defer_pop_scope(cc);

  // Still inside the body, JMP over all other branches
  // since we're done executing the body of the if statement
  emit_and_record_jmp(cc, E_OPCODE_JMP, end_label); // JUMP!

  // ELSE IFS
  for (u32 else_if = 0; else_if < E_GET_NODE(cc->ast, node)->if_stmt.nelse_ifs; else_if++) {
    // Emit the next in chain label for instructions to jump to.
    define_and_emit_label(cc, next_in_chain_label);
    next_in_chain_label = make_label_id(cc);

    // dont worry about it dont worry about it dont worry about it dont worry about it
    struct e_if_stmt* if_stmt = &E_GET_NODE(cc->ast, node)->if_stmt.else_ifs[else_if];

    // CONDITION
    e = compile(cc, if_stmt->condition);
    if (e) return e;

    /* Failed. Jump to the next in chain. */
    emit_and_record_jmp(cc, E_OPCODE_JZ, next_in_chain_label);

    e_stack_push_frame(cc->stack);
    e_emit_instruction(cc, E_OPCODE_PUSH_VARIABLES);
    defer_push_scope(cc);

    /* Condition true! Execute the body */
    for (u32 i = 0; i < if_stmt->nstmts; i++) {
      e = compile(cc, if_stmt->body[i]);
      if (e) return e;
    }

    e_stack_pop_frame(cc->stack);
    e_emit_instruction(cc, E_OPCODE_POP_VARIABLES);
    defer_emit_current_scope(cc);
    defer_pop_scope(cc);

    /* JMP over all other branches. */
    emit_and_record_jmp(cc, E_OPCODE_JMP, end_label); // skip remaining elseifs and else
  }

  /* Emit the final next in chain label for the else statement. */
  define_and_emit_label(cc, next_in_chain_label); // BAM!

  e_stack_push_frame(cc->stack);
  e_emit_instruction(cc, E_OPCODE_PUSH_VARIABLES);
  defer_push_scope(cc);

  /* ELSE BODY */
  for (u32 i = 0; i < E_GET_NODE(cc->ast, node)->if_stmt.nelse_stmts; i++) {
    e = compile(cc, E_GET_NODE(cc->ast, node)->if_stmt.else_body[i]);
    if (e) return e;

    /* No need to jump! we're already at the end :> */
  }

  e_stack_pop_frame(cc->stack);
  e_emit_instruction(cc, E_OPCODE_POP_VARIABLES);
  defer_emit_current_scope(cc);
  defer_pop_scope(cc);

  /* END LABEL. There's still one instruction after this and it's to ensure we always pop our variables. */
  define_and_emit_label(cc, end_label);

  /* Pop scope. */
  e_emit_instruction(cc, E_OPCODE_POP_VARIABLES);

  return 0;
}

static int
compile_while_statement(e_compiler* cc, int node)
{
  /* Computes the condition and jumps to the end label (breaks) if condition is false */
  const u32 pre_condition_label = make_label_id(cc);

  /* After the while loop, with one POP_VARIABLES to ensure we always pop our variables. */
  const u32 end_label = make_label_id(cc);

  /**
   * Push frame for the stack
   */
  defer_push_scope(cc);
  e_stack_push_frame(cc->stack);
  e_emit_instruction(cc, E_OPCODE_PUSH_VARIABLES);

  /* Append a loop entry to our compiler. */
  ecc_loop_location loop = {
    .continue_label = pre_condition_label,
    .break_label    = end_label,
    .defer_depth    = defer_get_current_depth(cc),
  };

  /* Ensure we don't overwrite it... */
  ecc_loop_location* last = cc->loop;
  cc->loop                = &loop;

  //
  define_and_emit_label(cc, pre_condition_label);
  //

  /* CONDITION */
  int e = compile(cc, E_GET_NODE(cc->ast, node)->while_stmt.condition);
  if (e) return e;

  // Break out of loop if condition is false.
  emit_and_record_jmp(cc, E_OPCODE_JZ, end_label);

  // WHILE BODY
  for (u32 i = 0; i < E_GET_NODE(cc->ast, node)->while_stmt.nstmts; i++) {
    e = compile(cc, E_GET_NODE(cc->ast, node)->while_stmt.stmts[i]);
    if (e) return e;
  }

  /* Jump to condition, body is done executing */
  emit_and_record_jmp(cc, E_OPCODE_JMP, pre_condition_label);

  // End label.
  define_and_emit_label(cc, end_label);

  defer_emit_current_scope(cc);
  defer_pop_scope(cc);

  /**
   * Pop frame for the stack
   */
  e_stack_pop_frame(cc->stack);

  // Pop the scope
  e_emit_instruction(cc, E_OPCODE_POP_VARIABLES);

  // swap the old loop metadata back in
  cc->loop = last;

  return e;
}

static int
compile_for_statement(e_compiler* cc, int node)
{
  int initializers = -1;
  int condition    = -1;

  /* Push a new scope */
  e_emit_instruction(cc, E_OPCODE_PUSH_VARIABLES);
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
    .defer_depth    = defer_get_current_depth(cc),
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
  define_and_emit_label(cc, top_label);

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
  emit_and_record_jmp(cc, E_OPCODE_JZ, end_label);

  defer_push_scope(cc);

  u32 old_depth = cc->loop->defer_depth;
  if (cc->loop) cc->loop->defer_depth = defer_get_current_depth(cc);

  // BODY
  u32 nstmts = E_GET_NODE(cc->ast, node)->for_stmt.nstmts;
  for (u32 i = 0; i < nstmts; i++) {
    int stmt = E_GET_NODE(cc->ast, node)->for_stmt.stmts[i];
    if (compile(cc, stmt) < 0) {
      cerror(E_GET_NODE(cc->ast, stmt)->common.span, "Failed to compile statement in body [for statement]");
      goto err;
    }
  }

  /**
   * Deposit the deferred statements before the
   * iterator region so all iterator values are
   * correct.
   */
  defer_emit_current_scope(cc);
  defer_pop_scope(cc);

  if (cc->loop) cc->loop->defer_depth = old_depth;

  // ITERATOR_LABEL
  define_and_emit_label(cc, iterator_label);

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
  emit_and_record_jmp(cc, E_OPCODE_JMP, top_label);

  // END_LABEL
  define_and_emit_label(cc, end_label);

  // For the compiler too
  e_stack_pop_frame(cc->stack);

  // Pop scope
  e_emit_instruction(cc, E_OPCODE_POP_VARIABLES);

  cc->loop = last;

  return 0;

err:
  return -1;
}

static int
compile_member_access(e_compiler* cc, int node)
{
  e_lval lv = e_make_value(cc, node);
  return e_emit_lvalue_load(cc, lv);
}

static int
compile_assign(e_compiler* cc, int node)
{
  int right = E_GET_NODE(cc->ast, node)->binaryop.right;
  int left  = E_GET_NODE(cc->ast, node)->binaryop.left;

  if (!e_can_make_value(cc->ast, left)) {
    e_filespan left_span = E_GET_NODE(cc->ast, left)->common.span;
    cerror(left_span, "Can not assign to left: Failed to lower to lvalue\n");
    return -1;
  }

  e_lval lv = e_make_value(cc, left);

  ecc_builtin_variables_table* builtin_vars_table = cc->builtin_var_table;

  e_var* exists = e_stack_find(cc->stack, lv.val.var.id);
  if (!exists
      && E_GET_NODE(cc->ast, node)->type == E_AST_NODE_VARIABLE) { // Doesn't exist and node is supposed to be a variable (Not member access or index)
    /* Check if the user is trying to modify a builtin variable. */
    for (u32 i = 0; i < builtin_vars_table->builtin_vars_count; i++) {
      if (lv.val.var.id == builtin_vars_table->builtin_var_hashes[i]) {
        cerror(E_GET_NODE(cc->ast, left)->common.span, "Attempting to modify builtin constant '%s'\n", lv.val.var.name);
        e_free_value(&lv);
        return -1;
      }
    }

    cerror(E_GET_NODE(cc->ast, left)->common.span, "Undeclared variable '%s'\n", lv.val.var.name);
    e_free_value(&lv);
    return -1;
  }

  if (exists && E_VAR_AS_INFO(exists)->is_const) {
    cerror(E_GET_NODE(cc->ast, left)->common.span, "Can not assign to const qualified variable '%s'\n", lv.val.var.name);
    e_free_value(&lv);
    return -1;
  }

  int e = e_emit_lvalue_assign(cc, right, lv);
  e_free_value(&lv);

  if (e) return e;

  return 0;
}

static int
compile_member_assign(e_compiler* cc, int node)
{
  int value = E_GET_NODE(cc->ast, node)->member_assign.value;

  if (!e_can_make_value(cc->ast, node)) {
    cerror(E_GET_NODE(cc->ast, node)->common.span, "Can not assign to member access\n");
    return -1;
  }

  e_lval lv = e_make_value(cc, node);
  int    e  = e_emit_lvalue_assign(cc, value, lv);
  e_free_value(&lv);

  return e;
}

static int
compile_return(e_compiler* cc, int node)
{
  if (E_GET_NODE(cc->ast, node)->ret.has_return_value) {
    /* Compile the return value */
    compile(cc, E_GET_NODE(cc->ast, node)->ret.expr_id);

    defer_emit_all_scopes(cc);

    e_emit_instruction(cc, E_OPCODE_RETURN);
    e_emit_u8(cc, true); // Specify that we're returning a value
  } else {
    defer_emit_all_scopes(cc);

    e_emit_instruction(cc, E_OPCODE_RETURN);
    e_emit_u8(cc, false); /* Returning void! */
  }
  return 0;
}

static int
append_struct_decleration(e_arena* a, const char* name, ecc_struct_information* deposit)
{
  char* dup  = e_arnstrdup(a, name);
  u32   hash = e_hash_fnv(name, strlen(name));

  if (deposit->fields_count >= deposit->field_capacity) {
    u32    field_cap_new    = deposit->field_capacity * 2;
    char** fields_new       = (char**)realloc(deposit->fields, field_cap_new * sizeof(char*));
    u32*   field_hashes_new = (u32*)realloc(deposit->field_hashes, field_cap_new * sizeof(u32));

    if (!fields_new) return -1;

    deposit->fields         = fields_new;
    deposit->field_capacity = field_cap_new;
    deposit->field_hashes   = field_hashes_new;
  }

  deposit->fields[deposit->fields_count]       = dup;
  deposit->field_hashes[deposit->fields_count] = hash;

  deposit->fields_count++;

  return 0;
}

static int
collect_struct_declerations(e_compiler* cc, int* stmts, u32 nstmts, ecc_struct_information* deposit)
{
  for (u32 i = 0; i < nstmts; i++) {
    e_ast_node_type type = E_GET_NODE(cc->ast, stmts[i])->type;
    if (type == E_AST_NODE_STATEMENT_LIST) {
      int* list_stmts  = E_GET_NODE(cc->ast, stmts[i])->stmts.stmts;
      u32  list_nstmts = E_GET_NODE(cc->ast, stmts[i])->stmts.nstmts;
      // RECURSE!
      int e = collect_struct_declerations(cc, list_stmts, list_nstmts, deposit);
      if (e) return e;
    } else if (type == E_AST_NODE_VARIABLE_DECL) {
      bool is_const = E_GET_NODE(cc->ast, stmts[i])->let.is_const;
      if (is_const) {
        cerror(E_GET_NODE(cc->ast, stmts[i])->let.span, "A member of a struct cannot be declared 'const' [unsupported]\n");
        return -1;
      }

      const char* name = E_GET_NODE(cc->ast, stmts[i])->let.name;
      append_struct_decleration(cc->arena, name, deposit);
    } else {
      const char* member_name = E_GET_NODE(cc->ast, stmts[i])->let.name;
      cerror(E_GET_NODE(cc->ast, stmts[i])->let.span, "Member index %u, with name %s is not allowed in a struct\n", i, member_name);
      return -1;
    }
  }

  return 0;
}

static int
compile_struct_constructor(e_compiler* fork, e_filespan span, const ecc_struct_information* struc)
{
  u32* arg_slots = (u32*)e_arnalloc(fork->arena, sizeof(u32) * struc->fields_count);

  for (u32 i = 0; i < struc->fields_count; i++) {
    u32 arg_hash = struc->field_hashes[i];
    arg_slots[i] = arg_hash;

    /* Add variable entry to stack */
    e_var* arg = e_stack_push_variable(arg_hash, fork->stack);

    /* Acquire a refdobj */
    arg->val.var_info                 = e_refdobj_pool_acquire(&ge_pool);
    E_VAR_AS_INFO(arg)->initializer   = -1; // Arguments aren't initialized.
    E_VAR_AS_INFO(arg)->current_value = -1; // Or initialized to null if you think about it.
    E_VAR_AS_INFO(arg)->name_hash     = arg_hash;
    E_VAR_AS_INFO(arg)->span          = span;
    E_VAR_AS_INFO(arg)->is_const      = false; // User can override the argument any time.
  }

  e_emit_instruction(fork, E_OPCODE_MK_STRUCT);
  e_emit_u32(fork, struc->fields_count); // number of members
  // hashes of all members
  for (u32 i = 0; i < struc->fields_count; i++) { e_emit_u32(fork, struc->field_hashes[i]); }

  /**
   * Assign the arguments to all members
   */
  for (u32 i = 0; i < struc->fields_count; i++) {
    e_emit_instruction(fork, E_OPCODE_DUP); // Duplicate struct (shallow copy)

    e_emit_instruction(fork, E_OPCODE_LOAD); // Load the argument
    e_emit_u32(fork, struc->field_hashes[i]);

    e_emit_instruction(fork, E_OPCODE_MEMBER_ASSIGN);
    e_emit_u32(fork, struc->field_hashes[i]); // Assign to that field
    e_emit_instruction(fork, E_OPCODE_POP);   // Pop member assign pushing the value back up. We only
                                              // want the struct to be on the stack (Member assign pushes struct + value, in that order).
  }

  // Return the accumulated struct.
  e_emit_instruction(fork, E_OPCODE_RETURN);
  e_emit_u8(fork, true); // has_return_value = true

  e_function f = {
    .code      = fork->emit,
    .code_size = fork->num_bytes_emitted,
    .arg_slots = arg_slots,
    .name_hash = struc->name_hash,
    .nargs     = struc->fields_count,
  };

  int e = append_function_entry(fork->arena, fork->function_table, &f);
  if (e) return e;

  return 0;
}

static int
compile_struct_decleration(e_compiler* cc, int node)
{
  const char* struct_name        = E_GET_NODE(cc->ast, node)->struct_decl.name;
  int*        struct_decl_stmts  = E_GET_NODE(cc->ast, node)->struct_decl.stmts;
  u32         struct_decl_nstmts = E_GET_NODE(cc->ast, node)->struct_decl.nstmts;
  u32         struct_name_hash   = e_hash_fnv(struct_name, strlen(struct_name));

  const u32 init_fields_capacity = 16;

  /* Gather all information the user provided into one big structure. */
  ecc_struct_information struct_data = {
    .name           = e_arnstrdup(cc->arena, struct_name),
    .name_hash      = struct_name_hash,
    .fields         = (char**)e_arnalloc(cc->arena, init_fields_capacity * sizeof(char*)),
    .field_hashes   = (u32*)e_arnalloc(cc->arena, init_fields_capacity * sizeof(u32)),
    .field_capacity = init_fields_capacity,
    .fields_count   = 0,
  };
  collect_struct_declerations(cc, struct_decl_stmts, struct_decl_nstmts, &struct_data);

  /* Generate the constructor function */
  e_compiler fork;

  int e = compiler_make_fork(cc, &fork);
  if (e) return e;

  e = e_stack_push_frame(cc->stack);
  if (e) return e;

  defer_push_scope(&fork);

  e = compile_struct_constructor(&fork, E_GET_NODE(cc->ast, node)->common.span, &struct_data);
  if (e) return e;

  defer_pop_scope(&fork);
  compiler_join_fork(&fork, cc);

  e_stack_pop_frame(cc->stack);

  return 0;
}

static int
compile_variable_decleration(e_compiler* cc, int node)
{
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

  // free(full);

  /* Add variable entry to stack */
  e_var* r = e_stack_push_variable(hash, cc->stack);

  /* Acquire a refdobj */
  r->type                         = E_VARTYPE_CC_VARIABLE;
  r->val.var_info                 = e_refdobj_pool_acquire(&ge_pool);
  E_VAR_AS_INFO(r)->initializer   = initializer;
  E_VAR_AS_INFO(r)->current_value = initializer; // current value is initializer, -1 if none
  E_VAR_AS_INFO(r)->name_hash     = hash;
  E_VAR_AS_INFO(r)->span          = E_GET_NODE(cc->ast, node)->common.span;
  E_VAR_AS_INFO(r)->is_const      = E_GET_NODE(cc->ast, node)->let.is_const;

  int e = 0;
  if (initializer >= 0) { e = compile(cc, initializer); }
  if (e) return e;

  e_emit_instruction(cc, E_OPCODE_INIT);
  e_emit_u32(cc, hash);

  // is_compound
  e_emit_u8(cc, initializer >= 0);

  return 0;
}

static int
compile_root(e_compiler* cc, int node)
{
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

  // Compiling headless. Emit a halt and return.
  if (!cc->info->executable) {
    e_emit_instruction(cc, E_OPCODE_HALT);
    e_emit_u32(cc, 0);
    return 0; // Done!
  }

  const char* custom_entry_point = cc->info->custom_entry_point;

  /* main if not specified. */
  u32 entry_point_hash = e_hash_fnv("main", strlen("main"));
  if (custom_entry_point != nullptr) { entry_point_hash = e_hash_fnv(custom_entry_point, strlen(custom_entry_point)); }

  /* Find entry point and ensure it doesn't ask for any arguments. */
  ecc_function_table* func_table = cc->function_table;

  bool found             = false;
  u32  entry_point_nargs = 0;
  for (u32 i = 0; i < func_table->functions_count; i++) {
    if (func_table->functions[i].name_hash == entry_point_hash) {
      found             = true;
      entry_point_nargs = func_table->functions[i].nargs;
    }
  }

  if (!found) {
    cerror(root->common.span, "Entry point undefined\n");
    return -1;
  }

  if (entry_point_nargs > 1) {
    cerror(root->common.span, "Entry point must either accept 1 argument (command line argument list) or no arguments at all.\n");
    return -1;
  }

  /* Push command line argument list to the stack, right before main as an argument. */
  if (entry_point_nargs == 1) { e_emit_instruction(cc, E_OPCODE_LOAD_ARG_LIST); }

  /* CALL to main */
  e_emit_instruction(cc, E_OPCODE_CALL);
  e_emit_u32(cc, entry_point_hash);
  e_emit_u16(cc, entry_point_nargs);

  return 0;
}

static int
compile_variable_load(e_compiler* cc, int node)
{
  char* full = mk_name(cc, E_GET_NODE(cc->ast, node)->ident.ident);
  u32   hash = e_hash_fnv(full, strlen(full));

  for (u32 i = 0; i < cc->builtin_var_table->builtin_vars_count; i++) {
    const e_builtin_var* builtin_var      = &cc->builtin_var_table->builtin_vars[i];
    u32                  builtin_var_hash = cc->builtin_var_table->builtin_var_hashes[i];

    if (builtin_var_hash == hash) {
      /* Instantiate a builtin variable only if it is used. */
      e_var v = {
        .type = builtin_var->type,
        .val  = builtin_var->value,
      };

      // free(full);
      return compile_literal_variable(cc, v); // compile_literal_variable loads the value! Return.
    }
  }

  // e_var* exists = e_stack_find(cc->stack, hash);
  // if (exists == nullptr) {
  //   cerror(E_GET_NODE(cc->ast, node)->common.span, "Undeclared variable '%s'\n", full);
  //   return -1;
  // }

  // free(full);

  e_lval lv = e_make_value(cc, node);
  if (e_emit_lvalue_load(cc, lv) < 0) {
    e_free_value(&lv);
    return -1;
  }

  e_free_value(&lv);
  return 0;
}

static int
compile_namespace_decleration(e_compiler* cc, int node)
{
  const char* ns_name        = E_GET_NODE(cc->ast, node)->namespace_decl.name;
  int*        ns_decl_stmts  = E_GET_NODE(cc->ast, node)->namespace_decl.stmts;
  u32         ns_decl_nstmts = E_GET_NODE(cc->ast, node)->namespace_decl.nstmts;

  ns_push(cc, ns_name);

  for (u32 i = 0; i < ns_decl_nstmts; i++) {
    int e = compile(cc, ns_decl_stmts[i]);
    if (e) {
      ns_pop(cc);
      return e;
    }
  }

  ns_pop(cc);

  return 0;
}

static int
compile(e_compiler* cc, int node)
{
  if (node < 0) return -1;

  switch (E_GET_NODE(cc->ast, node)->type) {
    case E_AST_NODE_NOP: return 0;

    case E_AST_NODE_ROOT: {
      if (compile_root(cc, node) < 0) { return -1; }
      return 0;
    }

    /* Don't push a scope */
    case E_AST_NODE_STATEMENT_LIST: {
      u32        nstmts = E_GET_NODE(cc->ast, node)->stmts.nstmts;
      const int* stmts  = E_GET_NODE(cc->ast, node)->stmts.stmts;
      for (u32 i = 0; i < nstmts; i++) {
        if (compile(cc, stmts[i]) < 0) return -1;
      }
      return 0;
    }

    case E_AST_NODE_DEFER: {
      int* stmts  = E_GET_NODE(cc->ast, node)->defer.stmts;
      u32  nstmts = E_GET_NODE(cc->ast, node)->defer.nstmts;
      append_defer_entry(cc, stmts, nstmts);
      return 0;
    }

    case E_AST_NODE_NAMESPACE_DECL: {
      if (compile_namespace_decleration(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_STRUCT_DECL: {
      if (compile_struct_decleration(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_INDEX: {
      if (compile_index(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_BINARYOP: {
      if (compile_binary_op(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_IF: {
      if (compile_if_statement(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_UNARYOP: {
      if (compile_unary_op(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_WHILE: {
      if (compile_while_statement(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_FOR: {
      if (compile_for_statement(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_MEMBER_ACCESS: {
      if (compile_member_access(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_BREAK: {
      if (!cc->loop) {
        cerror(E_GET_NODE(cc->ast, node)->common.span, "break used outside loop\n");
        return -1;
      }
      defer_emit_to_depth(cc, cc->loop->defer_depth);

      emit_and_record_jmp(cc, E_OPCODE_JMP, cc->loop->break_label);
      return 0;
    }

    case E_AST_NODE_CONTINUE: {
      if (!cc->loop) {
        cerror(E_GET_NODE(cc->ast, node)->common.span, "continue used outside loop\n");
        return -1;
      }
      defer_emit_to_depth(cc, cc->loop->defer_depth);

      emit_and_record_jmp(cc, E_OPCODE_JMP, cc->loop->continue_label);
      return 0;
    }

    case E_AST_NODE_RETURN: {
      if (compile_return(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_VARIABLE: {
      if (compile_variable_load(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_VARIABLE_DECL: {
      if (compile_variable_decleration(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_ASSIGN: {
      if (compile_assign(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_INDEX_ASSIGN: {
      if (compile_index_assign(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_MEMBER_ASSIGN: {
      if (compile_member_assign(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_INDEX_COMPOUND_OP: {
      if (compile_index_compound(cc, node)) { return -1; }
      return 0;
    }

    case E_AST_NODE_CALL: {
      if (compile_function_call(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_INT:
    case E_AST_NODE_CHAR:
    case E_AST_NODE_BOOL:
    case E_AST_NODE_STRING:
    case E_AST_NODE_FLOAT: {
      if (compile_literal(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_LIST: {
      if (compile_list(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_MAP: {
      if (compile_map(cc, node) < 0) return -1;
      return 0;
    }

    case E_AST_NODE_FUNCTION_DEFINITION: {
      if (compile_function_definition(cc, node) < 0) { return -1; }
      return 0;
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
    if (compile(cc, stmts[i]) < 0) return -1;
  }
  return 0;
}

static int
compile_builtin_structure(e_compiler* cc, const e_builtin_struct* b)
{
  ecc_struct_information st = {
    .name           = e_arnstrdup(cc->arena, b->name),
    .fields         = e_arnalloc(cc->arena, b->fields_count * sizeof(u32)),
    .field_hashes   = e_arnalloc(cc->arena, b->fields_count * sizeof(u32)),
    .fields_count   = b->fields_count,
    .field_capacity = b->fields_count,
    .name_hash      = e_hash_fnv(b->name, strlen(b->name)),
  };

  for (u32 j = 0; j < b->fields_count; j++) { st.fields[j] = e_arnstrdup(cc->arena, b->fields[j]); }
  for (u32 j = 0; j < b->fields_count; j++) { st.field_hashes[j] = e_hash_fnv(b->fields[j], strlen(b->fields[j])); }

  /* Generate the constructor function */
  e_compiler fork;

  int e = compiler_make_fork(cc, &fork);
  if (e) return e;

  e = e_stack_push_frame(cc->stack);
  if (e) return e;

  defer_push_scope(&fork);

  e = compile_struct_constructor(&fork, E_GET_NODE(cc->ast, cc->ast->root)->common.span, &st);
  if (e) return e;

  defer_pop_scope(&fork);
  compiler_join_fork(&fork, cc);

  e_stack_pop_frame(cc->stack);

  return 0;
}

/**
 * Load all builtin structures, even if they
 * aren't used. We can not safely say a structure
 * is used or not before compilation.
 * (Compiling the structurs only when they are seems
 *  to work, but will cause issues later, like with
 *  dynamic script execution).
 */
static int
compile_builtin_structures(e_compiler* cc)
{
  for (u32 i = 0; i < E_ARRLEN(eb_structs); i++) {
    const e_builtin_struct* b = &eb_structs[i];

    int e = compile_builtin_structure(cc, b);
    if (e) return e;
  }

  /**
   * Compile all hooked builtin structures.
   */
  for (u32 i = 0; i < cc->info->nhooked_structs; i++) {
    const e_builtin_struct* b = &cc->info->hook_structs[i];

    int e = compile_builtin_structure(cc, b);
    if (e) return e;
  }

  return 0;
}

int
e_compile(const ecc_info* info, e_compilation_result* result)
{
  const u32 init_code_capacity       = 256;
  const u32 init_literal_capacity    = 64;
  const u32 init_function_capacity   = 64;
  const u32 init_namespaces_capacity = 16;
  const u32 init_label_jump_capacity = 64;

  e_arena* arena = info->arena;

  e_arena fallback             = { 0 };
  bool    using_fallback_arena = false;
  if (!info->arena) {
    if (e_arena_init(1, &fallback)) return -1;
    arena                = &fallback;
    using_fallback_arena = true;
  }

  ecc_namespace_stack ns = {
    .namespaces  = e_arnalloc(arena, sizeof(char*) * init_namespaces_capacity),
    .nnamespaces = 0,
    .capacity    = init_namespaces_capacity,
  };

  /**
   * Compiler's builtin variables and the hooked variables combined.
   */
  u32 total_builtin_variable_count = E_ARRLEN(eb_vars) + info->nhooked_vars;

  u32*           builtin_variable_hashes = (u32*)e_arnalloc(arena, sizeof(u32) * total_builtin_variable_count);
  e_builtin_var* builtin_variables       = (e_builtin_var*)e_arnalloc(arena, sizeof(e_builtin_var) * total_builtin_variable_count);
  if (!builtin_variable_hashes) return -1;
  if (!builtin_variables) return -1;

  /**
   * Load up every hash and variable into the arrays.
   * First, the compiler's builtins, and then the hooked variables.
   * This ensures the compiler's definitions are seen earlier
   * than the later ones, preventing overshadowing of primitive types.
   */
  u32 builtin_var_ctr = 0;
  for (u32 i = 0; i < E_ARRLEN(eb_vars); i++, builtin_var_ctr++) {
    builtin_variable_hashes[builtin_var_ctr] = e_hash_fnv(eb_vars[i].name, strlen(eb_vars[i].name));
    memcpy(&builtin_variables[builtin_var_ctr], &eb_vars[i], sizeof(e_builtin_var));
  }
  for (u32 i = 0; i < info->nhooked_vars; i++, builtin_var_ctr++) {
    builtin_variable_hashes[builtin_var_ctr] = e_hash_fnv(info->hook_vars[i].name, strlen(info->hook_vars[i].name));
    memcpy(&builtin_variables[builtin_var_ctr], &info->hook_vars[i], sizeof(e_builtin_var));
  }

  /**
   * Temporary stack to track variables.
   * No values are assigned.
   */
  e_stack stack = { 0 };

  ecc_literal_table lit_table = {
    .literals          = (e_var*)e_arnalloc(arena, sizeof(e_var) * init_literal_capacity),
    .literal_hashes    = (u16*)e_arnalloc(arena, sizeof(u16) * init_literal_capacity),
    .literals_count    = 0,
    .literals_capacity = init_literal_capacity,
  };

  ecc_builtin_variables_table builtin_var_table = {
    .builtin_vars       = builtin_variables,
    .builtin_var_hashes = builtin_variable_hashes,
    .builtin_vars_count = total_builtin_variable_count,
  };

  ecc_function_table func_table = {
    .functions          = e_arnalloc(arena, sizeof(e_function) * init_function_capacity),
    .functions_capacity = init_function_capacity,
    .functions_count    = 0,
  };

  ecc_label_table label_table = {
    .labels_count    = 0,
    .labels_capacity = init_label_jump_capacity,
    .labels          = e_arnalloc(arena, sizeof(ecc_label_jumps_table) * init_label_jump_capacity),
  };

  e_compiler cc = {
    .arena             = arena,
    .ast               = info->ast,
    .info              = info,
    .loop              = NULL,
    .ns                = &ns,
    .stack             = &stack,
    .lit_table         = &lit_table,
    .builtin_var_table = &builtin_var_table,
    .function_table    = &func_table,
    .label_table       = &label_table,
    .emit              = (u8*)malloc(init_code_capacity),
    .num_bytes_emitted = 0,
    .emit_capacity     = init_code_capacity,
  };

  defer_push_scope(&cc);

  int e = 0;
  if ((e = e_stack_init(256, 32, 32, &stack))) return e;

  /**
   * Generate constructors for all builtin && hooked
   * structures.
   */
  e = compile_builtin_structures(&cc);
  if (e) return e;

  e = compile(&cc, info->root);
  if (e) return e;

  defer_pop_scope(&cc);

  /* Resolve all labels after compilation. Ensure this is the last optimization / cleanup function called! */
  // e_resolve_labels(cc.emit, cc.num_bytes_emitted);
  // for (size_t i = 0; i < cc.function_table->functions_count; i++) {
  //   e_resolve_labels(cc.function_table->functions[i].code, cc.function_table->functions[i].code_size);
  // } // resolve all function labels

  if (result) {
    result->literals      = cc.lit_table->literals;
    result->nliterals     = cc.lit_table->literals_count;
    result->functions     = func_table.functions;
    result->nfunctions    = func_table.functions_count;
    result->ninstructions = cc.num_bytes_emitted;
    result->instructions  = cc.emit;
  }

  e_stack_free(&stack);

  if (using_fallback_arena) { e_arena_free(&fallback); }

  // e_print_instruction_stream(cc.emit, cc.emitted);

  return e;
}