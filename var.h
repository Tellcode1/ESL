#ifndef ESL_VAR_H
#define ESL_VAR_H

#include "refcount.h"
#include "stdafx.h"

#include <string.h>

struct e_var;
struct e_list;
struct e_map;

/**
 * Bitmask to allow functions to just check
 * the masks. No variable can have more than one
 * bit set though.
 */
typedef enum e_vartype_bits {
  E_VARTYPE_VOID   = 1 << 0, // invalid / unset
  E_VARTYPE_INT    = 1 << 1,
  E_VARTYPE_BOOL   = 1 << 2,
  E_VARTYPE_CHAR   = 1 << 3,
  E_VARTYPE_FLOAT  = 1 << 4,
  E_VARTYPE_STRING = 1 << 5,
  E_VARTYPE_LIST   = 1 << 6,
  E_VARTYPE_MAP    = 1 << 7,
  E_VARTYPE_ERROR  = 1 << 8, // use e_error_string to get string representation of the error.
} e_vartype_bits;
typedef u32 e_vartype;

typedef struct e_list {
  e_refc*       refc; // Reference counter: e_var_acquire(), e_var_release()
  u64           size;
  u64           capacity;
  struct e_var* vars;
} e_list;

typedef struct e_map {
  e_refc*       refc; // Reference counter: e_var_acquire(), e_var_release()
  u64           size;
  u64           capacity;
  struct e_var* keys;
  struct e_var* vals;
} e_map;

typedef struct e_string {
  e_refc* refc; // Reference counter: e_var_acquire(), e_var_release()
  char*   s;
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
  e_varval  val;
} e_var;

int e_var_shallow_cpy(const e_var* var, e_var* dst);
int e_var_deep_cpy(const e_var* var, e_var* dst);

void   e_var_print(const struct e_var* v, FILE* f);
void   e_var_to_string(const struct e_var* v, char* buffer, size_t buffer_size);
size_t e_var_to_string_size(const struct e_var* v);

int  e_list_init(e_var* vars, u64 nvars, struct e_list* list);
void e_list_free(struct e_list* list);

e_var* e_list_index(struct e_list* list, u64 index);
int    e_list_append(e_var* v, struct e_list* list);
int    e_list_remove(u64 index, struct e_list* list);

void e_var_free(e_var* var);

static inline i32
e_var_acquire(e_var* v)
{
  e_refc* refc = nullptr;
  switch (v->type) {
    case E_VARTYPE_MAP: refc = v->val.map->refc; break;
    case E_VARTYPE_LIST: refc = v->val.list->refc; break;
    case E_VARTYPE_STRING: refc = v->val.s->refc; break;
  }

  if (refc == nullptr) return -1;
  return e_refc_acquire(refc);
}

static inline void
e_var_release(e_var* v)
{
  e_refc* refc = nullptr;
  switch (v->type) {
    case E_VARTYPE_MAP: refc = v->val.map->refc; break;
    case E_VARTYPE_LIST: refc = v->val.list->refc; break;
    case E_VARTYPE_STRING: refc = v->val.s->refc; break;
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
        if (!e_var_equal(&a->val.list->vars[i], &b->val.list->vars[i])) return false;
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
