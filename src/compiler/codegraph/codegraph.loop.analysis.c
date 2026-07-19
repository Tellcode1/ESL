#include "codegraph.h"

#include "../../../inc/kit.arena.h"
#include "../../../inc/kit.cc.h"

int
codegraph_loop_analysis(kit_compiler* cc, codegraph* cfg)
{
  typedef struct backedge {
    u32 header;
    u32 latch;
  } backedge;

  backedge* edges  = kit_arnalloc(cfg->arena, sizeof(backedge) * cfg->nblocks * 2);
  u32       nedges = 0;

  for (u32 src = 0; src < cfg->nblocks; src++) {
    codeblock* blk = &cfg->blocks[src];

    for (u32 i = 0; i < blk->nsuccessors; i++) {
      u32 dst = blk->successors[i];

      if (cfg->blocks[src].dominators[dst]) { edges[nedges++] = (backedge){ .header = dst, .latch = src }; }
    }
  }

  /* merge back edges that share a header into one loop. */
  cfg->loops  = kit_arnalloc(cfg->arena, sizeof(blockloop) * nedges);
  cfg->nloops = 0;

  for (u32 e = 0; e < nedges; e++) {
    u32 header = edges[e].header;

    blockloop* loop = NULL;
    for (u32 l = 0; l < cfg->nloops; l++) {
      if (cfg->loops[l].header == header) {
        loop = &cfg->loops[l];
        break;
      }
    }

    if (!loop) {
      loop  = &cfg->loops[cfg->nloops++];
      *loop = (blockloop){
        .header          = header,
        .latches         = kit_arnalloc(cfg->arena, sizeof(u32) * nedges),
        .nlatches        = 0,
        .is_block_member = kit_arnalloc(cfg->arena, cfg->nblocks * sizeof(bool)),
        .is_ins_member   = kit_arnalloc(cfg->arena, cc->ninstructions * sizeof(bool)),
      };
      memset(loop->is_block_member, 0, cfg->nblocks * sizeof(bool));
      memset(loop->is_ins_member, 0, cc->ninstructions * sizeof(bool));
      loop->is_block_member[header] = true;
    }

    loop->latches[loop->nlatches++]       = edges[e].latch;
    loop->is_block_member[edges[e].latch] = true;
  }

  for (u32 l = 0; l < cfg->nloops; l++) {
    blockloop* loop = &cfg->loops[l];

    u32  capacity = 1024;
    u32  top      = 0;
    u32* stack    = kit_arnalloc(cfg->arena, capacity * sizeof(u32));

    for (u32 i = 0; i < loop->nlatches; i++) { stack[top++] = loop->latches[i]; }

    while (top) {
      u32        b   = stack[--top];
      codeblock* blk = &cfg->blocks[b];

      for (u32 i = 0; i < blk->npredecessors; i++) {
        u32 pred = blk->predecessors[i];

        if (pred == loop->header) continue;

        if (!loop->is_block_member[pred]) {
          if (top >= capacity) {
            stack = kit_arnrealloc(cfg->arena, stack, capacity * sizeof(u32), capacity * 2UL * sizeof(u32));
            capacity *= 2;
          }
          loop->is_block_member[pred] = true;
          stack[top++]                = pred;
        }
      }
    }
  }

  for (u32 l = 0; l < cfg->nloops; l++) {
    blockloop* loop = &cfg->loops[l];

    for (u32 b = 0; b < cfg->nblocks; b++) {
      if (!loop->is_block_member[b]) continue;

      codeblock* blk = &cfg->blocks[b];

      for (u32 ip = blk->start; ip <= blk->end; ip++) { loop->is_ins_member[ip] = true; }
    }
  }

  return 0;
}