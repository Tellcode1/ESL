#ifndef E_CC_H
#define E_CC_H

#include "bc.h"
#include "fn.h"
#include "stdafx.h"
#include "var.h"

struct e_ast;

typedef struct ecc_loop_location {
  e_labelID continue_label;
  e_labelID break_label;
} ecc_loop_location;

/**'
 * struct used by the compiler to track variable state.
 */
typedef struct ecc_variable {
  bool is_initialized;
  bool was_modified;

  e_vartype type;

  int initializer_node_id;

  /**
   * If variable is a constant, its value
   */
  e_varval constant_value;
} ecc_variable;

typedef struct ecc_callframe {
  bool is_function;
  u32  restore_sp; // The stack pointer to return to when the scope exits.
} ecc_callframe;

typedef struct e_compiler {
  struct e_ast*      ast;
  ecc_loop_location* loop;

  e_var* literals;
  u16*   literal_hashes;
  u32    nliterals;
  u32    cliterals;

  u8* emit;
  u32 emitted;
  u32 code_capacity;

  e_function* functions;
  u32         functions_size;
  u32         functions_capacity;

  u32 next_label;
} e_compiler;

typedef struct e_compilation_result {
  u32         nliterals;
  u32         nfunctions;
  e_var*      literals;
  e_function* functions;
} e_compilation_result;

int e_compile(struct e_ast* ast, int root_node, e_compilation_result* result);

static inline void
ecc_stream_resize(e_compiler* cc, int new_cap)
{
  if (cc == nullptr || new_cap == 0) return;

  u8* newcode = (u8*)realloc(cc->emit, sizeof(u8) * new_cap);
  if (newcode == nullptr) { return; }

  cc->emit          = newcode;
  cc->code_capacity = new_cap;
}

#endif // E_CC_H