#ifndef E_CERR_H
#define E_CERR_H

#include "stdafx.h"

#include <stdarg.h>

typedef struct e_filespan {
  char* file;
  int   line;
  int   col;
} e_filespan;

static inline int
_cerror(const char* file, size_t line, e_filespan span, const char* msg, ...)
{
  va_list ap;
  va_start(ap, msg);

  fprintf(stderr, "[%s:%zu] [%s:%i:%i] compilation error: ", file, line, span.file, span.line, span.col);
  vfprintf(stderr, msg, ap);

  va_end(ap);

  return 0;
}

#define cerror(span, ...) _cerror(__FILE__, __LINE__, span, __VA_ARGS__)

#endif // E_CERR_H
