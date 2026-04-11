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

#ifndef E_MATH_BUILTIN_FUNCTIONS_H
#define E_MATH_BUILTIN_FUNCTIONS_H

#include "var.h"

#include <math.h>

// clang-format off
static inline e_var eb_sin(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = sin(evar_to_float(args[0]))}}; }
static inline e_var eb_cos(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = cos(evar_to_float(args[0]))}}; }
static inline e_var eb_tan(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = tan(evar_to_float(args[0]))}}; }
static inline e_var eb_asin(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = asin(evar_to_float(args[0]))}}; }
static inline e_var eb_acos(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = acos(evar_to_float(args[0]))}}; }
static inline e_var eb_atan(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = atan(evar_to_float(args[0]))}}; }
static inline e_var eb_atan2(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = atan2(evar_to_float(args[0]),evar_to_float(args[1]))}}; }
static inline e_var eb_sinh(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = sinh(evar_to_float(args[0]))}}; }
static inline e_var eb_cosh(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = cosh(evar_to_float(args[0]))}}; }
static inline e_var eb_tanh(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = tanh(evar_to_float(args[0]))}}; }
static inline e_var eb_acosh(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = acosh(evar_to_float(args[0]))}}; }
static inline e_var eb_asinh(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = asinh(evar_to_float(args[0]))}}; }
static inline e_var eb_atanh(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = atanh(evar_to_float(args[0]))}}; }
static inline e_var eb_exp(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = exp(evar_to_float(args[0]))}}; }
static inline e_var eb_log(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = log(evar_to_float(args[0]))}}; }
static inline e_var eb_log10(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = log10(evar_to_float(args[0]))}}; }
static inline e_var eb_pow(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = pow(evar_to_float(args[0]), evar_to_float(args[1]))}}; }
static inline e_var eb_sqrt(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = sqrt(evar_to_float(args[0]))}}; }
static inline e_var eb_ceil(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = ceil(evar_to_float(args[0]))}}; }
static inline e_var eb_floor(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = floor(evar_to_float(args[0]))}}; }
static inline e_var eb_fmod(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = fmod(evar_to_float(args[0]),evar_to_float(args[1]))}}; }
static inline e_var eb_round(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = round(evar_to_float(args[0]))}}; }
static inline e_var eb_trunc(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = trunc(evar_to_float(args[0]))}}; }
static inline e_var eb_abs(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = fabs(evar_to_float(args[0]))}}; }
static inline e_var eb_hypot(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = hypot(evar_to_float(args[0]), evar_to_float(args[1]))}}; }
static inline e_var eb_signbit(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_INT, .val = {.i = signbit(evar_to_float(args[0]))}}; }
// clang-format on

#endif // E_MATH_BUILTIN_FUNCTIONS_H