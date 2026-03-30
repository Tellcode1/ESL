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

#ifndef ESL_VAR_H
#define ESL_VAR_H

#include "pool.h"
#include "refcount.h"
#include "stdafx.h"

#include <string.h>

#define E_OBJ_AS_STRING(obj) ((e_string*)((obj)->data))
#define E_OBJ_AS_LIST(obj) ((e_list*)((obj)->data))
#define E_OBJ_AS_MAP(obj) ((e_map*)((obj)->data))

#define E_VAR_AS_STRING(var) ((e_string*)((var)->val.s->data))
#define E_VAR_AS_LIST(var) ((e_list*)((var)->val.list->data))
#define E_VAR_AS_MAP(var) ((e_map*)((var)->val.map->data))

struct e_var;
struct e_list;
struct e_map;
struct e_refdobj_pool;

/**
 * Bitmask to allow functions to just check
 * the masks. No variable can have more than one
 * bit set though.
 *
 * Compiler Info doesn't have an entry in the
 * struct because it doesn't need one. Every
 * variable during compilation is stored as such.
 */
typedef enum e_vartype {
  E_VARTYPE_VOID   = 1 << 0, // invalid / unset
  E_VARTYPE_INT    = 1 << 1,
  E_VARTYPE_BOOL   = 1 << 2,
  E_VARTYPE_CHAR   = 1 << 3,
  E_VARTYPE_FLOAT  = 1 << 4,
  E_VARTYPE_STRING = 1 << 5,
  E_VARTYPE_LIST   = 1 << 6,
  E_VARTYPE_MAP    = 1 << 7,
  E_VARTYPE_ERROR  = 1 << 8, // use e_error_string to get string representation of the error.
} e_vartype;
// typedef u32 e_vartype;

typedef struct e_list {
  struct e_var* vars;
  u32           size;
  u32           capacity;
} e_list;

typedef struct e_map {
  struct e_var* keys;
  struct e_var* vals;
  u64           size;
  u64           capacity;
} e_map;

typedef struct e_string {
  char* s;
} e_string;

typedef union e_varval {
  int  i;
  char c;
  bool b;
  int  errcode;

  /* No 32 bit floats :) */
  double f;

  e_refdobj* s;    // Use E_VAR_AS_STRING to access as e_string*
  e_refdobj* list; // Use E_VAR_AS_LIST to access as e_list*
  e_refdobj* map;  // Use E_VAR_AS_MAP to access as e_map*

  /* Compiler info for variables, not stored in runtime. */
  e_refdobj* compinfo;
} e_varval;

typedef struct e_var {
  e_vartype type;
  e_varval  val;
} e_var;

int e_var_shallow_cpy(const e_var* var, e_var* dst);
int e_var_deep_cpy(const e_var* var, e_var* dst);

void   e_var_print(const struct e_var* v, FILE* f);
void   e_var_to_string(const struct e_var* v, char* buffer, size_t buffer_size);
size_t e_var_to_string_size(const struct e_var* v);

int  e_list_init(e_var* vars, u32 nvars, struct e_list* list);
void e_list_free(struct e_list* list);

int  e_map_init(e_var* vars, u32 nvars, e_map* map);
void e_map_free(e_map* map);

e_var* e_map_find(e_map* map, const e_var* key);

e_var* e_list_index(struct e_list* list, u64 index);
int    e_list_append(e_var* v, struct e_list* list);
int    e_list_remove(u64 index, struct e_list* list);

void e_var_free(e_var* var);

static inline i32
e_var_acquire(e_var* v)
{
  e_refc* refc = nullptr;
  switch (v->type) {
    case E_VARTYPE_MAP: refc = &v->val.map->refc; break;
    case E_VARTYPE_LIST: refc = &v->val.list->refc; break;
    case E_VARTYPE_STRING: refc = &v->val.s->refc; break;
    default: refc = nullptr; break;
  }

  if (refc == nullptr) return -1;
  return e_refc_acquire(refc);
}

static inline void
e_var_release(e_var* v)
{
  e_refc* refc = nullptr;
  switch (v->type) {
    case E_VARTYPE_MAP: refc = &v->val.map->refc; break;
    case E_VARTYPE_LIST: refc = &v->val.list->refc; break;
    case E_VARTYPE_STRING: refc = &v->val.s->refc; break;
    default: refc = nullptr; break;
  }

  if (refc == nullptr) return;

  e_refc_release(refc);
  if (refc->ctr <= 0) e_var_free(v);
}

static inline u32
e_hash_fnv(const void* data, size_t size)
{
  const uchar* current = (const uchar*)data;

  u32 hash = 2166136261U;
  for (size_t i = 0; i < size; ++i) {
    hash ^= current[i];
    hash *= 16777619U;
  }

  return hash;
}

static inline u32
e_combine_hash(const void** list, size_t size, size_t var_size)
{
  u32 combined_hash = 0;
  for (size_t i = 0; i < size; i++) {
    u32 element_hash = e_hash_fnv(list[i], var_size);
    combined_hash    = combined_hash * 31 + element_hash;
  }
  return combined_hash;
}

static inline u32
e_var_hash(const e_var* var)
{
  switch (var->type) {
    case E_VARTYPE_VOID:
    case E_VARTYPE_ERROR:
    case E_VARTYPE_INT: return e_hash_fnv(&var->val.i, sizeof(var->val.i));
    case E_VARTYPE_BOOL: return e_hash_fnv(&var->val.b, sizeof(bool));
    case E_VARTYPE_CHAR: return e_hash_fnv(&var->val.c, sizeof(char));
    case E_VARTYPE_FLOAT: return e_hash_fnv(&var->val.f, sizeof(var->val.f));
    case E_VARTYPE_STRING: return e_hash_fnv(E_VAR_AS_STRING(var)->s, strlen(E_VAR_AS_STRING(var)->s));
    case E_VARTYPE_LIST: return e_combine_hash((const void**)E_VAR_AS_LIST(var)->vars, E_VAR_AS_LIST(var)->size, sizeof(e_var));
    case E_VARTYPE_MAP:
      return e_combine_hash((const void**)(E_VAR_AS_MAP(var)->keys), (E_VAR_AS_MAP(var)->size), sizeof(e_var))
          + 13 * e_combine_hash((const void**)(E_VAR_AS_MAP(var)->vals), (E_VAR_AS_MAP(var)->size), sizeof(e_var));
    default: return e_hash_fnv(&var->val, sizeof(var->val));
  }
}

static inline bool
e_var_equal(const e_var* a, const e_var* b)
{
  if (a->type != b->type) return false;

  switch (a->type) {
    case E_VARTYPE_VOID:
    case E_VARTYPE_ERROR:
    case E_VARTYPE_INT: return a->val.i == b->val.i;
    case E_VARTYPE_BOOL: return a->val.b == b->val.b;
    case E_VARTYPE_CHAR: return a->val.c == b->val.c;
    case E_VARTYPE_FLOAT: return a->val.f == b->val.f;
    case E_VARTYPE_STRING: return strcmp(E_VAR_AS_STRING(a)->s, E_VAR_AS_STRING(b)->s) == 0;
    case E_VARTYPE_LIST:
      if (E_VAR_AS_LIST(a)->size != E_VAR_AS_LIST(b)->size) return false;
      for (size_t i = 0; i < E_VAR_AS_LIST(a)->size; i++) {
        if (!e_var_equal(&E_VAR_AS_LIST(a)->vars[i], &E_VAR_AS_LIST(b)->vars[i])) return false;
      }
      return true;
    case E_VARTYPE_MAP:
      if (E_VAR_AS_MAP(a)->size != E_VAR_AS_MAP(b)->size) return false;
      for (u32 i = 0; i < E_VAR_AS_MAP(a)->size; i++) {
        if (!e_var_equal(&E_VAR_AS_MAP(a)->keys[i], &E_VAR_AS_MAP(b)->keys[i])) return false;
        if (!e_var_equal(&E_VAR_AS_MAP(a)->vals[i], &E_VAR_AS_MAP(b)->vals[i])) return false;
      }
      return true;
  }
  return false;
}

static inline const char*
e_var_type_to_string(e_vartype type)
{
  switch (type) {
    case E_VARTYPE_VOID: return "void";
    case E_VARTYPE_INT: return "int";
    case E_VARTYPE_BOOL: return "bool";
    case E_VARTYPE_CHAR: return "char";
    case E_VARTYPE_FLOAT: return "float";
    case E_VARTYPE_STRING: return "string";
    case E_VARTYPE_LIST: return "list";
    case E_VARTYPE_MAP: return "map";
    case E_VARTYPE_ERROR: return "error";
  }
  return "unknown";
}

#endif // ESL_H
