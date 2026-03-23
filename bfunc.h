#ifndef E_BUILTIN_FUNCTIONS_H
#define E_BUILTIN_FUNCTIONS_H

#include "cerr.h"
#include "refcount.h"
#include "stdafx.h"
#include "var.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct e_builtin_func {
  /**
   * Function name in script.
   * Not checked for conflicts. First one is used.
   */
  const char* name;

  /**
   * Variable types that are allowed.
   * Excludes VOID always. You can not pass void as a variable.
   * If a variable does not have a bit set in the mask,
   * The compiler will fail and error out.
   */
  e_vartype allowed_mask;

  u32 min_args;

  u32 max_args;

  /**
   * Returned variables must have a reference counter
   * initialized to 1, except void.
   */
  e_var (*func)(e_var* args, u32 nargs);
} e_builtin_func;

e_var eb_print(e_var* args, u32 nargs);

static inline e_var
eb_println(e_var* args, u32 nargs)
{
  eb_print(args, nargs);
  fputc('\n', stdout);
  return (e_var){ .type = E_VARTYPE_VOID };
}

e_var eb_cast_int(e_var* args, u32 nargs);
e_var eb_cast_char(e_var* args, u32 nargs);
e_var eb_cast_bool(e_var* args, u32 nargs);
e_var eb_cast_float(e_var* args, u32 nargs);
e_var eb_cast_string(e_var* args, u32 nargs);
e_var eb_cast_list(e_var* args, u32 nargs);

static inline e_var
eb_len(e_var* args, u32 nargs)
{
  (void)nargs;

  int len = 0;
  if (args[0].type == E_VARTYPE_STRING) {
    len = (int)strlen(args[0].val.s->s);
  } else if (args[0].type == E_VARTYPE_LIST) {
    len = (int)args[0].val.list->size;
  }
  return (e_var){ .type = E_VARTYPE_INT, .refc = e_refc_init(), .val = { .i = len } };
}

/**
 * Allow all arguments.
 */
static inline bool
eb_argval_passthrough(const e_var* args, u32 nargs, const struct e_filespan* span, FILE* errf)
{
  (void)args;
  (void)nargs;
  (void)span;
  (void)errf;
  return false;
}

static inline bool
eb_argval_scalars_only(const e_var* args, u32 nargs, const struct e_filespan* span, FILE* errf)
{
  for (u32 i = 0; i < nargs; i++) {
    if (args[i].type == E_VARTYPE_MAP || args[i].type == E_VARTYPE_STRING || args[i].type == E_VARTYPE_LIST) {
      fprintf(errf, "[%s:%i:%i] Function only expects scalar types (%s given)\n", span->file, span->line, span->col, e_var_type_to_string(args[i].type));
      return true;
    }
  }
  return false;
}

static inline bool
eb_argval_primitives_only(const e_var* args, u32 nargs, const struct e_filespan* span, FILE* errf)
{
  for (u32 i = 0; i < nargs; i++) {
    if (args[i].type == E_VARTYPE_MAP || args[i].type == E_VARTYPE_LIST) {
      fprintf(errf, "[%s:%i:%i] Function only expects primitive types\n", span->file, span->line, span->col);

      return true;
    }
  }
  return false;
}

static inline void
e_var_type_bitmask_to_string(e_vartype mask, char* buffer, size_t buffer_size)
{
  strlcpy(buffer, "[", buffer_size);

  if ((bool)(mask & E_VARTYPE_INT)) strlcat(buffer, "int,", buffer_size);
  if ((bool)(mask & E_VARTYPE_BOOL)) strlcat(buffer, "bool,", buffer_size);
  if ((bool)(mask & E_VARTYPE_CHAR)) strlcat(buffer, "char,", buffer_size);
  if ((bool)(mask & E_VARTYPE_FLOAT)) strlcat(buffer, "float,", buffer_size);
  if ((bool)(mask & E_VARTYPE_STRING)) strlcat(buffer, "string,", buffer_size);
  if ((bool)(mask & E_VARTYPE_LIST)) strlcat(buffer, "list,", buffer_size);
  if ((bool)(mask & E_VARTYPE_MAP)) strlcat(buffer, "map,", buffer_size);
  if ((bool)(mask & E_VARTYPE_ERROR)) strlcat(buffer, "error,", buffer_size);

  /* Set last pipe to NUL */
  char* last_comma = strrchr(buffer, ',');
  if (last_comma != nullptr) *last_comma = 0;

  strlcat(buffer, "]", buffer_size);
}

static const e_builtin_func eb_funcs[] = {
  /* Can print anything. */
  { "print", 0xFFFFFFFF, 1, UINT32_MAX, eb_print },
  { "println", 0xFFFFFFFF, 1, UINT32_MAX, eb_println },

  /* Can convert anything to string. */
  { "string", 0xFFFFFFFF, 1, 1, eb_cast_string },

  /* Scalar types */
  { "int", E_VARTYPE_INT | E_VARTYPE_CHAR | E_VARTYPE_BOOL | E_VARTYPE_FLOAT | E_VARTYPE_STRING, 1, 1, eb_cast_int },
  { "char", E_VARTYPE_INT | E_VARTYPE_CHAR | E_VARTYPE_BOOL | E_VARTYPE_FLOAT | E_VARTYPE_STRING, 1, 1, eb_cast_char },
  { "bool", E_VARTYPE_INT | E_VARTYPE_CHAR | E_VARTYPE_BOOL | E_VARTYPE_FLOAT | E_VARTYPE_STRING, 1, 1, eb_cast_bool },
  { "float", E_VARTYPE_INT | E_VARTYPE_CHAR | E_VARTYPE_BOOL | E_VARTYPE_FLOAT | E_VARTYPE_STRING, 1, 1, eb_cast_float },

  /* Anything can be added as an element to a list */
  { "list", 0xFFFFFF, 1, UINT32_MAX, eb_cast_list },

  { "len", E_VARTYPE_STRING | E_VARTYPE_LIST, 1, 1, eb_len }
};

#endif // E_BUILTIN_FUNCTIONS_H
