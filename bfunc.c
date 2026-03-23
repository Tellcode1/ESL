#include "bfunc.h"

#include "refcount.h"
#include "var.h"

#include <stdlib.h>

static inline double
to_float(e_var v)
{
  switch (v.type) {
    case E_VARTYPE_FLOAT: return v.val.f;
    case E_VARTYPE_INT: return (double)v.val.i;
    case E_VARTYPE_CHAR: return (double)v.val.c;
    case E_VARTYPE_BOOL: return (double)v.val.b;
    default: return 0.0;
  }
}

static inline int
to_int(e_var v)
{
  switch (v.type) {
    case E_VARTYPE_INT: return v.val.i;
    case E_VARTYPE_FLOAT: return (int)v.val.f;
    case E_VARTYPE_CHAR: return (int)v.val.c;
    case E_VARTYPE_BOOL: return (int)v.val.b;
    default: return 0;
  }
}

static inline char
to_char(e_var v)
{
  switch (v.type) {
    case E_VARTYPE_INT: return (char)v.val.i;
    case E_VARTYPE_FLOAT: return (char)v.val.f;
    case E_VARTYPE_CHAR: return (char)v.val.c;
    case E_VARTYPE_BOOL: return (char)v.val.b;
    default: return 0;
  }
}

static inline bool
to_bool(e_var v)
{
  switch (v.type) {
    case E_VARTYPE_INT: return (bool)v.val.i;
    case E_VARTYPE_FLOAT: return (bool)v.val.f;
    case E_VARTYPE_CHAR: return (bool)v.val.c;
    case E_VARTYPE_BOOL: return (bool)v.val.b;
    default: return false;
  }
}

e_var
eb_print(e_var* args, u32 nargs)
{
  for (u32 i = 0; i < nargs; i++) e_var_print(&args[i], stdout);
  return (e_var){ .type = E_VARTYPE_VOID };
}

e_var
eb_cast_int(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type = E_VARTYPE_INT, .refc = e_refc_init(), .val.i = to_int(args[0]) };
}

e_var
eb_cast_char(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type = E_VARTYPE_CHAR, .refc = e_refc_init(), .val.c = (char)to_int(args[0]) };
}

e_var
eb_cast_bool(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type = E_VARTYPE_BOOL, .refc = e_refc_init(), .val.b = (bool)to_int(args[0]) };
}

e_var
eb_cast_float(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type = E_VARTYPE_FLOAT, .refc = e_refc_init(), .val.f = to_float(args[0]) };
}

e_var
eb_cast_string(e_var* args, u32 nargs)
{
  (void)nargs;

  size_t len = e_var_to_string_size(&args[0]);
  char*  s   = malloc(len + 1);
  s[len]     = 0;

  e_var_to_string(&args[0], s, len + 1);

  struct e_string* estring = malloc(sizeof(struct e_string));

  estring->s = s;

  return (e_var){ .type = E_VARTYPE_STRING, .refc = e_refc_init(), .val.s = estring };
}

e_var
eb_cast_list(e_var* args, u32 nargs)
{
  e_var new_list = {
    .type     = E_VARTYPE_LIST,
    .refc     = e_refc_init(),
    .val.list = calloc(1, sizeof(e_list)),
  };

  e_list_init(args, nargs, new_list.val.list); // acquires the elements. Stack frees them later.

  return new_list;
}
