#include "refcount.h"
#include "var.h"

#include <string.h>

int
e_list_init(e_var* vars, u64 nvars, struct e_list* list)
{
  if (!list) return -1;

  memset(list, 0, sizeof(*list));

  u64 capacity = MAX(8, nvars);

  list->refc     = e_refc_init();
  list->size     = nvars;
  list->capacity = capacity;
  list->vars     = (e_var*)malloc(sizeof(e_var) * capacity);
  if (!list->vars) return -1;

  for (u64 i = 0; i < nvars; i++) {
    e_var_shallow_cpy(&vars[i], &list->vars[i]);
    e_var_acquire(&list->vars[i]); /* tell the big man we want the variables to outlive the list */
  }

  return 0;
}

int
e_list_append(e_var* v, struct e_list* list)
{
  if (!list || !v) return -1;

  if (list->size >= list->capacity) {
    u64    new_capacity = MAX(list->capacity * 2, 1);
    e_var* new_vars     = (e_var*)realloc(list->vars, sizeof(e_var) * new_capacity);
    if (!new_vars) return -1;

    list->vars     = new_vars;
    list->capacity = new_capacity;
  }

  e_var_shallow_cpy(v, &list->vars[list->size++]);
  return 0;
}

e_var*
e_list_index(struct e_list* list, u64 index)
{
  if (index >= list->size) return nullptr;
  return &list->vars[index];
}

int
e_list_remove(u64 index, struct e_list* list)
{
  if (index >= list->size) return -1;

  e_var_release(&list->vars[index]);

  // shift elements down. could use memmove but whatever.
  for (u64 i = index; i < list->size - 1; i++) { list->vars[i] = list->vars[i + 1]; }

  list->size--;
  return 0;
}

void
e_list_free(e_list* list)
{
  for (u64 i = 0; i < list->size; i++) { e_var_release(&list->vars[i]); }
  free(list->vars);
}