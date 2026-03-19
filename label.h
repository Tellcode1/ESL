#ifndef E_LABEL_PASS_H
#define E_LABEL_PASS_H

#include "bc.h"
#include "rwhelp.h"

static inline u32
e_instruction_operand_size(e_attr attrs, e_opcode op)
{
  switch (op) {
    case E_OPCODE_RETURN: return 1;  // has_return_value
    case E_OPCODE_LITERAL: return 2; // u16 index
    case E_OPCODE_INC:
    case E_OPCODE_DEC:
      if ((attrs & E_ATTR_COMPOUND) != 0) return 4; // variable ID
      return 0;
    case E_OPCODE_LOAD:               // variable ID
    case E_OPCODE_ASSIGN:             // variable ID
    case E_OPCODE_INIT: return 4;     // u32 id
    case E_OPCODE_CALL: return 4 + 2; // u32 id + u16 nargs
    case E_OPCODE_HALT:               // exit code
    case E_OPCODE_JMP:
    case E_OPCODE_JZ:
    case E_OPCODE_JE:
    case E_OPCODE_JNE:
    case E_OPCODE_JNZ:             // u32 target
    case E_OPCODE_LABEL: return 4; // u32 label id
    default: return 0;
  }
}

static inline void
_e_patch_jumps(u8* stream, u32 stream_size, u32 label_offset, u32 label_id)
{
  u8* const end = stream + stream_size;
  u8*       ip  = stream;
  while (ip < end) {
    e_opcode opcode = e_read_u8((const u8**)&ip);
    e_attr   attrs  = e_read_u8((const u8**)&ip);
    if ((opcode == E_OPCODE_JMP || opcode == E_OPCODE_JZ || opcode == E_OPCODE_JNZ || opcode == E_OPCODE_JE || opcode == E_OPCODE_JNE) && *(u32*)ip == label_id) {
      // printf("JMP for label %u patched to %u\n", label_id, label_offset);
      *(u32*)ip = label_offset;
      ip += 4; // CHANGE THIS WHEN YOU CHANGE LABEL SIZE!
    } else {
      ip += e_instruction_operand_size(attrs, opcode);
    }
  }
}

/**
 * Parse all labels and replace them with actual indices.
 * It isn't expected that the bytecode stream will have instructions removed or added
 * after this call EXCEPT e_remove_noops, which has explicit handling of that.
 */
static inline void
e_resolve_labels(u8* stream, u32 stream_size)
{
  u8* const end = stream + stream_size;
  u8*       ip  = stream;
  while (ip < end) {
    e_opcode opcode = e_read_u8((const u8**)&ip);
    e_attr   attrs  = e_read_u8((const u8**)&ip);

    if (opcode == E_OPCODE_LABEL) {
      u32 id     = e_read_u32((const u8**)&ip);
      u32 offset = ip - stream;
      _e_patch_jumps(stream, stream_size, offset, id);
    } else {
      ip += e_instruction_operand_size(attrs, opcode);
    }
  }
}

#endif // E_LABEL_PASS_H