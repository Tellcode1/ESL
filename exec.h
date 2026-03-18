#ifndef E_VM_H
#define E_VM_H

#include "fn.h"
#include "stdafx.h"
#include "var.h"

#include <stddef.h>

typedef struct e_exec_info {
  const u8*                code;
  const e_var*             args;
  const u32*               slots; // The IDs which the arguments take
  const e_var*             literals;
  const e_function*        funcs;
  const e_extern_function* extern_funcs;

  u32 code_size;
  u32 nargs;
  u32 nliterals;
  u32 nfuncs;
  u32 nextern_funcs;
} e_exec_info;

e_var e_exec(const e_exec_info* info);

#endif // E_VM_H