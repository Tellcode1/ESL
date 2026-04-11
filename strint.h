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

#ifndef E_STRING_INTERN_H
#define E_STRING_INTERN_H

#include "stdafx.h"

typedef struct e_str_interner {
  u32*   string_hashes;
  char** strings;
  u32    strings_count;
  u32    strings_capacity;
} e_str_interner;

static inline int
e_str_interner_init(u32 capacity, e_str_interner* table)
{
  table->string_hashes    = (u32*)malloc(sizeof(u32) * capacity);
  table->strings          = (char**)malloc(sizeof(char*) * capacity);
  table->strings_count    = 0;
  table->strings_capacity = capacity;
  return 0;
}

static inline const char*
e_str_intern(const char* s, e_str_interner* table)
{
  u32 hash = e_hash_fnv(s, strlen(s));
  for (u32 i = 0; i < table->strings_count; i++) {
    if (hash == table->string_hashes[i]) { return table->strings[i]; }
  }

  if (table->strings_count >= table->strings_capacity) {
    u32    new_capacity  = table->strings_capacity * 2;
    u32*   string_hashes = (u32*)realloc(table->string_hashes, sizeof(u32) * new_capacity);
    char** strings       = (char**)realloc(table->strings, sizeof(char*) * new_capacity);
    if (string_hashes == nullptr || strings == nullptr) return nullptr;

    table->string_hashes    = string_hashes;
    table->strings          = strings;
    table->strings_capacity = new_capacity;
  }

  u32   i                 = table->strings_count;
  char* sdup              = e_strdup(s);
  table->strings[i]       = sdup;
  table->string_hashes[i] = hash;
  table->strings_count++;

  return table->strings[i];
}

static inline void
e_str_interner_free(e_str_interner* table)
{
  free(table->string_hashes);
  for (u32 i = 0; i < table->strings_count; i++) { free(table->strings[i]); }
  free(table->strings);
}

#endif // #define E_STRING_INTERN_H
