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

#ifndef E_LEX_H
#define E_LEX_H

#include "cerr.h"

typedef enum e_token_type {
  E_TOKEN_TYPE_EOF,

  E_TOKEN_TYPE_SEMICOLON,
  E_TOKEN_TYPE_COLON,
  E_TOKEN_TYPE_DOUBLE_COLON,
  E_TOKEN_TYPE_COMMA,
  E_TOKEN_TYPE_DOT,
  E_TOKEN_TYPE_OPENBRACE,
  E_TOKEN_TYPE_CLOSEBRACE,
  E_TOKEN_TYPE_OPENBRACKET,
  E_TOKEN_TYPE_CLOSEBRACKET,
  E_TOKEN_TYPE_OPENPAREN,
  E_TOKEN_TYPE_CLOSEPAREN,

  E_TOKEN_TYPE_INT,
  E_TOKEN_TYPE_CHAR,
  E_TOKEN_TYPE_BOOL,
  E_TOKEN_TYPE_FLOAT,
  E_TOKEN_TYPE_STRING,

  E_TOKEN_TYPE_FN,      // fn keyword for functions, no value
  E_TOKEN_TYPE_LOCALFN, // local function
  E_TOKEN_TYPE_LET,     // let keyword for variable declerations, no value.
  E_TOKEN_TYPE_IF,
  E_TOKEN_TYPE_ELSE,
  E_TOKEN_TYPE_WHILE,
  E_TOKEN_TYPE_FOR,
  E_TOKEN_TYPE_BREAK,
  E_TOKEN_TYPE_CONTINUE,
  E_TOKEN_TYPE_RETURN,
  E_TOKEN_TYPE_NAMESPACE,

  E_TOKEN_TYPE_PLUS,       // +
  E_TOKEN_TYPE_MINUS,      // -
  E_TOKEN_TYPE_MULTIPLY,   // *
  E_TOKEN_TYPE_DIVIDE,     // /
  E_TOKEN_TYPE_EXPONENT,   // ^
  E_TOKEN_TYPE_MOD,        // %
  E_TOKEN_TYPE_PLUSPLUS,   // ++
  E_TOKEN_TYPE_MINUSMINUS, // --

  E_TOKEN_TYPE_AND, // &&
  E_TOKEN_TYPE_OR,  // ||
  E_TOKEN_TYPE_NOT, // !

  E_TOKEN_TYPE_BAND, // &
  E_TOKEN_TYPE_BOR,  // |
  E_TOKEN_TYPE_XOR,  // ^
  E_TOKEN_TYPE_BNOT, // ~

  E_TOKEN_TYPE_EQUAL,
  E_TOKEN_TYPE_NOTEQUAL,
  E_TOKEN_TYPE_DOUBLEEQUAL,

  E_TOKEN_TYPE_LT,
  E_TOKEN_TYPE_LTE,
  E_TOKEN_TYPE_GT,
  E_TOKEN_TYPE_GTE,

  E_TOKEN_TYPE_IDENT, // identifier/name
} e_token_type;

typedef union e_token_val {
  int    i;
  char   c;
  bool   b;
  double f;
  char*  s;
  char*  ident;
  struct {
    char c;
    bool is_compound;
  } op;
} e_token_val;

typedef struct e_token {
  e_token_type type;
  e_token_val  val;
  e_filespan   span; // The token owns this span
} e_token;

/**
 * Perform lexical analysis / tokenization on the input string.
 * outtoks will be set to a an array of tokens, allocated on the heap. Free using e_freetoks.
 * advertised_file is a string that should contain the file name that is shown to the user on errors/warnings by the compiler.
 */
int e_tokenize(const char* input, const char* advertised_file, e_token** outtoks, u32* ntoks);

void e_freetoks(e_token* toks, u32 ntoks);

static inline const char*
e_token_type_to_string(e_token_type e)
{
  switch (e) {
    case E_TOKEN_TYPE_EOF: return "EOF";
    case E_TOKEN_TYPE_SEMICOLON: return "SEMICOLON";
    case E_TOKEN_TYPE_COLON: return "COLON";
    case E_TOKEN_TYPE_COMMA: return "COMMA";
    case E_TOKEN_TYPE_INT: return "INT";
    case E_TOKEN_TYPE_FLOAT: return "FLOAT";
    case E_TOKEN_TYPE_STRING: return "STRING";
    case E_TOKEN_TYPE_FN: return "FN";
    case E_TOKEN_TYPE_LET: return "LET";
    case E_TOKEN_TYPE_PLUS: return "PLUS";
    case E_TOKEN_TYPE_MINUS: return "MINUS";
    case E_TOKEN_TYPE_MULTIPLY: return "MULTIPLY";
    case E_TOKEN_TYPE_DIVIDE: return "DIVIDE";
    case E_TOKEN_TYPE_EXPONENT: return "EXPONENT";
    case E_TOKEN_TYPE_MOD: return "MOD";
    case E_TOKEN_TYPE_AND: return "AND";
    case E_TOKEN_TYPE_OR: return "OR";
    case E_TOKEN_TYPE_EQUAL: return "EQUAL";
    case E_TOKEN_TYPE_DOUBLEEQUAL: return "DOUBLEEQUAL";
    case E_TOKEN_TYPE_IDENT: return "IDENT";
    case E_TOKEN_TYPE_OPENBRACE: return "OPENBRACE";
    case E_TOKEN_TYPE_CLOSEBRACE: return "CLOSEBRACE";
    case E_TOKEN_TYPE_OPENPAREN: return "OPENPAREN";
    case E_TOKEN_TYPE_CLOSEPAREN: return "CLOSEPAREN";
    case E_TOKEN_TYPE_CHAR: return "CHAR";
    case E_TOKEN_TYPE_BOOL: return "BOOL";
    case E_TOKEN_TYPE_IF: return "IF";
    case E_TOKEN_TYPE_ELSE: return "ELSE";
    case E_TOKEN_TYPE_WHILE: return "WHILE";
    case E_TOKEN_TYPE_FOR: return "FOR";
    case E_TOKEN_TYPE_BREAK: return "BREAK";
    case E_TOKEN_TYPE_CONTINUE: return "CONTINUE";
    case E_TOKEN_TYPE_RETURN: return "RETURN";
    case E_TOKEN_TYPE_PLUSPLUS: return "PLUSPLUS";
    case E_TOKEN_TYPE_MINUSMINUS: return "MINUSMINUS";
    case E_TOKEN_TYPE_NOT: return "NOT";
    case E_TOKEN_TYPE_BNOT: return "BNOT";
    case E_TOKEN_TYPE_NOTEQUAL: return "NOTEQUAL";
    case E_TOKEN_TYPE_LT: return "LT";
    case E_TOKEN_TYPE_LTE: return "LTE";
    case E_TOKEN_TYPE_GT: return "GT";
    case E_TOKEN_TYPE_GTE: return "GTE";
    case E_TOKEN_TYPE_DOT: return "DOT";
    case E_TOKEN_TYPE_OPENBRACKET: return "OPENBRACKET";
    case E_TOKEN_TYPE_CLOSEBRACKET: return "CLOSEBRACKET";
    case E_TOKEN_TYPE_LOCALFN: return "LOCALFN";
    case E_TOKEN_TYPE_NAMESPACE: return "NAMESPACE";
    case E_TOKEN_TYPE_BAND: return "BAND";
    case E_TOKEN_TYPE_BOR: return "BOR";
    case E_TOKEN_TYPE_XOR: return "XOR";
    case E_TOKEN_TYPE_DOUBLE_COLON: return "DOUBLE_COLON";
  }
  return "UNKNOWN";
}

#endif // E_LEX_H
