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

#include "map.h"

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
