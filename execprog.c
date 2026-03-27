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

#include "exec.h"
#include "fn.h"
#include "pool.h"
#include "rwhelp.h"
#include "var.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static e_var
say_hello_from_c(e_var* args)
{
  (void)args;
  printf("And this function was called from C, from the script!!\n");

  return (e_var){ .type = E_VARTYPE_VOID };
}

static e_var
file_open(e_var* args)
{
  e_var file_handle = {
    .type = E_VARTYPE_INT,
  };

  FILE* f = fopen(E_VAR_AS_STRING(&args[0])->s, E_VAR_AS_STRING(&args[1])->s);
  memcpy(&file_handle.val, &f, sizeof(f));

  return file_handle;
}

static e_var
file_close(e_var* args)
{
  FILE* f;
  memcpy(&f, &args[0].val, sizeof(f));

  fclose(f);

  return (e_var){ .type = E_VARTYPE_VOID };
}

static e_var
file_read(e_var* args)
{
  FILE* f;
  memcpy(&f, &args[0].val, sizeof(f));

  fseek(f, 0, SEEK_END);
  long size = ftell(f);

  char* str = malloc(size + 1);

  fseek(f, 0, SEEK_SET);
  fread(str, size, 1, f);

  str[size] = 0;

  struct e_refdobj* es = e_refdobj_pool_acquire(&ge_pool);

  E_OBJ_AS_STRING(es)->s = str;

  return (e_var){
    .type  = E_VARTYPE_STRING,
    .val.s = es,
  };
}

static e_var
file_size(e_var* args)
{
  FILE* f;
  memcpy(&f, &args[0].val, sizeof(f));

  long pos = ftell(f);
  if (pos <= 0) return (e_var){ 0 };

  fseek(f, 0, SEEK_END);
  long size = ftell(f);

  fseek(f, pos, SEEK_SET);

  return (e_var){
    .type  = E_VARTYPE_INT,
    .val.i = (int)size,
  };
}

int
main(int argc, char* argv[])
{
  assert(argc == 2);

  FILE* f = fopen(argv[1], "rb");

  if (!f) {
    perror("eexec: Failed to open file");
    return -1;
  }

  void*       root_allocation = nullptr;
  e_var*      lits            = nullptr;
  u8*         ins             = nullptr;
  e_function* funcs           = nullptr;
  u32         nlits           = 0;
  u32         nins            = 0;
  u32         nfuncs          = 0;
  if (e_file_load(f, &root_allocation, &nlits, &lits, &nfuncs, &funcs)) { fprintf(stderr, "eexec: Failed to open input file\n"); }

  e_extern_function hello = {
    .hash  = e_hash_fnv("say_hello_from_c", strlen("say_hello_from_c")),
    .nargs = 0,
    .func  = say_hello_from_c,
  };

  e_extern_function reg_file_open = {
    .hash  = e_hash_fnv("file_open", strlen("file_open")),
    .nargs = 0,
    .func  = file_open,
  };
  e_extern_function reg_file_close = {
    .hash  = e_hash_fnv("file_close", strlen("file_close")),
    .nargs = 0,
    .func  = file_close,
  };
  e_extern_function reg_file_read = {
    .hash  = e_hash_fnv("file_read", strlen("file_read")),
    .nargs = 0,
    .func  = file_read,
  };
  e_extern_function reg_file_size = {
    .hash  = e_hash_fnv("file_size", strlen("file_size")),
    .nargs = 0,
    .func  = file_size,
  };

  const u32 main_func = e_hash_fnv("main", strlen("main"));
  for (u32 i = 0; i < nfuncs; i++) {
    if (funcs[i].name_hash == main_func) {
      ins  = funcs[i].code;
      nins = funcs[i].code_size;
      break;
    }
  }

  e_stack stack;
  if (e_stack_init(512, 16, 32, &stack)) return -1;

  if (e_refdobj_pool_init(16, &ge_pool)) return -1;

  e_exec_info info = {
    .code          = ins,
    .args          = nullptr,
    .slots         = nullptr,
    .literals      = lits,
    .funcs         = funcs,
    .code_size     = nins,
    .nargs         = 0,
    .nliterals     = nlits,
    .nfuncs        = nfuncs,
    .stack         = &stack,
    .nextern_funcs = 5,
    .extern_funcs  = (e_extern_function[]){ hello, reg_file_open, reg_file_close, reg_file_read, reg_file_size },
  };

  e_var v = e_exec(&info);

  fclose(f);
  e_stack_free(&stack);

  e_refdobj_pool_free(&ge_pool);

  /* No need to free anything else */
  free(root_allocation);

  int e = 0;
  if (v.type == E_VARTYPE_INT) { e = v.val.i; }
  e_var_release(&v);
  return e;
}