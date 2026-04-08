#ifndef E_REFERENCE_OBJECT_POOL_H
#define E_REFERENCE_OBJECT_POOL_H

#include "stdafx.h"

/**
 * Number of leaves (rcounted objects) per branch.
 * Must be less than or equal to 32.
 */
#define ERPOOL_NUM_LEAVES_PER_BRANCH 32

typedef struct erpool_branch {
  u32 leaf_size;        // size of each leave
  u32 free_leaves_mask; // bitmask
} erpool_branch;

typedef struct erpool {
  erpool_branch** branches;
  u32             branches_count;
} erpool;

/**
 * Initialize a pool with branch sizes.
 * 
 */
int  erpool_init(const u32* branch_sizes, u32 nbranches, erpool* pool);
void erpool_free(erpool* pool);

#endif // E_REFERENCE_OBJECT_POOL_H