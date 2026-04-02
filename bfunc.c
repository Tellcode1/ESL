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

#include "bfunc.h"

#include "pool.h"
#include "var.h"

#include <stdlib.h>

static inline double
to_float(e_var v)
{
  switch (v.type) {
    case E_VARTYPE_FLOAT: return v.val.f;
    case E_VARTYPE_INT: return (double)v.val.i;
    case E_VARTYPE_CHAR: return (double)v.val.c;
    case E_VARTYPE_BOOL: return (double)v.val.b;
    default: return 0.0;
  }
}

static inline int
to_int(e_var v)
{
  switch (v.type) {
    case E_VARTYPE_INT: return v.val.i;
    case E_VARTYPE_FLOAT: return (int)v.val.f;
    case E_VARTYPE_CHAR: return (int)v.val.c;
    case E_VARTYPE_BOOL: return (int)v.val.b;
    default: return 0;
  }
}

e_var
eb_print(e_var* args, u32 nargs)
{
  for (u32 i = 0; i < nargs; i++) e_var_print(&args[i], stdout);
  return (e_var){ .type = E_VARTYPE_NULL };
}

e_var
eb_cast_int(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type = E_VARTYPE_INT, .val.i = to_int(args[0]) };
}

e_var
eb_cast_char(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type = E_VARTYPE_CHAR, .val.c = (char)to_int(args[0]) };
}

e_var
eb_cast_bool(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type = E_VARTYPE_BOOL, .val.b = (bool)to_int(args[0]) };
}

e_var
eb_cast_float(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type = E_VARTYPE_FLOAT, .val.f = to_float(args[0]) };
}

e_var
eb_cast_string(e_var* args, u32 nargs)
{
  (void)nargs;

  size_t len = e_var_to_string_size(&args[0]);
  char*  s   = malloc(len + 1);
  s[len]     = 0;

  e_var_to_string(&args[0], s, len + 1);

  e_refdobj* obj = e_refdobj_pool_acquire(&ge_pool);

  E_OBJ_AS_STRING(obj)->s = s;

  return (e_var){ .type = E_VARTYPE_STRING, .val.s = obj };
}

e_var
eb_cast_list(e_var* args, u32 nargs)
{
  e_var new_list = {
    .type     = E_VARTYPE_LIST,
    .val.list = e_refdobj_pool_acquire(&ge_pool),
  };

  e_list_init(args, nargs, E_OBJ_AS_LIST(new_list.val.list)); // acquires the elements. Stack frees them later.

  return new_list;
}
