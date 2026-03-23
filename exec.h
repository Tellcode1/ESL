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

#ifndef E_VM_H
#define E_VM_H

#include "fn.h"
#include "stack.h"
#include "stdafx.h"
#include "var.h"

#include <stddef.h>

typedef struct e_exec_info {
  const u8*                code;
  const e_var*             args;     // nullptr if nargs == 0
  const u32*               slots;    // The IDs which the arguments take
  const e_var*             literals; // must outlive the exec function.
  const e_function*        funcs;
  const e_extern_function* extern_funcs;

  /**
   * Must not be NULL.
   * A temporary scratchpad for the VM.
   */
  e_stack* stack;

  u32 code_size; // must not be equal to 0
  u32 nargs;     // can be equal to 0 (no arguments passed :<)
  u32 nliterals;
  u32 nfuncs;
  u32 nextern_funcs;
} e_exec_info;

/**
 * Execute a bytecode stream.
 * You can hook external (C) functions to the program
 * using the extern_funcs member of info.
 */
e_var e_exec(const e_exec_info* info);

#endif // E_VM_H