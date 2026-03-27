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

#ifndef E_DECOMPILER_H
#define E_DECOMPILER_H

#include "bc.h"
#include "rwhelp.h"
#include "stdafx.h"

#include <stdio.h>
#include <string.h>

static inline void
e_attrs_to_string(e_attr a, char* buffer, size_t buffer_size)
{
  if (a == 0) {
    strlcpy(buffer, "E_ATTR_NONE", buffer_size);
    return;
  }
  if ((a & E_ATTR_COMPOUND) != 0) strlcat(buffer, "Compound|", buffer_size);
  if ((a & E_ATTR_MEMBER_ACCESS) != 0) strlcat(buffer, "MemberAccess|", buffer_size);
  if ((a & E_ATTR_INLINE) != 0) strlcat(buffer, "Inline|", buffer_size);
  if ((a & E_ATTR_VARIABLE) != 0) strlcat(buffer, "Variable|", buffer_size);
  if ((a & E_ATTR_MULTIPLE) != 0) strlcat(buffer, "Multiple|", buffer_size);
}

static inline void
e_print_instruction(e_opcode o, e_attr a, const u8** ip)
{
  if (a != E_ATTR_NONE) {
    char buffer[512] = { 0 };
    e_attrs_to_string(a, buffer, sizeof(buffer));

    char* last_pipe = strrchr(buffer, '|');
    if (last_pipe != nullptr) *last_pipe = 0;

    printf("[%s] ", buffer);
  }

  switch ((e_opcode_bck)o) {
    case E_OPCODE_NOOP: printf("noop\n"); break;
    case E_OPCODE_ADD: printf("add\n"); break;
    case E_OPCODE_SUB: printf("sub\n"); break;
    case E_OPCODE_MUL: printf("mul\n"); break;
    case E_OPCODE_DIV: printf("div\n"); break;
    case E_OPCODE_MOD: printf("mod\n"); break;
    case E_OPCODE_EXP: printf("exp\n"); break;
    case E_OPCODE_AND: printf("and\n"); break;
    case E_OPCODE_OR: printf("or\n"); break;
    case E_OPCODE_XOR: printf("xor\n"); break;
    case E_OPCODE_NOT: printf("not\n"); break;
    case E_OPCODE_BAND: printf("band\n"); break;
    case E_OPCODE_BOR: printf("bor\n"); break;
    case E_OPCODE_BNOT: printf("bnot\n"); break;
    case E_OPCODE_EQL: printf("eql\n"); break;
    case E_OPCODE_NEQ: printf("neq\n"); break;
    case E_OPCODE_LT: printf("lt\n"); break;
    case E_OPCODE_LTE: printf("lte\n"); break;
    case E_OPCODE_GT: printf("gt\n"); break;
    case E_OPCODE_GTE: printf("gte\n"); break;
    case E_OPCODE_NEG: printf("neg\n"); break;

    case E_OPCODE_INC: {
      u32 to = e_read_u32((const u8**)&ip);
      printf("inc [%u]\n", to);
      break;
    }
    case E_OPCODE_DEC: {
      u32 to = e_read_u32((const u8**)&ip);
      printf("dec [%u]\n", to);
      break;
    }
    case E_OPCODE_CALL: {
      u16 nargs = e_read_u16(ip);
      u32 id    = e_read_u32(ip);
      printf("call [%u] [%u]\n", id, nargs);
      break;
    }
    case E_OPCODE_RETURN: {
      u8 has_return_value = e_read_u8(ip);
      printf("ret [%s]\n", (bool)has_return_value ? "true" : "false");
      break;
    }
    case E_OPCODE_LITERAL: printf("literal [%u]\n", e_read_u16(ip)); break;
    case E_OPCODE_LOAD: printf("load %u\n", e_read_u32(ip)); break;
    // case E_OPCODE_LOAD_REFERENCE: printf("load_reference\n"); break;
    case E_OPCODE_ASSIGN: printf("store %u\n", e_read_u32(ip)); break;
    case E_OPCODE_INIT: printf("init %u\n", e_read_u32(ip)); break;
    case E_OPCODE_LABEL: printf("label %u\n", e_read_u32(ip)); break;
    case E_OPCODE_JMP: printf("jmp [%u]\n", e_read_u32(ip)); break;
    case E_OPCODE_JE: printf("je [%u]\n", e_read_u32(ip)); break;
    case E_OPCODE_JNE: printf("jne [%u]\n", e_read_u32(ip)); break;
    case E_OPCODE_JZ: printf("jz [%u]\n", e_read_u32(ip)); break;
    case E_OPCODE_JNZ: printf("jnz [%u]\n", e_read_u32(ip)); break;
    case E_OPCODE_PUSH_VARIABLES: printf("save vars\n"); break;
    case E_OPCODE_POP_VARIABLES: printf("restore vars\n"); break;
    case E_OPCODE_HALT: printf("halt [%u]\n", e_read_u32(ip)); break;
    case E_OPCODE_COUNT: printf("count\n"); break;
    case E_OPCODE_MK_LIST: {
      u32 nelems = e_read_u32(ip);
      printf("mklist [%u]\n", nelems);
      break;
    }
    case E_OPCODE_MK_MAP: {
      u32 nelems = e_read_u32(ip);
      printf("mkmap [%u]\n", nelems);
      break;
    }
    case E_OPCODE_INDEX: printf("index\n"); break;
    case E_OPCODE_INDEX_ASSIGN: printf("idx_assign\n"); break;
    case E_OPCODE_MEMBER_ACCESS: printf("member_access\n"); break;
  }
}

static inline void
e_print_instruction_stream(const u8* stm, u32 stm_size, int indent)
{
  const u8* ip  = stm;
  const u8* end = stm + stm_size;
  while (ip < end) {
    u32 instruction_offset = ip - stm;

    e_opcode e = *(e_opcode*)ip;
    ip += sizeof(e_opcode);

    e_attr attrs = *(e_attr*)ip;
    ip += sizeof(e_attr);

    for (int i = 0; i < indent; i++) fputc(' ', stdout);

    printf("%u: ", instruction_offset); // Print offset of instruction
    e_print_instruction(e, attrs, &ip);
  }
}

#endif // E_DECOMPILER_H