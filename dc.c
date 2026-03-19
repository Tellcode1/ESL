#include "dc.h"

#include "fn.h"
#include "rwhelp.h"
#include "stdafx.h"

#include <stdio.h>

int
main(int argc, char** argv)
{
  if (argc != 2) {
    fprintf(stderr, "edc: [input_binary]\n");
    return -1;
  }

  const char* bin_file = argv[1];

  FILE* f = fopen(bin_file, "rb");
  if (!f) {
    perror("edc: Failed to open input file");
    return -1;
  }

  u32         nins;
  u8*         inss;
  u32         nlits;
  e_var*      lits;
  u32         nfuncs;
  e_function* funcs;
  e_file_load(f, &nins, &inss, &nlits, &lits, &nfuncs, &funcs);

  e_print_instruction_stream((const u8*)inss, nins, 0);

  for (int i = 0; i < nfuncs; i++) {
    printf("%u(%u):\n", funcs[i].name_hash, funcs[i].nargs);
    e_print_instruction_stream((const u8*)funcs[i].code, funcs[i].code_size, 4);
  }

  fclose(f);
  return 0;
}