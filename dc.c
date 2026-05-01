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

#include "cc.h"
#include "fn.h"
#include "rwhelp.h"
#include "stdafx.h"
#include "var.h"

#include <stdio.h>
#include <string.h>

int
main(int argc, char** argv)
{
  if (argc != 2) {
    fprintf(stderr, "edc: [input_binary]\n");
    return -1;
  }

  FILE* f = NULL;

  const char* bin_file = argv[1];
  if (strcmp(bin_file, "-") == 0) {
    f = stdin;
  } else {
    f = fopen(bin_file, "r");
    if (!f) {
      perror("edc: Failed to open input file");
      return -1;
    }
  }

  void*                root_allocation = nullptr;
  e_compilation_result r               = { 0 };

  int e = e_file_load(&r, &root_allocation, f);
  if (e) {
    fprintf(stderr, "eexec: Failed to parse input file: %i\n", e);
    return -1;
  }

  e_print_instruction_stream((const u8*)r.instructions, r.ninstructions, 0);
  for (u32 i = 0; i < r.nfunctions; i++) {
    printf("%u(%u):\n", r.functions[i].name_hash, r.functions[i].nargs);
    e_print_instruction_stream((const u8*)r.functions[i].code, r.functions[i].code_size, 4);
  }

  printf("literals:\n");
  for (u32 i = 0; i < r.nliterals; i++) {
    printf("[%u | %u] = ", i, e_var_hash(&r.literals[i]));
    e_var_print(&r.literals[i], stdout);
    fputc('\n', stdout);
  }

  if (strcmp(bin_file, "-") != 0) fclose(f);

  free(root_allocation);
  return 0;
}