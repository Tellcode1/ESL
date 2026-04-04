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

#ifndef E_ARRLEN
#  define E_ARRLEN(array) (sizeof(array) / sizeof(*array))
#endif

#ifndef E_ARR_ALLOC
#  define E_ARR_ALLOC(elem_type, n) ((elem_type*)calloc(sizeof(elem_type), (n)))
#endif

#ifndef E_ARR_REALLOC
#  define E_ARR_REALLOC(arr, elem_type, n) ((elem_type*)realloc(arr, sizeof(*arr) * (n)))
#endif

#ifndef E_ARR_FREE
#  define E_ARR_FREE(arr) (free((void*)arr))
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

static inline u32
e_hash_fnv(const void* data, size_t size)
{
  const uchar* current = (const uchar*)data;

  u32 hash = 2166136261U;
  for (size_t i = 0; i < size; ++i) {
    hash ^= current[i];
    hash *= 16777619U;
  }

  return hash;
}

// static inline void*
// allocator_callback(const char* file, size_t line, size_t size)
// {
//   printf("[%s:%zu] %zu bytes allocated\n", file, line, size);
//   memory_usage += size;
//   return calloc(size, 1);
// }
// #define calloc(size, n) allocator_callback(__FILE__, __LINE__, (size) * (n))
// #define malloc(size) allocator_callback(__FILE__, __LINE__, (size))

static inline size_t
_strlcat(char* d, const char* s, size_t dsize)
{
  size_t dl = strlen(d);
  size_t sl = strlen(s);
  size_t i;

  if (dsize <= dl) { return dsize + sl; }

  for (i = 0; i < sl && (dl + i) < dsize - 1; i++) { d[dl + i] = s[i]; }
  d[dl + i] = 0;

  return dl + sl;
}

static inline char*
_strlpcat(char* d, const char* s, const char* dabs, size_t dsize)
{
  size_t off = d - dabs;
  if (off >= dsize) return d;
  dsize -= off;

  size_t dl = strlen(d);
  size_t sl = strlen(s);
  size_t i;

  if (dsize <= dl) { return d; }

  for (i = 0; i < sl && (dl + i) < dsize - 1; i++) { d[dl + i] = s[i]; }
  d[dl + i] = 0;

  return &d[dl + i];
}

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