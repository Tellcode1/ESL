#ifndef KIT_COMPILER_TREE_H
#define KIT_COMPILER_TREE_H

#include "../../inc/kit.stdafx.h"

struct kit_compiler;

RETURNS_ERRCODE int compiler_make_fork(const struct kit_compiler* old_c, struct kit_compiler* new_c);
void                compiler_join_fork(struct kit_compiler* copy, struct kit_compiler* cc);
void                compiler_free_fork_entirely(kit_compiler* cc);

#endif // KIT_COMPILER_TREE_H