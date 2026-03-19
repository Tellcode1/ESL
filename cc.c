#include "cc.h"

#include "ast.h"
#include "bc.h"
#include "fn.h"
#include "label.h"
#include "lval.h"
#include "refcount.h"
#include "rwhelp.h"
#include "stdafx.h"
#include "var.h"

#include <error.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline int compile_literal(e_compiler* cc, int node);
static int        compile_function_definition(struct e_compiler* cc, int node);
static int        compile_function_call(struct e_compiler* cc, int node);
static int        compile_if_statement(struct e_compiler* cc, int node);
static int        compile_while_statement(struct e_compiler* cc, int node);
static int        compile_binary_op(struct e_compiler* cc, int node);
static int        compile_unary_op(struct e_compiler* cc, int node);
static int        compile(struct e_compiler* cc, int node);

static inline u32
ec_get_pos(const e_compiler* cc)
{
  return cc->emitted;
}

static inline u32
make_label_id()
{
  static u32 counter = 0;
  counter++;
  return counter;
}

static inline int
lower_node_to_literal(const e_asnode* node, e_var* o)
{
  switch (node->type) {
    case E_ASNODE_INT: *o = (e_var){ .type = E_VARTYPE_INT, .refc = e_refc_init(), .val.i = node->val.i }; return 0;
    case E_ASNODE_FLOAT: *o = (e_var){ .type = E_VARTYPE_FLOAT, .refc = e_refc_init(), .val.f = node->val.f }; return 0;
    case E_ASNODE_CHAR: *o = (e_var){ .type = E_VARTYPE_CHAR, .refc = e_refc_init(), .val.c = node->val.c }; return 0;
    case E_ASNODE_BOOL: *o = (e_var){ .type = E_VARTYPE_BOOL, .refc = e_refc_init(), .val.b = node->val.b }; return 0;
    case E_ASNODE_STRING: {
      struct e_string* s = malloc(sizeof(e_string));

      s->s = strdup(node->val.s);
      *o   = (e_var){ .type = E_VARTYPE_STRING, .refc = e_refc_init(), .val.s = s };
      return 0;
    }
    case E_ASNODE_LIST:
    case E_ASNODE_MAP: abort(); break;

    default: return 1;
  }
  return 1;
}

static inline int
compile_literal(e_compiler* cc, int node)
{
  if (cc->nliterals >= cc->cliterals) {
    size_t new_c              = cc->cliterals * 2;
    e_var* new_literals       = realloc(cc->literals, sizeof(e_var) * new_c);
    u16*   new_literal_hashes = realloc(cc->literal_hashes, sizeof(u16) * new_c);
    if (!new_literals || !new_literal_hashes) return -1;

    cc->literals       = new_literals;
    cc->literal_hashes = new_literal_hashes;
    cc->cliterals      = new_c;
  }

  bool found = false;
  u16  id    = cc->nliterals;

  e_var v = { 0 };
  if (lower_node_to_literal(E_GET_NODE(cc->ast, node), &v)) return -1;

  u16 hash = (u16)e_var_hash(&v);
  for (u32 i = 0; i < cc->nliterals; i++) {
    if (cc->literal_hashes[i] == hash && e_var_equal(&v, &cc->literals[i])) {
      id    = i;
      found = true;
      break;
    }
  }

  if (!found) {
    id                     = cc->nliterals;
    cc->literals[id]       = v;
    cc->literal_hashes[id] = e_var_hash(&v);
    cc->nliterals++;
  } else {
    e_var_free(&v); // free discarded variable

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

static int
compile_function_definition(struct e_compiler* cc, int node)
{
  const char* function_name = E_GET_NODE(cc->ast, node)->val.func.name;
  u32         hash          = e_hash_fnv(function_name, strlen(function_name));

  /* Ensure it doesn't already exist */
  for (u32 i = 0; i < cc->functions_size; i++) {
    if (cc->functions[i].name_hash == hash) {
      _UNREACHABLE();
      cerror(E_GET_NODE(cc->ast, node)->span, "Multiple definitions of function \"%s\"", function_name);
      return -1;
    }
  }

  u32 init_code_capacity = 256;

  struct e_compiler copy = {
    .ast            = cc->ast,
    .loop           = nullptr, // reset loop on function.
    .literals       = cc->literals,
    .literal_hashes = cc->literal_hashes,
    .nliterals      = cc->nliterals,
    .cliterals      = cc->cliterals,
    .emit           = (u8*)malloc(init_code_capacity),
    .emitted        = 0,
    .code_capacity  = init_code_capacity,
  };

  int e = e_compile_function(&copy, node);

  /* Always return void if no other return value was specified */
  e_emit_instruction(&copy, E_OPCODE_RETURN, E_ATTR_NONE);
  e_emit_u8(&copy, false);

  cc->literals  = copy.literals;
  cc->nliterals = copy.nliterals;
  cc->cliterals = copy.cliterals;

  u32 nargs = E_GET_NODE(cc->ast, node)->val.func.nargs;

  u32* arg_slots = (u32*)malloc(sizeof(u32) * nargs);
  for (u32 i = 0; i < nargs; i++) {
    const char* arg_name = E_GET_NODE(cc->ast, node)->val.func.args[i];
    arg_slots[i]         = e_hash_fnv(arg_name, strlen(arg_name));
  }

  const char* name = E_GET_NODE(cc->ast, node)->val.func.name;

  e_function f = {
    .code      = copy.emit,
    .code_size = copy.emitted,
    .arg_slots = arg_slots,
    .name_hash = (u32)e_hash_fnv(name, strlen(name)),
    .nargs     = nargs,
  };

  if (cc->functions_size >= cc->functions_capacity) {
    u32         new_capacity  = cc->functions_capacity * 2;
    e_function* new_functions = realloc(cc->functions, sizeof(e_function) * new_capacity);
    if (!new_functions) return -1;

    cc->functions          = new_functions;
    cc->functions_capacity = new_capacity;
  }

  cc->functions[cc->functions_size] = f;
  cc->functions_size++;

  return e;
}

static int
compile_binary_op(struct e_compiler* cc, int node)
{
  int e = 0;

  e = compile(cc, E_GET_NODE(cc->ast, node)->val.binaryop.left);
  if (e) return e;

  e = compile(cc, E_GET_NODE(cc->ast, node)->val.binaryop.right);
  if (e) return e;

  e_opcode opcode = -1;
  switch (E_GET_NODE(cc->ast, node)->val.binaryop.op) {
      // clang-format off
          case E_OPERATOR_ADD: opcode = E_OPCODE_ADD; break; 
          case E_OPERATOR_SUB: opcode = E_OPCODE_SUB; break; 
          case E_OPERATOR_MUL: opcode = E_OPCODE_MUL; break; 
          case E_OPERATOR_DIV: opcode = E_OPCODE_DIV; break; 
          case E_OPERATOR_MOD: opcode = E_OPCODE_MOD; break; 
          case E_OPERATOR_EXP: opcode = E_OPCODE_EXP; break; 
          case E_OPERATOR_AND: opcode = E_OPCODE_AND; break; 
          case E_OPERATOR_OR: opcode = E_OPCODE_OR; break; 
          case E_OPERATOR_BAND: opcode = E_OPCODE_BAND; break; 
          case E_OPERATOR_BOR: opcode = E_OPCODE_BOR; break; 
          case E_OPERATOR_XOR: opcode = E_OPCODE_XOR; break; 
          case E_OPERATOR_EQL: opcode = E_OPCODE_EQL; break; 
          case E_OPERATOR_NEQ: opcode = E_OPCODE_NEQ; break; 
          case E_OPERATOR_LT: opcode = E_OPCODE_LT; break; 
          case E_OPERATOR_LTE: opcode = E_OPCODE_LTE; break; 
          case E_OPERATOR_GT: opcode = E_OPCODE_GT; break; 
          case E_OPERATOR_GTE: opcode = E_OPCODE_GTE; break;
      // clang-format on

    default: printf("Operator %u can not be used as a binary operator\n", E_GET_NODE(cc->ast, node)->val.binaryop.op); return -1;
  }

  e_attr attrs = E_ATTR_NONE;
  if (E_GET_NODE(cc->ast, node)->val.binaryop.is_compound) { attrs |= E_ATTR_COMPOUND; }

  e_emit_instruction(cc, opcode, attrs);

  return e;
}

static int
compile_unary_op(struct e_compiler* cc, int node)
{
  int e = compile(cc, E_GET_NODE(cc->ast, node)->val.unaryop.right);
  if (e) return e;

  e_opcode opcode = -1;

  // clang-format off
      switch (E_GET_NODE(cc->ast, node)->val.unaryop.op)
      {
        case E_OPERATOR_NOT: opcode = E_OPCODE_NOT; break;
        case E_OPERATOR_BNOT: opcode = E_OPCODE_BNOT; break;
        case E_OPERATOR_INC: opcode = E_OPCODE_INC; break;
        case E_OPERATOR_DEC: opcode = E_OPCODE_DEC; break;
        default: printf("Operator %u can not be used as a unary operator\n", E_GET_NODE(cc->ast, node)->val.unaryop.op); return -1;
      }
  // clang-format on

  e_attr attrs = E_ATTR_NONE;
  if (E_GET_NODE(cc->ast, node)->val.unaryop.is_compound) { attrs |= E_ATTR_COMPOUND; }

  e_emit_instruction(cc, opcode, attrs);

  if (E_GET_NODE(cc->ast, node)->val.unaryop.is_compound) {
    int right_node = E_GET_NODE(cc->ast, node)->val.unaryop.right;
    e_emit_u32(cc, e_make_value(cc->ast, right_node).val.var_id);
  }

  return 0;
}

static int
compile_function_call(struct e_compiler* cc, int node)
{
  int e = 0;
  for (u32 i = 0; i < E_GET_NODE(cc->ast, node)->val.call.nargs; i++) {
    e = compile(cc, E_GET_NODE(cc->ast, node)->val.call.args[i]);
    if (e) return e;
  }

  if (e) return e;

  const char* function = E_GET_NODE(cc->ast, node)->val.call.function;
  u16         nargs    = E_GET_NODE(cc->ast, node)->val.call.nargs;
  u32         id       = e_hash_fnv(function, strlen(function));

  // Find the function, if it is user defined and check if the argument count matches
  {
    e_function* func = nullptr;
    for (u32 i = 0; i < cc->functions_size; i++) {
      if (cc->functions[i].name_hash == id) {
        func = &cc->functions[i];
        break;
      }
    }

    if (func && func->nargs != nargs) {
      cerror(E_GET_NODE(cc->ast, node)->span, "Function \"%s\" expects %u arguments (%u were given)\n", function, func->nargs, nargs);
      return -1;
    }
  }

  e_emit_instruction(cc, E_OPCODE_CALL, E_ATTR_NONE);        // 2 bytes
  e_emit_u16(cc, E_GET_NODE(cc->ast, node)->val.call.nargs); // 2 bytes, number of arguments
  e_emit_u32(cc, id);                                        // 4 bytes, function ID

  return 0;
}

static int
compile_if_statement(struct e_compiler* cc, int node)
{
  u32 end_label           = make_label_id();
  u32 next_in_chain_label = make_label_id();

  e_emit_instruction(cc, E_OPCODE_PUSH_VARIABLES, E_ATTR_NONE);

  // condition
  int e = compile(cc, E_GET_NODE(cc->ast, node)->val.if_stmt.condition);
  if (e) return e;

  // condition failed :<
  e_emit_instruction(cc, E_OPCODE_JZ, E_ATTR_CLEAN);
  e_emit_u32(cc, next_in_chain_label);

  for (u32 i = 0; i < E_GET_NODE(cc->ast, node)->val.if_stmt.nstmts; i++) {
    e = compile(cc, E_GET_NODE(cc->ast, node)->val.if_stmt.body[i]);
    if (e) return e;
  }

  e_emit_instruction(cc, E_OPCODE_JMP, E_ATTR_NONE); // JUMP!
  e_emit_u32(cc, end_label);

  for (u32 else_if = 0; else_if < E_GET_NODE(cc->ast, node)->val.if_stmt.nelse_ifs; else_if++) {
    e_emit_label(cc, next_in_chain_label);
    next_in_chain_label = make_label_id();

    struct e_if_stmt* if_stmt = &E_GET_NODE(cc->ast, node)->val.if_stmt.else_ifs[else_if];

    e = compile(cc, if_stmt->condition);
    if (e) return e;

    e_emit_instruction(cc, E_OPCODE_JZ, E_ATTR_NONE);
    e_emit_u32(cc, next_in_chain_label);

    for (u32 i = 0; i < if_stmt->nstmts; i++) {
      e = compile(cc, if_stmt->body[i]);
      if (e) return e;
    }

    e_emit_instruction(cc, E_OPCODE_JMP, E_ATTR_NONE); // skip remaining elseifs and else
    e_emit_u32(cc, end_label);
  }

  e_emit_label(cc, next_in_chain_label); // BAM!
  for (u32 i = 0; i < E_GET_NODE(cc->ast, node)->val.if_stmt.nelse_stmts; i++) {
    e = compile(cc, E_GET_NODE(cc->ast, node)->val.if_stmt.else_body[i]);
    if (e) return e;
  }

  e_emit_label(cc, end_label);

  e_emit_instruction(cc, E_OPCODE_POP_VARIABLES, E_ATTR_NONE);

  return 0;
}

static int
compile_while_statement(struct e_compiler* cc, int node)
{
  e_emit_instruction(cc, E_OPCODE_PUSH_VARIABLES, E_ATTR_NONE);

  const u32 condition_label = make_label_id();
  const u32 end_label       = make_label_id();

  ecc_loop_location loop = {
    .continue_label = condition_label,
    .break_label    = end_label,
  };
  ecc_loop_location* last = cc->loop;
  cc->loop                = &loop;

  e_emit_label(cc, condition_label);

  int e = compile(cc, E_GET_NODE(cc->ast, node)->val.while_stmt.condition);
  if (e) return e;

  e_emit_instruction(cc, E_OPCODE_JZ, E_ATTR_CLEAN);
  e_emit_u32(cc, end_label);

  for (u32 i = 0; i < E_GET_NODE(cc->ast, node)->val.while_stmt.nexprs; i++) {
    e = compile(cc, E_GET_NODE(cc->ast, node)->val.while_stmt.exprs[i]);
    if (e) return e;
  }

  // Jump to condition
  e_emit_instruction(cc, E_OPCODE_JMP, E_ATTR_NONE);
  e_emit_u32(cc, condition_label);

  e_emit_label(cc, end_label);

  e_emit_instruction(cc, E_OPCODE_POP_VARIABLES, E_ATTR_NONE);

  cc->loop = last;

  return e;
}

static int
compile(struct e_compiler* cc, int node)
{
  if (node < 0) return -1;

  switch (E_GET_NODE(cc->ast, node)->type) {
    case E_ASNODE_ROOT: {
      e_asnode* root = E_GET_NODE(cc->ast, node);
      for (u32 i = 0; i < root->val.root.nexprs; i++) {
        int e = compile(cc, root->val.root.exprs[i]);
        if (e) { return e; }
      }
      e_emit_instruction(cc, E_OPCODE_HALT, E_ATTR_NONE);
      e_emit_u32(cc, 0); // Return 0 by default.
      return 0;
    }

    case E_ASNODE_EXPRESSION_LIST: {
      int e = 0;
      for (u32 i = 0; i < E_GET_NODE(cc->ast, node)->val.exprs.nexprs; i++) {
        e = compile(cc, E_GET_NODE(cc->ast, node)->val.exprs.exprs[i]);
        if (e) return e;
      }
      return 0;
    }

    case E_ASNODE_BINARYOP: {
      return compile_binary_op(cc, node);
    }

    // This is the dirtiest of them all...
    case E_ASNODE_IF: {
      return compile_if_statement(cc, node);
    }

    case E_ASNODE_UNARYOP: {
      return compile_unary_op(cc, node);
    }

    case E_ASNODE_WHILE: {
      return compile_while_statement(cc, node);
    }

    case E_ASNODE_FOR: break;

    case E_ASNODE_BREAK: {
      if (!cc->loop) {
        cerror(E_GET_NODE(cc->ast, node)->span, "break Used outside loop\n");
        return -1;
      }
      e_emit_instruction(cc, E_OPCODE_JMP, E_ATTR_NONE);
      e_emit_u32(cc, cc->loop->break_label);
      return 0;
    }
    case E_ASNODE_CONTINUE: {
      if (!cc->loop) {
        cerror(E_GET_NODE(cc->ast, node)->span, "continue Used outside loop\n");
        return -1;
      }
      e_emit_instruction(cc, E_OPCODE_JMP, E_ATTR_NONE);
      e_emit_u32(cc, cc->loop->continue_label);
      break;
    }

    case E_ASNODE_RETURN: {
      if (E_GET_NODE(cc->ast, node)->val.ret.has_return_value) {
        /* Compile the return value */
        compile(cc, E_GET_NODE(cc->ast, node)->val.ret.expr_id);

        e_emit_instruction(cc, E_OPCODE_RETURN, E_ATTR_NONE);
        e_emit_u8(cc, true); // Specify that we're returning a value
      } else {
        e_emit_instruction(cc, E_OPCODE_RETURN, E_ATTR_NONE);
        e_emit_u8(cc, false); /* Returning void! */
      }
      return 0;
    }

    case E_ASNODE_VARIABLE: {
      e_emit_lvalue_load(cc, e_make_value(cc->ast, node));
      return 0;
    }

    case E_ASNODE_VARIABLE_DECL: {
      const char* name        = E_GET_NODE(cc->ast, node)->val.let.name;
      u32         id          = e_hash_fnv(name, strlen(name));
      int         initializer = E_GET_NODE(cc->ast, node)->val.let.initializer;

      int    e     = 0;
      e_attr attrs = E_ATTR_NONE;
      if (initializer >= 0) {
        e = compile(cc, initializer);
        attrs |= E_ATTR_COMPOUND; // Compound sets variable value to top of stack.
      }
      if (e) return e;

      e_emit_instruction(cc, E_OPCODE_INIT, attrs);
      e_emit_u32(cc, id);
      return 0;
    }

    case E_ASNODE_ASSIGN: {
      int e = compile(cc, E_GET_NODE(cc->ast, node)->val.assign.right);
      if (e) return e;

      u32 left_id = e_make_value(cc->ast, E_GET_NODE(cc->ast, node)->val.assign.left).val.var_id;

      e_emit_instruction(cc, E_OPCODE_ASSIGN, E_ATTR_NONE);
      e_emit_u32(cc, left_id);
      return 0;
    }

    case E_ASNODE_CALL: {
      return compile_function_call(cc, node);
    }

    case E_ASNODE_INT:
    case E_ASNODE_CHAR:
    case E_ASNODE_BOOL:
    case E_ASNODE_STRING:
    case E_ASNODE_FLOAT:
    case E_ASNODE_LIST:
    case E_ASNODE_MAP: {
      return compile_literal(cc, node);
    }

    case E_ASNODE_FUNCTION_DEFINITION: {
      return compile_function_definition(cc, node);
    }
    case E_ASNODE_MEMBER_ACCESS:
    case E_ASNODE_MEMBER_ASSIGN: break;
  }

  return -1;
}

int
e_compile_function(e_compiler* cc, int node)
{
  u32  nexprs = E_GET_NODE(cc->ast, node)->val.func.nexprs;
  int* exprs  = E_GET_NODE(cc->ast, node)->val.func.exprs;
  for (u32 i = 0; i < nexprs; i++) {
    int e = compile(cc, exprs[i]);
    if (e) return e;
  }
  return 0;
}

int
e_compile(struct e_ast* ast, int root_node, e_compilation_result* result)
{
  const u32 init_code_capacity     = 256;
  const u32 init_literal_capacity  = 64;
  const u32 init_function_capacity = 64;

  e_compiler cc = {
    .ast                = ast,
    .loop               = NULL,
    .literals           = (e_var*)malloc(sizeof(e_var) * init_literal_capacity),
    .literal_hashes     = (u16*)malloc(sizeof(u16) * init_literal_capacity),
    .nliterals          = 0,
    .cliterals          = init_literal_capacity,
    .code_capacity      = init_code_capacity,
    .emit               = (u8*)malloc(sizeof(u8) * init_code_capacity),
    .emitted            = 0,
    .functions_capacity = init_function_capacity,
    .functions_size     = 0,
    .functions          = malloc(sizeof(e_function) * init_function_capacity),
  };

  int e = compile(&cc, root_node);
  if (e) return e;

  /* Resolve all labels after compilation. Ensure this is the last optimization / cleanup function called! */
  e_resolve_labels(cc.emit, cc.emitted);
  for (size_t i = 0; i < cc.functions_size; i++) { e_resolve_labels(cc.functions[i].code, cc.functions[i].code_size); } // resolve all function labels

  if (result) {
    result->bytecode      = cc.emit;
    result->bytecode_size = cc.emitted;
    result->literals      = cc.literals;
    result->nliterals     = cc.nliterals;
    result->functions     = cc.functions;
    result->nfunctions    = cc.functions_size;
  }

  free(cc.literal_hashes);

  // e_print_instruction_stream(cc.emit, cc.emitted);

  return e;
}