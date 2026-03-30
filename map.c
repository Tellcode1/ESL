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
  map->hashes   = (u32*)malloc(sizeof(u32) * capacity);
  if (!map->keys || !map->vals) return -1;

  for (u32 i = 0, j = 0; i < (npairs * 2); i += 2, j++) {
    e_var_shallow_cpy(&vars[i], &map->keys[j]);
    e_var_shallow_cpy(&vars[i + 1], &map->vals[j]);

    e_var_acquire(&map->keys[j]);
    e_var_acquire(&map->vals[j]);

    map->hashes[j] = e_var_hash(&vars[i]);

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
  free(map->keys);
  free(map->vals);
  free(map->hashes);
}

e_var*
e_map_find(e_map* map, const e_var* key)
{
  u32 hash = e_var_hash(key);

  for (u32 i = 0; i < map->size; i++) {
    if (hash == map->hashes[i] && e_var_equal(key, &map->keys[i])) { return &map->vals[i]; }
  }
  return nullptr;
}
