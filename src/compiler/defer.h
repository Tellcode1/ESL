#ifndef KIT_COMPILER_DEFER_H
#define KIT_COMPILER_DEFER_H

#include "../../inc/kit.ast.h"
#include "../../inc/kit.ir.h"
#include "../../inc/kit.stdafx.h"

struct kit_compiler;

RETURNS_ERRCODE int defer_push_scope(struct kit_compiler* cc);
void                defer_pop_scope(struct kit_compiler* cc);
RETURNS_ERRCODE int defer_emit_current_scope(struct kit_compiler* cc);
RETURNS_ERRCODE int defer_emit_all_scopes(struct kit_compiler* cc);
RETURNS_ERRCODE int defer_emit_to_depth(struct kit_compiler* cc, u32 target_depth);
u32                 defer_get_current_depth(struct kit_compiler* cc);

#endif // KIT_COMPILER_DEFER_H