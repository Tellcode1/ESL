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
e_print_instruction(e_opcode o, const u8** ip)
{
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
    case E_OPCODE_BAND: printf("band\n"); break;
    case E_OPCODE_BOR: printf("bor\n"); break;
    case E_OPCODE_EQL: printf("eql\n"); break;
    case E_OPCODE_NEQ: printf("neq\n"); break;
    case E_OPCODE_LT: printf("lt\n"); break;
    case E_OPCODE_LTE: printf("lte\n"); break;
    case E_OPCODE_GT: printf("gt\n"); break;
    case E_OPCODE_GTE: printf("gte\n"); break;

    case E_OPCODE_NOT: printf("not %s\n", (bool)e_read_u8(ip) ? "true" : "false"); break;
    case E_OPCODE_BNOT: printf("bnot %s\n", (bool)e_read_u8(ip) ? "true" : "false"); break;
    case E_OPCODE_NEG: printf("neg %s\n", (bool)e_read_u8(ip) ? "true" : "false"); break;

    case E_OPCODE_INC: {
      (void)e_read_u8(ip); // always compound!
      printf("inc\n");
      break;
    }
    case E_OPCODE_DEC: {
      (void)e_read_u8(ip); // always compound!
      printf("dec\n");
      break;
    }
    case E_OPCODE_CALL: {
      u32 id    = e_read_u32(ip);
      u16 nargs = e_read_u16(ip);
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
    case E_OPCODE_INIT: printf("init %u %s\n", e_read_u32(ip), (bool)e_read_u8(ip) ? "true" : "false"); break;
    case E_OPCODE_LABEL: printf("label %u\n", e_read_u32(ip)); break;
    case E_OPCODE_JMP: printf("jmp [%u]\n", e_read_u32(ip)); break;
    case E_OPCODE_JE: printf("je [%u]\n", e_read_u32(ip)); break;
    case E_OPCODE_JNE: printf("jne [%u]\n", e_read_u32(ip)); break;
    case E_OPCODE_JZ: printf("jz [%u]\n", e_read_u32(ip)); break;
    case E_OPCODE_JNZ: printf("jnz [%u]\n", e_read_u32(ip)); break;
    case E_OPCODE_PUSH_VARIABLES: printf("save vars\n"); break;
    case E_OPCODE_POP_VARIABLES: printf("restore vars\n"); break;
    case E_OPCODE_HALT: printf("halt [%u]\n", e_read_u32(ip)); break;
    case E_OPCODE_COUNT: printf("invalid\n"); break;
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
    case E_OPCODE_POP: printf("pop\n"); break;
    case E_OPCODE_INDEX_ASSIGN: printf("idx_assign\n"); break;
    case E_OPCODE_MEMBER_ACCESS: printf("member_access [%u]\n", e_read_u32(ip)); break;
    case E_OPCODE_MK_STRUCT: {
      u32 nmembers = e_read_u32(ip);
      printf("mk_struct [%u]\n", nmembers);
      for (u32 i = 0; i < nmembers; i++) { printf("\t[%u] = null\n", e_read_u32(ip)); }
      break;
    }
    case E_OPCODE_MEMBER_ASSIGN: {
      u32 member = e_read_u32(ip);
      printf("member_assign [%u]\n", member);
      break;
    }
    case E_OPCODE_DUP: printf("dup\n"); break;
    case E_OPCODE_INDEX_ASSIGN_VAR: printf("index_assign_to_var [%u]\n", e_read_u32(ip)); break;
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

    for (int i = 0; i < indent; i++) fputc(' ', stdout);

    printf("%u: ", instruction_offset); // Print offset of instruction
    e_print_instruction(e, &ip);
  }
}

#endif // E_DECOMPILER_H