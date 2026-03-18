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
read_file(const char* path, int* size)
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