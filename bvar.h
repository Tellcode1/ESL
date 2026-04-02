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

#ifndef E_BUILTIN_VARIABLES_H
#define E_BUILTIN_VARIABLES_H

#include "var.h"

#include <float.h>
#include <math.h>
#include <stdint.h>

/**
 * Builtin variables structure.
 * All variables within are loaded by the compiler ONCE at
 * start of initialization.
 * Any occurence of these variables are replaced with their values.
 */
typedef struct e_builtin_var {
  const char* name;
  e_vartype   type;
  e_varval    value;
} e_builtin_var;

/**
 * List is not checked for conflicting entries!
 * All variables must have constant, compile time (literal) values.
 */
static const e_builtin_var eb_vars[] = {
  { .name = "null", .type = E_VARTYPE_NULL, .value = { 0 } },
  { .name = "math::PI", .type = E_VARTYPE_FLOAT, .value = { .f = M_PI } },
  { .name = "math::PIBY2", .type = E_VARTYPE_FLOAT, .value = { .f = M_PI_2 } },
  { .name = "math::PIBY4", .type = E_VARTYPE_FLOAT, .value = { .f = M_PI_4 } },
  { .name = "math::PIX2", .type = E_VARTYPE_FLOAT, .value = { .f = M_PI * 2.0 } },
  { .name = "math::TAU", .type = E_VARTYPE_FLOAT, .value = { .f = M_PI * 2.0 } },
  { .name = "math::PHI", .type = E_VARTYPE_FLOAT, .value = { .f = 1.6180339887498948 } }, // golden ratio
  { .name = "math::e", .type = E_VARTYPE_FLOAT, .value = { .f = M_E } },
  { .name = "math::ROOT2", .type = E_VARTYPE_FLOAT, .value = { .f = 1.4142135623730950 } },
  { .name = "math::ROOT3", .type = E_VARTYPE_FLOAT, .value = { .f = 1.7320508075688772 } },
  { .name = "math::ROOT5", .type = E_VARTYPE_FLOAT, .value = { .f = 2.2360679774997896 } },
  { .name = "math::LN2", .type = E_VARTYPE_FLOAT, .value = { .f = M_LN2 } },
  { .name = "math::LN10", .type = E_VARTYPE_FLOAT, .value = { .f = M_LN10 } },

  { .name = "int::MAX", .type = E_VARTYPE_INT, .value = { .i = INT32_MAX } },
  { .name = "int::MIN", .type = E_VARTYPE_INT, .value = { .i = INT32_MIN } },

  { .name = "float::MAX", .type = E_VARTYPE_FLOAT, .value = { .f = DBL_MAX } },
  { .name = "float::MIN", .type = E_VARTYPE_FLOAT, .value = { .f = DBL_MIN } },

  { .name = "fs::FILE", .type = E_VARTYPE_INT, .value = { .i = 0 } },
  { .name = "fs::LINK", .type = E_VARTYPE_INT, .value = { .i = 1 } },
  { .name = "fs::DIRECTORY", .type = E_VARTYPE_INT, .value = { .i = 2 } },
  { .name = "fs::UNKNOWN", .type = E_VARTYPE_INT, .value = { .i = -1 } },
};

#endif // E_BUILTIN_VARIABLES_H