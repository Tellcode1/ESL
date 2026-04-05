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

#include "refcount.h"
#include "stdafx.h"

#include <string.h>

#define E_OBJ_AS_STRING(obj) ((e_string*)((obj)->data))
#define E_OBJ_AS_LIST(obj) ((e_list*)((obj)->data))
#define E_OBJ_AS_MAP(obj) ((e_map*)((obj)->data))
#define E_OBJ_AS_STRUCT(obj) ((e_struct*)((obj)->data))

#define E_VAR_AS_STRING(var) ((e_string*)((var)->val.s->data))
#define E_VAR_AS_LIST(var) ((e_list*)((var)->val.list->data))
#define E_VAR_AS_MAP(var) ((e_map*)((var)->val.map->data))
#define E_VAR_AS_STRUCT(var) ((e_struct*)((var)->val.map->data))

struct e_var;
struct e_list;
struct e_map;
struct e_struct;
struct e_refdobj_pool;

/**
 * Bitmask to allow functions to just check
 * the masks. No variable can have more than one
 * bit set though.
 */
typedef enum e_vartype {
  E_VARTYPE_NULL        = 1 << 0, // unset
  E_VARTYPE_VOID        = 1 << 1,
  E_VARTYPE_INT         = 1 << 2,
  E_VARTYPE_BOOL        = 1 << 3,
  E_VARTYPE_CHAR        = 1 << 4,
  E_VARTYPE_FLOAT       = 1 << 5,
  E_VARTYPE_STRING      = 1 << 6,
  E_VARTYPE_LIST        = 1 << 7,
  E_VARTYPE_MAP         = 1 << 8,
  E_VARTYPE_STRUCT      = 1 << 9,
  E_VARTYPE_ERROR       = 1 << 10, // use e_error_string to get string representation of the error.
  E_VARTYPE_CC_VARIABLE = 1 << 11,
  E_VARTYPE_CC_STRUCT   = 1 << 12,
} e_vartype;
// typedef u32 e_vartype;

/**
 * Variable payload
 */
typedef union e_varval {
  int  i;
  char c;
  bool b;
  int  errcode;

  /* No 32 bit floats :) */
  double f;

  struct e_refdobj* s;     // Use E_VAR_AS_STRING to access as e_string*
  struct e_refdobj* list;  // Use E_VAR_AS_LIST to access as e_list*
  struct e_refdobj* map;   // Use E_VAR_AS_MAP to access as e_map*
  struct e_refdobj* struc; // Use E_VAR_AS_STRUCT to access as e_struct*

  /* Compiler info for variables, not stored in runtime. */
  struct e_refdobj* var_info;
  struct e_refdobj* struct_info;

  void* generic_ptr;
} e_varval;

typedef struct e_var {
  e_vartype type;
  e_varval  val;
} e_var;

typedef struct e_struct {
  e_var* members;
  u32*   member_hashes;
  u32    nmembers;
} e_struct;

e_var e_make_var_from_string(char* s);

int e_var_shallow_cpy(const e_var* var, e_var* dst);
int e_var_deep_cpy(const e_var* var, e_var* dst);

void   e_var_print(const struct e_var* v, FILE* f);
void   e_var_to_string(const struct e_var* v, char* buffer, size_t buffer_size);
size_t e_var_to_string_size(const struct e_var* v);

i32  e_var_acquire(e_var* v);
void e_var_release(e_var* v);

u32  e_var_hash(const e_var* var);
bool e_var_equal(const e_var* a, const e_var* b);

void e_var_free(e_var* var);

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
    case E_VARTYPE_CC_VARIABLE: return "var::compile_info";
    case E_VARTYPE_CC_STRUCT: return "struct::compile_info";
    case E_VARTYPE_NULL: return "null";
    case E_VARTYPE_STRUCT: return "struct";
  }
  return "unknown";
}

#endif // ESL_H
