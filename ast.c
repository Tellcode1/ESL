#include "ast.h"

#include "astfree.h"
#include "lex.h"

#include <error.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define peek(parser) e_ast_peek(parser)
#define next(parser) e_ast_next(parser)

static inline bool
_asterror(const char* file, size_t line, e_filespan span, const char* msg, ...)
{
  va_list ap;
  va_start(ap, msg);

  fprintf(stderr, "[%s:%zu] [%s:%i:%i] ast error: ", file, line, span.file, span.line, span.col);
  vfprintf(stderr, msg, ap);

  va_end(ap);

  return true;
}

#define asterror(span, ...) _asterror(__FILE__, __LINE__, span, __VA_ARGS__)

static inline e_filespan
clonespan(e_filespan s)
{
  e_filespan dup = {
    .file = s.file ? strdup(s.file) : nullptr,
    .line = s.line,
    .col  = s.col,
  };
  return dup;
}

/* Returns -1 on invalid !!!!! */
static inline e_operator
conv_token_type_to_operator(e_tokentype t)
{
  switch (t) {
    case E_TOKENTYPE_PLUS: return E_OPERATOR_ADD;
    case E_TOKENTYPE_MINUS: return E_OPERATOR_SUB;
    case E_TOKENTYPE_MULTIPLY: return E_OPERATOR_MUL;
    case E_TOKENTYPE_DIVIDE: return E_OPERATOR_DIV;
    case E_TOKENTYPE_MOD: return E_OPERATOR_MOD;
    case E_TOKENTYPE_EXPONENT: return E_OPERATOR_EXP;
    case E_TOKENTYPE_AND: return E_OPERATOR_AND;
    case E_TOKENTYPE_NOT: return E_OPERATOR_NOT;
    case E_TOKENTYPE_OR: return E_OPERATOR_OR;
    case E_TOKENTYPE_BAND: return E_OPERATOR_BAND;
    case E_TOKENTYPE_BOR: return E_OPERATOR_BOR;
    case E_TOKENTYPE_BNOT: return E_OPERATOR_BNOT;
    case E_TOKENTYPE_XOR: return E_OPERATOR_XOR;
    case E_TOKENTYPE_EQUAL: return E_OPERATOR_EQL;
    case E_TOKENTYPE_NOTEQUAL: return E_OPERATOR_NEQ;
    case E_TOKENTYPE_LT: return E_OPERATOR_LT;
    case E_TOKENTYPE_LTE: return E_OPERATOR_LTE;
    case E_TOKENTYPE_GT: return E_OPERATOR_GT;
    case E_TOKENTYPE_GTE: return E_OPERATOR_GTE;
    default: return -1;
  }
}

/**
 * Single line if statements like if (x < 0) x = -x;
 * are handled by this because the expression parses consumes the semi colon itself,
 * Keeping the parser from getting polluted
 */
static inline int
parse_braces(e_ast* p, int** outstmts, u32* outnstmts)
{
  u32  capacity = 16;
  u32  nstmts   = 0;
  int* stmts    = malloc(sizeof(int) * capacity);
  if (!stmts) goto err;

  while (peek(p) != nullptr && peek(p)->type != E_TOKENTYPE_CLOSEBRACE) {
    if (nstmts + 1 >= capacity) {
      u32  newcap   = MAX(capacity * 2, 1);
      int* newstmts = realloc(stmts, sizeof(int) * newcap);
      if (!newstmts) { goto err; }

      stmts    = newstmts;
      capacity = newcap;
    }

    e_filespan span = peek(p)->span;

    // We need the stmt to figure out if we need to chek for a limiter(;) at the end or not.
    int stmt = e_ast_expr(p, 0);
    if (stmt < 0) {
      asterror(span, "Failed to parse statement in braces\n");
      goto err;
    }

    stmts[nstmts++] = stmt;

    e_asnode* node = e_ast_get_node(p, stmt);

    if (!e_ast_is_limiter_exempt(node->type)) {
      int e = e_ast_expect(p, E_TOKENTYPE_SEMICOLON);
      if (e) {
        asterror(span, "Expected semi colon\n"); // use older span, newer one is token after where semicolon should be
        goto err;
      }
    }
  }

  e_filespan span = peek(p)->span;
  if (e_ast_expect(p, E_TOKENTYPE_CLOSEBRACE)) {
    asterror(span, "Expected closing brace ('}')\n");
    goto err;
  }

  if (outstmts) *outstmts = stmts;
  if (outnstmts) *outnstmts = nstmts;

  return 0;

err:
  for (u32 i = 0; i < nstmts; i++) e_asnode_free(p, stmts[i]);
  if (stmts) free(stmts);
  if (outstmts) *outstmts = nullptr;
  if (outnstmts) *outnstmts = 0;

  return -1;
}

/**
 * Parse a body of a statement.
 * This parses the tail end of the following statements:
 * if (true) { let x = 0; let y = 10; println(x+y); }
 *
 * while(true) i++; or for(;;i++); if you're cool.
 */
static inline int
parse_body(e_ast* p, int** outstmts, u32* outnstmts)
{
  int  nstmts = 0;
  int* stmts  = nullptr;
  int  stmt   = 0;

  if (peek(p) && peek(p)->type == E_TOKENTYPE_OPENBRACE) {
    next(p); // consume {
    return parse_braces(p, outstmts, outnstmts);
  }

  // Explicity handle zero expression lists.
  else if (peek(p) && peek(p)->type == E_TOKENTYPE_SEMICOLON) {
    if (outstmts) *outstmts = nullptr;
    if (outnstmts) *outnstmts = 0;
  }
  // Single line expressions ending with a semi colon.
  else {
    stmts = malloc(sizeof(int));
    if (!stmts) goto err;

    stmt = e_ast_expr(p, 0);

    stmts[0] = stmt;
    nstmts   = 1;

    if (outstmts) *outstmts = stmts;
    if (outnstmts) *outnstmts = nstmts;

    if (e_ast_expect(p, E_TOKENTYPE_SEMICOLON)) {
      asterror(peek(p)->span, "Expected semi colon after expression\n");
      return -1;
    }
  }

  return 0;

err:
  for (u32 i = 0; i < nstmts; i++) { e_asnode_free(p, stmts[i]); }
  free(stmts);
  return -1;
}

int
e_ast_expect(e_ast* p, e_tokentype type)
{
  // printf("%s\n", e_tokentype_to_string(peek(p)->type));
  return next(p)->type == type ? 0 : 1;
}

int
e_ast_expr(e_ast* p, int rbp)
{
  int left = e_ast_nud(p, e_ast_next(p));
  if (left < 0) return left;

  while (e_ast_peek(p)) {
    e_token* op = e_ast_peek(p);

    // printf("peek: %s\n", e_tokentype_to_string(op->type));

    int left_bp = 0, right_bp = 0;
    e_getbp(op->type, &left_bp, &right_bp);

    // printf("bp: %d %d\n", left_bp, right_bp);

    if (left_bp <= rbp) break;

    left = e_ast_led(p, e_ast_next(p), left, right_bp);
    if (left < 0) return left;
  }
  return left;
}

/**
 * Returns nodeid on success, a negative integer on error.
 */
static int
parse_variable_decleration(e_ast* ast, int nodeid)
{
  if (nodeid < 0) return nodeid;

  char* name        = nullptr;
  int   initializer = -1;

  e_token* name_tk = peek(ast);
  if (!name_tk || name_tk->type != E_TOKENTYPE_IDENT) {
    asterror(name_tk ? name_tk->span : (e_filespan){ 0 }, "Expected identifier\n");
    goto err;
  }

  next(ast);
  name = strdup(name_tk->val.ident);

  // printf("%s declared\n", name);

  E_GET_NODE(ast, nodeid)->type         = E_ASNODE_VARIABLE_DECL;
  E_GET_NODE(ast, nodeid)->val.let.name = name; // name is strdup'd.

  /* This is a variable without an initializer */
  if (peek(ast)->type == E_TOKENTYPE_SEMICOLON) {
    E_GET_NODE(ast, nodeid)->val.let.initializer = -1;
    return 0; // Not an error!
  }

  if (e_ast_expect(ast, E_TOKENTYPE_EQUAL)) {
    asterror(peek(ast)->span, "Expected semicolon or initializer\n");
    goto err;
  }

  initializer = e_ast_expr(ast, 0);
  if (initializer < 0) {
    asterror(peek(ast)->span, "Error parsing initializer\n");
    goto err;
  }

  E_GET_NODE(ast, nodeid)->val.let.initializer = initializer;

  return nodeid;

err:
  free(name);
  e_asnode_free(ast, initializer);
  return -1;
}

static int
parse_if(e_ast* p, int nodeid)
{
  if (nodeid < 0) return nodeid;

  u32        cap_else_ifs = 4;
  u32        num_else_ifs = 0;
  e_if_stmt* else_ifs     = malloc(sizeof(e_if_stmt) * cap_else_ifs);

  int* body   = nullptr;
  u32  nstmts = 0;

  int* else_body   = nullptr;
  u32  nelse_stmts = 0;

  int  condition         = -1;
  int  else_if_condition = -1;
  int* else_if_body      = nullptr;
  u32  else_if_nstmts    = 0;

  if (else_ifs == nullptr) goto err;

  if (e_ast_expect(p, E_TOKENTYPE_OPENPAREN)) {
    asterror(peek(p)->span, "Expected '(' after 'if'\n");
    goto err;
  }

  condition = e_ast_expr(p, 0);

  if (e_ast_expect(p, E_TOKENTYPE_CLOSEPAREN)) {
    asterror(peek(p)->span, "Expected ')' after 'if condition'\n");
    goto err;
  }

  // Take the span before so it starts where the body would start.
  e_filespan body_span = peek(p)->span;

  if (parse_body(p, &body, &nstmts)) {
    asterror(body_span, "Error in parsing body of if statement\n");
    goto err;
  }

  if (peek(p) && peek(p)->type == E_TOKENTYPE_ELSE) {
    while (true) {
      if (!peek(p) || peek(p)->type != E_TOKENTYPE_ELSE) break; // no more else chains

      next(p); // consume else

      if (peek(p) && peek(p)->type == E_TOKENTYPE_IF) // else if
      {
        next(p);

        if (num_else_ifs >= cap_else_ifs) {
          u32        newcap           = MAX(cap_else_ifs * 2, 1);
          e_if_stmt* new_else_ifs     = realloc(else_ifs, sizeof(e_if_stmt) * newcap);
          u32        new_cap_else_ifs = newcap;
          if (!else_ifs) goto err;

          else_ifs     = new_else_ifs;
          cap_else_ifs = new_cap_else_ifs;
        }

        if (e_ast_expect(p, E_TOKENTYPE_OPENPAREN)) {
          asterror(peek(p)->span, "Expected '(' after 'else if'\n");
          goto err;
        }

        else_if_condition = e_ast_expr(p, 0);

        if (e_ast_expect(p, E_TOKENTYPE_CLOSEPAREN)) {
          asterror(peek(p)->span, "Expected ')' after condition of 'else if'\n");
          goto err;
        }

        if (parse_body(p, &else_if_body, &else_if_nstmts)) {
          asterror(peek(p)->span, "Error in parsing body of else if statement\n");
          goto err;
        }

        else_ifs[num_else_ifs++] = (e_if_stmt){
          .condition = else_if_condition,
          .body      = else_if_body,
          .nstmts    = else_if_nstmts,
        };
        continue;
      } else {
        e_filespan else_span = peek(p)->span;

        if (parse_body(p, &else_body, &nelse_stmts)) {
          asterror(else_span, "Expected body or statements after 'else'\n");
          goto err;
        }

        break; // no more deals
      }
    }
  }

  // Write the collected infomation to the struct.
  E_GET_NODE(p, nodeid)->type                    = E_ASNODE_IF;
  E_GET_NODE(p, nodeid)->val.if_stmt.body        = body;
  E_GET_NODE(p, nodeid)->val.if_stmt.nstmts      = nstmts;
  E_GET_NODE(p, nodeid)->val.if_stmt.condition   = condition;
  E_GET_NODE(p, nodeid)->val.if_stmt.else_body   = else_body;
  E_GET_NODE(p, nodeid)->val.if_stmt.nelse_stmts = nelse_stmts;
  E_GET_NODE(p, nodeid)->val.if_stmt.else_ifs    = else_ifs;
  E_GET_NODE(p, nodeid)->val.if_stmt.nelse_ifs   = num_else_ifs;

  return nodeid;
err:

  e_asnode_free(p, condition);

  for (u32 i = 0; i < nstmts; i++) e_asnode_free(p, body[i]);
  free(body);

  for (u32 i = 0; i < num_else_ifs; i++) {
    e_asnode_free(p, else_ifs[i].condition);
    for (u32 j = 0; j < else_ifs[i].nstmts; j++) e_asnode_free(p, else_ifs[i].body[j]);
    free(else_ifs[i].body);
  }
  free(else_ifs);

  for (u32 i = 0; i < nelse_stmts; i++) { e_asnode_free(p, else_body[i]); }
  free(else_body);

  return -1;
}

static int
parse_while(e_ast* p, int nodeid)
{
  if (nodeid < 0) return nodeid;

  int  cnd    = -1;
  int* stmts  = nullptr;
  u32  nstmts = 0;

  if (e_ast_expect(p, E_TOKENTYPE_OPENPAREN)) {
    asterror(e_ast_peek(p)->span, "Expected ( after 'while'\n");
    goto err;
  }

  cnd = e_ast_expr(p, 0);
  if (cnd < 0) {
    asterror(e_ast_peek(p)->span, "Expected expression\n");
    goto err;
  }

  if (e_ast_expect(p, E_TOKENTYPE_CLOSEPAREN)) {
    asterror(e_ast_peek(p)->span, "Expected ) after 'while'\n");
    goto err;
  }

  if (parse_body(p, &stmts, &nstmts)) {
    asterror(e_ast_peek(p)->span, "Expected while statement body\n");
    goto err;
  }

  E_GET_NODE(p, nodeid)->type                     = E_ASNODE_WHILE;
  E_GET_NODE(p, nodeid)->val.while_stmt.condition = cnd;
  E_GET_NODE(p, nodeid)->val.while_stmt.stmts     = stmts;
  E_GET_NODE(p, nodeid)->val.while_stmt.nstmts    = nstmts;

  return nodeid;

err:
  if (cnd >= 0) e_asnode_free(p, cnd);
  for (u32 i = 0; i < nstmts; i++) e_asnode_free(p, stmts[i]);
  free(stmts);
  return -1;
}

static int
parse_for(e_ast* p, int nodeid)
{
  int init = -1;
  int cond = -1;
  int iter = -1;

  // (
  if (e_ast_expect(p, E_TOKENTYPE_OPENPAREN)) {
    asterror(e_ast_peek(p)->span, "Expected ( after 'for'\n");
    goto err;
  }

  // let i = 0
  if (peek(p) && peek(p)->type != E_TOKENTYPE_SEMICOLON) {
    init = e_ast_expr(p, 0);
    if (init < 0) {
      asterror(e_ast_peek(p)->span, "Expected expression (initializers) ['for' statement]\n");
      goto err;
    }
  } else {
    next(p);
  }

  // ;
  if (e_ast_expect(p, E_TOKENTYPE_SEMICOLON)) {
    asterror(e_ast_peek(p)->span, "Expected ';' after initializer list ['for' statement]\n");
    goto err;
  }

  // i < 10
  if (peek(p) && peek(p)->type != E_TOKENTYPE_SEMICOLON) {
    cond = e_ast_expr(p, 0);
    if (cond < 0) {
      asterror(e_ast_peek(p)->span, "Expected expression (condition) ['for' statement]\n");
      goto err;
    }
  } else {
    next(p);
  }

  // ;
  if (e_ast_expect(p, E_TOKENTYPE_SEMICOLON)) {
    asterror(e_ast_peek(p)->span, "Expected ';' after condition ['for' statement]\n");
    goto err;
  }

  // i++
  if (peek(p) && peek(p)->type != E_TOKENTYPE_SEMICOLON) {
    iter = e_ast_expr(p, 0);
    if (iter < 0) {
      asterror(e_ast_peek(p)->span, "Expected expression (iterators) ['for' statement]\n");
      goto err;
    }
  } else {
    next(p);
  }

  // )
  if (e_ast_expect(p, E_TOKENTYPE_CLOSEPAREN)) {
    asterror(e_ast_peek(p)->span, "Expected ')' ['for' statement]\n");
    goto err;
  }

  int* stmts;
  u32  nstmts;
  if (parse_body(p, &stmts, &nstmts)) {
    asterror(e_ast_peek(p)->span, "Expected body ['for' statement]\n");
    goto err;
  }

  E_GET_NODE(p, nodeid)->type                      = E_ASNODE_FOR;
  E_GET_NODE(p, nodeid)->val.for_stmt.initializers = init;
  E_GET_NODE(p, nodeid)->val.for_stmt.condition    = cond;
  E_GET_NODE(p, nodeid)->val.for_stmt.iterators    = iter;
  E_GET_NODE(p, nodeid)->val.for_stmt.stmts        = stmts;
  E_GET_NODE(p, nodeid)->val.for_stmt.nstmts       = nstmts;

  return nodeid;

err:
  e_asnode_free(p, init);
  e_asnode_free(p, cond);
  e_asnode_free(p, iter);
  return -1;
}

static int
parse_function(e_ast* p, int nodeid)
{
  if (nodeid < 0) return nodeid;

  u32      names_capacity = 0;
  u32      arg_names_size = 0;
  char**   arg_names      = nullptr;
  char*    function_name  = nullptr;
  e_token* name_tk        = nullptr;

  name_tk = peek(p);
  if (e_ast_expect(p, E_TOKENTYPE_IDENT)) {
    asterror(e_ast_peek(p)->span, "Expected function name after 'fn' [function header]\n");
    goto err;
  }

  if (e_ast_expect(p, E_TOKENTYPE_OPENPAREN)) {
    asterror(e_ast_peek(p)->span, "Expected ( [function header]\n");
    goto err;
  }

  function_name = strdup(name_tk->val.ident);

  names_capacity = 32; // NON ZERO
  arg_names_size = 0;
  arg_names      = malloc(sizeof(char*) * names_capacity);

  while (peek(p) && peek(p)->type != E_TOKENTYPE_CLOSEPAREN) {
    e_token* tk = peek(p);
    if (tk->type != E_TOKENTYPE_IDENT) {
      asterror(peek(p)->span, "Expected parameter name [function header]\n");
      goto err;
    }
    next(p);

    if (arg_names_size >= names_capacity) {
      u32    new_capacity = names_capacity * 2;
      char** new_names    = realloc(arg_names, sizeof(char*) * new_capacity);

      if (!new_names) {
        asterror(peek(p)->span, "Allocation error! [function header]\n");
        goto err;
      }

      arg_names      = new_names;
      names_capacity = new_capacity;
    }

    arg_names[arg_names_size] = strdup(tk->val.ident);
    arg_names_size++;

    if (peek(p) && peek(p)->type == E_TOKENTYPE_CLOSEPAREN) { break; }

    if (e_ast_expect(p, E_TOKENTYPE_COMMA)) {
      asterror(e_ast_peek(p)->span, "Expected ',' [function header]\n");
      goto err;
    }
  }

  if (e_ast_expect(p, E_TOKENTYPE_CLOSEPAREN)) {
    asterror(e_ast_peek(p)->span, "Expected ')' [function header]\n");
    goto err;
  }

  e_filespan parsing_span = peek(p)->span;

  int* stmts;
  u32  nstmts;
  if (parse_body(p, &stmts, &nstmts)) {
    asterror(parsing_span, "Failed to parse function body\n");
    goto err;
  }

  E_GET_NODE(p, nodeid)->type            = E_ASNODE_FUNCTION_DEFINITION;
  E_GET_NODE(p, nodeid)->val.func.name   = function_name;
  E_GET_NODE(p, nodeid)->val.func.args   = arg_names;
  E_GET_NODE(p, nodeid)->val.func.nargs  = arg_names_size;
  E_GET_NODE(p, nodeid)->val.func.stmts  = stmts;
  E_GET_NODE(p, nodeid)->val.func.nstmts = nstmts;

  return nodeid;

err:
  for (u32 i = 0; i < arg_names_size; i++) { free(arg_names[i]); }
  free(arg_names);
  free(function_name);
  for (u32 i = 0; i < nstmts; i++) { e_asnode_free(p, stmts[i]); }
  free(stmts);
  return -1;
}

static int
parse_function_call(e_ast* p, e_token* tk, int node)
{
  u32  capacity = 4;
  u32  nargs    = 0;
  int* args     = malloc(capacity * sizeof(int)); // argument nodes

  if (e_ast_expect(p, E_TOKENTYPE_OPENPAREN)) // (
  {
    asterror(peek(p)->span, "Expected '(' after function name\n");
    goto err;
  }

  E_GET_NODE(p, node)->type              = E_ASNODE_CALL;
  E_GET_NODE(p, node)->val.call.function = strdup(tk->val.ident);
  // printf("AST: Detected function call: %s\n", tk->val.ident);

  while (peek(p) && peek(p)->type != E_TOKENTYPE_CLOSEPAREN) {
    if (nargs >= capacity) {
      u32  newcap       = MAX(capacity * 2, 1);
      int* new_args     = realloc(args, newcap * sizeof(int));
      u32  new_capacity = newcap;

      if (new_args == nullptr) goto err;

      args     = new_args;
      capacity = new_capacity;
    }

    /**
     * 0 binding power is to parse everything
     * until the closing parenthesis.
     */
    args[nargs++] = e_ast_expr(p, 0);
    if (peek(p)->type == E_TOKENTYPE_CLOSEPAREN) { break; }
    if (e_ast_expect(p, E_TOKENTYPE_COMMA)) {
      asterror(peek(p)->span, "Expected ',' or ')'\n");
      goto err;
    }
  }
  // consume )
  next(p);

  E_GET_NODE(p, node)->val.call.args  = args;
  E_GET_NODE(p, node)->val.call.nargs = nargs;

  return node;

err:
  for (u32 i = 0; i < nargs; i++) { e_asnode_free(p, args[i]); }
  free(args);
  return -1;
}

/**
 * On success, returns the node id.
 */
static inline int
make_literal_node(e_ast* p, int nodeid, const e_token* tk)
{
  if (nodeid < 0) return nodeid;

  e_asnode* node = e_ast_get_node(p, nodeid);
  if (tk->type == E_TOKENTYPE_INT) {
    node->type  = E_ASNODE_INT;
    node->val.i = tk->val.i;
  } else if (tk->type == E_TOKENTYPE_FLOAT) {
    node->type  = E_ASNODE_FLOAT;
    node->val.f = tk->val.f;
  } else if (tk->type == E_TOKENTYPE_BOOL) {
    node->type  = E_ASNODE_BOOL;
    node->val.b = tk->val.b;
  } else if (tk->type == E_TOKENTYPE_CHAR) {
    node->type  = E_ASNODE_CHAR;
    node->val.c = tk->val.c;
  } else if (tk->type == E_TOKENTYPE_STRING) {
    node->type  = E_ASNODE_STRING;
    node->val.s = strdup(tk->val.s);
    if (!node->val.s) return -1;
  }
  return nodeid;
}

int
e_ast_nud(e_ast* p, e_token* tk)
{
  if (!tk || !p) return -1;

  /**
   * !!!
   * Node is owned by this function, and led!!!
   * If any error occurs, only these function are allowed
   * to free the node.
   */
  int node = e_ast_make_node(p);
  if (node < 0) return node; // ERROR!

  /* zero out the node for safety. */
  memset(E_GET_NODE(p, node), 0, sizeof(e_asnode));

  // if (tk->type == E_TOKENTYPE_EOF) return -1;

  e_filespan clone              = clonespan(tk->span);
  e_ast_get_node(p, node)->span = clone;

  switch (tk->type) {
    case E_TOKENTYPE_INT:
    case E_TOKENTYPE_CHAR:
    case E_TOKENTYPE_BOOL:
    case E_TOKENTYPE_FLOAT:
    case E_TOKENTYPE_STRING: {
      int e = make_literal_node(p, node, tk);
      if (e < 0) {
        /**
         * This doesn't really have anything much to do with releasing the literal
         * Mostly just about freeing the node metadata.
         */
        e_asnode_free(p, node);
        return -1;
      }
      return node;
    }

    case E_TOKENTYPE_IDENT: {
      // Function call
      if (peek(p)->type == E_TOKENTYPE_OPENPAREN) {
        if (parse_function_call(p, tk, node) < 0) {
          e_asnode_free(p, node);
          return -1;
        }
        return node;
      }

      // Just a variable!
      E_GET_NODE(p, node)->type      = E_ASNODE_VARIABLE;
      E_GET_NODE(p, node)->val.ident = strdup(tk->val.ident);
      return node;
    }

    case E_TOKENTYPE_OPENPAREN: {
      /* we're replacing the node, so free the old one that we allocated */
      e_asnode_free(p, node);

      node = e_ast_expr(p, 0);
      if (e_ast_expect(p, E_TOKENTYPE_CLOSEPAREN)) {
        asterror(peek(p)->span, "Expected ')'\n");
        e_asnode_free(p, node);
        return -1;
      }
      return node;
    }

    /* { stmts;stmts;stmts; } */
    case E_TOKENTYPE_OPENBRACE: {
      int* stmts  = nullptr;
      u32  nstmts = 0;
      if (parse_braces(p, &stmts, &nstmts)) {
        e_asnode_free(p, node);
        return -1;
      }

      E_GET_NODE(p, node)->type             = E_ASNODE_EXPRESSION_LIST;
      E_GET_NODE(p, node)->val.stmts.stmts  = stmts;
      E_GET_NODE(p, node)->val.stmts.nstmts = nstmts;

      return node;
    }

    case E_TOKENTYPE_BREAK: {
      E_GET_NODE(p, node)->type = E_ASNODE_BREAK;
      return node;
    }

    case E_TOKENTYPE_CONTINUE: {
      E_GET_NODE(p, node)->type = E_ASNODE_CONTINUE;
      return node;
    }

    case E_TOKENTYPE_RETURN: {
      E_GET_NODE(p, node)->type = E_ASNODE_RETURN;
      if (peek(p) && peek(p)->type != E_TOKENTYPE_SEMICOLON) {
        // Expect an expression
        int expr = e_ast_expr(p, 0);
        if (expr < 0) {
          e_asnode_free(p, node);
          return expr; // Parser will eat semicolon
        }

        E_GET_NODE(p, node)->val.ret.expr_id          = expr;
        E_GET_NODE(p, node)->val.ret.has_return_value = true;
      } else {
        E_GET_NODE(p, node)->val.ret.expr_id          = -1;
        E_GET_NODE(p, node)->val.ret.has_return_value = false;
      }

      return node;
    }

    case E_TOKENTYPE_LET: {
      if (parse_variable_decleration(p, node) < 0) {
        e_asnode_free(p, node);
        return -1;
      }
      return node;
    }

    case E_TOKENTYPE_IF: {
      if (parse_if(p, node) < 0) {
        e_asnode_free(p, node);
        return -1;
      }
      return node;
    }

    case E_TOKENTYPE_WHILE: {
      if (parse_while(p, node) < 0) {
        e_asnode_free(p, node);
        return -1;
      }
      return node;
    }

    case E_TOKENTYPE_FOR: {
      if (parse_for(p, node) < 0) {
        e_asnode_free(p, node);
        return -1;
      }
      return node;
    }

    case E_TOKENTYPE_FN: {
      if (parse_function(p, node) < 0) {
        e_asnode_free(p, node);
        return -1;
      }
      return node;
    }

    case E_TOKENTYPE_SEMICOLON: {
      E_GET_NODE(p, node)->type = E_ASNODE_NOP;
      return node;
    }

    // Try to parse it as a unary operator
    default: {
      int left_bp = 0, right_bp = 0;
      if (!e_getbp(tk->type, &left_bp, &right_bp)) {
        asterror(tk->span, "Unexpected token (supposedly a unary operator) '%s'\n", e_tokentype_to_string(tk->type));
        e_asnode_free(p, node);
        return -1;
      }

      e_operator op = conv_token_type_to_operator(tk->type);
      if (op < 0) {
        asterror(tk->span, "Unexpected token (supposedly a unary operator) '%s'\n", e_tokentype_to_string(tk->type));
        e_asnode_free(p, node);
        return -1;
      }

      int right = e_ast_expr(p, right_bp);
      if (right < 0) {
        asterror(tk->span, "Failed to evaluate RHS of unary operator\n");
        e_asnode_free(p, node);
        return -1;
      }

      E_GET_NODE(p, node)->type                    = E_ASNODE_UNARYOP;
      E_GET_NODE(p, node)->val.unaryop.op          = op;
      E_GET_NODE(p, node)->val.unaryop.is_compound = tk->val.op.is_compound;
      E_GET_NODE(p, node)->val.unaryop.right       = right;
      return node;
    }
  }

  /* The handlers consume all the tokens that nud() is supposed to consume. */
  return -1;
}

int
e_ast_led(e_ast* p, e_token* tk, int leftidx, int rbp)
{
  if (!tk || !p || leftidx < 0) return -1;

  int node = e_ast_make_node(p);
  if (node < 0) return node; // ERROR!

  switch (tk->type) {
    case E_TOKENTYPE_EQUAL: {
      // printf("ASSIGN YAYY!!\n");

      // Assigning to a member of a struct
      if (e_ast_get_node(p, leftidx)->type == E_ASNODE_MEMBER_ACCESS) {
        int rightidx = e_ast_expr(p, rbp);
        if (rightidx < 0) {
          asterror(peek(p)->span, "Invalid RHS in member assignment\n");
          e_asnode_free(p, node);
          return -1;
        }

        E_GET_NODE(p, node)->type                    = E_ASNODE_MEMBER_ASSIGN;
        E_GET_NODE(p, node)->val.member_assign.left  = leftidx;
        E_GET_NODE(p, node)->val.member_assign.value = rightidx;
        return node;
      }

      int rightidx = e_ast_expr(p, rbp);
      if (rightidx < 0) {
        asterror(peek(p)->span, "Invalid RHS in assignment\n");
        e_asnode_free(p, node);
        return -1;
      }

      E_GET_NODE(p, node)->type               = E_ASNODE_ASSIGN;
      E_GET_NODE(p, node)->val.binaryop.op    = E_OPERATOR_EQL;
      E_GET_NODE(p, node)->val.binaryop.left  = leftidx;
      E_GET_NODE(p, node)->val.binaryop.right = rightidx;

      return node;
    }

    case E_TOKENTYPE_PLUSPLUS: {
      E_GET_NODE(p, node)->type                    = E_ASNODE_UNARYOP;
      E_GET_NODE(p, node)->val.unaryop.op          = E_OPERATOR_INC;
      E_GET_NODE(p, node)->val.unaryop.is_compound = true;
      E_GET_NODE(p, node)->val.unaryop.right       = leftidx; // operand is left
      return node;
    }

    case E_TOKENTYPE_MINUSMINUS: {
      E_GET_NODE(p, node)->type                    = E_ASNODE_UNARYOP;
      E_GET_NODE(p, node)->val.unaryop.op          = E_OPERATOR_DEC;
      E_GET_NODE(p, node)->val.unaryop.is_compound = true;
      E_GET_NODE(p, node)->val.unaryop.right       = leftidx;
      return node;
    }

    default: {
      int left_bp = 0, right_bp = 0;
      if (!e_getbp(tk->type, &left_bp, &right_bp)) {
        asterror(tk->span, "Unexpected token (supposedly a binary operator) '%s'\n", e_tokentype_to_string(tk->type));
        e_asnode_free(p, node);
        return -1;
      }

      e_operator op = conv_token_type_to_operator(tk->type);
      if (op == -1) {
        asterror(tk->span, "Unknown binary operator\n");
        e_asnode_free(p, node);
        return -1;
      }

      int right = e_ast_expr(p, right_bp);
      if (right < 0) {
        asterror(tk->span, "Failed to evaluate RHS of binary operator\n");
        e_asnode_free(p, node);
        return -1;
      }

      E_GET_NODE(p, node)->type                     = E_ASNODE_BINARYOP;
      E_GET_NODE(p, node)->val.binaryop.op          = op;
      E_GET_NODE(p, node)->val.binaryop.left        = leftidx;
      E_GET_NODE(p, node)->val.binaryop.is_compound = tk->val.op.is_compound;
      E_GET_NODE(p, node)->val.binaryop.right       = right;
      return node;
    }
  }
}

int
e_ast_parse(e_ast* p, int* out_root_node)
{
  if (!p) return -1;

  u32  cap    = 16;
  u32  nstmts = 0;
  int  node   = -1;
  int* stmts  = malloc(cap * sizeof(int));

  int rootnode = e_ast_make_node(p);
  if (rootnode < 0) return -1;

  if (out_root_node) *out_root_node = rootnode;

  e_ast_get_node(p, rootnode)->type = E_ASNODE_ROOT;

  if (!stmts) { goto err; }

  // for (int i = 0; i < p->ntoks; i++) // Only run from 0 to ntoks-1 because the last token is EOF.
  while (peek(p) && peek(p)->type != E_TOKENTYPE_EOF) {
    e_filespan take_span = peek(p)->span;

    node = e_ast_expr(p, 0);
    if (node < 0) {
      asterror(take_span, "Error parsing AST\n");
      goto err;
    }

    e_asnode_type type = E_GET_NODE(p, node)->type;
    if (type != E_ASNODE_FUNCTION_DEFINITION) {
      // asterror(take_span, "Expected function definition or variable declerations in global scope\n");
      asterror(take_span, "Expected only function definition in global scope\n");
      goto err;
    }

    if (!e_ast_is_limiter_exempt(e_ast_get_node(p, node)->type) && e_ast_expect(p, E_TOKENTYPE_SEMICOLON)) {
      asterror(take_span, "Expected semicolon after expression\n");
      goto err;
    }

    if (nstmts >= cap) {
      u32  newcap   = MAX(cap * 2, 1);
      int* newstmts = realloc(stmts, newcap * sizeof(int));
      if (!newstmts) {
        asterror(take_span, "realloc failed! Can not continue\n");
        goto err;
      }
      stmts = newstmts;
      cap   = newcap;
    }

    // Add it to our list.
    stmts[nstmts++] = node;
  }

  e_ast_get_node(p, rootnode)->val.root.stmts  = stmts;
  e_ast_get_node(p, rootnode)->val.root.nstmts = nstmts;

  return 0;

err:
  if (rootnode >= 0) { e_asnode_free(p, rootnode); }
  if (node >= 0) { e_asnode_free(p, node); }
  /* node will never be in stmts. */
  for (u32 i = 0; i < nstmts; i++) { e_asnode_free(p, stmts[i]); }
  free(stmts);
  return -1;
}

int
e_ast_init(e_token* toks, u32 ntoks, e_ast* prsr)
{
  if (prsr) {
    const u32 init_nodes = 64;

    *prsr = (e_ast){
      .nodes    = malloc(sizeof(e_asnode) * init_nodes),
      .nnodes   = 0,
      .capacity = init_nodes,
      .toks     = toks,
      .ntoks    = ntoks,
      .head     = 0,
    };
    if (!prsr->nodes) return -1;
  }
  return 0;
}

void
e_ast_free(e_ast* prsr)
{
  free(prsr->nodes);
}
