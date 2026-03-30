/**
 * MIT License
 *
 * Copyright (c) 2026 Tellcode1
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef E_LABEL_PASS_H
#define E_LABEL_PASS_H

#include "bc.h"
#include "rwhelp.h"

static inline u32
e_instruction_operand_size(e_opcode op)
{
  switch (op) {
    case E_OPCODE_RETURN: return 1;
    case E_OPCODE_LITERAL: return 2; // u16

    case E_OPCODE_INC:
    case E_OPCODE_DEC: return 1;

    case E_OPCODE_INIT: return 4 + 1;

    case E_OPCODE_LOAD:
    case E_OPCODE_ASSIGN:
    case E_OPCODE_MEMBER_ACCESS:
    case E_OPCODE_MK_LIST:
    case E_OPCODE_MK_MAP: return 4;

    case E_OPCODE_CALL: return 2 + 4;

    case E_OPCODE_HALT:
    case E_OPCODE_JMP:
    case E_OPCODE_JZ:
    case E_OPCODE_JE:
    case E_OPCODE_JNE:
    case E_OPCODE_JNZ:
    case E_OPCODE_LABEL: return 4;

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

    // clang-format off
    if (
      opcode == E_OPCODE_JMP ||
      opcode == E_OPCODE_JZ  ||
      opcode == E_OPCODE_JNZ ||
      opcode == E_OPCODE_JE  ||
      opcode == E_OPCODE_JNE
    )
    // clang-format on
    {
      u32* target = (u32*)ip;

      if (*target == label_id) { *target = label_offset; }

      ip += 4;
    } else {
      ip += e_instruction_operand_size(opcode);
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

    if (opcode == E_OPCODE_LABEL) {
      u32 id     = e_read_u32((const u8**)&ip);
      u32 offset = ip - stream;
      _e_patch_jumps(stream, stream_size, offset, id);
    } else {
      ip += e_instruction_operand_size(opcode);
    }
  }
}

#endif // E_LABEL_PASS_H