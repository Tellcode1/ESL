#ifndef E_AST_H
#define E_AST_H

#include "../std/include/containers/list.h"
#include "cerr.h"
#include "lex.h"

typedef enum e_asnodetype {
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
  E_ASNODE_MEMBER_ACCESS,
  E_ASNODE_MEMBER_ASSIGN,

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
  int nstmts;
  int nelse_stmts;
  int nelse_ifs;
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
    int  nelems;
  } list;

  struct {
    int* kvpairs;
    int  npairs;
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
    int        right; // index of right
    e_operator op;
    bool       is_compound;
  } unaryop;

  struct {
    int* exprs;
    int  nexprs;
  } exprs, root;

  struct {
    int left;
    int value;
  } member_assign;

  struct {
    char* function;
    int*  args;
    int   nargs;
  } call;

  struct {
    char*  name;
    char** args;  // Allocated, + each string is allocated individually.
    int*   exprs; // Function body
    int    nargs;
    int    nexprs;
  } func;

  struct {
    int  condition;
    int* exprs;
    int  nexprs;
  } while_stmt;

  struct {
    int  initializers;
    int  condition;
    int  iterators;
    int  nexprs;
    int* exprs;
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
  int       nnodes;
  int       capacity;

  e_token* toks;
  int      ntoks;
  int      head;
} e_ast;

nv_error e_ast_init(e_token* toks, int ntoks, e_ast* prsr);
void     e_ast_free(e_ast* prsr);

static inline e_token*
e_ast_next(struct e_ast* prsr)
{
  if (NV_UNLIKELY(prsr->head >= prsr->ntoks)) return NULL;
  return &prsr->toks[prsr->head++];
}
static inline e_token*
e_ast_peek(const struct e_ast* prsr)
{
  if (NV_UNLIKELY(prsr->head >= prsr->ntoks)) return NULL;
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
    int       newcap   = NV_MAX(p->capacity * 2, 1);
    e_asnode* newnodes = (e_asnode*)realloc(p->nodes, newcap * sizeof(e_asnode));
    if (newnodes == NULL) {
      perror("Allocation failed");
      return -1;
    }

    p->nodes    = newnodes;
    p->capacity = newcap;
  }

  int idx = p->nnodes++;
  memset(&p->nodes[idx], 0, sizeof(e_asnode));

  return idx;
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
{
  return t == E_ASNODE_IF || t == E_ASNODE_WHILE || t == E_ASNODE_FOR || t == E_ASNODE_FUNCTION_DEFINITION || t == E_ASNODE_EXPRESSION_LIST;
}

nv_error e_ast_parse(e_ast* p, int* root_nodeID);
int      e_ast_nud(e_ast* p, e_token* token);
int      e_ast_expr(e_ast* p, int rbp);
int      e_ast_led(e_ast* p, e_token* token, int leftidx, int rbp);

nv_error e_ast_expect(e_ast* p, e_tokentype type);

#endif // E_AST_H
