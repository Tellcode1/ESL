#include "exec.h"
#include "fn.h"
#include "rwhelp.h"
#include "var.h"

#include <stdio.h>

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

  e_var*      lits;
  u8*         ins;
  e_function* funcs;
  u32         nlits;
  u32         nins;
  u32         nfuncs;
  e_file_load(f, &nins, &ins, &nlits, &lits, &nfuncs, &funcs);
  // printf("nlits=%u nins=%u\n", nlits, nins);

  // Don't let exec free a literal!
  // for (u32 i = 0; i < nlits; i++) { e_var_acquire(&lits[i]); }

  e_extern_function hello = {
    .hash  = e_hash_fnv("say_hello_from_c", strlen("say_hello_from_c")),
    .nargs = 0,
    .func  = say_hello_from_c,
  };
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
  free(ins);
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