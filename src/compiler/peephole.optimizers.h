#ifndef KIT_COMPILER_PEEPHOLE_OPTIMIZERS_H
#define KIT_COMPILER_PEEPHOLE_OPTIMIZERS_H

#include "../../inc/kit.cc.h"
#include "../../inc/kit.stdafx.h"

int remove_jmp_where_it_would_fallthrough(kit_compiler* cc);
int strip_noops(kit_compiler* cc);

#endif // KIT_COMPILER_PEEPHOLE_OPTIMIZERS_H