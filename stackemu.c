#include "stackemu.h"

#include "cc.h"

#include <stdlib.h>
#include <string.h>

int
e_stackemu_init(e_stackemu* emu)
{
  const u32 new_capacity = 32;

  memset(emu, 0, sizeof *emu);

  u32* new_frames = (u32*)calloc(new_capacity, sizeof(u32));
  if (!new_frames) return -1;

  struct ecc_variable_information* new_vars = calloc(new_capacity, sizeof(ecc_variable_information));
  if (!new_vars) {
    free(new_frames);
    return -1;
  }

  emu->frames = new_frames;
  emu->vars   = new_vars;

  emu->vars_count    = 0;
  emu->vars_capacity = new_capacity;

  emu->strucs_count    = 0;
  emu->strucs_capacity = 0;

  emu->frame_top      = 0;
  emu->frame_capacity = new_capacity;

  return 0;
}

void
e_stackemu_free(e_stackemu* emu)
{
  free(emu->vars);
  free(emu->frames);
  memset(emu, 0, sizeof *emu);
}

void
e_stackemu_push(e_stackemu* emu)
{
  if (emu->frame_top >= emu->frame_capacity) {
    u32  new_capacity = emu->frame_capacity * 2;
    u32* new_frames   = (u32*)realloc(emu->frames, sizeof(u32) * new_capacity);
    if (!new_frames) return;

    emu->frame_capacity = new_capacity;
    emu->frames         = new_frames;
  }

  emu->frames[emu->frame_top - 1]++;
}

void
e_stackemu_pop(e_stackemu* emu)
{ emu->frames[emu->frame_top - 1]--; }

void
e_stackemu_push_frame(e_stackemu* emu)
{ emu->frame_top++; }

void
e_stackemu_pop_frame(e_stackemu* emu)
{
  while (emu->vars_count > 0 && emu->vars[emu->vars_count - 1].stack_depth >= emu->frame_top) { emu->vars_count--; }
  emu->frame_top--;
}

u32
e_stackemu_top(const e_stackemu* emu)
{ return emu->frames[emu->frame_top - 1]; }

u32
e_stackemu_fp(const e_stackemu* emu)
{ return emu->frame_top; }

int
e_stackemu_push_var(e_stackemu* emu, const ecc_variable_information* info)
{
  if (emu->vars_count >= emu->vars_capacity) {
    u32                              new_capacity = emu->vars_count * 2;
    struct ecc_variable_information* new_vars     = realloc(emu->vars, sizeof(ecc_variable_information) * new_capacity);
    if (!new_vars) return -1;

    emu->vars_capacity = new_capacity;
    emu->vars          = new_vars;
  }

  memcpy(&emu->vars[emu->vars_count++], info, sizeof(*info));
  return 0;
}

struct ecc_variable_information*
e_stackemu_find_var(const e_stackemu* emu, u32 id)
{
  for (u32 i = 0; i < emu->vars_count; i++) {
    if (emu->vars[i].name_hash == id) { return &emu->vars[i]; }
  }
  return NULL;
}