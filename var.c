#include "var.h"

#include "refcount.h"
#include "stdafx.h"

#include <stdlib.h>

static const u32 klist_init_capacity = 16;

int
e_list_init(e_var* vars_to_reference, u64 nvars, struct e_list* list)
{
  list->size     = nvars;
  list->capacity = MAX(klist_init_capacity, nvars);

  /**
   * Pointers to the variables.
   */
  list->vars = (e_var**)malloc(sizeof(void*) * nvars);
  if (list->vars == nullptr) return -1;

  for (u64 i = 0; i < nvars; i++) { e_var_shallow_cpy(&vars_to_reference[i], list->vars[i]); }

  return 0;
}

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
    case E_VARTYPE_LIST:
    case E_VARTYPE_MAP:
    case E_VARTYPE_ERROR: break;
  }

  return 0;
}

void
e_var_free(e_var* var)
{
  e_refc_free(var->refc);
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
      for (u64 i = 0; i < var->val.list->size; i++) { e_var_release(var->val.list->vars[i]); }
      free(var->val.list);
      break;

    case E_VARTYPE_MAP:
      for (u64 i = 0; i < var->val.map->size; i++) { e_var_release(var->val.map->keys[i]); }
      for (u64 i = 0; i < var->val.map->size; i++) { e_var_release(var->val.map->vals[i]); }
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
    case E_VARTYPE_FLOAT: fprintf(f, "%f", v->val.f); break;
    case E_VARTYPE_STRING: fprintf(f, "%s", v->val.s->s); break;
    case E_VARTYPE_LIST: {
      break;
    }
    case E_VARTYPE_MAP: break;
    case E_VARTYPE_ERROR: {
      fprintf(f, "%i", v->val.errcode);
      break;
    }
  }
}