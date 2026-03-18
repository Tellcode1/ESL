#include "exec.h"
#include "rwhelp.h"

#include <stdio.h>

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
  e_exec_info info = {
    .code      = ins,
    .args      = nullptr,
    .slots     = nullptr,
    .literals  = lits,
    .funcs     = funcs,
    .code_size = nins,
    .nargs     = 0,
    .nliterals = nlits,
    .nfuncs    = nfuncs,
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