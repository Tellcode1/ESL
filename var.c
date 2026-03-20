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
   */
  dst->val = var->val;

  // Share the refc pointer
  dst->refc = var->refc;

  return 0;
}

int
e_var_deep_cpy(const e_var* var, e_var* dst)
{
  if (!dst || !var) return -1;

  dst->type = var->type;
  dst->refc = e_refc_init();

  switch (var->type) {
    case E_VARTYPE_VOID:
    case E_VARTYPE_INT:
    case E_VARTYPE_BOOL:
    case E_VARTYPE_CHAR:
    case E_VARTYPE_FLOAT: dst->val = var->val; break;
    case E_VARTYPE_STRING:
      dst->val.s    = malloc(sizeof(e_string));
      dst->val.s->s = strdup(var->val.s->s);
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
  if (var->refc) {
    e_refc_free(var->refc);
    var->refc = nullptr;
  }
  switch (var->type) {
    case E_VARTYPE_VOID:
    case E_VARTYPE_INT:
    case E_VARTYPE_CHAR:
    case E_VARTYPE_BOOL:
    case E_VARTYPE_ERROR:
    case E_VARTYPE_FLOAT: break;

    case E_VARTYPE_STRING:
      free(var->val.s->s);
      free(var->val.s);
      break;

    case E_VARTYPE_LIST:
      e_list_free(var->val.list);
      free(var->val.list);
      break;

    case E_VARTYPE_MAP:
      for (u64 i = 0; i < var->val.map->size; i++) { e_var_release(&var->val.map->keys[i]); }
      for (u64 i = 0; i < var->val.map->size; i++) { e_var_release(&var->val.map->vals[i]); }
      free(var->val.map);
      break;
  }
}

void
e_var_print(const struct e_var* v, FILE* f)
{
  switch (v->type) {
    case E_VARTYPE_VOID: fprintf(f, "void"); break;
    case E_VARTYPE_INT: fprintf(f, "%i", v->val.i); break;
    case E_VARTYPE_CHAR: fprintf(f, "%c", v->val.c); break;
    case E_VARTYPE_BOOL: fprintf(f, "%s", v->val.b ? "true" : "false"); break;
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