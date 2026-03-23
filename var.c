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

#include "refcount.h"
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
      dst->val.s       = malloc(sizeof(e_string));
      dst->val.s->refc = e_refc_init();
      dst->val.s->s    = strdup(var->val.s->s);
      break;
    case E_VARTYPE_LIST: {
      dst->val.list = malloc(sizeof(e_list));
      return e_list_init(var->val.list->vars, var->val.list->size, dst->val.list);
    }
    case E_VARTYPE_MAP:
    case E_VARTYPE_ERROR: break;
  }

  return 0;
}

void
e_var_free(e_var* var)
{
  switch (var->type) {
    case E_VARTYPE_VOID:
    case E_VARTYPE_INT:
    case E_VARTYPE_CHAR:
    case E_VARTYPE_BOOL:
    case E_VARTYPE_ERROR:
    case E_VARTYPE_FLOAT: break;

    case E_VARTYPE_STRING:
      e_refc_free(var->val.s->refc);
      free(var->val.s->s);
      free(var->val.s);
      break;

    case E_VARTYPE_LIST:
      e_refc_free(var->val.list->refc);
      e_list_free(var->val.list);
      free(var->val.list);
      break;

    case E_VARTYPE_MAP:
      e_refc_free(var->val.map->refc);
      for (u64 i = 0; i < var->val.map->size; i++) { e_var_release(&var->val.map->keys[i]); }
      for (u64 i = 0; i < var->val.map->size; i++) { e_var_release(&var->val.map->vals[i]); }
      free(var->val.map);
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
    case E_VARTYPE_STRING: fprintf(f, "%s", v->val.s->s); break;
    case E_VARTYPE_LIST: {
      fputc('[', f);
      for (u32 i = 0; i < v->val.list->size; i++) {
        e_var_print(&v->val.list->vars[i], f);
        if (i < v->val.list->size - 1) { fputs(", ", f); }
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
    case E_VARTYPE_STRING: snprintf(buffer, buffer_size, "%s", v->val.s->s); break;
    case E_VARTYPE_LIST: {
      strlcpy(buffer, "[", buffer_size);

      for (u32 i = 0; i < v->val.list->size; i++) {
        size_t size = e_var_to_string_size(v);

        char tmp[size + 1];
        e_var_to_string(v, tmp, size + 1);
        strlcat(buffer, tmp, buffer_size);

        if (i < v->val.list->size - 1) { strlcat(buffer, ", ", buffer_size); }
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
    case E_VARTYPE_STRING: total += strlen(v->val.s->s); break;
    case E_VARTYPE_LIST: {
      total += 1;
      for (u32 i = 0; i < v->val.list->size; i++) {
        total += e_var_to_string_size(&v->val.list->vars[i]);
        if (i < v->val.list->size - 1) { total += 2; } // ", "
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
