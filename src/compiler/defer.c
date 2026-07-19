#include "defer.h"

#include "../../inc/kit.cc.h"
#include "compile_routines.h"
#include "constants.h"

RETURNS_ERRCODE int
defer_push_scope(kit_compiler* cc)
{
  kitc_defer_scope* scope = kit_arnalloc(cc->arena, sizeof(kitc_defer_scope));
  if (!scope) return -1;

  scope->entries = kit_xalloc(init_defer_entry_capacity, sizeof(kitc_defer_entry));
  if (!scope->entries) return -1;

  scope->count    = 0;
  scope->capacity = init_defer_entry_capacity;
  scope->parent   = cc->defer_stack;
  cc->defer_stack = scope;
  return 0;
}

void
defer_pop_scope(kit_compiler* cc)
{
  kitc_defer_scope* scope = cc->defer_stack;
  cc->defer_stack         = scope->parent;
  free(scope->entries);
}

// LIFO
RETURNS_ERRCODE int
defer_emit_current_scope(kit_compiler* cc)
{
  kitc_defer_scope* scope = cc->defer_stack;
  if (!scope) return 0;

  for (i64 i = (i64)scope->count - 1; i >= 0; i--) {
    u32        nexprs = scope->entries[i].nexprs;
    const int* exprs  = scope->entries[i].exprs;
    for (u32 j = 0; j < nexprs; j++) {
      int e = compile(cc, exprs[j]);
      if (e < 0) return e;
    }
  }
  return 0;
}

/**
 * Emit the deferred statements for all scopes up to now.
 */
RETURNS_ERRCODE int
defer_emit_all_scopes(kit_compiler* cc)
{
  kitc_defer_scope* scope = cc->defer_stack;
  while (scope) {
    for (i64 i = (i64)scope->count - 1; i >= 0; i--) {
      u32        nexprs = scope->entries[i].nexprs;
      const int* exprs  = scope->entries[i].exprs;
      for (u32 j = 0; j < nexprs; j++) {
        int e = compile(cc, exprs[j]);
        if (e < 0) {
          cerror(KIT_GET_NODE(cc->ast, exprs[j])->common.span, "Failed to compile deferred statement [defer]\n");
          return e;
        }
      }
    }
    scope = scope->parent;
  }
  return 0;
}

u32
defer_get_current_depth(kit_compiler* cc)
{
  u32               d     = 0;
  kitc_defer_scope* scope = cc->defer_stack;
  while (scope) {
    d++;
    scope = scope->parent;
  }
  return d;
}

// flush all defers up to (but not including) depth
RETURNS_ERRCODE int
defer_emit_to_depth(kit_compiler* cc, u32 target_depth)
{
  kitc_defer_scope* scope = cc->defer_stack;
  u32               depth = defer_get_current_depth(cc);
  while (scope && depth > target_depth) {
    for (i64 i = (i64)scope->count - 1; i >= 0; i--) {
      for (u32 j = 0; j < scope->entries[i].nexprs; j++) {
        int e = compile(cc, scope->entries[i].exprs[j]);
        if (e < 0) return e;
      }
    }
    scope = scope->parent;
    depth--;
  }
  return 0;
}