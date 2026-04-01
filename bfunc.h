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

#ifndef E_BUILTIN_FUNCTIONS_H
#define E_BUILTIN_FUNCTIONS_H

#include "cerr.h"
#include "pool.h"
#include "refcount.h"
#include "stdafx.h"
#include "var.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct e_builtin_func {
  /**
   * Function name in script.
   * Not checked for conflicts. First one is used.
   */
  const char* name;

  /**
   * Variable types that are allowed.
   * Excludes VOID always. You can not pass void as a variable.
   * If a variable does not have a bit set in the mask,
   * The compiler will fail and error out.
   */
  u32 allowed_mask;

  u32 min_args;

  u32 max_args;

  /**
   * Returned variables must have a reference counter
   * initialized to 1, except void.
   */
  e_var (*func)(e_var* args, u32 nargs);
} e_builtin_func;

e_var eb_print(e_var* args, u32 nargs);

static inline e_var
eb_println(e_var* args, u32 nargs)
{
  eb_print(args, nargs);
  fputc('\n', stdout);
  return (e_var){ .type = E_VARTYPE_VOID };
}

e_var eb_cast_int(e_var* args, u32 nargs);
e_var eb_cast_char(e_var* args, u32 nargs);
e_var eb_cast_bool(e_var* args, u32 nargs);
e_var eb_cast_float(e_var* args, u32 nargs);
e_var eb_cast_string(e_var* args, u32 nargs);
e_var eb_cast_list(e_var* args, u32 nargs);

// clang-format off
static inline e_var eb_sin(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = sin(args[0].val.f)}}; }
static inline e_var eb_cos(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = cos(args[0].val.f)}}; }
static inline e_var eb_tan(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = tan(args[0].val.f)}}; }
static inline e_var eb_asin(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = asin(args[0].val.f)}}; }
static inline e_var eb_acos(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = acos(args[0].val.f)}}; }
static inline e_var eb_atan(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = atan(args[0].val.f)}}; }
static inline e_var eb_atan2(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = atan2(args[0].val.f,args[1].val.f)}}; }
static inline e_var eb_sinh(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = sinh(args[0].val.f)}}; }
static inline e_var eb_cosh(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = cosh(args[0].val.f)}}; }
static inline e_var eb_tanh(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = tanh(args[0].val.f)}}; }
static inline e_var eb_acosh(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = acosh(args[0].val.f)}}; }
static inline e_var eb_asinh(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = asinh(args[0].val.f)}}; }
static inline e_var eb_atanh(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = atanh(args[0].val.f)}}; }
static inline e_var eb_exp(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = exp(args[0].val.f)}}; }
static inline e_var eb_log(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = log(args[0].val.f)}}; }
static inline e_var eb_log10(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = log10(args[0].val.f)}}; }
static inline e_var eb_pow(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = pow(args[0].val.f, args[1].val.f)}}; }
static inline e_var eb_sqrt(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = sqrt(args[0].val.f)}}; }
static inline e_var eb_ceil(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = ceil(args[0].val.f)}}; }
static inline e_var eb_floor(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = floor(args[0].val.f)}}; }
static inline e_var eb_fmod(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = fmod(args[0].val.f,args[1].val.f)}}; }
static inline e_var eb_round(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = round(args[0].val.f)}}; }
static inline e_var eb_trunc(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = trunc(args[0].val.f)}}; }
static inline e_var eb_abs(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = fabs(args[0].val.f)}}; }
static inline e_var eb_hypot(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = hypot(args[0].val.f, args[1].val.f)}}; }
// clang-format on

// Form a string from a list of characters
e_var eb_str_from_list(e_var* args, u32 nargs);
static inline e_var
eb_str_equal(e_var* args, u32 nargs)
{
  (void)(nargs);
  return (e_var){ .type = E_VARTYPE_BOOL, .val = { .b = strcmp(E_VAR_AS_STRING(&args[0])->s, E_VAR_AS_STRING(&args[1])->s)==0 } };
}
e_var eb_str_append(e_var* args, u32 nargs);
e_var eb_str_substr(e_var* args, u32 nargs); // substring: string, int start, int length
e_var eb_str_repeat(e_var* args, u32 nargs); // repeat: string, int times
e_var eb_str_lstrip(e_var* args, u32 nargs);
e_var eb_str_rstrip(e_var* args, u32 nargs);
e_var eb_str_split(e_var* args, u32 nargs); // Get list of strings partition by args[1] (string)
static inline e_var
eb_str_strip(e_var* args, u32 nargs)
{
  e_var l = eb_str_lstrip(args, nargs);
  return eb_str_rstrip(&l, 1);
}
e_var eb_str_len(e_var* args, u32 nargs);

// Make list from elements
e_var eb_list_make(e_var* args, u32 nargs);

// append(list, value)
e_var eb_list_append(e_var* args, u32 nargs);

// pop(list)
e_var eb_list_pop(e_var* args, u32 nargs);     // fast

// remove(list, index)
e_var eb_list_remove(e_var* args, u32 nargs);  // expensive

// insert(list, index, value)
e_var eb_list_insert(e_var* args, u32 nargs);  // Replaces value if it exists!

// find(list, value) => -1 if non existent
e_var eb_list_find(e_var* args, u32 nargs);    // Returns index, -1 if it doesn't exist.

// reserve(list, elements_to_reserve)
e_var eb_list_reserve(e_var* args, u32 nargs); // number of new variables to reserve

// len(list)
e_var eb_list_len(e_var* args, u32 nargs);

static inline e_var
eb_len(e_var* args, u32 nargs)
{
  (void)nargs;

  int len = 0;
  if (args[0].type == E_VARTYPE_STRING) {
    len = (int)strlen(E_VAR_AS_STRING(&args[0])->s);
  } else if (args[0].type == E_VARTYPE_LIST) {
    len = (int)E_VAR_AS_LIST(&args[0])->size;
  } else if (args[0].type == E_VARTYPE_MAP) {
    len = (int)E_VAR_AS_MAP(&args[0])->size;
  }
  return (e_var){ .type = E_VARTYPE_INT, .val = { .i = len } };
}

static inline void
e_var_type_bitmask_to_string(e_vartype mask, char* buffer, size_t buffer_size)
{
  strlcpy(buffer, "[", buffer_size);

  if ((bool)(mask & E_VARTYPE_INT)) strlcat(buffer, "int,", buffer_size);
  if ((bool)(mask & E_VARTYPE_BOOL)) strlcat(buffer, "bool,", buffer_size);
  if ((bool)(mask & E_VARTYPE_CHAR)) strlcat(buffer, "char,", buffer_size);
  if ((bool)(mask & E_VARTYPE_FLOAT)) strlcat(buffer, "float,", buffer_size);
  if ((bool)(mask & E_VARTYPE_STRING)) strlcat(buffer, "string,", buffer_size);
  if ((bool)(mask & E_VARTYPE_LIST)) strlcat(buffer, "list,", buffer_size);
  if ((bool)(mask & E_VARTYPE_MAP)) strlcat(buffer, "map,", buffer_size);
  if ((bool)(mask & E_VARTYPE_ERROR)) strlcat(buffer, "error,", buffer_size);

  /* Set last pipe to NUL */
  char* last_comma = strrchr(buffer, ',');
  if (last_comma != nullptr) *last_comma = 0;

  strlcat(buffer, "]", buffer_size);
}

#define E_ALL_TYPES E_VARTYPE_INT | E_VARTYPE_CHAR | E_VARTYPE_BOOL | E_VARTYPE_FLOAT | E_VARTYPE_LIST | E_VARTYPE_MAP | E_VARTYPE_STRING

static const e_builtin_func eb_funcs[] = {
  /* Can print anything. */
  { "print", 0xFFFFFFFF, 1, UINT32_MAX, eb_print },
  { "println", 0xFFFFFFFF, 1, UINT32_MAX, eb_println },

  /* Can convert anything to string. */
  { "string", 0xFFFFFFFF, 1, 1, eb_cast_string },

  /* Scalar types */
  { "int", E_VARTYPE_INT | E_VARTYPE_CHAR | E_VARTYPE_BOOL | E_VARTYPE_FLOAT | E_VARTYPE_STRING, 1, 1, eb_cast_int },
  { "char", E_VARTYPE_INT | E_VARTYPE_CHAR | E_VARTYPE_BOOL | E_VARTYPE_FLOAT | E_VARTYPE_STRING, 1, 1, eb_cast_char },
  { "bool", E_VARTYPE_INT | E_VARTYPE_CHAR | E_VARTYPE_BOOL | E_VARTYPE_FLOAT | E_VARTYPE_STRING, 1, 1, eb_cast_bool },
  { "float", E_VARTYPE_INT | E_VARTYPE_CHAR | E_VARTYPE_BOOL | E_VARTYPE_FLOAT | E_VARTYPE_STRING, 1, 1, eb_cast_float },

  /* Anything can be added as an element to a list */
  { "list", 0xFFFFFF, 1, UINT32_MAX, eb_cast_list },

  { "len", E_VARTYPE_STRING | E_VARTYPE_LIST | E_VARTYPE_MAP, 1, 1, eb_len },

  /* Math builtins */
  { "math::sin", E_VARTYPE_FLOAT, 1, 1, eb_sin },
  { "math::cos", E_VARTYPE_FLOAT, 1, 1, eb_cos },
  { "math::tan", E_VARTYPE_FLOAT, 1, 1, eb_tan },
  { "math::asin", E_VARTYPE_FLOAT, 1, 1, eb_asin },
  { "math::acos", E_VARTYPE_FLOAT, 1, 1, eb_acos },
  { "math::atan", E_VARTYPE_FLOAT, 1, 1, eb_atan },
  { "math::atan2", E_VARTYPE_FLOAT, 2, 2, eb_atan2 },
  { "math::sinh", E_VARTYPE_FLOAT, 1, 1, eb_sinh },
  { "math::cosh", E_VARTYPE_FLOAT, 1, 1, eb_cosh },
  { "math::tanh", E_VARTYPE_FLOAT, 1, 1, eb_tanh },
  { "math::acosh", E_VARTYPE_FLOAT, 1, 1, eb_acosh },
  { "math::asinh", E_VARTYPE_FLOAT, 1, 1, eb_asinh },
  { "math::atanh", E_VARTYPE_FLOAT, 1, 1, eb_atanh },
  { "math::exp", E_VARTYPE_FLOAT, 1, 1, eb_exp },
  { "math::log", E_VARTYPE_FLOAT, 1, 1, eb_log },
  { "math::log10", E_VARTYPE_FLOAT, 1, 1, eb_log10 },
  { "math::pow", E_VARTYPE_FLOAT, 1, 1, eb_pow },
  { "math::sqrt", E_VARTYPE_FLOAT, 1, 1, eb_sqrt },
  { "math::ceil", E_VARTYPE_FLOAT, 1, 1, eb_ceil },
  { "math::floor", E_VARTYPE_FLOAT, 1, 1, eb_floor },
  { "math::fmod", E_VARTYPE_FLOAT, 1, 1, eb_fmod },
  { "math::round", E_VARTYPE_FLOAT, 1, 1, eb_round },
  { "math::trunc", E_VARTYPE_FLOAT, 1, 1, eb_trunc },
  { "math::abs", E_VARTYPE_FLOAT, 1, 1, eb_abs },
  { "math::hypot", E_VARTYPE_FLOAT, 1, 1, eb_hypot },

  { "str::equal", E_VARTYPE_STRING, 2, 2, eb_str_equal },
  { "str::append", E_VARTYPE_STRING, 1, UINT32_MAX, eb_str_append },
  { "str::substr", E_VARTYPE_STRING, 1, 3, eb_str_substr },
  { "str::repeat", E_VARTYPE_STRING, 1, 2, eb_str_repeat },
  { "str::lstrip", E_VARTYPE_STRING, 1, 1, eb_str_lstrip },
  { "str::rstrip", E_VARTYPE_STRING, 1, 1, eb_str_rstrip },
  { "str::strip", E_VARTYPE_STRING, 1, 1, eb_str_strip },
  { "str::split", E_VARTYPE_STRING, 2, 2, eb_str_split },
  { "str::len", E_VARTYPE_STRING, 1, 1, eb_str_len }, // equivalent to len()

  { "list::make", E_ALL_TYPES, 1, UINT32_MAX, eb_list_make },
  { "list::append", E_ALL_TYPES, 2, 2, eb_list_append },
  { "list::pop", E_ALL_TYPES, 1, 1, eb_list_pop },
  { "list::remove", E_VARTYPE_LIST | E_VARTYPE_INT, 2, 2, eb_list_remove },
  { "list::insert", E_ALL_TYPES, 3, 3, eb_list_insert },
  { "list::find", E_ALL_TYPES, 2, 2, eb_list_find },
  { "list::reserve", E_VARTYPE_LIST | E_VARTYPE_INT, 2, 2, eb_list_reserve },
  { "list::len", E_VARTYPE_LIST, 1, 1, eb_list_len },

  { "fs::exists" },
  { "fs::type" },  // See bvar.h list constants (list::FILE,list::LINK,list::DIR)
  { "fs::write" }, // Write entire string contents to file
  { "fs::read" },  // retrieve entire contents of file as a string.
  { "fs::get_cwd" },
  { "fs::path_resolve" }, // make path absolute
  { "fs::list_directory" },
};

#endif // E_BUILTIN_FUNCTIONS_H
