#ifndef E_FUNCTION_H
#define E_FUNCTION_H

#include "stdafx.h"
#include "var.h"

struct e_compiler;

typedef struct e_function {
  u32  name_hash;
  u32  nargs;
  u32  code_size;
  u8*  code;
  u32* arg_slots; /* The ID of the arguments, in order. */
} e_function;

/* Call C function from the script. */
typedef struct e_extern_function {
  /**
   * For any value returned from an external function, ensure
   * it has its ref counter initialized (in global memory)
   * using e_refc_init().
   */
  u32 hash;
  u32 nargs;                  // Only checked at runtime...
  e_var (*func)(e_var* args); // Must not be NULL!
} e_extern_function;

/* Defined in cc.c */

/**
 * Modify the compiler struct to write to a newly allocated stream!
 * Otherwise this will just overwrite your root stream or append to it.
 * Returns 0 on success.
 */
int e_compile_function(struct e_compiler* cc, int node);

#endif // E_FUNCTION_H
