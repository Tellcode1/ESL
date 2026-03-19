#ifndef E_LEX_H
#define E_LEX_H

#include "cerr.h"

typedef enum e_tokentype {
  E_TOKENTYPE_EOF,

  E_TOKENTYPE_SEMICOLON,
  E_TOKENTYPE_COLON,
  E_TOKENTYPE_COMMA,
  E_TOKENTYPE_OPENBRACE,
  E_TOKENTYPE_CLOSEBRACE,
  E_TOKENTYPE_OPENPAREN,
  E_TOKENTYPE_CLOSEPAREN,

  E_TOKENTYPE_INT,
  E_TOKENTYPE_CHAR,
  E_TOKENTYPE_BOOL,
  E_TOKENTYPE_FLOAT,
  E_TOKENTYPE_STRING,

  E_TOKENTYPE_FN,  // fn keyword for functions, no value
  E_TOKENTYPE_LET, // let keyword for variable declerations, no value.
  E_TOKENTYPE_IF,
  E_TOKENTYPE_ELSE,
  E_TOKENTYPE_WHILE,
  E_TOKENTYPE_FOR,
  E_TOKENTYPE_BREAK,
  E_TOKENTYPE_CONTINUE,
  E_TOKENTYPE_RETURN,

  E_TOKENTYPE_PLUS,       // +
  E_TOKENTYPE_MINUS,      // -
  E_TOKENTYPE_MULTIPLY,   // *
  E_TOKENTYPE_DIVIDE,     // /
  E_TOKENTYPE_EXPONENT,   // ^
  E_TOKENTYPE_MOD,        // %
  E_TOKENTYPE_PLUSPLUS,   // ++
  E_TOKENTYPE_MINUSMINUS, // --

  E_TOKENTYPE_AND, // &&
  E_TOKENTYPE_OR,  // ||
  E_TOKENTYPE_NOT, // !

  E_TOKENTYPE_BAND, // &
  E_TOKENTYPE_BOR,  // |
  E_TOKENTYPE_XOR,  // ^
  E_TOKENTYPE_BNOT, // ~

  E_TOKENTYPE_EQUAL,
  E_TOKENTYPE_NOTEQUAL,
  E_TOKENTYPE_DOUBLEEQUAL,

  E_TOKENTYPE_LT,
  E_TOKENTYPE_LTE,
  E_TOKENTYPE_GT,
  E_TOKENTYPE_GTE,

  E_TOKENTYPE_IDENT, // identifier/name
} e_tokentype;

typedef union e_tokenval {
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
} e_tokenval;

typedef struct e_token {
  e_tokentype type;
  e_tokenval  val;
  e_filespan  span; // The token owns this span
} e_token;

/**
 * Perform lexical analysis / tokenization on the input string.
 * outtoks will be set to a an array of tokens, allocated on the heap. Free using e_freetoks.
 * advertised_file is a string that should contain the file name that is shown to the user on errors/warnings by the compiler.
 */
int e_tokenize(const char* input, const char* advertised_file, e_token** outtoks, u32* ntoks);

void e_freetoks(e_token* toks, u32 ntoks);

static inline const char*
e_tokentype_to_string(e_tokentype e)
{
  switch (e) {
    case E_TOKENTYPE_EOF: return "EOF";
    case E_TOKENTYPE_SEMICOLON: return "SEMICOLON";
    case E_TOKENTYPE_COLON: return "COLON";
    case E_TOKENTYPE_COMMA: return "COMMA";
    case E_TOKENTYPE_INT: return "INT";
    case E_TOKENTYPE_FLOAT: return "FLOAT";
    case E_TOKENTYPE_STRING: return "STRING";
    case E_TOKENTYPE_FN: return "FN";
    case E_TOKENTYPE_LET: return "LET";
    case E_TOKENTYPE_PLUS: return "PLUS";
    case E_TOKENTYPE_MINUS: return "MINUS";
    case E_TOKENTYPE_MULTIPLY: return "MULTIPLY";
    case E_TOKENTYPE_DIVIDE: return "DIVIDE";
    case E_TOKENTYPE_EXPONENT: return "EXPONENT";
    case E_TOKENTYPE_MOD: return "MOD";
    case E_TOKENTYPE_AND: return "AND";
    case E_TOKENTYPE_OR: return "OR";
    case E_TOKENTYPE_EQUAL: return "EQUAL";
    case E_TOKENTYPE_DOUBLEEQUAL: return "DOUBLEEQUAL";
    case E_TOKENTYPE_IDENT: return "IDENT";
    case E_TOKENTYPE_OPENBRACE: return "OPENBRACE";
    case E_TOKENTYPE_CLOSEBRACE: return "CLOSEBRACE";
    case E_TOKENTYPE_OPENPAREN: return "OPENPAREN";
    case E_TOKENTYPE_CLOSEPAREN: return "CLOSEPAREN";
    case E_TOKENTYPE_CHAR: return "CHAR";
    case E_TOKENTYPE_BOOL: return "BOOL";
    case E_TOKENTYPE_IF: return "IF";
    case E_TOKENTYPE_ELSE: return "ELSE";
    case E_TOKENTYPE_WHILE: return "WHILE";
    case E_TOKENTYPE_FOR: return "FOR";
    case E_TOKENTYPE_BREAK: return "BREAK";
    case E_TOKENTYPE_CONTINUE: return "CONTINUE";
    case E_TOKENTYPE_RETURN: return "RETURN";
    case E_TOKENTYPE_PLUSPLUS: return "PLUSPLUS";
    case E_TOKENTYPE_MINUSMINUS: return "MINUSMINUS";
    case E_TOKENTYPE_NOT: return "NOT";
    case E_TOKENTYPE_BNOT: return "BNOT";
    case E_TOKENTYPE_NOTEQUAL: return "NOTEQUAL";
    case E_TOKENTYPE_LT: return "LT";
    case E_TOKENTYPE_LTE: return "LTE";
    case E_TOKENTYPE_GT: return "GT";
    case E_TOKENTYPE_GTE: return "GTE";
    default: return "UNKNOWN";
  }
}

#endif // E_LEX_H
