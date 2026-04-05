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

#ifndef E_BYTECODE_STREAM_READ_WRITE_HELP_H
#define E_BYTECODE_STREAM_READ_WRITE_HELP_H

#include "bc.h"
#include "cc.h"
#include "fn.h"
#include "pool.h"
#include "refcount.h"
#include "stdafx.h"
#include "var.h"

#include <stdio.h>

#define E_FILE_MAGIC 0xF5F6F7F8

// See BYTECODE FORMAT section of README.

// clang-format off
#define E_GEN_READ_FUNCTION(type) static inline type e_read_##type(const u8** _ip) { type v; memcpy(&v, *_ip, sizeof(type)); *(_ip) += sizeof(type); return v; }
        E_GEN_READ_FUNCTION(u32)
        E_GEN_READ_FUNCTION(u16)
        E_GEN_READ_FUNCTION(u8)

#define E_GEN_EMIT_FN(type) static inline void e_emit_##type(e_compiler* cc, type value) { if (cc->num_bytes_emitted + sizeof(type) > cc->emit_capacity) ecc_stream_resize(cc, MAX(cc->num_bytes_emitted + sizeof(type), cc->emit_capacity * 2)); memcpy((type*)((uchar*)cc->emit + cc->num_bytes_emitted), &value, sizeof(value)); cc->num_bytes_emitted += sizeof(type); }
        E_GEN_EMIT_FN(u32)
        E_GEN_EMIT_FN(u64)
        E_GEN_EMIT_FN(u16)
        E_GEN_EMIT_FN(u8)

// clang-format on

static inline void
e_emit_instruction(e_compiler* cc, e_opcode opcode)
{
  // u32 align = cc->info->min_instruction_alignment;

  // if (align != 0) {
  //   while (((uintptr_t)(cc->emit + cc->emitted) % align) != 0) { e_emit_u8(cc, E_OPCODE_NOOP); }
  // }

  e_emit_u8(cc, opcode);
}

static inline void
e_emit_label(e_compiler* cc, u32 labelid)
{
  e_emit_u8(cc, E_OPCODE_LABEL);
  e_emit_u32(cc, labelid);
}

static inline int
e_file_load(FILE* f, void** root_allocation, u32* ninstructions, u8** instructions, u32* nlits, e_var** lits, u32* nfunctions, e_function** functions)
{
  u32 magic = 0;
  fread(&magic, sizeof(magic), 1, f);

  if (magic != E_FILE_MAGIC) return -1;

  u32 bytes_req = 0;
  fread(&bytes_req, sizeof(bytes_req), 1, f);

  *root_allocation = malloc(bytes_req);
  if (*root_allocation == nullptr) return -2;

  uchar* alloc = (uchar*)*root_allocation;

  fread(nlits, sizeof(*nlits), 1, f);

  e_var* alits = (e_var*)(alloc);
  alloc += sizeof(e_var) * (*nlits);
  *lits = alits;
  if (*lits == nullptr) return -3;

  for (u32 i = 0; i < *nlits; i++) {
    fread(&alits[i].type, sizeof(e_vartype), 1, f);

    e_var* lit = &alits[i];
    switch (lit->type) {
      case E_VARTYPE_STRING: {
        u32 len = 0;
        fread(&len, sizeof(len), 1, f);

        lit->val.s = (e_refdobj*)(alloc);
        alloc += sizeof(e_refdobj);

        /* Initialize ref counter to 1. Not used for literals but VM expects it */
        lit->val.s->refc.ctr = 1;

        E_VAR_AS_STRING(lit)->s = (char*)(alloc);
        alloc += len + 1;

        fread(E_VAR_AS_STRING(lit)->s, sizeof(char), len, f);

        E_VAR_AS_STRING(lit)->s[len] = 0;
        break;
      }

      case E_VARTYPE_NULL: *lit = (e_var){ .type = E_VARTYPE_NULL }; break;
      case E_VARTYPE_VOID:
        *lit = (e_var){ .type = E_VARTYPE_NULL };
        break;
        break;
      case E_VARTYPE_INT: fread(&lit->val.i, sizeof(lit->val.i), 1, f); break;
      case E_VARTYPE_BOOL: fread(&lit->val.b, sizeof(lit->val.b), 1, f); break;
      case E_VARTYPE_CHAR: fread(&lit->val.c, sizeof(lit->val.c), 1, f); break;
      case E_VARTYPE_FLOAT: fread(&lit->val.f, sizeof(lit->val.f), 1, f); break;
      case E_VARTYPE_STRUCT: break;
      case E_VARTYPE_VEC2: fread(&lit->val.vec2, sizeof(lit->val.vec2), 1, f); break;
      case E_VARTYPE_VEC3: fread(&lit->val.vec3, sizeof(lit->val.vec3), 1, f); break;
      case E_VARTYPE_VEC4: fread(&lit->val.vec4, sizeof(lit->val.vec4), 1, f); break;
      default: break;
    }
  }

  fread(nfunctions, sizeof(*nfunctions), 1, f);

  *functions = (e_function*)(alloc);
  alloc += sizeof(e_function) * (*nfunctions);

  for (u32 i = 0; i < *nfunctions; i++) {
    e_function func;
    fread(&func.code_size, sizeof(func.code_size), 1, f);
    fread(&func.nargs, sizeof(func.nargs), 1, f);
    fread(&func.name_hash, sizeof(func.name_hash), 1, f);

    alloc = (uchar*)(((uintptr_t)alloc + 3) & ~3);

    func.arg_slots = (u32*)(alloc);
    alloc += sizeof(u32) * func.nargs;

    if (func.arg_slots == nullptr) return -4;

    fread(func.arg_slots, sizeof(*func.arg_slots), func.nargs, f);

    func.code = (u8*)(alloc);
    alloc += func.code_size;
    if (func.code == nullptr) return -5;

    fread(func.code, 1, func.code_size, f);
    (*functions)[i] = func;
  }

  fread(ninstructions, sizeof(*ninstructions), 1, f);

  *instructions = (u8*)alloc;
  fread(*instructions, sizeof(u8), *ninstructions, f);

  return 0;
}

static inline u32
e_file_bytes_required(const e_compilation_result* r)
{
  u32 size = 1024;

  // literals array
  size += sizeof(e_var) * r->nliterals;

  for (u32 i = 0; i < r->nliterals; i++) {
    const e_var* lit = &r->literals[i];

    if (lit->type == E_VARTYPE_STRING) {
      u32 len = strlen(E_VAR_AS_STRING(lit)->s);

      size += sizeof(e_refdobj);
      size += len + 1; // nnull
    }
  }

  // functions array
  size += sizeof(e_function) * r->nfunctions;

  for (u32 i = 0; i < r->nfunctions; i++) {
    const e_function* fn = &r->functions[i];

    size = (size + 3) & ~3;
    size += sizeof(u32) * fn->nargs; // arg_slots

    size += fn->code_size; // code
  }

  size += r->ninstructions;

  return size;
}

static inline void
e_file_write(const e_compilation_result* r, FILE* f)
{
  u32 magic = E_FILE_MAGIC;
  fwrite(&magic, sizeof(magic), 1, f);

  u32 bytes_req = e_file_bytes_required(r);
  fwrite(&bytes_req, sizeof(bytes_req), 1, f);

  fwrite(&r->nliterals, sizeof(r->nliterals), 1, f);
  for (u32 i = 0; i < r->nliterals; i++) {
    const e_var* lit = &r->literals[i];
    fwrite(&lit->type, sizeof(lit->type), 1, f);

    if (lit->type == E_VARTYPE_STRING) {
      u32 len = strlen(E_VAR_AS_STRING(lit)->s);
      fwrite(&len, sizeof(len), 1, f);
      fwrite(E_VAR_AS_STRING(lit)->s, sizeof(char), len, f);
    } else {
      switch (lit->type) {
        case E_VARTYPE_INT: fwrite(&lit->val.i, sizeof(lit->val.i), 1, f); break;
        case E_VARTYPE_BOOL: fwrite(&lit->val.b, sizeof(lit->val.b), 1, f); break;
        case E_VARTYPE_CHAR: fwrite(&lit->val.c, sizeof(lit->val.c), 1, f); break;
        case E_VARTYPE_FLOAT: fwrite(&lit->val.f, sizeof(lit->val.f), 1, f); break;
        case E_VARTYPE_VEC2: fwrite(&lit->val.vec2, sizeof(lit->val.vec2), 1, f); break;
        case E_VARTYPE_VEC3: fwrite(&lit->val.vec3, sizeof(lit->val.vec3), 1, f); break;
        case E_VARTYPE_VEC4: fwrite(&lit->val.vec4, sizeof(lit->val.vec4), 1, f); break;
        default: break;
      }
    }
  }
  fwrite(&r->nfunctions, sizeof(r->nfunctions), 1, f);
  for (u32 i = 0; i < r->nfunctions; i++) {
    const e_function* fn = &r->functions[i];
    if (fn->nargs > 100) {
      printf("Function %u has %u arguments\n", fn->name_hash, fn->nargs);
      abort();
    }
    fwrite(&fn->code_size, sizeof(fn->code_size), 1, f);
    fwrite(&fn->nargs, sizeof(fn->nargs), 1, f);
    fwrite(&fn->name_hash, sizeof(fn->name_hash), 1, f);
    fwrite(fn->arg_slots, sizeof(*fn->arg_slots), fn->nargs, f);
    fwrite(fn->code, 1, fn->code_size, f);
  }

  fwrite(&r->ninstructions, sizeof(r->ninstructions), 1, f);
  fwrite(r->instructions, sizeof(u8), r->ninstructions, f);
}

static inline void
e_compilation_result_free(e_compilation_result* r)
{
  for (u32 i = 0; i < r->nfunctions; i++) {
    free(r->functions[i].code);
    // free(r->functions[i].arg_slots);
  }
  // free(r->functions);
  // for (u32 i = 0; i < r->nliterals; i++) { e_var_free(&r->literals[i]); }
  // free(r->literals);
  free(r->instructions);
}

#endif // E_BYTECODE_STREAM_READ_WRITE_HELP_H