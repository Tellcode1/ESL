#ifndef KIT_COMPILER_VREG_H
#define KIT_COMPILER_VREG_H

#include "../../inc/kit.cc.h"

static inline kit_vreg_t
vreg_alloc(kit_compiler* cc)
{ return cc->next_vreg++; }

static inline u32
make_label_id(kit_compiler* cc)
{ return cc->next_label++; }

#endif // KIT_COMPILER_VREG_H