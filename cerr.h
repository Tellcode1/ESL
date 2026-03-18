#ifndef E_CERR_H
#define E_CERR_H

#include "../std/include/error.h"
#include "../std/include/types.h"

typedef struct e_filespan {
  char* file;
  int   line;
  int   col;
} e_filespan;

#endif // E_CERR_H
