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

#include "dc.h"

#include "fn.h"
#include "rwhelp.h"
#include "stdafx.h"
#include "var.h"

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

  void* root_allocation;

  u32         nlits;
  e_var*      lits;
  u32         nfuncs;
  u32         nins;
  u8*         ins;
  e_function* funcs;
  if (e_file_load(f, &root_allocation, &nins, &ins, &nlits, &lits, &nfuncs, &funcs)) {
    perror("Failed to open input file");
    return -1;
  }

  e_print_instruction_stream((const u8*)ins, nins, 0);
  for (int i = 0; i < nfuncs; i++) {
    printf("%u(%u):\n", funcs[i].name_hash, funcs[i].nargs);
    e_print_instruction_stream((const u8*)funcs[i].code, funcs[i].code_size, 4);
  }

  printf(".literals:\n");
  for (u32 i = 0; i < nlits; i++) {
    printf("[%u] = ", i);
    e_var_print(&lits[i], stdout);
    fputc('\n', stdout);
  }

  fclose(f);

  free(root_allocation);
  return 0;
}