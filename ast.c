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

#include "ast.h"

#include "arena.h"
#include "cerr.h"
#include "lex.h"

#include <error.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define prev(parser) e_ast_prev(parser)
#define peek(parser) e_ast_peek(parser)
#define next(parser) e_ast_next(parser)

#if defined(__has_attribute) && __has_attribute(format)
#  define _FORMAT_(...) __attribute__((format(__VA_ARGS__)))
#endif

static inline _FORMAT_(printf, 4, 5) bool _asterror(const char* file, size_t line, e_filespan span, const char* msg, ...)
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
clonespan(e_ast* a, e_filespan s)
{
  e_filespan dup = {
    .file = e_arnstrdup(a->arena, s.file),
    .line = s.line,
    .col  = s.col,
  };
  // printf("[%s:%i:%i]\n", dup.file, dup.line, dup.col);
  return dup;
}

/* Returns -1 on invalid !!!!! */
static inline e_operator
conv_token_type_to_operator(e_token_type t)
{
  switch (t) {
    case E_TOKEN_TYPE_PLUS: return E_OPERATOR_ADD;
    case E_TOKEN_TYPE_MINUS: return E_OPERATOR_SUB;
    case E_TOKEN_TYPE_MULTIPLY: return E_OPERATOR_MUL;
    case E_TOKEN_TYPE_DIVIDE: return E_OPERATOR_DIV;
    case E_TOKEN_TYPE_MOD: return E_OPERATOR_MOD;
    case E_TOKEN_TYPE_EXPONENT: return E_OPERATOR_EXP;
    case E_TOKEN_TYPE_AND: return E_OPERATOR_AND;
    case E_TOKEN_TYPE_NOT: return E_OPERATOR_NOT;
    case E_TOKEN_TYPE_OR: return E_OPERATOR_OR;
    case E_TOKEN_TYPE_BAND: return E_OPERATOR_BAND;
    case E_TOKEN_TYPE_BOR: return E_OPERATOR_BOR;
    case E_TOKEN_TYPE_BNOT: return E_OPERATOR_BNOT;
    case E_TOKEN_TYPE_XOR: return E_OPERATOR_XOR;
    case E_TOKEN_TYPE_DOUBLEEQUAL: return E_OPERATOR_ISEQL;
    case E_TOKEN_TYPE_NOTEQUAL: return E_OPERATOR_ISNEQ;
    case E_TOKEN_TYPE_LT: return E_OPERATOR_LT;
    case E_TOKEN_TYPE_LTE: return E_OPERATOR_LTE;
    case E_TOKEN_TYPE_GT: return E_OPERATOR_GT;
    case E_TOKEN_TYPE_GTE: return E_OPERATOR_GTE;
    default: return -1;
  }
}

/**
 * Single line if statements like if (x < 0) x = -x;
 * are handled by this because the expression parses consumes the semi colon itself,
 * Keeping the parser from getting polluted
 *
 * Returns 0 on succcess.
 */
static inline int
parse_braces(e_ast* p, int** outstmts, u32* outnstmts)
{
  u32  capacity = 16;
  u32  nstmts   = 0;
  int* stmts    = e_arnalloc(p->arena, sizeof(int) * capacity);
  if (!stmts) goto err;

  while (peek(p) != nullptr && peek(p)->type != E_TOKEN_TYPE_CLOSEBRACE) {
    if (nstmts + 1 >= capacity) {
      u32  newcap   = MAX(capacity * 2, 1);
      int* newstmts = e_arnrealloc(p->arena, stmts, sizeof(int) * newcap);
      if (!newstmts) { goto err; }

      stmts    = newstmts;
      capacity = newcap;
    }

    e_filespan span = peek(p)->span;

    // We need the stmt to figure out if we need to chek for a limiter(;) at the end or not.
    int stmt = e_ast_expr(p, 0);
    if (stmt < 0) {
      asterror(span, "Failed to parse braced statement [braced statement list]\n");
      goto err;
    }

    stmts[nstmts++] = stmt;

    if (!e_ast_is_limiter_exempt(E_GET_NODE(p, stmt)->type)) {
      if (e_ast_expect(p, E_TOKEN_TYPE_SEMICOLON)) {
        /* we need to be expecting a semi colon a character after the last */
        span.col++;
        asterror(
            prev(p)->span,
            "Expected semi colon ';', got '%s' [braced statement list]\n",
            e_token_type_to_string(prev(p)->type)); // use older span, newer one is token after where semicolon should be
        goto err;
      }
    }
  }

  if (e_ast_expect(p, E_TOKEN_TYPE_CLOSEBRACE)) {
    asterror(prev(p)->span, "Expected closing brace '}' [braced statement list]\n");
    goto err;
  }

  if (outstmts) *outstmts = stmts;
  if (outnstmts) *outnstmts = nstmts;

  return 0;

err:
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
 *
 * Returns 0 on succcess.
 */
static inline int
parse_body(e_ast* p, int** outstmts, u32* outnstmts)
{
  int  nstmts = 0;
  int* stmts  = nullptr;
  int  stmt   = 0;

  if (peek(p) && peek(p)->type == E_TOKEN_TYPE_OPENBRACE) {
    next(p); // consume {
    return parse_braces(p, outstmts, outnstmts);
  }

  // Explicity handle zero expression lists.
  else if (peek(p) && peek(p)->type == E_TOKEN_TYPE_SEMICOLON) {
    if (outstmts) *outstmts = nullptr;
    if (outnstmts) *outnstmts = 0;
  }
  // Single line expressions ending with a semi colon.
  else {
    stmts = e_arnalloc(p->arena, sizeof(int));
    if (!stmts) goto err;

    e_filespan prev_span = peek(p)->span;

    stmt = e_ast_expr(p, 0);
    if (stmt < 0) {
      asterror(prev_span, "Failed to parse single line statement\n");
      goto err;
    }

    stmts[0] = stmt;
    nstmts   = 1;

    if (outstmts) *outstmts = stmts;
    if (outnstmts) *outnstmts = nstmts;

    if (e_ast_expect(p, E_TOKEN_TYPE_SEMICOLON)) {
      asterror(prev(p)->span, "Expected semi colon after expression\n");
      goto err;
    }
  }

  return 0;

err:
  return -1;
}

int
e_ast_expect(e_ast* p, e_token_type type)
{
  // printf("%s\n", e_token_type_to_string(peek(p)->type));
  return next(p)->type == type ? 0 : 1;
}

int
e_ast_expr(e_ast* p, int rbp)
{
  e_token* tk   = next(p);
  int      left = e_ast_nud(p, tk);
  if (left < 0) return left;

  E_GET_NODE(p, left)->common.span = clonespan(p, tk->span);

  while (e_ast_peek(p)) {
    e_token* op = e_ast_peek(p);

    // printf("peek: %s\n", e_token_type_to_string(op->type));

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
 * Returns node on success, a negative integer on error.
 */
static int
parse_variable_decleration(e_ast* ast, bool is_const, int node)
{
  if (node < 0) return node;

  u32   capacity = 4;
  u32   ndecls   = 0;
  int*  decls    = e_arnalloc(ast->arena, sizeof(int) * capacity);
  char* name     = nullptr;
  if (!decls) goto err;

  while (true) {
    e_token* name_tk = peek(ast);
    if (!name_tk || name_tk->type != E_TOKEN_TYPE_IDENT) {
      asterror(name_tk->span, "Expected identifier after 'let', got '%s' [variable decleration]\n", e_token_type_to_string(name_tk->type));
      goto err;
    }

    next(ast);

    int decl_node = e_ast_make_node(ast);
    if (decl_node < 0) goto err;

    name = e_arnstrdup(ast->arena, name_tk->val.s);
    if (!name) goto err;

    E_GET_NODE(ast, decl_node)->type         = E_AST_NODE_VARIABLE_DECL;
    E_GET_NODE(ast, decl_node)->let.span     = clonespan(ast, name_tk->span);
    E_GET_NODE(ast, decl_node)->let.name     = name;
    E_GET_NODE(ast, decl_node)->let.is_const = is_const;

    int initializer = -1;

    if (peek(ast)->type == E_TOKEN_TYPE_EQUAL) {
      next(ast); // consume =
      initializer = e_ast_expr(ast, 0);
      if (initializer < 0) {
        asterror(peek(ast)->span, "Error parsing initializer [variable decleration]\n");
        goto err;
      }
    }

    E_GET_NODE(ast, decl_node)->let.initializer = initializer;

    if (ndecls >= capacity) {
      u32  newcap = capacity * 2;
      int* tmp    = e_arnrealloc(ast->arena, decls, sizeof(int) * newcap);
      if (!tmp) goto err;
      decls    = tmp;
      capacity = newcap;
    }

    decls[ndecls++] = decl_node;

    if (!peek(ast) || peek(ast)->type != E_TOKEN_TYPE_COMMA) break;

    next(ast); // consume ,
  }

  // optimizations ooo
  if (ndecls == 1) {
    // ooo optimizations ruined span collection yay
    e_ast_node* src = E_GET_NODE(ast, decls[0]);
    e_ast_node* dst = E_GET_NODE(ast, node);

    dst->type            = src->type;
    dst->let.name        = src->let.name;
    dst->let.span        = src->let.span;
    dst->let.initializer = src->let.initializer;
    dst->let.is_const    = src->let.is_const;

    // prevent double free
    src->let.name      = NULL;
    src->let.span.file = NULL;
    src->type          = E_AST_NODE_NOP;

    // free(decls);

    return node;
  }

  E_GET_NODE(ast, node)->type         = E_AST_NODE_STATEMENT_LIST;
  E_GET_NODE(ast, node)->stmts.stmts  = decls;
  E_GET_NODE(ast, node)->stmts.nstmts = ndecls;

  return node;

err:
  return -1;
}

static int
parse_if(e_ast* p, int node)
{
  if (node < 0) return node;

  u32        cap_else_ifs = 4;
  u32        num_else_ifs = 0;
  e_if_stmt* else_ifs     = e_arnalloc(p->arena, sizeof(e_if_stmt) * cap_else_ifs);

  int* body   = nullptr;
  u32  nstmts = 0;

  int* else_body   = nullptr;
  u32  nelse_stmts = 0;

  int  condition         = -1;
  int  else_if_condition = -1;
  int* else_if_body      = nullptr;
  u32  else_if_nstmts    = 0;

  if (else_ifs == nullptr) goto err;

  if (e_ast_expect(p, E_TOKEN_TYPE_OPENPAREN)) {
    asterror(prev(p)->span, "Expected '(' after 'if', got '%s' [if statement]\n", e_token_type_to_string(prev(p)->type));
    goto err;
  }

  e_filespan prev_span = peek(p)->span;

  condition = e_ast_expr(p, 0);
  if (condition < 0) {
    asterror(prev_span, "Expected condition, got '%s' [if statement]\n", e_token_type_to_string(prev(p)->type));
    goto err;
  }

  if (e_ast_expect(p, E_TOKEN_TYPE_CLOSEPAREN)) {
    asterror(prev(p)->span, "Expected ')' after condition, but got '%s' [if statement]\n", e_token_type_to_string(prev(p)->type));
    goto err;
  }

  // Take the span before so it starts where the body would start.
  e_filespan body_span = peek(p)->span;

  if (parse_body(p, &body, &nstmts)) {
    asterror(body_span, "Error in parsing body, but got '%s' [if statement]\n", e_token_type_to_string(prev(p)->type));
    goto err;
  }

  if (peek(p) && peek(p)->type == E_TOKEN_TYPE_ELSE) {
    while (true) {
      if (!peek(p) || peek(p)->type != E_TOKEN_TYPE_ELSE) break; // no more else chains

      next(p); // consume else

      if (peek(p) && peek(p)->type == E_TOKEN_TYPE_IF) // else if
      {
        next(p);

        if (num_else_ifs >= cap_else_ifs) {
          u32        newcap           = MAX(cap_else_ifs * 2, 1);
          e_if_stmt* new_else_ifs     = e_arnrealloc(p->arena, else_ifs, sizeof(e_if_stmt) * newcap);
          u32        new_cap_else_ifs = newcap;
          if (!new_else_ifs) goto err;

          else_ifs     = new_else_ifs;
          cap_else_ifs = new_cap_else_ifs;
        }

        if (e_ast_expect(p, E_TOKEN_TYPE_OPENPAREN)) {
          asterror(prev(p)->span, "Expected '(' after 'else if', but got '%s' [if statement :: else if]\n", e_token_type_to_string(prev(p)->type));
          goto err;
        }

        else_if_condition = e_ast_expr(p, 0);

        if (e_ast_expect(p, E_TOKEN_TYPE_CLOSEPAREN)) {
          asterror(prev(p)->span, "Expected ')' after condition, but got '%s' [if statement :: else if]\n", e_token_type_to_string(prev(p)->type));
          goto err;
        }

        body_span = peek(p)->span;
        if (parse_body(p, &else_if_body, &else_if_nstmts)) {
          asterror(body_span, "Error in parsing body, but got '%s' [if statement :: else if]\n", e_token_type_to_string(prev(p)->type));
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
          asterror(else_span, "Error in parsing body, but got '%s' [if statement :: else]\n", e_token_type_to_string(prev(p)->type));
          goto err;
        }

        break; // no more deals
      }
    }
  }

  // Write the collected infomation to the struct.
  E_GET_NODE(p, node)->type                = E_AST_NODE_IF;
  E_GET_NODE(p, node)->if_stmt.body        = body;
  E_GET_NODE(p, node)->if_stmt.nstmts      = nstmts;
  E_GET_NODE(p, node)->if_stmt.condition   = condition;
  E_GET_NODE(p, node)->if_stmt.else_body   = else_body;
  E_GET_NODE(p, node)->if_stmt.nelse_stmts = nelse_stmts;
  E_GET_NODE(p, node)->if_stmt.else_ifs    = else_ifs;
  E_GET_NODE(p, node)->if_stmt.nelse_ifs   = num_else_ifs;

  return node;
err:
  return -1;
}

static int
parse_while(e_ast* p, int node)
{
  if (node < 0) return node;

  int  cnd    = -1;
  int* stmts  = nullptr;
  u32  nstmts = 0;

  // if (e_ast_expect(p, E_TOKEN_TYPE_OPENPAREN)) {
  //   asterror(prev(p)->span, "Expected ( after 'while'\n");
  //   goto err;
  // }
  if (peek(p)->type == E_TOKEN_TYPE_OPENPAREN) next(p);

  e_filespan prev_span = peek(p)->span;

  cnd = e_ast_expr(p, 0);
  if (cnd < 0) {
    asterror(prev_span, "Expected expression\n");
    goto err;
  }

  if (peek(p)->type == E_TOKEN_TYPE_CLOSEPAREN) next(p);

  // if (e_ast_expect(p, E_TOKEN_TYPE_CLOSEPAREN)) {
  //   asterror(prev(p)->span, "Expected ) after 'while'\n");
  //   goto err;
  // }

  prev_span = peek(p)->span;
  if (parse_body(p, &stmts, &nstmts)) {
    asterror(prev_span, "Failed to parse while statement body\n");
    goto err;
  }

  E_GET_NODE(p, node)->type                 = E_AST_NODE_WHILE;
  E_GET_NODE(p, node)->while_stmt.condition = cnd;
  E_GET_NODE(p, node)->while_stmt.stmts     = stmts;
  E_GET_NODE(p, node)->while_stmt.nstmts    = nstmts;

  return node;

err:
  return -1;
}

static int
parse_for(e_ast* p, int node)
{
  int init = -1;
  int cond = -1;

  u32  niterators = 0;
  u32  capacity   = 8;
  int* iterators  = e_arnalloc(p->arena, capacity * sizeof(int));

  // (
  if (e_ast_expect(p, E_TOKEN_TYPE_OPENPAREN)) {
    asterror(prev(p)->span, "Expected ( after 'for', got '%s' [for statement]\n", e_token_type_to_string(prev(p)->type));
    goto err;
  }

  // let i = 0
  if (peek(p) && peek(p)->type != E_TOKEN_TYPE_SEMICOLON) {
    e_filespan prev_span = peek(p)->span;

    init = e_ast_expr(p, 0);
    if (init < 0) {
      asterror(prev_span, "Expected expression (initializers), got '%s' [for statement]\n", e_token_type_to_string(prev(p)->type));
      goto err;
    }
  } else {
    next(p);
  }

  // ;
  if (e_ast_expect(p, E_TOKEN_TYPE_SEMICOLON)) {
    asterror(prev(p)->span, "Expected ';' after initializer list, got '%s' [for statement]\n", e_token_type_to_string(prev(p)->type));
    goto err;
  }

  // i < 10
  if (peek(p) && peek(p)->type != E_TOKEN_TYPE_SEMICOLON) {
    e_filespan prev_span = peek(p)->span;
    cond                 = e_ast_expr(p, 0);
    if (cond < 0) {
      asterror(prev_span, "Expected expression (condition), got '%s' [for statement]\n", e_token_type_to_string(prev(p)->type));
      goto err;
    }
  } else {
    next(p);
  }

  // ;
  if (e_ast_expect(p, E_TOKEN_TYPE_SEMICOLON)) {
    asterror(prev(p)->span, "Expected ';' after condition, got '%s' [for statement]\n", e_token_type_to_string(prev(p)->type));
    goto err;
  }

  // i++,y++

  if (peek(p) && peek(p)->type == E_TOKEN_TYPE_SEMICOLON) {
    next(p);
  } else {
    while (true) {
      if (niterators >= capacity) {
        u32  new_capacity  = capacity * 2;
        int* new_iterators = e_arnrealloc(p->arena, iterators, new_capacity * sizeof(int));

        if (!new_iterators) { goto err; }

        iterators = new_iterators;
        capacity  = new_capacity;
      }

      e_filespan prev_span = peek(p)->span;
      int        iter      = e_ast_expr(p, 0);
      if (iter < 0) {
        asterror(prev_span, "Expected expression (iterators), got '%s' [for statement]\n", e_token_type_to_string(prev(p)->type));
        goto err;
      }

      iterators[niterators++] = iter;

      if (peek(p) && peek(p)->type != E_TOKEN_TYPE_COMMA) break;
      next(p); // consume ,
    }
  }

  // )
  if (e_ast_expect(p, E_TOKEN_TYPE_CLOSEPAREN)) {
    asterror(prev(p)->span, "Expected ')' after header, got '%s' [for statement]\n", e_token_type_to_string(prev(p)->type));
    goto err;
  }

  e_filespan prev_span = peek(p)->span;

  int* stmts;
  u32  nstmts;
  if (parse_body(p, &stmts, &nstmts)) {
    asterror(prev_span, "Failed to parse body [for statement]\n");
    goto err;
  }

  E_GET_NODE(p, node)->type                  = E_AST_NODE_FOR;
  E_GET_NODE(p, node)->for_stmt.initializers = init;
  E_GET_NODE(p, node)->for_stmt.condition    = cond;
  E_GET_NODE(p, node)->for_stmt.niterators   = niterators;
  E_GET_NODE(p, node)->for_stmt.iterators    = iterators;
  E_GET_NODE(p, node)->for_stmt.stmts        = stmts;
  E_GET_NODE(p, node)->for_stmt.nstmts       = nstmts;

  return node;

err:
  return -1;
}

static int
parse_function(e_ast* p, bool external, int node)
{
  if (node < 0) return node;

  u32      names_capacity = 0;
  u32      arg_names_size = 0;
  char**   arg_names      = nullptr;
  char*    function_name  = nullptr;
  char*    arg_name       = nullptr;
  e_token* name_tk        = nullptr;
  int*     stmts          = nullptr;
  u32      nstmts         = 0;

  name_tk = peek(p);
  if (e_ast_expect(p, E_TOKEN_TYPE_IDENT)) {
    asterror(prev(p)->span, "Expected function name after 'fn'/'extern', got '%s' [function header]\n", e_token_type_to_string(prev(p)->type));
    goto err;
  }

  if (e_ast_expect(p, E_TOKEN_TYPE_OPENPAREN)) {
    asterror(prev(p)->span, "Expected (, got '%s' [function header]\n", e_token_type_to_string(prev(p)->type));
    goto err;
  }

  function_name = e_arnstrdup(p->arena, name_tk->val.ident);

  names_capacity = 32; // NON ZERO
  arg_names_size = 0;
  arg_names      = e_arnalloc(p->arena, sizeof(char*) * names_capacity);

  while (peek(p) && peek(p)->type != E_TOKEN_TYPE_CLOSEPAREN) {
    e_token* tk = peek(p);
    if (tk->type != E_TOKEN_TYPE_IDENT) {
      asterror(peek(p)->span, "Expected parameter name, got '%s' [function header]\n", e_token_type_to_string(prev(p)->type));
      goto err;
    }
    next(p);

    if (arg_names_size >= names_capacity) {
      u32    new_capacity = names_capacity * 2;
      char** new_names    = e_arnrealloc(p->arena, arg_names, sizeof(char*) * new_capacity);

      if (!new_names) {
        asterror(peek(p)->span, "Allocation error! [function header]\n");
        goto err;
      }

      arg_names      = new_names;
      names_capacity = new_capacity;
    }

    arg_name = e_arnstrdup(p->arena, tk->val.ident);
    if (!arg_name) {
      asterror(peek(p)->span, "Alloc error [function argument name]\n");
      goto err;
    }

    arg_names[arg_names_size] = arg_name;
    arg_names_size++;

    if (peek(p) && peek(p)->type == E_TOKEN_TYPE_CLOSEPAREN) { break; }

    if (e_ast_expect(p, E_TOKEN_TYPE_COMMA)) {
      asterror(prev(p)->span, "Expected ',', got '%s' [function header]\n", e_token_type_to_string(prev(p)->type));
      goto err;
    }
  }

  if (e_ast_expect(p, E_TOKEN_TYPE_CLOSEPAREN)) {
    asterror(prev(p)->span, "Expected ')', got '%s' [function header]\n", e_token_type_to_string(prev(p)->type));
    goto err;
  }

  e_filespan parsing_span = peek(p)->span;

  if (external) {
    if (e_ast_expect(p, E_TOKEN_TYPE_SEMICOLON)) {
      asterror(prev(p)->span, "Expected ')', got '%s' [external function decleration]\n", e_token_type_to_string(prev(p)->type));
      return -1;
    }
  } else {
    if (parse_body(p, &stmts, &nstmts)) {
      asterror(parsing_span, "Failed to parse function body [function definition]\n");
      goto err;
    }
  }

  E_GET_NODE(p, node)->type        = E_AST_NODE_FUNCTION_DEFINITION;
  E_GET_NODE(p, node)->func.name   = function_name;
  E_GET_NODE(p, node)->func.args   = arg_names;
  E_GET_NODE(p, node)->func.nargs  = arg_names_size;
  E_GET_NODE(p, node)->func.stmts  = stmts;
  E_GET_NODE(p, node)->func.nstmts = nstmts;

  return node;

err:
  return -1;
}

static int
parse_function_call(e_ast* p, e_token* tk, int node)
{
  u32   capacity  = 4;
  u32   nargs     = 0;
  int*  args      = e_arnalloc(p->arena, capacity * sizeof(int)); // argument nodes
  char* func_name = e_arnstrdup(p->arena, tk->val.ident);

  if (!func_name || !args) goto err;

  if (e_ast_expect(p, E_TOKEN_TYPE_OPENPAREN)) // (
  {
    asterror(prev(p)->span, "Expected '(' after function name, got '%s'\n", e_token_type_to_string(prev(p)->type));
    goto err;
  }

  E_GET_NODE(p, node)->type               = E_AST_NODE_CALL;
  E_GET_NODE(p, node)->call.function_name = func_name;
  // printf("AST: Detected function call: %s\n", tk->val.ident);

  while (peek(p) && peek(p)->type != E_TOKEN_TYPE_CLOSEPAREN) {
    if (nargs >= capacity) {
      u32  newcap       = MAX(capacity * 2, 1);
      int* new_args     = e_arnrealloc(p->arena, args, newcap * sizeof(int));
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

    if (peek(p)->type == E_TOKEN_TYPE_CLOSEPAREN) { break; }

    if (e_ast_expect(p, E_TOKEN_TYPE_COMMA)) {
      asterror(prev(p)->span, "Expected ',' or ')', got '%s'\n", e_token_type_to_string(prev(p)->type));
      goto err;
    }
  }
  // consume )
  next(p);

  E_GET_NODE(p, node)->call.args  = args;
  E_GET_NODE(p, node)->call.nargs = nargs;

  return node;

err:
  return -1;
}

/**
 * On success, returns the node id.
 */
static inline int
make_literal_node(e_ast* p, int node, const e_token* tk)
{
  if (node < 0) return node;

  e_ast_node* nodep = e_ast_get_node(p, node);
  if (tk->type == E_TOKEN_TYPE_INT) {
    nodep->type = E_AST_NODE_INT;
    nodep->i.i  = tk->val.i;
  } else if (tk->type == E_TOKEN_TYPE_FLOAT) {
    nodep->type = E_AST_NODE_FLOAT;
    nodep->f.f  = tk->val.f;
  } else if (tk->type == E_TOKEN_TYPE_BOOL) {
    nodep->type = E_AST_NODE_BOOL;
    nodep->b.b  = tk->val.b;
  } else if (tk->type == E_TOKEN_TYPE_CHAR) {
    nodep->type = E_AST_NODE_CHAR;
    nodep->c.c  = tk->val.c;
  } else if (tk->type == E_TOKEN_TYPE_STRING) {
    nodep->type = E_AST_NODE_STRING;
    nodep->s.s  = e_arnstrdup(p->arena, tk->val.s);
    if (!nodep->s.s) return -1;
  }
  return node;
}

static int
parse_list(e_ast* p, int node)
{
  u32  capelems = 8;
  u32  nelems   = 0;
  int* elems    = e_arnalloc(p->arena, sizeof(int) * capelems);

  while (peek(p) && peek(p)->type != E_TOKEN_TYPE_CLOSEBRACKET) {
    int elem = e_ast_expr(p, 0);

    if (nelems >= capelems) {
      u32  new_cap   = MAX(capelems * 2, 1);
      int* new_elems = e_arnrealloc(p->arena, elems, new_cap * sizeof(int));
      if (!new_elems) { goto err; }

      elems    = new_elems;
      capelems = new_cap;
    }

    elems[nelems++] = elem;

    if (peek(p) && peek(p)->type == E_TOKEN_TYPE_CLOSEBRACKET) { break; }

    if (e_ast_expect(p, E_TOKEN_TYPE_COMMA)) {
      asterror(prev(p)->span, "Expected ',' or ']', got '%s' [list literal]\n", e_token_type_to_string(prev(p)->type));
      goto err;
    }
  }

  if (e_ast_expect(p, E_TOKEN_TYPE_CLOSEBRACKET)) {
    asterror(prev(p)->span, "Expected ']', got '%s' [list literal]\n", e_token_type_to_string(prev(p)->type));
    goto err;
  }

  E_GET_NODE(p, node)->type        = E_AST_NODE_LIST;
  E_GET_NODE(p, node)->list.elems  = elems;
  E_GET_NODE(p, node)->list.nelems = nelems;

  return node;

err:
  // free(elems);
  return -1;
}

static int
parse_map(e_ast* p, int node)
{
  // if (e_ast_expect(p, E_TOKEN_TYPE_OPEN_BRACE)) {
  //   asterror(prev(p)->span, "Expected '{' after '#' [map literal]\n");
  //   return -1;
  // }

  u32 capacity = 8;
  u32 npairs   = 0;

  int* keys   = e_arnalloc(p->arena, sizeof(int) * capacity);
  int* values = e_arnalloc(p->arena, sizeof(int) * capacity);

  if (!keys || !values) return -1;

  while (peek(p) && peek(p)->type != E_TOKEN_TYPE_CLOSEBRACE) {
    // key
    int key = e_ast_expr(p, 0);
    if (key < 0) return -1;

    if (e_ast_expect(p, E_TOKEN_TYPE_COLON)) {
      asterror(prev(p)->span, "Expected ':' [map literal]\n");
      return -1;
    }

    // value
    int value = e_ast_expr(p, 0);
    if (value < 0) return -1;

    if (npairs >= capacity) {
      u32 newcap = capacity * 2;
      keys       = e_arnrealloc(p->arena, keys, sizeof(int) * newcap);
      values     = e_arnrealloc(p->arena, values, sizeof(int) * newcap);
      capacity   = newcap;
    }

    keys[npairs]   = key;
    values[npairs] = value;
    npairs++;

    if (peek(p)->type == E_TOKEN_TYPE_CLOSEBRACE) break;

    if (e_ast_expect(p, E_TOKEN_TYPE_COMMA)) {
      asterror(prev(p)->span, "Expected ',' or '}' [map literal]\n");
      return -1;
    }
  }

  if (e_ast_expect(p, E_TOKEN_TYPE_CLOSEBRACE)) {
    asterror(prev(p)->span, "Expected '}' [map literal]\n");
    return -1;
  }

  E_GET_NODE(p, node)->type       = E_AST_NODE_MAP;
  E_GET_NODE(p, node)->map.keys   = keys;
  E_GET_NODE(p, node)->map.values = values;
  E_GET_NODE(p, node)->map.npairs = npairs;

  return node;
}

static int
parse_namespace_decleration(e_ast* p, int node)
{
  if (!peek(p) || peek(p)->type != E_TOKEN_TYPE_IDENT) {
    asterror(peek(p)->span, "Expected namespace name, got '%s' [namespace decleration]\n", e_token_type_to_string(prev(p)->type));
    return -1;
  }

  e_token* name_tk = next(p);

  if (e_ast_expect(p, E_TOKEN_TYPE_OPENBRACE)) {
    asterror(prev(p)->span, "Expected '{' after namespace name, got '%s' [namespace decleration]\n", e_token_type_to_string(prev(p)->type));
    return -1;
  }

  int* stmts  = NULL;
  u32  nstmts = 0;

  if (parse_braces(p, &stmts, &nstmts)) { return -1; }

  for (u32 i = 0; i < nstmts; i++) {
    e_ast_node_type type = E_GET_NODE(p, stmts[i])->common.type;
    e_filespan      span = E_GET_NODE(p, stmts[i])->common.span;

    if (type == E_AST_NODE_STATEMENT_LIST) {
      u32  list_nstmts = E_GET_NODE(p, stmts[i])->stmts.nstmts;
      int* list_stmts  = E_GET_NODE(p, stmts[i])->stmts.stmts;
      for (u32 j = 0; j < list_nstmts; j++) {
        type = E_GET_NODE(p, list_stmts[j])->common.type;
        if (type != E_AST_NODE_FUNCTION_DEFINITION && type != E_AST_NODE_STRUCT_DECL && type != E_AST_NODE_NAMESPACE_DECL
            && type != E_AST_NODE_VARIABLE_DECL) {
          asterror(span, "Expected only function definitions, variable declerations or namespace declerations in namespace scope\n");

          return -1;
        }
      }
    } else if (
        type != E_AST_NODE_FUNCTION_DEFINITION && type != E_AST_NODE_STRUCT_DECL && type != E_AST_NODE_NAMESPACE_DECL
        && type != E_AST_NODE_VARIABLE_DECL) {
      asterror(span, "Expected only function definitions, variable declerations or namespace declerations in namespace scope\n");

      return -1;
    }
  }

  E_GET_NODE(p, node)->type                  = E_AST_NODE_NAMESPACE_DECL;
  E_GET_NODE(p, node)->namespace_decl.name   = e_arnstrdup(p->arena, name_tk->val.ident);
  E_GET_NODE(p, node)->namespace_decl.stmts  = stmts;
  E_GET_NODE(p, node)->namespace_decl.nstmts = nstmts;

  return node;
}

static int
parse_namespace_call(e_ast* p, char* name, int node)
{
  u32  capacity = 4;
  u32  nargs    = 0;
  int* args     = e_arnalloc(p->arena, sizeof(int) * capacity);
  if (!args) { return -1; }

  next(p); // '('

  while (peek(p) && peek(p)->type != E_TOKEN_TYPE_CLOSEPAREN) {
    if (nargs >= capacity) {
      u32  newcap   = MAX(capacity * 2, 1);
      int* new_args = e_arnrealloc(p->arena, args, sizeof(int) * newcap);
      if (!new_args) {
        // free(args);
        return -1;
      }
      args     = new_args;
      capacity = newcap;
    }

    args[nargs++] = e_ast_expr(p, 0);

    if (peek(p)->type == E_TOKEN_TYPE_CLOSEPAREN) break;

    if (e_ast_expect(p, E_TOKEN_TYPE_COMMA)) {
      asterror(prev(p)->span, "Expected ',' or ')' [qualified function call]\n");
      // free(args);
      return -1;
    }
  }

  next(p); // ')'

  E_GET_NODE(p, node)->type               = E_AST_NODE_CALL;
  E_GET_NODE(p, node)->call.function_name = name;
  E_GET_NODE(p, node)->call.args          = args;
  E_GET_NODE(p, node)->call.nargs         = nargs;

  return node;
}

static int
parse_namespace_access(e_ast* p, int leftidx, int node)
{
  if (!peek(p) || peek(p)->type != E_TOKEN_TYPE_IDENT) {
    asterror(peek(p)->span, "Expected identifier after :: [namespace access]\n");

    return -1;
  }

  e_token* ident = next(p);

  e_ast_node* left = E_GET_NODE(p, leftidx);

  if (left->type != E_AST_NODE_VARIABLE) {
    asterror(peek(p)->span, "LHS of :: must be an identifier [namespace access]\n");
    return -1;
  }

  size_t len      = strlen(left->ident.ident) + strlen(ident->val.ident) + 3; // +2 for ::, 1 for \0
  char*  fullname = e_arnalloc(p->arena, len);
  if (!fullname) { return -1; }

  snprintf(fullname, len, "%s::%s", left->ident.ident, ident->val.ident);

  // If function call
  if (peek(p) && peek(p)->type == E_TOKEN_TYPE_OPENPAREN) {
    if (parse_namespace_call(p, fullname, node) < 0) {
      // free(fullname);
      return -1;
    }
    return node;
  }

  // just a namespaced variable.
  E_GET_NODE(p, node)->type        = E_AST_NODE_VARIABLE;
  E_GET_NODE(p, node)->ident.ident = fullname;

  return node;
}

static int
parse_struct_decleration(e_ast* p, int node)
{
  if (!peek(p) || peek(p)->type != E_TOKEN_TYPE_IDENT) {
    asterror(peek(p)->span, "Expected structure name, got '%s' [structure decleration]\n", e_token_type_to_string(prev(p)->type));
    return -1;
  }

  e_token* name_tk = next(p);

  if (e_ast_expect(p, E_TOKEN_TYPE_OPENBRACE)) {
    asterror(prev(p)->span, "Expected '{' after structure name, got '%s' [structure decleration]\n", e_token_type_to_string(prev(p)->type));
    return -1;
  }

  int* stmts  = NULL;
  u32  nstmts = 0;

  if (parse_braces(p, &stmts, &nstmts)) { return -1; }

  for (u32 i = 0; i < nstmts; i++) {
    e_ast_node_type type = E_GET_NODE(p, stmts[i])->common.type;
    e_filespan      span = E_GET_NODE(p, stmts[i])->common.span;

    const char* s = "Expected only function definitions or variable declerations in structure decleration scope\n";

    if (type == E_AST_NODE_STATEMENT_LIST) {
      u32  list_nstmts = E_GET_NODE(p, stmts[i])->stmts.nstmts;
      int* list_stmts  = E_GET_NODE(p, stmts[i])->stmts.stmts;
      for (u32 j = 0; j < list_nstmts; j++) {
        type = E_GET_NODE(p, list_stmts[j])->common.type;
        if (type != E_AST_NODE_FUNCTION_DEFINITION && type != E_AST_NODE_VARIABLE_DECL) {
          asterror(span, "%s", s);
          return -1;
        }
      }
    } else if (type != E_AST_NODE_FUNCTION_DEFINITION && type != E_AST_NODE_VARIABLE_DECL) {
      asterror(span, "%s", s);
      return -1;
    }
  }

  E_GET_NODE(p, node)->type               = E_AST_NODE_STRUCT_DECL;
  E_GET_NODE(p, node)->struct_decl.name   = e_arnstrdup(p->arena, name_tk->val.ident);
  E_GET_NODE(p, node)->struct_decl.stmts  = stmts;
  E_GET_NODE(p, node)->struct_decl.nstmts = nstmts;

  return node;
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
  memset(E_GET_NODE(p, node), 0, sizeof(e_ast_node));

  E_GET_NODE(p, node)->common.span = clonespan(p, tk->span);

  // if (tk->type == E_TOKEN_TYPE_EOF) return -1;

  switch (tk->type) {
    case E_TOKEN_TYPE_INT:
    case E_TOKEN_TYPE_CHAR:
    case E_TOKEN_TYPE_BOOL:
    case E_TOKEN_TYPE_FLOAT:
    case E_TOKEN_TYPE_STRING: {
      int e = make_literal_node(p, node, tk);
      if (e < 0) { return -1; }
      return node;
    }

    /* List declerator ([elem1,elem2,]) */
    case E_TOKEN_TYPE_OPENBRACKET: {
      if (parse_list(p, node) < 0) { return -1; }
      return node;
    }

    /* Map declerator */
    case E_TOKEN_TYPE_HASH_OPENBRACE: {
      if (parse_map(p, node) < 0) { return -1; }
      return node;
    }

    case E_TOKEN_TYPE_IDENT: {
      // Function call
      if (peek(p)->type == E_TOKEN_TYPE_OPENPAREN) {
        if (parse_function_call(p, tk, node) < 0) { return -1; }
        return node;
      }

      char* name = e_arnstrdup(p->arena, tk->val.ident);
      if (!name) goto err1;

      // Just a variable!
      E_GET_NODE(p, node)->type        = E_AST_NODE_VARIABLE;
      E_GET_NODE(p, node)->ident.ident = name;
      // printf("%s\n", tk->val.ident);

      return node;

    err1:

      // free(name);
      return -1;
    }

    case E_TOKEN_TYPE_OPENPAREN: {
      int old_node = node;

      node = e_ast_expr(p, 0);
      if (e_ast_expect(p, E_TOKEN_TYPE_CLOSEPAREN)) {
        e_filespan opening_paren_span = E_GET_NODE(p, old_node)->common.span;
        // asterror(opening_paren_span, "Expected ')' here [%s:%i:%i] for this '('\n", peek(p)->span.file, peek(p)->span.line, peek(p)->span.col);
        asterror(
            E_GET_NODE(p, node)->common.span,
            "Expected ')' for this '(' [%s:%i:%i], got '%s'\n",
            opening_paren_span.file,
            opening_paren_span.line,
            opening_paren_span.col,
            e_token_type_to_string(prev(p)->type));
        return -1;
      }

      return node;
    }

    /* { stmts;stmts;stmts; } */
    case E_TOKEN_TYPE_OPENBRACE: {
      int* stmts  = nullptr;
      u32  nstmts = 0;
      if (parse_braces(p, &stmts, &nstmts)) { return -1; }

      E_GET_NODE(p, node)->type         = E_AST_NODE_STATEMENT_LIST;
      E_GET_NODE(p, node)->stmts.stmts  = stmts;
      E_GET_NODE(p, node)->stmts.nstmts = nstmts;

      return node;
    }

    case E_TOKEN_TYPE_BREAK: {
      E_GET_NODE(p, node)->type = E_AST_NODE_BREAK;
      return node;
    }

    case E_TOKEN_TYPE_CONTINUE: {
      E_GET_NODE(p, node)->type = E_AST_NODE_CONTINUE;
      return node;
    }

    case E_TOKEN_TYPE_RETURN: {
      E_GET_NODE(p, node)->type = E_AST_NODE_RETURN;
      if (peek(p) && peek(p)->type != E_TOKEN_TYPE_SEMICOLON) {
        // Expect an expression
        int expr = e_ast_expr(p, 0);
        if (expr < 0) {
          return expr; // Parser will eat semicolon
        }

        E_GET_NODE(p, node)->ret.expr_id          = expr;
        E_GET_NODE(p, node)->ret.has_return_value = true;
      } else {
        E_GET_NODE(p, node)->ret.expr_id          = -1;
        E_GET_NODE(p, node)->ret.has_return_value = false;
      }

      return node;
    }

    case E_TOKEN_TYPE_CONST:
    case E_TOKEN_TYPE_LET: {
      // const x = 69;
      // const let gay = false;
      bool is_const = (tk->type == E_TOKEN_TYPE_CONST);

      // skip the 2nd let tok if it exists
      if (tk->type == E_TOKEN_TYPE_CONST && peek(p) && peek(p)->type == E_TOKEN_TYPE_LET) { next(p); }

      // skip the 2nd const tok if it exists
      if (tk->type == E_TOKEN_TYPE_LET && peek(p) && peek(p)->type == E_TOKEN_TYPE_CONST) {
        is_const = true;
        next(p);
      }

      if (parse_variable_decleration(p, is_const, node) < 0) { return -1; }
      return node;
    }

    case E_TOKEN_TYPE_STRUCT: {
      if (parse_struct_decleration(p, node) < 0) { return -1; }
      return node;
    }

    case E_TOKEN_TYPE_IF: {
      if (parse_if(p, node) < 0) { return -1; }
      return node;
    }

    case E_TOKEN_TYPE_WHILE: {
      if (parse_while(p, node) < 0) { return -1; }
      return node;
    }

    case E_TOKEN_TYPE_FOR: {
      if (parse_for(p, node) < 0) { return -1; }
      return node;
    }

    case E_TOKEN_TYPE_FN: {
      if (parse_function(p, false, node) < 0) { return -1; }
      return node;
    }

    case E_TOKEN_TYPE_EXTERN: {
      if (parse_function(p, true, node) < 0) { return -1; }
      return node;
    }

    case E_TOKEN_TYPE_NAMESPACE: {
      if (parse_namespace_decleration(p, node) < 0) { return -1; }
      return node;
    }

    case E_TOKEN_TYPE_SEMICOLON: {
      E_GET_NODE(p, node)->type = E_AST_NODE_NOP;
      return node;
    }

    case E_TOKEN_TYPE_MINUS:
    case E_TOKEN_TYPE_NOT:
    case E_TOKEN_TYPE_BNOT: {
      int left_bp = 0, right_bp = 0;
      if (!e_getbp(tk->type, &left_bp, &right_bp)) {
        asterror(tk->span, "Unexpected token: '%s'\n", e_token_type_to_string(tk->type));

        return -1;
      }

      e_operator op = conv_token_type_to_operator(tk->type);
      if (op < 0) {
        asterror(tk->span, "Unexpected token: '%s'\n", e_token_type_to_string(tk->type));

        return -1;
      }

      int right = e_ast_expr(p, right_bp);
      if (right < 0) {
        asterror(tk->span, "Failed to evaluate RHS of unary operator\n");

        return -1;
      }

      E_GET_NODE(p, node)->type                = E_AST_NODE_UNARYOP;
      E_GET_NODE(p, node)->unaryop.op          = op;
      E_GET_NODE(p, node)->unaryop.is_compound = tk->val.op.is_compound;
      E_GET_NODE(p, node)->unaryop.right       = right;
      return node;
    }

    default: asterror(tk->span, "Unexpected token: '%s'\n", e_token_type_to_string(tk->type)); return -1;
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

  E_GET_NODE(p, node)->common.span = clonespan(p, tk->span);

  switch (tk->type) {
    case E_TOKEN_TYPE_EQUAL: {
      // printf("ASSIGN YAYY!!\n");

      // Assigning to a member of a (list/struct)
      if (e_ast_get_node(p, leftidx)->type == E_AST_NODE_INDEX) {
        int rightidx = e_ast_expr(p, rbp);
        if (rightidx < 0) {
          asterror(peek(p)->span, "Invalid RHS in index assignment\n");

          return -1;
        }

        E_GET_NODE(p, node)->type               = E_AST_NODE_INDEX_ASSIGN;
        E_GET_NODE(p, node)->index_assign.base  = E_GET_NODE(p, leftidx)->index.base;
        E_GET_NODE(p, node)->index_assign.index = E_GET_NODE(p, leftidx)->index.index;
        E_GET_NODE(p, node)->index_assign.value = rightidx;

        // we stole lefts' children, we don't want to recursively free it.
        // free(E_GET_NODE(p, leftidx)->common.span.file);
        E_GET_NODE(p, leftidx)->common.span.file = NULL;
        E_GET_NODE(p, leftidx)->type             = E_AST_NODE_NOP;

        return node;
      }

      int rightidx = e_ast_expr(p, rbp);
      if (rightidx < 0) {
        asterror(peek(p)->span, "Invalid RHS in assignment\n");

        return -1;
      }

      E_GET_NODE(p, node)->type           = E_AST_NODE_ASSIGN;
      E_GET_NODE(p, node)->binaryop.left  = leftidx;
      E_GET_NODE(p, node)->binaryop.right = rightidx;

      return node;
    }

    // Index: LEFT [ INDEX ]
    case E_TOKEN_TYPE_OPENBRACKET: {
      e_filespan span = peek(p)->span;

      int index = e_ast_expr(p, 0);
      if (index < 0) {
        cerror(span, "Failed to parse index statement\n");

        return -1;
      }

      span = peek(p)->span;
      if (e_ast_expect(p, E_TOKEN_TYPE_CLOSEBRACKET)) {
        cerror(span, "Expected ']' [Index expression]\n");
        return -1;
      }

      E_GET_NODE(p, node)->type        = E_AST_NODE_INDEX;
      E_GET_NODE(p, node)->index.base  = leftidx;
      E_GET_NODE(p, node)->index.index = index;
      return node;
    }

    // Member access
    // x.y
    case E_TOKEN_TYPE_DOT: {
      char* s = peek(p)->val.ident;
      if (e_ast_expect(p, E_TOKEN_TYPE_IDENT)) {
        cerror(prev(p)->span, "Expected identifier [Member Access]\n");
        return -1;
      }

      E_GET_NODE(p, node)->type                = E_AST_NODE_MEMBER_ACCESS;
      E_GET_NODE(p, node)->member_access.left  = leftidx;
      E_GET_NODE(p, node)->member_access.right = e_arnstrdup(p->arena, s);

      return node;
    }

    // Namespace access
    // x::y
    case E_TOKEN_TYPE_DOUBLE_COLON: {
      if (parse_namespace_access(p, leftidx, node) < 0) { return -1; }
      return node;
    }

    case E_TOKEN_TYPE_PLUSPLUS: {
      E_GET_NODE(p, node)->type                = E_AST_NODE_UNARYOP;
      E_GET_NODE(p, node)->unaryop.op          = E_OPERATOR_INC;
      E_GET_NODE(p, node)->unaryop.is_compound = true;
      E_GET_NODE(p, node)->unaryop.right       = leftidx; // operand is left
      return node;
    }

    case E_TOKEN_TYPE_MINUSMINUS: {
      E_GET_NODE(p, node)->type                = E_AST_NODE_UNARYOP;
      E_GET_NODE(p, node)->unaryop.op          = E_OPERATOR_DEC;
      E_GET_NODE(p, node)->unaryop.is_compound = true;
      E_GET_NODE(p, node)->unaryop.right       = leftidx;
      return node;
    }

    case E_TOKEN_TYPE_DOUBLEEQUAL:
    case E_TOKEN_TYPE_NOTEQUAL:
    case E_TOKEN_TYPE_PLUS:
    case E_TOKEN_TYPE_MINUS:
    case E_TOKEN_TYPE_MULTIPLY:
    case E_TOKEN_TYPE_DIVIDE:
    case E_TOKEN_TYPE_EXPONENT:
    case E_TOKEN_TYPE_MOD:
    case E_TOKEN_TYPE_BAND:
    case E_TOKEN_TYPE_BOR:
    case E_TOKEN_TYPE_AND:
    case E_TOKEN_TYPE_OR:
    case E_TOKEN_TYPE_XOR:
    case E_TOKEN_TYPE_BNOT:
    case E_TOKEN_TYPE_LT:
    case E_TOKEN_TYPE_LTE:
    case E_TOKEN_TYPE_GT:
    case E_TOKEN_TYPE_GTE: {
      int left_bp = 0, right_bp = 0;
      if (!e_getbp(tk->type, &left_bp, &right_bp)) {
        asterror(
            tk->span, "Operator %s doesn't have a binding power set in e_getbp! assuming 0 [binary operator]\n", e_token_type_to_string(tk->type));
        //
        // return -1;
      }

      e_operator op = conv_token_type_to_operator(tk->type);
      if (op == -1) {
        asterror(tk->span, "Unexpected token: '%s', Expected operator [binary operator]\n", e_token_type_to_string(tk->type));

        return -1;
      }

      e_filespan prev_span = peek(p)->span;

      int right = e_ast_expr(p, right_bp);
      if (right < 0) {
        asterror(prev_span, "Failed to evaluate RHS [binary operator]\n");

        return -1;
      }

      if (E_GET_NODE(p, leftidx)->type == E_AST_NODE_INDEX) {
        E_GET_NODE(p, node)->type                 = E_AST_NODE_INDEX_COMPOUND_OP;
        E_GET_NODE(p, node)->index_compound.op    = op;
        E_GET_NODE(p, node)->index_compound.base  = E_GET_NODE(p, leftidx)->index.base;
        E_GET_NODE(p, node)->index_compound.index = E_GET_NODE(p, leftidx)->index.index;
        E_GET_NODE(p, node)->index_compound.value = right;

        // free(E_GET_NODE(p, leftidx)->common.span.file);
        E_GET_NODE(p, leftidx)->type = E_AST_NODE_NOP;

        return node;
      }

      E_GET_NODE(p, node)->type                 = E_AST_NODE_BINARYOP;
      E_GET_NODE(p, node)->binaryop.op          = op;
      E_GET_NODE(p, node)->binaryop.left        = leftidx;
      E_GET_NODE(p, node)->binaryop.is_compound = tk->val.op.is_compound;
      E_GET_NODE(p, node)->binaryop.right       = right;
      return node;
    }

    default: {
      asterror(tk->span, "Unexpected token: '%s'\n", e_token_type_to_string(tk->type));

      return -1;
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
  int* stmts  = e_arnalloc(p->arena, cap * sizeof(int));

  int rootnode = e_ast_make_node(p);
  if (rootnode < 0) return -1;

  if (out_root_node) *out_root_node = rootnode;

  e_ast_get_node(p, rootnode)->type = E_AST_NODE_ROOT;

  if (!stmts) { goto err; }

  // for (int i = 0; i < p->ntoks; i++) // Only run from 0 to ntoks-1 because the last token is EOF.
  while (peek(p) && peek(p)->type != E_TOKEN_TYPE_EOF) {
    e_filespan take_span = peek(p)->span;

    node = e_ast_expr(p, 0);
    if (node < 0) {
      asterror(take_span, "Error parsing expression [Root statement compilation failed, retreating]\n");
      goto err;
    }

    e_ast_node_type type = E_GET_NODE(p, node)->type;
    if (type != E_AST_NODE_FUNCTION_DEFINITION && type != E_AST_NODE_NAMESPACE_DECL && type != E_AST_NODE_STRUCT_DECL
        && type != E_AST_NODE_VARIABLE_DECL) {
      // asterror(take_span, "Expected function definition or variable declerations in global scope\n");
      asterror(
          take_span, "Expected only function definitions, variable declerations,  namespace declerations or struct declerations in global scope\n");
      goto err;
    }

    if (!e_ast_is_limiter_exempt(e_ast_get_node(p, node)->type) && e_ast_expect(p, E_TOKEN_TYPE_SEMICOLON)) {
      asterror(prev(p)->span, "Expected semicolon after expression\n");
      goto err;
    }

    if (nstmts >= cap) {
      u32  newcap   = MAX(cap * 2, 1);
      int* newstmts = e_arnrealloc(p->arena, stmts, newcap * sizeof(int));
      if (!newstmts) {
        asterror(take_span, "e_arnrealloc p->arena, failed! Can not continue\n");
        goto err;
      }
      stmts = newstmts;
      cap   = newcap;
    }

    // Add it to our list.
    stmts[nstmts++] = node;
  }

  e_ast_get_node(p, rootnode)->root.stmts  = stmts;
  e_ast_get_node(p, rootnode)->root.nstmts = nstmts;

  return 0;

err:
  return -1;
}

int
e_ast_init(e_token* toks, u32 ntoks, e_arena* arena, e_ast* prsr)
{
  if (prsr) {
    const u32 init_nodes = 64;

    *prsr = (e_ast){
      .nodes    = e_arnalloc(arena, sizeof(e_ast_node) * init_nodes),
      .nnodes   = 0,
      .capacity = init_nodes,
      .arena    = arena,
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
}
