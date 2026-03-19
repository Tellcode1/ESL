#include "exec.h"
#include "fn.h"
#include "rwhelp.h"
#include "var.h"

#include <stdio.h>
#include <string.h>

e_var
say_hello_from_c(e_var* args)
{
  (void)args;
  printf("And this function was called from C, fromt the script!!\n");

  return (e_var){ .type = E_VARTYPE_VOID };
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

  e_var*      lits   = nullptr;
  u8*         ins    = nullptr;
  e_function* funcs  = nullptr;
  u32         nlits  = 0;
  u32         nins   = 0;
  u32         nfuncs = 0;
  e_file_load(f, &nlits, &lits, &nfuncs, &funcs);
  // printf("nlits=%u nins=%u\n", nlits, nins);

  // Don't let exec free a literal!
  // for (u32 i = 0; i < nlits; i++) { e_var_acquire(&lits[i]); }

  e_extern_function hello = {
    .hash  = e_hash_fnv("say_hello_from_c", strlen("say_hello_from_c")),
    .nargs = 0,
    .func  = say_hello_from_c,
  };

  const u32 main_func = e_hash_fnv("main", strlen("main"));
  for (u32 i = 0; i < nfuncs; i++) {
    if (funcs[i].name_hash == main_func) {
      ins  = funcs[i].code;
      nins = funcs[i].code_size;
      break;
    }
  }
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
    .nextern_funcs = 1,
    .extern_funcs  = (e_extern_function[]){ hello },
  };

  e_var v = e_exec(&info);

  fclose(f);

  for (u32 i = 0; i < nlits; i++) { e_var_release(&lits[i]); }
  free(lits);
  for (u32 i = 0; i < nfuncs; i++) {
    free(funcs[i].arg_slots);
    free(funcs[i].code);
  }
  free(funcs);

  int e = 0;
  if (v.type == E_VARTYPE_INT) { e = v.val.i; }
  e_var_release(&v);
  return e;
}