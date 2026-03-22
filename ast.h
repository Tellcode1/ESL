#ifndef E_AST_H
#define E_AST_H

#include "cerr.h"
#include "lex.h"

#include <stdio.h>
#include <stdlib.h>

typedef enum e_asnodetype {
  E_ASNODE_NOP,

  E_ASNODE_ROOT,
  E_ASNODE_BINARYOP,
  E_ASNODE_UNARYOP,
  E_ASNODE_EXPRESSION_LIST,

  E_ASNODE_FOR,
  E_ASNODE_WHILE,
  E_ASNODE_BREAK,
  E_ASNODE_CONTINUE,
  E_ASNODE_RETURN,
  E_ASNODE_IF,
  E_ASNODE_FUNCTION_DEFINITION,

  E_ASNODE_VARIABLE,

  // let <name> [ : type] [ = initializer];
  E_ASNODE_VARIABLE_DECL,
  E_ASNODE_ASSIGN,
  E_ASNODE_INDEX,
  E_ASNODE_INDEX_ASSIGN,      // Assign to member
  E_ASNODE_INDEX_COMPOUND_OP, // Assign to member, using a compound operator (unary/binary)

  E_ASNODE_CALL,
  E_ASNODE_INT,
  E_ASNODE_CHAR,
  E_ASNODE_BOOL,
  E_ASNODE_STRING,
  E_ASNODE_FLOAT,
  E_ASNODE_LIST,
  E_ASNODE_MAP,
} e_asnode_type;

typedef enum e_operator {
  E_OPERATOR_ADD,
  E_OPERATOR_SUB,
  E_OPERATOR_MUL,
  E_OPERATOR_DIV,
  E_OPERATOR_MOD, // Integral remainder
  E_OPERATOR_EXP, // Exponent
  E_OPERATOR_AND,
  E_OPERATOR_NOT,
  E_OPERATOR_OR,
  E_OPERATOR_BAND,
  E_OPERATOR_BOR,
  E_OPERATOR_BNOT,
  E_OPERATOR_XOR,
  E_OPERATOR_EQL, // Equal?
  E_OPERATOR_NEQ, // Not equal?

  E_OPERATOR_LT,
  E_OPERATOR_LTE,
  E_OPERATOR_GT,
  E_OPERATOR_GTE,

  /**
   * Contrary to C, the VM doesn't push any value for these. Only increments the variable.
   * Treated as compound operators always!
   */
  E_OPERATOR_DEC,
  E_OPERATOR_INC,
} e_operator;

typedef enum e_if_level {
  E_IF_LEVEL_TOP,
  E_IF_LEVEL_ELSE_IF,
  E_IF_LEVEL_ELSE,
} e_if_level;

typedef struct e_if_stmt {
  int*              body;
  int*              else_body; // NULL for no else statement
  struct e_if_stmt* else_ifs;  // Allocated, free after compilation.

  int condition;
  u32 nstmts;
  u32 nelse_stmts;
  u32 nelse_ifs;
} e_if_stmt;

typedef union e_asnode_val {
  int    i;
  bool   b;
  char   c;
  double f;
  char*  s;
  char*  ident;

  struct {
    char* name;
    int   initializer; // -1 if not given.
  } let;

  struct {
    // Node ID of the expression
    int expr_id;
    // If false, expr_id is < 0 and function must return void.
    bool has_return_value;
  } ret;

  struct {
    int* elems;
    u32  nelems;
  } list;

  struct {
    int* kvpairs;
    u32  npairs;
  } map;

  struct {
    int        left; // index of left
    int        right;
    e_operator op;
    bool       is_compound;
  } binaryop;

  struct {
    int left;
    int right;
  } assign;

  struct {
    int base;
    int offset;
  } index;

  struct {
    int base;   // list/structure
    int offset; // index: integer
    int value;  // any value.
  } index_assign;

  struct {
    int        base;  // list/structure
    int        index; // index: integer
    int        value; // any value.
    e_operator op;
  } index_compound;

  struct {
    int        right; // index of right
    e_operator op;
    bool       is_compound;
  } unaryop;

  struct {
    int* stmts;
    u32  nstmts;
  } stmts, root;

  struct {
    int left;
    int value;
  } member_assign;

  struct {
    char* function;
    int*  args;
    u32   nargs;
  } call;

  struct {
    char*  name;
    char** args;  // Allocated, + each string is allocated individually.
    int*   stmts; // Function body
    u32    nargs;
    u32    nstmts;
  } func;

  struct {
    int  condition;
    int* stmts;
    u32  nstmts;
  } while_stmt;

  struct {
    int  initializers; // for (<let x = 0>;
    int  condition;    // x >= 0;
    int  iterators;    // x++)
    u32  nstmts;
    int* stmts;
  } for_stmt;

  e_if_stmt if_stmt;
} e_asnode_val;

typedef struct e_asnode {
  e_asnode_type type;
  e_asnode_val  val;
  e_filespan    span;
} e_asnode;

typedef struct e_ast {
  e_asnode* nodes;
  u32       nnodes;
  u32       capacity;

  e_token* toks;
  u32      ntoks;
  u32      head;
} e_ast;

int  e_ast_init(e_token* toks, u32 ntoks, e_ast* prsr);
void e_ast_free(e_ast* prsr);

static inline e_token*
e_ast_next(struct e_ast* prsr)
{
  if (prsr->head >= prsr->ntoks) return NULL;
  return &prsr->toks[prsr->head++];
}
static inline e_token*
e_ast_peek(const struct e_ast* prsr)
{
  if (prsr->head >= prsr->ntoks) return NULL;
  return &prsr->toks[prsr->head];
}

static inline bool
e_getbp(e_tokentype type, int* left, int* right)
{
  switch (type) {
    case E_TOKENTYPE_EQUAL:
      *left  = 10;
      *right = 9;
      break;

    case E_TOKENTYPE_OR:
      *left  = 20;
      *right = 21;
      break;
    case E_TOKENTYPE_AND:
      *left  = 30;
      *right = 31;
      break;
    case E_TOKENTYPE_NOT:
      *left  = 0;
      *right = 9;
      break;

    // bitwise
    case E_TOKENTYPE_BOR: // |
      *left  = 32;
      *right = 33;
      break;
    case E_TOKENTYPE_XOR: // ^
      *left  = 34;
      *right = 35;
      break;
    case E_TOKENTYPE_BAND: // &
      *left  = 36;
      *right = 37;
      break;

    // compares
    case E_TOKENTYPE_GT:
    case E_TOKENTYPE_LT:
    case E_TOKENTYPE_GTE:
    case E_TOKENTYPE_LTE:
    case E_TOKENTYPE_DOUBLEEQUAL:
    case E_TOKENTYPE_NOTEQUAL:
      *left  = 40;
      *right = 41;
      break;

    case E_TOKENTYPE_PLUS:
    case E_TOKENTYPE_MINUS:
      *left  = 50;
      *right = 51;
      break;
    case E_TOKENTYPE_MULTIPLY:
    case E_TOKENTYPE_DIVIDE:
    case E_TOKENTYPE_MOD:
      *left  = 60;
      *right = 61;
      break;

    case E_TOKENTYPE_BNOT: // ~
      *left  = 0;
      *right = 65;
      break;

      // member access
      // case K_OP_DOT:
      //   *left  = 70;
      //   *right = 69;
      //   break;

    case E_TOKENTYPE_OPENPAREN:
      *left  = 90;
      *right = 100;
      break;

    case E_TOKENTYPE_OPENBRACE:
    case E_TOKENTYPE_OPENBRACKET:
      *left  = 100;
      *right = 99;
      break;

    case E_TOKENTYPE_PLUSPLUS:
    case E_TOKENTYPE_MINUSMINUS:
      *left  = 80; // postfix only!!
      *right = 0;
      break;

    default: *left = *right = 0; return false;
  }
  return true;
}

static inline int
e_ast_make_node(e_ast* p)
{
  if (p->nnodes + 1 >= p->capacity) {
    u32       newcap   = MAX(p->capacity * 2, 1);
    e_asnode* newnodes = (e_asnode*)realloc(p->nodes, newcap * sizeof(e_asnode));
    if (newnodes == NULL) {
      perror("Allocation failed");
      return -1;
    }

    p->nodes    = newnodes;
    p->capacity = newcap;
  }

  u32 idx = p->nnodes++;
  memset(&p->nodes[idx], 0, sizeof(e_asnode));

  return (int)idx;
}

#define E_GET_NODE e_ast_get_node
static inline e_asnode*
e_ast_get_node(const e_ast* p, int idx)
{
  if (idx < 0) return NULL;
  return &p->nodes[idx];
}

static inline bool
e_ast_is_limiter_exempt(e_asnode_type t)
{ return t == E_ASNODE_IF || t == E_ASNODE_WHILE || t == E_ASNODE_FOR || t == E_ASNODE_FUNCTION_DEFINITION || t == E_ASNODE_EXPRESSION_LIST; }

int e_ast_parse(e_ast* p, int* root_nodeID);
int e_ast_nud(e_ast* p, e_token* token);
int e_ast_expr(e_ast* p, int rbp);
int e_ast_led(e_ast* p, e_token* token, int leftidx, int rbp);

int e_ast_expect(e_ast* p, e_tokentype type);

#endif // E_AST_H
