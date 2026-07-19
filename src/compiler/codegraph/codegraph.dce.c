#include "codegraph.h"

#include "../../../inc/kit.cc.h"

bool
codegraph_eliminate_unreachable_code(kit_compiler* cc, codegraph* cfg)
{
  bool changed = false;

  bool* reachable = kit_arnalloc(cfg->arena, cfg->nblocks * sizeof(bool));
  memset(reachable, 0, cfg->nblocks * sizeof(bool));
  reachable[0] = true;

  /* Do a DFS to find dead blocks. */
  u32* stack  = kit_arnalloc(cfg->arena, cfg->nblocks * sizeof(u32));
  u32  sp     = 0;
  stack[sp++] = 0;
  while (sp > 0) {
    u32 b = stack[--sp];
    for (u32 s = 0; s < cfg->blocks[b].nsuccessors; s++) {
      u32 successor = cfg->blocks[b].successors[s];
      if (!reachable[successor]) {
        reachable[successor] = true;
        stack[sp++]          = successor;
      }
    }
  }

  /* unreachable block. nop it out. */
  for (u32 i = 0; i < cfg->nblocks; i++) {
    if (reachable[i]) continue;

    codeblock* blk = &cfg->blocks[i];
    for (u32 ip = blk->start; ip <= blk->end; ip++) {
      // dont kill labels!!
      if (cc->instructions[ip].opcode != KIT_IR_OPCODE_LABEL) {
        changed                     = true;
        cc->instructions[ip].opcode = KIT_IR_OPCODE_NOP;
      }
    }
  }

  return changed;
  // kit_arnfree(cfg->arena, reachable);
}
