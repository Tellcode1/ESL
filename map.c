#include "bfunc.h"
#include "var.h"

int
e_map_init(e_var* vars, u32 npairs, e_map* map)
{
  if (!map) return -1;

  memset(map, 0, sizeof(*map));

  u32 capacity = MAX(8, npairs);

  map->size     = npairs;
  map->capacity = capacity;
  map->keys     = (e_var*)malloc(sizeof(e_var) * capacity);
  map->vals     = (e_var*)malloc(sizeof(e_var) * capacity);
  if (!map->keys || !map->vals) return -1;

  for (u32 i = 0, j = 0; i < (npairs * 2); i += 2, j++) {
    e_var_shallow_cpy(&vars[i], &map->keys[j]);
    e_var_shallow_cpy(&vars[i + 1], &map->vals[j]);

    e_var_acquire(&map->keys[j]);
    e_var_acquire(&map->vals[j]);

    // eb_print(&map->keys[j], 1);
    // printf(" : ");
    // eb_println(&map->vals[j], 1);
  }
  return 0;
}

void
e_map_free(e_map* map)
{
  for (u32 i = 0; i < map->size; i++) {
    e_var_release(&map->keys[i]);
    e_var_release(&map->vals[i]);
  }
}

e_var*
e_map_find(e_map* map, const e_var* key)
{
  for (u32 i = 0; i < map->size; i++) {
    const e_var* m_key = &map->keys[i];
    bool         equal = e_var_equal(key, m_key);
    if (equal) { return &map->vals[i]; }
  }
  return nullptr;
}
