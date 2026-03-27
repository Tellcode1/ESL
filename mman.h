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

#ifndef E_MEMORY_MANAGER_H
#define E_MEMORY_MANAGER_H

/**
 * A general memory manager.
 * Memory can be allocated using nmalloc and must be freed using nfree.
 *
 * Memory is allocated internally using malloc, in pages of size E_PAGE_SIZE
 * This page is divided into 'frames'.
 * Frames are further divided, finally, into blocks.
 * On allocation, the memory manager only responds with blocks (which
 * have no defined size).
 */

#include "stdafx.h"

/**
 * Size of a sblock minus metadata.
 * Addressable memory.
 */
#define E_SBLOCK_SIZE (64 - sizeof(u32))
#define E_PAGE_SIZE 4096

struct emm_sblock;
struct emm_block;
struct emm_page;

/**
 * Branches.
 * The pool (heap) allocates a branch when it doesn't
 * doesn't see any free nodes.
 */
typedef struct emm_sblock {
  /* Index in parent page */
  u32 index;
  u8  data[E_SBLOCK_SIZE];
} emm_sblock;

/**
 * Pool of referenced objects.
 * Root.
 */
typedef struct emm_page {
  emm_sblock sblocks[E_PAGE_SIZE / sizeof(emm_sblock)];
} emm_page;

typedef struct emm_block {
  size_t size;
} emm_block;

#endif // E_MEMORY_MANAGER_H