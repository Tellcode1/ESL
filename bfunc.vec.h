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

#ifndef E_VECTOR_BUILTINS_H
#define E_VECTOR_BUILTINS_H

#include "mathstrucs.h"
#include "var.h"

#include <math.h>

static inline e_var
eb_vec_norm(e_var* args, u32 nargs)
{
  double len = evector_length(&args[0]).val.f;
  e_vec4 zx  = evector_zero_extend(&args[0]).val.vec4;
  (void)nargs;
  switch (args[0].type) {
    case E_VARTYPE_VEC2: return e_make_vec2(zx.x / len, zx.y / len);
    case E_VARTYPE_VEC3: return e_make_vec3(zx.x / len, zx.y / len, zx.x / len);
    case E_VARTYPE_VEC4: return e_make_vec4(zx.x / len, zx.y / len, zx.x / len, zx.w / len);
    default: break;
  }
  return (e_var){ .type = E_VARTYPE_NULL };
}

static inline e_var
eb_vec_len(e_var* args, u32 nargs)
{
  (void)nargs;
  switch (args[0].type) {
    case E_VARTYPE_VEC2:
    case E_VARTYPE_VEC3:
    case E_VARTYPE_VEC4: return evector_length(&args[0]);
    default: break;
  }
  return (e_var){ .type = E_VARTYPE_NULL };
}

static inline e_var
eb_vec_len2(e_var* args, u32 nargs)
{
  (void)nargs;
  e_vec4 zx = evector_zero_extend(&args[0]).val.vec4;

  switch (args[0].type) {
    case E_VARTYPE_VEC2: return e_var_from_float(zx.x * zx.x + zx.y * zx.y);
    case E_VARTYPE_VEC3: return e_var_from_float(zx.x * zx.x + zx.y * zx.y + zx.z * zx.z);
    case E_VARTYPE_VEC4: return e_var_from_float(zx.x * zx.x + zx.y * zx.y + zx.z * zx.z + zx.w * zx.w);
    default: break;
  }
  return (e_var){ .type = E_VARTYPE_NULL };
}

static inline e_var
eb_vec_dist(e_var* args, u32 nargs)
{
  (void)nargs;
  e_vec4 x = evector_zero_extend(&args[0]).val.vec4;
  e_vec4 y = evector_zero_extend(&args[0]).val.vec4;

  double xl = x.x - y.x;
  double yl = x.y - y.y;
  double zl = x.z - y.z; // 0 if not vec3, zero extended
  double wl = x.w - y.w; // 0 if not vec4, zero extended

  switch (args[0].type) {
    case E_VARTYPE_VEC2:
    case E_VARTYPE_VEC3:
    case E_VARTYPE_VEC4: return e_var_from_float(sqrt(xl + yl + zl + wl));
    default: break;
  }
  return (e_var){ .type = E_VARTYPE_NULL };
}

static inline e_var
eb_vec_dist2(e_var* args, u32 nargs)
{
  (void)nargs;
  e_vec4 x = evector_zero_extend(&args[0]).val.vec4;
  e_vec4 y = evector_zero_extend(&args[0]).val.vec4;

  double xl = x.x - y.x;
  double yl = x.y - y.y;
  double zl = x.z - y.z; // 0 if not vec3, zero extended
  double wl = x.w - y.w; // 0 if not vec4, zero extended

  switch (args[0].type) {
    case E_VARTYPE_VEC2:
    case E_VARTYPE_VEC3:
    case E_VARTYPE_VEC4: return e_var_from_float(xl + yl + zl + wl);
    default: break;
  }
  return (e_var){ .type = E_VARTYPE_NULL };
}

#endif // E_VECTOR_BUILTINS_H