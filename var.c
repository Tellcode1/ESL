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

#include "var.h"

#include "pool.h"
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>

int
e_var_shallow_cpy(const e_var* var, e_var* dst)
{
  if (!dst || !var) return -1;

  dst->type = var->type;

  /**
   * Variables are stored elsewhere, not in containers.
   * This allows the container to just maintain a pointer to those variables,
   * Which can be shallow copied easily.
   * The reference counter pointer is also copied.
   */
  dst->val = var->val;

  return 0;
}

int
e_var_deep_cpy(const e_var* var, e_var* dst)
{
  if (!dst || !var) return -1;

  dst->type = var->type;

  switch (var->type) {
    case E_VARTYPE_VOID:
    case E_VARTYPE_INT:
    case E_VARTYPE_BOOL:
    case E_VARTYPE_CHAR:
    case E_VARTYPE_FLOAT: dst->val = var->val; break;
    case E_VARTYPE_STRING:
      dst->val.s              = e_refdobj_pool_acquire(&ge_pool);
      E_VAR_AS_STRING(dst)->s = strdup(E_VAR_AS_STRING(var)->s);
      break;
    case E_VARTYPE_LIST: {
      return e_list_init(E_VAR_AS_LIST(var)->vars, E_VAR_AS_LIST(var)->size, E_VAR_AS_LIST(dst));
    }
    case E_VARTYPE_MAP: {
      /**
        * Create an array of all key value pairs as a map
        * And use it to create the map.
      */
      e_var* flattened = malloc(sizeof(e_var) * 2 * E_VAR_AS_MAP(var)->size);
      memcpy(flattened, E_VAR_AS_MAP(var)->keys, sizeof(e_var) * E_VAR_AS_MAP(var)->size);
      memcpy(flattened + E_VAR_AS_MAP(var)->size, E_VAR_AS_MAP(var)->vals, sizeof(e_var) * E_VAR_AS_MAP(var)->size);
      int e = e_map_init(flattened, E_VAR_AS_MAP(var)->size, E_VAR_AS_MAP(dst));
      free(flattened);
      return e;
    }

    case E_VARTYPE_ERROR: break;
  }

  return 0;
}

i32
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

void
e_var_release(e_var* v)
{
  if (!v) return;
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

void
e_var_free(e_var* var)
{
  if (!var) return;

  switch (var->type) {
    case E_VARTYPE_VOID:
    case E_VARTYPE_INT:
    case E_VARTYPE_CHAR:
    case E_VARTYPE_BOOL:
    case E_VARTYPE_ERROR:
    case E_VARTYPE_FLOAT: break;

    case E_VARTYPE_STRING:
      free(E_VAR_AS_STRING(var)->s);
      e_refdobj_pool_return(&ge_pool, var->val.s);
      break;

    case E_VARTYPE_LIST:
      e_list_free(E_VAR_AS_LIST(var));
      e_refdobj_pool_return(&ge_pool, var->val.list);
      break;

    case E_VARTYPE_MAP:
      e_map_free(E_VAR_AS_MAP(var));
      e_refdobj_pool_return(&ge_pool, var->val.list);
      break;
  }

  /* safety: Zero out the variable */
  memset(var, 0, sizeof(*var));
}

void
e_var_print(const struct e_var* v, FILE* f)
{
  switch (v->type) {
    case E_VARTYPE_VOID: fprintf(f, "void"); break;
    case E_VARTYPE_INT: fprintf(f, "%i", v->val.i); break;
    case E_VARTYPE_CHAR: fprintf(f, "%c", v->val.c); break;
    case E_VARTYPE_BOOL: fprintf(f, "%s", (int)v->val.b ? "true" : "false"); break;
    case E_VARTYPE_FLOAT: fprintf(f, "%g", v->val.f); break;
    case E_VARTYPE_STRING: fprintf(f, "%s", E_VAR_AS_STRING(v)->s); break;
    case E_VARTYPE_LIST: {
      fputc('[', f);
      for (u32 i = 0; i < E_VAR_AS_LIST(v)->size; i++) {
        e_var_print(&E_VAR_AS_LIST(v)->vars[i], f);
        if (i < E_VAR_AS_LIST(v)->size - 1) { fputs(", ", f); }
      }
      fputc(']', f);
      break;
    }
    case E_VARTYPE_MAP: break;
    case E_VARTYPE_ERROR: {
      fprintf(f, "%i", v->val.errcode);
      break;
    }
  }
}

void
e_var_to_string(const struct e_var* v, char* buffer, size_t buffer_size)
{
  switch (v->type) {
    case E_VARTYPE_VOID: strlcpy(buffer, "void", buffer_size); break;
    case E_VARTYPE_INT: snprintf(buffer, buffer_size, "%i", v->val.i); break;
    case E_VARTYPE_CHAR: snprintf(buffer, buffer_size, "%c", v->val.c); break;
    case E_VARTYPE_BOOL: snprintf(buffer, buffer_size, "%s", (int)v->val.b ? "true" : "false"); break;
    case E_VARTYPE_FLOAT: snprintf(buffer, buffer_size, "%g", v->val.f); break;
    case E_VARTYPE_STRING: snprintf(buffer, buffer_size, "%s", E_VAR_AS_STRING(v)->s); break;
    case E_VARTYPE_LIST: {
      strlcpy(buffer, "[", buffer_size);

      size_t offset = strlen(buffer);

      for (u32 i = 0; i < E_VAR_AS_LIST(v)->size; i++) {
        e_var_to_string(&E_VAR_AS_LIST(v)->vars[i], buffer + offset, buffer_size - offset);

        offset = strlen(buffer);

        if (i < E_VAR_AS_LIST(v)->size - 1) {
          strlcat(buffer, ", ", buffer_size);
          offset = strlen(buffer);
        }
      }

      strlcat(buffer, "]", buffer_size);

      break;
    }
    case E_VARTYPE_MAP: break;
    case E_VARTYPE_ERROR: {
      snprintf(buffer, buffer_size, "%i", v->val.errcode);
      break;
    }
  }
}

size_t
e_var_to_string_size(const struct e_var* v)
{
  size_t total = 0;
  switch (v->type) {
    case E_VARTYPE_VOID: total += strlen("void"); break;
    case E_VARTYPE_INT: total += snprintf(nullptr, 0, "%i", v->val.i); break;
    case E_VARTYPE_CHAR: total += snprintf(nullptr, 0, "%c", v->val.c); break;
    case E_VARTYPE_BOOL: return strlen((int)v->val.b ? "true" : "false"); break;
    case E_VARTYPE_FLOAT: total += snprintf(nullptr, 0, "%g", v->val.f); break;
    case E_VARTYPE_STRING: total += strlen(E_VAR_AS_STRING(v)->s); break;
    case E_VARTYPE_LIST: {
      total += 1;
      for (u32 i = 0; i < E_VAR_AS_LIST(v)->size; i++) {
        total += e_var_to_string_size(&E_VAR_AS_LIST(v)->vars[i]);
        if (i < E_VAR_AS_LIST(v)->size - 1) { total += 2; } // ", "
      }
      total += 1;
      break;
    }
    case E_VARTYPE_MAP: break;
    case E_VARTYPE_ERROR: {
      total += snprintf(nullptr, 0, "%i", v->val.errcode);
      break;
    }
  }
  return total;
}

u32
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

bool
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
