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

#ifndef E_STDAFX_H
#define E_STDAFX_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef nullptr
#  define nullptr (0)
#endif

#ifndef MAX
#  define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#  define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef _UNREACHABLE
#  if defined(__has_builtin) && __has_builtin(__builtin_unreachable)
#    define _UNREACHABLE() __builtin_unreachable()
#  else
#    define _UNREACHABLE()
#  endif
#endif

#ifndef E_ARRLEN
#  define E_ARRLEN(array) (sizeof(array) / sizeof(*array))
#endif

typedef uint8_t uchar;

typedef uint8_t  uchar;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

static inline char*
read_file(const char* path, u64* size)
{
  FILE* f        = nullptr;
  char* contents = nullptr;
  long  fsize    = 0;

  f = fopen(path, "rb");
  if (f == nullptr) return nullptr;

  if ((bool)fseek(f, 0, SEEK_END)) goto CLEANUP;

  fsize = ftell(f);
  if (fsize <= 0) goto CLEANUP;

  if (size != nullptr) *size = (int)fsize;

  if ((bool)fseek(f, 0, SEEK_SET)) goto CLEANUP;

  contents = (char*)malloc(fsize + 1);
  if (fread(contents, fsize, 1, f) != 1) { goto CLEANUP; }
  contents[fsize] = 0;

  fclose(f);

  return contents;

CLEANUP:
  if (f != nullptr) fclose(f);
  if (contents != nullptr) free(contents);
  return nullptr;
}

#ifdef __cplusplus
}
#endif

#endif // E_STDAFX_H