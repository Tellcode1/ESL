#include "validate.h"

#include "bc.h"
#include "bvar.h"
#include "exec.h"
#include "rwhelp.h"
#include "stackemu.h"

int
e_validate(const struct e_exec_info* info, FILE* f)
{
  e_stackemu emu = { 0 };
  int        e   = 0;

  e = e_stackemu_init(&emu);
  if (e) return e;

  const u8* ip  = info->code;
  const u8* end = info->code + info->code_size;

  for (u32 i = 0; i < info->nargs; i++) {
    ecc_variable_information var = { .name_hash = info->arg_slots[i], .stack_depth = e_stackemu_fp(&emu) };
    e_stackemu_push_var(&emu, &var);
  }

  // u32 stack_top = 0;

  while (ip < end) {
    e_ins ins = e_read_ins(&ip);

    if (ins.opcode == E_OPCODE_INIT) {
      ecc_variable_information var = { .name_hash = ins.v.init, .stack_depth = e_stackemu_fp(&emu) };
      e_stackemu_push_var(&emu, &var);
    }

    else if (ins.opcode == E_OPCODE_LOAD) {
      const u32 id = ins.v.load;

      bool found = false;
      for (u32 i = 0; i < info->nextern_vars; i++) {
        const char* name = info->extern_vars[i].name;
        if (e_hash_fnv(name, strlen(name)) == id) {
          found = true;
          break;
        }
      }

      for (u32 i = 0; i < E_ARRLEN(eb_vars); i++) {
        const char* name = eb_vars[i].name;
        if (e_hash_fnv(name, strlen(name)) == id) {
          found = true;
          break;
        }
      }

      if (e_stackemu_find_var(&emu, id) != NULL) { found = true; }

      if (!found) {
        fprintf(f, "Variable %u LOAD'd but undeclared\n", id);
        e = -1;
      }

      // stack_top++;
    }

    else if (ins.opcode == E_OPCODE_LITERAL) {
      const u32 id = ins.v.literal;

      bool found = false;
      for (u32 i = 0; i < info->nliterals; i++) {
        if (info->literals_hashes[i] == id) {
          found = true;
          break;
        }
      }

      if (!found) {
        fprintf(f, "Out of bound literal access (%u)\n", id);
        e = -1;
      }
    }

    else if (ins.opcode == E_OPCODE_ASSIGN) {
      const u32 id = ins.v.assign;

      bool found = false;
      if (e_stackemu_find_var(&emu, id) != NULL) { found = true; }

      if (!found) {
        fprintf(f, "ASSIGN target %u undeclared\n", id);
        e = -1;
      }

      // stack_top--;
    }

    else if (ins.opcode == E_OPCODE_PUSH_FRAME) {
      e_stackemu_push_frame(&emu);
    } else if (ins.opcode == E_OPCODE_POP_FRAME) {
      e_stackemu_pop_frame(&emu);
    }
  }

  e_stackemu_free(&emu);

  return e;
}