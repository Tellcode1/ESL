#ifndef E_BYTECODE_STREAM_READ_WRITE_HELP_H
#define E_BYTECODE_STREAM_READ_WRITE_HELP_H

#include "bc.h"
#include "cc.h"
#include "fn.h"
#include "refcount.h"
#include "stdafx.h"

// clang-format off
#define E_GEN_READ_FUNCTION(type) static inline type e_read_##type(const u8** _ip) { type v = *(type*)(*(_ip)); *(_ip) += sizeof(type); return v; }
        E_GEN_READ_FUNCTION(u32)
        E_GEN_READ_FUNCTION(u16)
        E_GEN_READ_FUNCTION(u8)

#define E_GEN_EMIT_FN(type) static inline void e_emit_##type(e_compiler* cc, type value) { if (cc->emitted + sizeof(type) > cc->code_capacity) ecc_stream_resize(cc, MAX(cc->emitted + sizeof(type), cc->code_capacity * 2)); *(type*)((uchar*)cc->emit + cc->emitted) = value; cc->emitted += sizeof(type); }
        E_GEN_EMIT_FN(u32)
        E_GEN_EMIT_FN(u64)
        E_GEN_EMIT_FN(u16)
        E_GEN_EMIT_FN(u8)

// clang-format on

static inline void
e_emit_instruction(e_compiler* cc, e_opcode opcode, e_attr attrs)
{
  e_emit_u8(cc, opcode);
  e_emit_u8(cc, attrs);
}

static inline void
e_emit_label(e_compiler* cc, u32 labelid)
{
  e_emit_u8(cc, E_OPCODE_LABEL);
  e_emit_u8(cc, E_ATTR_NONE);
  e_emit_u32(cc, labelid);
}

static inline void
e_file_load(FILE* f, u32* nlits, e_var** lits, u32* nfunctions, e_function** functions)
{
  fread(nlits, sizeof(*nlits), 1, f);

  e_var* alits = (e_var*)malloc(sizeof(e_var) * (*nlits));
  *lits        = alits;

  for (u32 i = 0; i < *nlits; i++) {
    fread(&alits[i].type, sizeof(e_vartype), 1, f);

    e_var* lit = &alits[i];
    lit->refc  = e_refc_init();
    if (alits[i].type == E_VARTYPE_STRING) {
      u32 len = 0;
      fread(&len, sizeof(len), 1, f);

      lit->val.s    = (e_string*)malloc(sizeof(e_string));
      lit->val.s->s = (char*)malloc(len + 1);
      fread(lit->val.s->s, sizeof(char), len, f);

      lit->val.s->s[len] = 0;
    } else {
      fread(&alits[i].val, sizeof(e_varval), 1, f);
    }
  }

  fread(nfunctions, sizeof(*nfunctions), 1, f);
  *functions = (e_function*)malloc(sizeof(e_function) * (*nfunctions));
  for (u32 i = 0; i < *nfunctions; i++) {
    e_function func;
    fread(&func.code_size, sizeof(func.code_size), 1, f);
    fread(&func.nargs, sizeof(func.nargs), 1, f);
    fread(&func.name_hash, sizeof(func.name_hash), 1, f);

    func.arg_slots = (u32*)malloc(sizeof(u32) * func.nargs);
    fread(func.arg_slots, sizeof(*func.arg_slots), func.nargs, f);

    func.code = (u8*)malloc(func.code_size);
    fread(func.code, 1, func.code_size, f);
    (*functions)[i] = func;
  }
}

static inline void
e_file_write(e_compilation_result* r, FILE* f)
{
  fwrite(&r->nliterals, sizeof(r->nliterals), 1, f);
  for (u32 i = 0; i < r->nliterals; i++) {
    const e_var* lit = &r->literals[i];
    fwrite(&lit->type, sizeof(e_vartype), 1, f);

    if (lit->type == E_VARTYPE_STRING) {
      u32 len = strlen(lit->val.s->s);
      fwrite(&len, sizeof(len), 1, f);
      fwrite(lit->val.s->s, sizeof(char), len, f);
    } else {
      fwrite(&lit->val, sizeof(lit->val), 1, f);
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
}

static inline void
e_compilation_result_free(e_compilation_result* r)
{
  for (u32 i = 0; i < r->nfunctions; i++) {
    free(r->functions[i].code);
    free(r->functions[i].arg_slots);
  }
  free(r->functions);
  for (u32 i = 0; i < r->nliterals; i++) { e_var_release(&r->literals[i]); }
  free(r->literals);
}

#endif // E_BYTECODE_STREAM_READ_WRITE_HELP_H