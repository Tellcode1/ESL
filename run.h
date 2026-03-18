#ifndef E_VM_H
#define E_VM_H

#include "var.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

struct e_function;

/**
 * nargs is allowed to be 0.
 * nins is not allowed to be 0.
 */
e_var e_exec(u32 nargs, const e_var* args, const u32* slots, u32 nins, const u8* inss, u32 nlits, const e_var* lits, u32 nfuncs, struct e_function* funcs);

#endif // E_VM_H