#ifndef E_STR_INTERNER_H
#define E_STR_INTERNER_H

#include "../std/include/stdafx.h"
#include "../std/include/types.h"
#include "var.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct str_interner
{
  char** strings; // IDX of string is ID
  u32*   hashes;
  size_t nstrings;
  size_t cstrings;
};

static int
interner_init(struct str_interner* table)
{
  const int init_capacity = 16;
  table->strings          = (char**)malloc(sizeof(char*) * init_capacity);
  table->hashes           = (u32*)malloc(sizeof(u32) * init_capacity);
  table->nstrings         = 0;
  table->cstrings         = init_capacity;

  if (table->strings == nullptr || table->hashes == nullptr) return -1;

  return 0;
}

static void
interner_free(struct str_interner* table)
{
  free(table->strings);
  free(table->hashes);
}

static u32
intern_string(struct str_interner* table, const char* s)
{
  u32 hash = e_hash_fnv(s, strlen(s));
  for (size_t i = 0; i < table->nstrings; i++)
  {
    if (table->hashes[i] == hash && strcmp(s, table->strings[i]) == 0) { return i; }
  }

  // add string to table
  if (table->nstrings >= table->cstrings)
  {
    size_t new_cap  = table->cstrings * 2;
    char** new_strs = (char**)realloc(table->strings, sizeof(char*) * new_cap);
    if (new_strs == nullptr)
    {
      perror("malloc() failed");
      return -1;
    }
    u32* new_hashes = (u32*)realloc(table->hashes, sizeof(u32) * new_cap);

    table->cstrings = new_cap;
    table->strings  = new_strs;
    table->hashes   = new_hashes;
  }

  u32 idx             = table->nstrings;
  table->strings[idx] = strdup(s);
  table->hashes[idx]  = e_hash_fnv(s, strlen(s));
  table->nstrings++;

  return idx;
}

#endif // E_STR_INTERNER_H