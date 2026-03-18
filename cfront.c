// Compiler front end

#include "ast.h"
#include "astfree.h"
#include "cc.h"
#include "fn.h"
#include "lex.h"
#include "var.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(int argc, char* argv[])
{
  //             0  1  2  3  4  5  6  7  8  9
  // int x[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

  bool tokenizer_only = false;
  bool ast_only       = false;

  const char* out = NULL;
  for (int i = 0; i < argc; i++) {
    const char* opt = argv[i];

    if (*opt == '-') { opt++; }
    if (*opt == '-') { opt++; }

    if (strcmp(opt, "o") == 0) {
      assert(i + 1 < argc);
      out = argv[i + 1];
      i++; // consumed path!
      continue;
    } else if (strcmp(opt, "tokens") == 0) {
      tokenizer_only = true;
    } else if (strcmp(opt, "ast") == 0) {
      ast_only = true;
    }
  }

  if (out == NULL) {
    fprintf(stderr, "No output file specified. Not doing anything\n");
    return 0; // well technically its not an error... :3
  }

  char* in = argv[1];

  char* contents = read_file(in, nullptr);
  if (contents == nullptr) {
    fprintf(stderr, "ec: Failed to load input file: %s\n", strerror(errno));
    return -1;
  }

  e_token* tokens = 0;
  int      ntoks  = 0;
  int      e      = e_tokenize(contents, in, &tokens, &ntoks);
  if (e) {
    fprintf(stderr, "ec: Failed to tokenize input string\n");
    return -1;
  }

  if (tokenizer_only) {
    for (int i = 0; i < ntoks; i++) {
      printf("%s", e_tokentype_to_string(tokens[i].type));
      if (i != ntoks - 1) { fputs(" ", stdout); }
    }
    fputc('\n', stdout);
  }

  e_ast ast;
  e = e_ast_init(tokens, ntoks, &ast);
  if (e) {
    fprintf(stderr, "ec: AST initialization failed\n");
    return -1;
  }

  int root = -1;

  e = e_ast_parse(&ast, &root);
  if (root < 0 || e) {
    fprintf(stderr, "ec: AST parsing failed\n");
    return -1;
  }

  if (ast_only) {
    // TODO: Add AST printing / visualization
    return 0;
  }

  u8*         bytecode;
  u32         bytecode_size;
  e_var*      literals;
  u32         nliterals;
  e_function* functions;
  u32         nfunctions;

  e = e_compile(&ast, root, &bytecode, &bytecode_size, &literals, &nliterals, &functions, &nfunctions);
  if (e) {
    fprintf(stderr, "ec: Compilation failed\n");
    return -1;
  }

  FILE* f = fopen(out, "wb");
  if (!f) {
    perror("Failed to open out file");
    return -1;
  }

  fwrite(&nliterals, sizeof(nliterals), 1, f);
  for (int i = 0; i < nliterals; i++) {
    const e_var* lit = &literals[i];
    fwrite(&lit->type, sizeof(e_vartype), 1, f);

    if (lit->type == E_VARTYPE_STRING) {
      u32 len = strlen(lit->val.s->s);
      fwrite(&len, sizeof(len), 1, f);
      fwrite(lit->val.s->s, sizeof(char), len, f);
    } else {
      fwrite(&lit->val, sizeof(lit->val), 1, f);
    }
  }
  fwrite(&nfunctions, sizeof(nfunctions), 1, f);
  for (u32 i = 0; i < nfunctions; i++) {
    const e_function* fn = &functions[i];
    fwrite(&fn->code_size, sizeof(fn->code_size), 1, f);
    fwrite(&fn->nargs, sizeof(fn->nargs), 1, f);
    fwrite(&fn->hash, sizeof(fn->hash), 1, f);
    fwrite(fn->arg_slots, sizeof(*fn->arg_slots), fn->nargs, f);
    fwrite(fn->code, 1, fn->code_size, f);
  }

  fwrite(&bytecode_size, sizeof(bytecode_size), 1, f);
  fwrite(bytecode, 1, bytecode_size, f);

  fclose(f);
  e_asnode_free(&ast, root);
  for (int i = 0; i < nfunctions; i++) {
    free(functions[i].code);
    free(functions[i].arg_slots);
  }
  free(functions);
  for (int i = 0; i < nliterals; i++) { e_var_release(&literals[i]); }
  free(literals);
  free(bytecode);
  e_ast_free(&ast);
  e_freetoks(tokens, ntoks);
  free(contents);
}