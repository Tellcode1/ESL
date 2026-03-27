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

#ifndef E_POOL_H
#define E_POOL_H

#include "refcount.h"
#include "stdafx.h"

/**
 * Must be a power of 2!
 */
#define E_REFLEAVE_COUNT 64

/**
 * Size of a leaf, excluding the reference counter.
 */
#define E_REFLEAVE_SIZE 40

struct e_refdobj;
struct e_refdobj_branch;
struct e_refdobj_pool;

/**
 * Common referenced object struct.
 * All referenced structures (currently lists,maps and strings)
 * must follow this layout EXCEPT, the reference counter they hold
 * is a pointer to the one in this struct.
 * Leaf.
 */
typedef struct e_refdobj {
  u8     data[E_REFLEAVE_SIZE];
  e_refc refc; /* Reference counter */
} e_refdobj;

/**
 * Branches.
 * The pool (heap) allocates a branch when it doesn't
 * doesn't see any free nodes.
 */
typedef struct e_refdobj_branch {
  /* If a leaf is in use or not */
  e_refdobj leaves[E_REFLEAVE_COUNT];
  bool      leaves_in_use[E_REFLEAVE_COUNT];

  /* Number of free leaves on this branch */
  u32 free_leaves;

  /* Parent pool */
  struct e_refdobj_pool* pool;

} e_refdobj_branch;

/**
 * Pool of referenced objects.
 * Root.
 */
typedef struct e_refdobj_pool {
  /**
   *  Heap allocated branches.
   */
  e_refdobj_branch** branches;
  u32                nbranches;
} e_refdobj_pool;

extern e_refdobj_pool ge_pool;

/* Returns 0 on success. */
int  e_refdobj_pool_init(u32 nbranches, e_refdobj_pool* pool);
void e_refdobj_pool_free(e_refdobj_pool* pool);

/* Acquire an object from the pool. */
e_refdobj* e_refdobj_pool_acquire(e_refdobj_pool* pool);

/* Return an object to the pool from the first free branch. */
void e_refdobj_pool_return(e_refdobj_pool* pool, e_refdobj* obj);

/* Free all branches not currently in use. */
void e_refdobj_pool_trim(e_refdobj_pool* pool);

#endif // E_POOL_H