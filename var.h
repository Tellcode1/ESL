#ifndef ESL_VAR_H
#define ESL_VAR_H

#include "refcount.h"
#include "stdafx.h"

#include <string.h>

struct e_var;
struct e_list;
struct e_map;

typedef enum e_vartype {
  E_VARTYPE_VOID, // invalid / unset
  E_VARTYPE_INT,
  E_VARTYPE_BOOL,
  E_VARTYPE_CHAR,
  E_VARTYPE_FLOAT,
  E_VARTYPE_STRING,
  E_VARTYPE_LIST,
  E_VARTYPE_MAP,
  E_VARTYPE_ERROR, // use e_error_string to get string representation of the error.
} e_vartype;

struct e_list {
#ifdef NV_64BIT
  u64            size;
  u64            capacity;
  struct e_var** vars;
#else
  u32            size;
  u32            capacity;
  struct e_var** vars;
#endif
};

struct e_map {
#ifdef NV_64BIT
  u64            size;
  u64            capacity;
  struct e_var** keys;
  struct e_var** vals;
#else
  u32            size;
  u32            capacity;
  struct e_var** keys;
  struct e_var** vals;
#endif
};

typedef struct e_string {
  char* s;
} e_string;

typedef union e_varval {
  int              i;
  char             c;
  bool             b;
  int              errcode;
  double           f;
  struct e_string* s;
  struct e_list*   list;
  struct e_map*    map;
} e_varval;

typedef struct e_var {
  e_vartype type;
  e_refc*   refc; // Reference counter: e_var_acquire(), e_var_release()
  e_varval  val;
} e_var;

int e_var_shallow_cpy(const e_var* var, e_var* dst);
int e_var_deep_cpy(const e_var* var, e_var* dst);

void e_var_print(const struct e_var* v, FILE* f);

int e_list_init(e_var* vars_to_reference, u64 nvars, struct e_list* list);

void e_var_free(e_var* var);

static inline i32
e_var_acquire(e_var* v)
{
  if (v->refc == nullptr) return -1;
  return e_refc_acquire(v->refc);
}

static inline void
e_var_release(e_var* v)
{
  if (v->refc == nullptr) return;

  e_refc_release(v->refc);
  if (v->refc->ctr <= 0) e_var_free(v);
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
    case E_VARTYPE_STRING: return e_hash_fnv(var->val.s->s, strlen(var->val.s->s));
    case E_VARTYPE_LIST: return e_combine_hash((const void**)var->val.list->vars, var->val.list->size, sizeof(e_var));
    case E_VARTYPE_MAP:
      return e_combine_hash((const void**)var->val.map->keys, var->val.map->size, sizeof(e_var))
          + 13 * e_combine_hash((const void**)var->val.map->vals, var->val.map->size, sizeof(e_var));
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
    case E_VARTYPE_STRING: return strcmp(a->val.s->s, b->val.s->s) == 0;
    case E_VARTYPE_LIST:
      if (a->val.list->size != b->val.list->size) return false;
      for (size_t i = 0; i < a->val.list->size; i++) {
        if (!e_var_equal(a->val.list->vars[i], b->val.list->vars[i])) return false;
      }
      return true;
    case E_VARTYPE_MAP:
      exit(-1); /* TODO: Implement */
      return false;
  }
#if defined(__has_builtin) && __has_builtin(__builtin_unreachable)
  __builtin_unreachable();
#endif
  return false;
}

#endif // ESL_H
