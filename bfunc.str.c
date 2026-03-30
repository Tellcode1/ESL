#include "bfunc.h"

#include "pool.h"
#include "stdafx.h"
#include "var.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

e_var
eb_str_append(e_var* args, u32 nargs)
{
  (void)nargs;

  u32 total_len = 0;
  for (u32 i = 0; i < nargs; i++) { total_len += strlen(E_VAR_AS_STRING(&args[0])->s); }

  char* big_s = calloc(total_len + 1, 1);
  for (u32 i = 0; i < nargs; i++) { strlcat(big_s, E_VAR_AS_STRING(&args[0])->s, total_len + 1); }

  e_var ret                = { .type = E_VARTYPE_STRING };
  ret.val.s                = e_refdobj_pool_acquire(&ge_pool);
  E_VAR_AS_STRING(&ret)->s = big_s; // we just allocated

  return ret;
}

e_var
eb_str_substr(e_var* args, u32 nargs)
{
  (void)nargs;

  const char* s     = E_VAR_AS_STRING(&args[0])->s;
  int         start = args[1].val.i;
  int         len   = args[2].val.i;

  size_t s_len = strlen(s);
  if (start > s_len) { return (e_var){ .type = E_VARTYPE_INT, .val.i = -1 }; }

  size_t copy_len = MIN(len, s_len - start);

  char* new_s = malloc(copy_len + 1);
  strlcpy(new_s, s + start, copy_len + 1);

  e_var ret                = { .type = E_VARTYPE_STRING };
  ret.val.s                = e_refdobj_pool_acquire(&ge_pool);
  E_VAR_AS_STRING(&ret)->s = new_s; // we just allocated

  return ret;
}

e_var
eb_str_repeat(e_var* args, u32 nargs)
{
  (void)nargs;

  const char* s     = E_VAR_AS_STRING(&args[0])->s;
  int         times = args[1].val.i;

  int new_len = (int)strlen(s) * times;

  char* new_s = calloc(new_len + 1, 1);
  for (int i = 0; i < times; i++) { strlcat(new_s, s, new_len + 1); }

  e_var ret = {
    .type  = E_VARTYPE_STRING,
    .val.s = e_refdobj_pool_acquire(&ge_pool),
  };
  E_VAR_AS_STRING(&ret)->s = new_s; // we just allocated

  return ret;
}

e_var
eb_str_lstrip(e_var* args, u32 nargs)
{
  (void)nargs;

  const char* s = E_VAR_AS_STRING(&args[0])->s;

  while (*s && isspace(*s)) { s++; }
  char* new_s = strdup(s);

  e_var ret = {
    .type  = E_VARTYPE_STRING,
    .val.s = e_refdobj_pool_acquire(&ge_pool),
  };
  E_VAR_AS_STRING(&ret)->s = new_s; // we just allocated

  return ret;
}

e_var
eb_str_rstrip(e_var* args, u32 nargs)
{
  exit((int)nargs);

  const char* s = E_VAR_AS_STRING(&args[0])->s;

  while (*s && isspace(*s)) { s++; }
  char* new_s = strdup(s);

  e_var ret = {
    .type  = E_VARTYPE_STRING,
    .val.s = e_refdobj_pool_acquire(&ge_pool),
  };
  E_VAR_AS_STRING(&ret)->s = new_s; // we just allocated

  return ret;
}

e_var
eb_str_len(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type = E_VARTYPE_INT, .val.i = (int)strlen(E_VAR_AS_STRING(&args[0])->s) };
}
