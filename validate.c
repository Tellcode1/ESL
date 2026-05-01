#include "validate.h"

#include "bc.h"
#include "bvar.h"
#include "exec.h"
#include "fn.h"
#include "rwhelp.h"
#include "stackemu.h"

static inline const char*
find_name(u32 id, const e_exec_info* info)
{
  for (u32 i = 0; i < info->nnames; i++) {
    if (info->names_hashes[i] == id) { return info->names[i]; }
  }
  return "(Unknown)";
}

static int
validate_stream(
    const u8*          code,
    u32                code_size,
    const u32*         arg_slots,
    u32                nargs,
    const e_stackemu*  global_namespace,
    e_stackemu*        emu,
    const e_exec_info* info,
    FILE*              f)
{
  int e = 0;

  const u8* ip  = code;
  const u8* end = code + code_size;

  for (u32 i = 0; i < nargs; i++) {
    ecc_variable_information var = { .name_hash = arg_slots[i], .stack_depth = e_stackemu_fp(emu) };
    e_stackemu_push_var(emu, &var);
  }

  // u32 stack_top = 0;

  while (ip < end) {
    e_ins ins = e_read_ins(&ip);

    if (ins.opcode == E_OPCODE_INIT) {
      ecc_variable_information var = { .name_hash = ins.v.init, .stack_depth = e_stackemu_fp(emu) };
      e_stackemu_push_var(emu, &var);
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

      if (e_stackemu_find_var(emu, id) != NULL) { found = true; }
      if (!found && e_stackemu_find_var(global_namespace, id) != NULL) { found = true; }

      if (!found) {
        fprintf(f, "Variable %s LOAD'd but undeclared\n", find_name(id, info));
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
      if (e_stackemu_find_var(emu, id) != NULL) { found = true; }
      if (!found && e_stackemu_find_var(global_namespace, id) != NULL) { found = true; }

      if (!found) {
        fprintf(f, "ASSIGN target %s undeclared\n", find_name(id, info));
        e = -1;
      }

      // stack_top--;
    }

    else if (ins.opcode == E_OPCODE_PUSH_FRAME) {
      e_stackemu_push_frame(emu);
    } else if (ins.opcode == E_OPCODE_POP_FRAME) {
      e_stackemu_pop_frame(emu);
    }
  }

  return e;
}

int
e_validate(const struct e_exec_info* info, FILE* f)
{
  e_stackemu global_namespace = { 0 };

  int e = e_stackemu_init(&global_namespace);
  if (e) return e;

  e = validate_stream(info->code, info->code_size, NULL, 0, &global_namespace, &global_namespace, info, f);
  if (e) goto RET;

  for (u32 i = 0; i < info->nfuncs; i++) {
    e_stackemu emu = { 0 };

    e = e_stackemu_init(&emu);
    if (e) return e;

    const e_function* fn = &info->funcs[i];

    e = validate_stream(fn->code, fn->code_size, fn->arg_slots, fn->nargs, &global_namespace, &emu, info, f);
    e_stackemu_free(&emu);

    if (e) { goto RET; }
  }

RET:
  e_stackemu_free(&global_namespace);
  return e;
}