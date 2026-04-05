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

#ifndef E_ARENA_H
#define E_ARENA_H

#include "stdafx.h"

#include <stddef.h>
#include <stdlib.h>

/**
 * Minimum size of memory block that is allocated.
 */
#define E_PAGE_SIZE (64 * 1024)

/**
 * Minimum bytes that memory allocated is aligned to.
 * Applies both to malloc and realloc.
 */
#define E_MEMALIGN 8

/* Data is (uchar*)&page + sizeof(size_t) */
typedef struct e_arena_page {
  struct e_arena_page* next;
  size_t               size;
  size_t               head;
} e_arena_page;

typedef struct e_arena {
  struct e_arena_page* root;
  struct e_arena_page* current;
} e_arena;

static inline uintptr_t
align_up(uintptr_t data, size_t alignment)
{
  if (data % alignment == 0) return data;
  return ((data + alignment - 1) & ~(alignment - 1));
}

static inline int
e__create_and_link_page(size_t size, e_arena* arena)
{
  size = ((size + E_PAGE_SIZE - 1) / E_PAGE_SIZE) * E_PAGE_SIZE;

  e_arena_page* next = arena->root;

  e_arena_page* page = (e_arena_page*)malloc(size);
  if (page == nullptr) return -1;

  page->size = size - sizeof(e_arena_page);
  page->head = 0;
  page->next = next;

  arena->root    = page;
  arena->current = page;

  return 0;
}

static inline int
e_arena_init(u32 npages, e_arena* arena)
{
  npages = MAX(npages, 1);
  memset(arena, 0, sizeof(*arena));

  int e = 0;
  for (u32 i = 0; i < npages; i++) {
    e = e__create_and_link_page(E_PAGE_SIZE, arena);
    if (e < 0) return e;
  }

  return e;
}

static inline void
e_arena_resize(e_arena* a)
{ e__create_and_link_page(E_PAGE_SIZE, a); }

static inline void*
e_arnalloc(e_arena* a, size_t size)
{
  size_t total = size + sizeof(size_t);
  total        = align_up(total, E_MEMALIGN);

  /* can't fit in regular page. */
  if (total > (E_PAGE_SIZE - sizeof(e_arena_page))) {
    e_arena_page* page = (e_arena_page*)malloc(sizeof(e_arena_page) + total);
    page->size         = total;
    page->head         = total;
    page->next         = a->root;
    a->root            = page;

    uchar* data = (uchar*)page + sizeof(*page);
    memcpy(data, &size, sizeof(size));

    void* p = (void*)align_up((uintptr_t)(data + sizeof(size)), E_MEMALIGN);
    if ((uintptr_t)p & (E_MEMALIGN - 1)) abort();
    return p;
  }

  /**
   * Page with enough capacity.
   */
  e_arena_page* fits = a->root;
  while (fits != nullptr) {
    if ((fits->size - fits->head) >= total) break;
    fits = fits->next;
  }

  // If current pages doesn't meet our requirements,
  // Create an link a new one

  if (fits == nullptr) {
    e__create_and_link_page(E_PAGE_SIZE, a);
    fits = a->current;
  }

  uchar* data = ((uchar*)fits + sizeof(*fits)) + fits->head;

  // Write the size to the start of the pointer
  memcpy(data, &size, sizeof(size));

  // And return the pointer after it.
  void* ptr = (void*)align_up((uintptr_t)data + sizeof(size), E_MEMALIGN);
  if ((uintptr_t)ptr & (E_MEMALIGN - 1)) abort();

  fits->head += total;

  /**
   * If we detect the branch is close to being full,
   * we can place an order to the OS for more memory,
   * before we actually need it.
   * This generally improves performance by amortizing
   * malloc cost.
   *
   *
   * Causing more problems than fixing! Removed it.
   */

  return ptr;
}

static inline void*
e_arnrealloc(e_arena* a, void* ptr, size_t size)
{
  size_t old_size = 0;
  memcpy(&old_size, (size_t*)ptr - 1, sizeof(size_t));

  if (old_size >= size) { return ptr; }

  void* new_blk = e_arnalloc(a, size);
  if (ptr == nullptr) return new_blk;

  memcpy(new_blk, ptr, MIN(old_size, size));

  return new_blk;
}

static inline char*
e_arnstrdup(e_arena* arena, const char* s)
{
  char* new_s = nullptr;
  if (s != nullptr) {
    size_t l = strlen(s);
    new_s    = (char*)e_arnalloc(arena, l + 1);
    strncpy(new_s, s, l);
    new_s[l] = 0;
  }
  return new_s;
}

static inline void
e_arena_free(e_arena* arena)
{
  e_arena_page* next = arena->root;
  while (next != nullptr) {
    e_arena_page* new_next = next->next;
    free(next);
    next = new_next;
  }
}

#endif // E_ARENA_H