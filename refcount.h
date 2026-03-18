#ifndef E_REF_COUNTER_H
#define E_REF_COUNTER_H

#include <stdlib.h>

typedef struct e_refc {
  int ctr;
} e_refc;

static inline e_refc*
e_refc_init()
{
  e_refc* ref = (e_refc*)malloc(sizeof(e_refc));
  ref->ctr    = 1;
  return ref;
}

static inline int
e_refc_acquire(e_refc* refc)
{
  return refc->ctr++;
}

static inline void
e_refc_release(e_refc* refc)
{
  if (refc->ctr > 0) refc->ctr--;
}

static inline void
e_refc_free(e_refc* refc)
{
  free(refc);
}

#endif // E_REF_COUNTER_H