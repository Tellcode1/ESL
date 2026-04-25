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
#include "stdafx.h"
#include "var.h"

#include <stdio.h>
#include <stdlib.h>

#define E_FILE_MAGIC 0xF5F6F7F8

// See BYTECODE FORMAT section of README.

#define E_GEN_READ_FUNCTION(type)                                                                                                                    \
  static inline type e_read_##type(const u8** _ip)                                                                                                   \
  {                                                                                                                                                  \
    *(_ip) = (const u8*)e_align_ptr((void*)*(_ip), sizeof(type)); /* Align pointer so we don't get a SIGBUS */                                       \
    type v = 0;                                                                                                                                      \
    memcpy(&v, *_ip, sizeof(type));                                                                                                                  \
    *(_ip) += sizeof(type);                                                                                                                          \
    return v;                                                                                                                                        \
  }
E_GEN_READ_FUNCTION(u32)
E_GEN_READ_FUNCTION(u16)
E_GEN_READ_FUNCTION(u8)

#define E_GEN_EMIT_FN(type)                                                                                                                          \
  static inline void e_emit_##type(e_compiler* cc, type value)                                                                                       \
  {                                                                                                                                                  \
    if (cc->num_bytes_emitted + sizeof(type) > cc->emit_capacity)                                                                                    \
      ecc_stream_resize(cc, MAX(cc->num_bytes_emitted + sizeof(type), (size_t)cc->emit_capacity * 2));                                               \
    void* unaligned = (uchar*)cc->emit + cc->num_bytes_emitted;                                                                                      \
    void* aligned   = e_align_ptr((uchar*)cc->emit + cc->num_bytes_emitted, sizeof(type)); /* Align pointer so we don't get a SIGBUS */              \
    memset(unaligned, E_OPCODE_NOOP, (uchar*)aligned - (uchar*)unaligned);                 /* Fill the memory we're skipping over with NOOPs */      \
    memcpy((type*)aligned, &value, sizeof(value));                                                                                                   \
    cc->num_bytes_emitted += ((uchar*)aligned - (uchar*)unaligned);                                                                                  \
    cc->num_bytes_emitted += sizeof(type);                                                                                                           \
  }
E_GEN_EMIT_FN(u32)
E_GEN_EMIT_FN(u64)
E_GEN_EMIT_FN(u16)
E_GEN_EMIT_FN(u8)

typedef enum e_file_read_error {
  E_FILE_READ_SUCCESS                    = 0,
  E_FILE_READ_ERR_INVALID_MAGIC          = -1,
  E_FILE_READ_ERR_ROOT_ALLOCATION_FAILED = -2,
  E_FILE_READ_ERR_INVALID_FILE           = -3,
} e_file_read_error;

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

static inline e_file_read_error
e_file_load(
    FILE*        f,
    void**       root_allocation,
    u32*         ninstructions,
    u8**         instructions,
    u32*         nlits,
    e_var**      lits,
    u32**        lits_hashes,
    u32*         nfunctions,
    e_function** functions)
{
  *root_allocation = NULL;
  *lits            = NULL;
  *nlits           = 0;
  *ninstructions   = 0;
  *instructions    = NULL;
  *nfunctions      = 0;
  *functions       = NULL;

  u32 magic = 0;
  if (fread(&magic, sizeof(magic), 1, f) != 1) goto ERR;

  if (magic != E_FILE_MAGIC) return E_FILE_READ_ERR_INVALID_MAGIC;

  u32 bytes_req = 0;
  if (fread(&bytes_req, sizeof(bytes_req), 1, f) != 1) goto ERR;

  *root_allocation = calloc(bytes_req, 1);
  if (*root_allocation == nullptr) return E_FILE_READ_ERR_ROOT_ALLOCATION_FAILED;

  uchar* alloc = (uchar*)*root_allocation;

  if (fread(nlits, sizeof(*nlits), 1, f) != 1) goto ERR;

  *lits = (e_var*)alloc;
  alloc += sizeof(e_var) * (*nlits);

  *lits_hashes = (u32*)alloc;
  alloc += sizeof(u32) * (*nlits);

  for (u32 i = 0; i < *nlits; i++) {
    if (fread(&(*lits)[i].type, sizeof(e_vartype), 1, f) != 1) goto ERR;

    e_var* lit = &(*lits)[i];
    switch (lit->type) {
      case E_VARTYPE_STRING: {
        u32 len = 0;
        if (fread(&len, sizeof(len), 1, f) != 1) goto ERR;

        alloc      = e_align_ptr(alloc, 16);
        lit->val.s = (e_refdobj*)(alloc);
        alloc += sizeof(e_refdobj);

        /* Initialize ref counter to 1. Not used for literals but VM expects it */
        lit->val.s->refc = 1;

        E_VAR_AS_STRING(lit)->s = (char*)(alloc);
        alloc += len + 1;

        if (fread(E_VAR_AS_STRING(lit)->s, sizeof(char), len, f) != len) goto ERR;

        E_VAR_AS_STRING(lit)->s[len] = 0;
        break;
      }

        // clang-format off
      case E_VARTYPE_NULL:
      case E_VARTYPE_VOID:
      case E_VARTYPE_STRUCT: *lit = (e_var){ .type = E_VARTYPE_NULL }; break;
      case E_VARTYPE_INT: if (fread(&lit->val.i, sizeof(lit->val.i), 1, f) != 1) goto ERR; break;
      case E_VARTYPE_BOOL: if (fread(&lit->val.b, sizeof(lit->val.b), 1, f) != 1) goto ERR; break;
      case E_VARTYPE_CHAR: if (fread(&lit->val.c, sizeof(lit->val.c), 1, f) != 1) goto ERR; break;
      case E_VARTYPE_FLOAT: if (fread(&lit->val.f, sizeof(lit->val.f), 1, f) != 1) goto ERR; break;
      case E_VARTYPE_VEC2: if (fread(&lit->val.vec2, sizeof(lit->val.vec2), 1, f) != 1) goto ERR; break;
      case E_VARTYPE_VEC3: if (fread(&lit->val.vec3, sizeof(lit->val.vec3), 1, f) != 1) goto ERR; break;
      case E_VARTYPE_VEC4: if (fread(&lit->val.vec4, sizeof(lit->val.vec4), 1, f) != 1) goto ERR; break;
      default: break;
        // clang-format on
    }
    (*lits_hashes)[i] = e_var_hash(lit);
  }

  if (fread(nfunctions, sizeof(*nfunctions), 1, f) != 1) goto ERR;

  alloc      = e_align_ptr(alloc, 8);
  *functions = (e_function*)(alloc);
  alloc += sizeof(e_function) * (*nfunctions);

  for (u32 i = 0; i < *nfunctions; i++) {
    e_function func = { 0 };
    if (fread(&func.code_size, sizeof(func.code_size), 1, f) != 1) goto ERR;
    if (fread(&func.nargs, sizeof(func.nargs), 1, f) != 1) goto ERR;
    if (fread(&func.name_hash, sizeof(func.name_hash), 1, f) != 1) goto ERR;

    alloc = e_align_ptr(alloc, 4);

    func.arg_slots = (u32*)alloc;
    alloc += sizeof(u32) * func.nargs;
    if (fread(func.arg_slots, sizeof(*func.arg_slots), func.nargs, f) != func.nargs) goto ERR;

    alloc = e_align_ptr(alloc, 8);

    func.code = (u8*)alloc;
    alloc += func.code_size;
    if (func.code == nullptr) return -1;

    if (fread(func.code, 1, func.code_size, f) != func.code_size) goto ERR;
    (*functions)[i] = func;
  }

  if (fread(ninstructions, sizeof(*ninstructions), 1, f) != 1) goto ERR;

  alloc         = e_align_ptr(alloc, 8);
  *instructions = (u8*)alloc;
  if (fread(*instructions, sizeof(u8), *ninstructions, f) != *ninstructions) goto ERR;

  return E_FILE_READ_SUCCESS;

ERR:
  free(*root_allocation);
  return E_FILE_READ_ERR_INVALID_FILE;
}

static inline u32
e_file_bytes_required(const e_compilation_result* r)
{
  u32 size = 0;

  // literals array
  size += sizeof(e_var) * r->nliterals;
  // literal hashes array (we don't write this tho)
  size += sizeof(u32) * r->nliterals;

  for (u32 i = 0; i < r->nliterals; i++) {
    const e_var* lit = &r->literals[i];

    if (lit->type == E_VARTYPE_STRING) {
      u32 len = strlen(E_VAR_AS_STRING(lit)->s);

      size = (size + 15) & ~15;
      size += sizeof(e_refdobj);
      size += len + 1; // nnull
    }
  }

  // functions array
  size = (size + 7) & ~7;
  size += sizeof(e_function) * r->nfunctions;

  for (u32 i = 0; i < r->nfunctions; i++) {
    const e_function* fn = &r->functions[i];

    size = (size + 3) & ~3;
    size += sizeof(u32) * fn->nargs; // arg_slots

    size += (size + 7) & ~7;
    size += fn->code_size; // code
  }

  size += (size + 7) & ~7;
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
  for (u32 i = 0; i < r->nfunctions; i++) { free(r->functions[i].code); }
  free(r->literals);
  free(r->literals_hashes);
  free(r->functions);
  free(r->instructions);
}

#endif // E_BYTECODE_STREAM_READ_WRITE_HELP_H