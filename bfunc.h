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
   * Description string, printed when a compile
   * error / warning takes place. Must be single line.
   */
  const char* desc;

  /**
   * String containing how the function looks if it
   * were written in the script. To reduce ambiguity on
   * errors.
   */
  const char* signature;

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
e_var eb_readln(e_var* args, u32 nargs);

static inline e_var
eb_println(e_var* args, u32 nargs)
{
  eb_print(args, nargs);
  fputc('\n', stdout);
  return (e_var){ .type = E_VARTYPE_NULL };
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
  return (e_var){ .type = E_VARTYPE_BOOL, .val = { .b = strcmp(E_VAR_AS_STRING(&args[0])->s, E_VAR_AS_STRING(&args[1])->s) == 0 } };
}
e_var eb_str_append(e_var* args, u32 nargs);
e_var eb_str_substr(e_var* args, u32 nargs); // substring: string, int start, int length
e_var eb_str_repeat(e_var* args, u32 nargs); // repeat: string, int times
e_var eb_str_ltrim(e_var* args, u32 nargs);
e_var eb_str_rtrim(e_var* args, u32 nargs);
e_var eb_str_trim(e_var* args, u32 nargs);
e_var eb_str_split(e_var* args, u32 nargs); // Get list of strings partition by args[1] (string)
e_var eb_str_len(e_var* args, u32 nargs);

/* read(fp, nbytes), nbytes is the amount of bytes read. */
e_var eb_io_read(e_var* args, u32 nargs);
/* write(fp, str) */
e_var eb_io_write(e_var* args, u32 nargs);
/* seek(fp, offset, io::REL_TO_START/io::REL_TO_END/io::REL_TO_CURR)) */
e_var eb_io_seek(e_var* args, u32 nargs);
/* ptell(fp) */
e_var eb_io_ptell(e_var* args, u32 nargs);
/* Get error allocated as string. */
e_var eb_io_error(e_var* args, u32 nargs);

e_var eb_io_readln(e_var* args, u32 nargs);
e_var eb_io_println(e_var* args, u32 nargs);
e_var eb_io_print(e_var* args, u32 nargs);
e_var eb_io_type(e_var* args, u32 nargs);
e_var eb_io_at_eof(e_var* args, u32 nargs);
e_var eb_io_open(e_var* args, u32 nargs);  // file path, mode
e_var eb_io_close(e_var* args, u32 nargs); // fd
e_var eb_io_getc(e_var* args, u32 nargs);  // fd
e_var eb_io_putc(e_var* args, u32 nargs);  // fd, char

/**
 * Duplicate a variable.
 */
static inline e_var
eb_var_dup(e_var* args, u32 nargs)
{
  (void)nargs;
  e_var v;
  e_var_deep_cpy(&args[0], &v);
  return v;
}

// Make list from elements
e_var eb_list_make(e_var* args, u32 nargs);

// append(list, value)
e_var eb_list_append(e_var* args, u32 nargs);

// pop(list)
e_var eb_list_pop(e_var* args, u32 nargs); // fast

// remove(list, index)
e_var eb_list_remove(e_var* args, u32 nargs); // expensive

// insert(list, index, value)
e_var eb_list_insert(e_var* args, u32 nargs); // Replaces value if it exists!

// find(list, value) => -1 if non existent
e_var eb_list_find(e_var* args, u32 nargs); // Returns index, -1 if it doesn't exist.

// reserve(list, elements_to_reserve)
e_var eb_list_reserve(e_var* args, u32 nargs); // number of new variables to reserve

// len(list)
e_var eb_list_len(e_var* args, u32 nargs);

e_var eb_get_command_line_args(e_var* args, u32 nargs);

e_var eb_vec2(e_var* args, u32 nargs);
e_var eb_vec3(e_var* args, u32 nargs);
e_var eb_vec4(e_var* args, u32 nargs);
e_var eb_mat3(e_var* args, u32 nargs);
e_var eb_mat4(e_var* args, u32 nargs);

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
  char* p = strncpy(buffer, "[", buffer_size - 1);

  if ((bool)(mask & E_VARTYPE_INT)) p = e_strlpcat(p, "int,", buffer, buffer_size);
  if ((bool)(mask & E_VARTYPE_BOOL)) p = e_strlpcat(p, "bool,", buffer, buffer_size);
  if ((bool)(mask & E_VARTYPE_CHAR)) p = e_strlpcat(p, "char,", buffer, buffer_size);
  if ((bool)(mask & E_VARTYPE_FLOAT)) p = e_strlpcat(p, "float,", buffer, buffer_size);
  if ((bool)(mask & E_VARTYPE_STRING)) p = e_strlpcat(p, "string,", buffer, buffer_size);
  if ((bool)(mask & E_VARTYPE_LIST)) p = e_strlpcat(p, "list,", buffer, buffer_size);
  if ((bool)(mask & E_VARTYPE_MAP)) p = e_strlpcat(p, "map,", buffer, buffer_size);
  if ((bool)(mask & E_VARTYPE_ERROR)) p = e_strlpcat(p, "error,", buffer, buffer_size);

  /* Set last pipe to NUL */
  char* last_comma = strrchr(buffer, ',');
  if (last_comma != nullptr) *last_comma = 0;

  p = e_strlpcat(p, "]", buffer, buffer_size);
}

char* e_read_full_line(FILE* f);

static inline int
evar_to_int(e_var v)
{
  switch (v.type) {
    case E_VARTYPE_NULL: return 0;
    case E_VARTYPE_INT: return v.val.i;
    case E_VARTYPE_FLOAT: return (int)v.val.f;
    case E_VARTYPE_CHAR: return (int)v.val.c;
    case E_VARTYPE_BOOL: return (int)v.val.b;
    case E_VARTYPE_STRING: return atoi(E_VAR_AS_STRING(&v)->s);
    case E_VARTYPE_LIST: return E_VAR_AS_LIST(&v)->size;
    case E_VARTYPE_MAP: return E_VAR_AS_MAP(&v)->size;
    case E_VARTYPE_STRUCT: return E_VAR_AS_STRUCT(&v)->nmembers;
    default: return 0;
  }
}

static inline double
evar_to_float(e_var v)
{
  switch (v.type) {
    case E_VARTYPE_NULL: return 0.0;
    case E_VARTYPE_FLOAT: return v.val.f;
    case E_VARTYPE_INT: return (double)v.val.i;
    case E_VARTYPE_CHAR: return (double)v.val.c;
    case E_VARTYPE_BOOL: return (double)v.val.b;
    case E_VARTYPE_STRING: return atof(E_VAR_AS_STRING(&v)->s);
    default: return (double)evar_to_int(v);
  }
}

static inline bool
evar_to_bool(e_var v)
{
  switch (v.type) {
    case E_VARTYPE_NULL: return false;
    case E_VARTYPE_INT: return (bool)v.val.i;
    case E_VARTYPE_FLOAT: return (bool)v.val.f;
    case E_VARTYPE_CHAR: return (bool)v.val.c;
    case E_VARTYPE_BOOL: return (bool)v.val.b;
    case E_VARTYPE_STRING: return strlen(E_VAR_AS_STRING(&v)->s) != 0;
    case E_VARTYPE_LIST: return E_VAR_AS_LIST(&v)->size != 0;
    case E_VARTYPE_MAP: return E_VAR_AS_MAP(&v)->size != 0;
    case E_VARTYPE_STRUCT: return E_VAR_AS_STRUCT(&v)->nmembers != 0;
    default: return false;
  }
}

#define E_ALL_TYPES E_VARTYPE_INT | E_VARTYPE_CHAR | E_VARTYPE_BOOL | E_VARTYPE_FLOAT | E_VARTYPE_LIST | E_VARTYPE_MAP | E_VARTYPE_STRING

// clang-format off
static const e_builtin_func eb_funcs[] = {
  /* Can print anything. */
  { "print", "Print all provided variables to stdout", "fn print(...) -> null", 0xFFFFFFFF, 1, UINT32_MAX, eb_print },
  { "println", "Print all provided variables and a newline to stdout", "fn println(...) -> null", 0xFFFFFFFF, 1, UINT32_MAX, eb_println },

  /* Read line from stdin */
  { "readln", "Read a line from stdin and retrieve it as a string. null on error.", "fn readln(...) -> string|null", 0, 0, 0, eb_readln },

  /* Can convert anything to string. */
  { "string", "Cast a variable to a string", "fn string(v) -> string", 0xFFFFFFFF, 1, 1, eb_cast_string },

  {"vec2", "Cast two floats in to a vec2", "fn vec2(x, y) -> vec2", E_VARTYPE_FLOAT, 2, 2, eb_vec2},
  {"vec3", "Cast two floats in to a vec2", "fn vec3(x, y, z) -> vec3", E_VARTYPE_FLOAT, 3, 3, eb_vec3},
  {"vec4", "Cast two floats in to a vec2", "fn vec4(x, y, z, w) -> vec4", E_VARTYPE_FLOAT, 4, 4, eb_vec4},

  {"mat3", "Cast three vector3's into a mat3", "fn mat3(row0, row1, row2, row3) -> vec2", E_VARTYPE_VEC3, 4, 4, eb_mat3},
  {"mat4", "Cast three vector4's into a mat3", "fn mat4(row0, row1, row2, row3) -> vec2", E_VARTYPE_VEC4, 4, 4, eb_mat4},

  /* Scalar types */
  { "int", "Cast a variable to a int", "fn int(v) -> int", E_VARTYPE_INT | E_VARTYPE_CHAR | E_VARTYPE_BOOL | E_VARTYPE_FLOAT | E_VARTYPE_STRING,    1,    1, eb_cast_int },
  { "char", "Cast a variable to a char", "fn char(v) -> char", E_VARTYPE_INT | E_VARTYPE_CHAR | E_VARTYPE_BOOL | E_VARTYPE_FLOAT | E_VARTYPE_STRING,    1,    1, eb_cast_char },
  { "bool", "Cast a variable to a bool", "fn bool(v) -> bool", E_VARTYPE_INT | E_VARTYPE_CHAR | E_VARTYPE_BOOL | E_VARTYPE_FLOAT | E_VARTYPE_STRING,    1,    1, eb_cast_bool },
  { "float", "Cast a variable to a float", "fn float(v) -> float", E_VARTYPE_INT | E_VARTYPE_CHAR | E_VARTYPE_BOOL | E_VARTYPE_FLOAT | E_VARTYPE_STRING,    1,    1, eb_cast_float },

  /* Anything can be added as an element to a list */
  { "list", "Make a list of provided elements", "fn list(...) -> list", 0xFFFFFF, 1, UINT32_MAX, eb_cast_list },

  { "len", "Get the type dependant length of the given variable (String=Length, List=Size, Map=NPairs)", "fn len(v : string|list|map) -> int", E_VARTYPE_STRING | E_VARTYPE_LIST | E_VARTYPE_MAP,    1,    1, eb_len },

  /* Math builtins */
  { "math::sin", "Compute the sine of the given argument (radians)", "fn math::sin(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_sin },
  { "math::cos", "Compute the cosine of the given argument (radians)", "fn math::cos(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_cos },
  { "math::tan", "Compute the tangent of the given argument (radians)", "fn math::tan(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_tan },
  { "math::asin", "Compute the arcsine of the given argument (radians)", "fn math::asin(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_asin },
  { "math::acos", "Compute the arccosine of the given argument (radians)", "fn math::acos(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_acos },
  { "math::atan", "Compute the arctangent of the given argument (radians)", "fn math::atan(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_atan },
  { "math::atan2", "Compute the arctangent of the given arguments (x, y) (radians)", "fn math::atan2(x, y) -> float", E_VARTYPE_FLOAT, 2, 2, eb_atan2 },
  { "math::sinh", "Compute the hyperbolic sine of the given argument (radians)", "fn math::sinh(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_sinh },
  { "math::cosh", "Compute the hyperbolic cosine of the given argument (radians)", "fn math::cosh(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_cosh },
  { "math::tanh", "Compute the hyperbolic tangent of the given argument (radians)", "fn math::tanh(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_tanh },
  { "math::asinh", "Compute the hyperbolic arcsine of the given argument (radians)", "fn math::asinh(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_asinh },
  { "math::acosh", "Compute the hyperbolic arccosine of the given argument (radians)", "fn math::acosh(x) -> float", E_VARTYPE_FLOAT,    1,    1, eb_acosh },
  { "math::atanh", "Compute the hyperbolic arctangent of the given argument (radians)", "fn math::atanh(x) -> float", E_VARTYPE_FLOAT,    1,    1, eb_atanh },
  { "math::exp", "Compute e^n (n is argument)", "fn math::exp(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_exp },
  { "math::log", "Compute the natural logarithm of the given argument", "fn math::log(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_log },
  { "math::log10", "Compute the logarithm in base 10 of the given argument", "fn math::log10(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_log10 },
  { "math::pow", "Compute x ^ y, with both x and y being arguments", "fn math::pow(x, y) -> float", E_VARTYPE_FLOAT, 2, 2, eb_pow },
  { "math::sqrt", "Compute the square root of the given argument", "fn math::sqrt(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_sqrt },
  { "math::ceil", "Round the given value up", "fn math::ceil(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_ceil },
  { "math::floor", "Round the given value down", "fn math::floor(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_floor },
  { "math::fmod", "Compute the remainder of the given arguments", "fn math::fmod(x, y) -> float", E_VARTYPE_FLOAT, 2, 2, eb_fmod },
  { "math::round", "Round the given argument", "fn math::round(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_round },
  { "math::trunc", "Truncate the decimal part of the argument (result is still floating point)", "fn math::trunc(x) -> float", E_VARTYPE_FLOAT,    1,    1, eb_trunc },
  { "math::abs", "Compute the absolute value of the given argument", "fn math::abs(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_abs },
  { "math::hypot", "Compute the sqrt(x^2 + y^2), with maximum accuracy", "fn math::hypot(x, y) -> float", E_VARTYPE_FLOAT, 2, 2, eb_hypot },

  { "io::open", "Open a file. null on error.", "fn io::open( path:string, mode:string ) -> fd", E_VARTYPE_INT, 2, 2, eb_io_open },
  { "io::close", "Close a file.", "fn io::close( fd ) -> null", E_VARTYPE_INT, 1, 1, eb_io_close },
  { "io::putc", "\"Put\" a character into file.", "fn io::putc( fd, ch ) -> null", E_VARTYPE_INT, 1, 1, eb_io_close },
  { "io::getc", "Get a character from file. null on error.", "fn io::getc( fd ) -> char|null", E_VARTYPE_INT, 1, 1, eb_io_close },
  { "io::read", "Read given number of bytes from file. null on error.", "fn io::read( fd, nbytes:int ) -> string|null", E_VARTYPE_INT, 2, 2, eb_io_read },
  { "io::readln", "Read a line from file. null on error.", "fn io::readln( fd ) -> string|null", E_VARTYPE_INT, 1, 1, eb_io_readln },
  { "io::write", "Write string to file. null on error. number of bytes written on success.", "fn io::write( fd, data : string ) -> int|null", E_VARTYPE_INT, 2, 2, eb_io_write },
  { "io::seek", "Seek to a random position in file.", "fn io::seek( fd, offset, relative_to ) -> null", E_VARTYPE_INT, 3, 3, eb_io_seek },
  { "io::ptell", "Get location in file. null on error.", "fn io::ptell( fd ) -> int|null", E_VARTYPE_INT, 1, 1, eb_io_ptell },
  { "io::println", "Print all given variables and a newline to file. null on error. number of bytes written on success.", "fn io::println( fd, ... ) -> int|null", E_VARTYPE_INT,    1, UINT32_MAX, eb_io_println },
  { "io::print", "Print all given variables to file. null on error. number of bytes written on success.", "fn io::print( fd, ... ) -> int|null", E_VARTYPE_INT, 2, UINT32_MAX, eb_io_print },
  { "io::type", "Get the type of an object on the disk. Possible return values are io::FILE|io::LINK|io::DIRECTORY|io::UNKNOWN", "fn io::type( path ) -> int", E_VARTYPE_INT, 1, 1, eb_io_type },
  { "io::list_dir", "List a directory. Returns the file (and directory) paths as a list.", "fn io::list_dir( path ) -> list", E_VARTYPE_INT, 3, 3, eb_io_seek },
  { "io::at_eof", "At the end of file? Boolean result only", "fn io::at_eof(fd) -> bool", E_VARTYPE_INT, 1, 1, eb_io_at_eof },
  { "io::error", "Get error string. Won't always be success for successful operations.", "fn io::error(fd) -> string", 0, 0, 0, eb_io_error },

  { "str::dup", "Duplicate a string. Alias to generic::dup", "fn str::dup(s : string) -> string", E_VARTYPE_STRING, 1, 1, eb_var_dup },
  { "str::equal", "Compare two strings. Boolean result.", "fn str::equal(s1 : string, s2 : string) -> bool", E_VARTYPE_STRING, 2, 2, eb_str_equal },
  { "str::append", "Catenate all given strings. null on error.", "fn str::append(...) -> string", E_VARTYPE_STRING, 1, UINT32_MAX, eb_str_append },
  { "str::substr", "Make a substring of given string, starting from 'start' with the length 'len'", "fn str::substr(s : string, start : int, len : int) -> string", E_VARTYPE_STRING, 3, 3, eb_str_substr },
  { "str::repeat", "Repeat a string n times", "fn str::repeat(s : string, n : int) -> string", E_VARTYPE_STRING, 2, 2, eb_str_repeat },
  { "str::ltrim", "Trim all spaces from the left in the string and return a copy", "fn str::ltrim(s : string) -> string", E_VARTYPE_STRING,    1,    1, eb_str_ltrim },
  { "str::rtrim", "Trim all spaces from the right in the string and return a copy", "fn str::rtrim(s : string) -> string", E_VARTYPE_STRING,    1,    1, eb_str_rtrim },
  { "str::trim", "Trim all spaces from the string and return a copy", "fn str::trim(s : string) -> string", E_VARTYPE_STRING, 1, 1, eb_str_trim },
  { "str::split", "Split a string by a another string (the delimiter) and return a list containing each token", "fn str::split(s : string, )", E_VARTYPE_STRING, 2, 2, eb_str_split },
  { "str::len", "Get the length of the string. Alias to len", "fn str::len(s : string) -> int", E_VARTYPE_STRING, 1, 1, eb_str_len }, // equivalent to len()

  { "list::dup", "Duplicate a list. Alias to dup", "fn list::dup(list) -> list", E_VARTYPE_LIST, 1, 1, eb_var_dup },
  { "list::make", "Make a list using given elements", "fn list::make(...) -> list", E_ALL_TYPES, 1, UINT32_MAX, eb_list_make },
  { "list::append", "Append a value to the list", "fn list::append(list, var) -> null", E_ALL_TYPES, 2, 2, eb_list_append },
  { "list::pop", "Remove the last element of the list and return it", "fn list::pop(list) -> var", E_ALL_TYPES, 1, 1, eb_list_pop },
  { "list::remove", "Remove the element from the index in the list and return it", "fn list::remove(list, index:int) -> var", E_VARTYPE_LIST | E_VARTYPE_INT, 2, 2, eb_list_remove },
  { "list::insert", "Insert an element into the given index in the list", "fn list::insert(list, index : int) -> null", E_ALL_TYPES, 3, 3, eb_list_insert },
  { "list::find", "Find an element in the list. -1 if nonexistent.", "fn list::find(list, to_find : var) -> int", E_ALL_TYPES, 2, 2, eb_list_find },
  { "list::reserve", "Reserve capacity for n elements.", "fn list::reserve(list, elems_to_reserve:int) -> null", E_VARTYPE_LIST | E_VARTYPE_INT, 2, 2, eb_list_reserve },
  { "list::len", "Get number of elements in list.", "fn list::len(list) -> int", E_VARTYPE_LIST, 1, 1, eb_list_len },

  { "sys::get_command_line_args", "Get the command line arguments passed, as a list", "fn sys::get_command_line_args() -> list|null", E_VARTYPE_VOID, 0, 0, eb_get_command_line_args }
};
// clang-format on

#endif // E_BUILTIN_FUNCTIONS_H
