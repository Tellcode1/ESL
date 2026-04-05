#include "bfunc.h"

#include "list.h"
#include "pool.h"
#include "sysexpose.h"
#include "var.h"

#include <string.h>

char** e_argv = nullptr;
int    e_argc = 0;

e_var
eb_get_command_line_args(e_var* args, u32 nargs)
{
  (void)args;
  (void)nargs;

  e_var l = {
    .type     = E_VARTYPE_LIST,
    .val.list = e_refdobj_pool_acquire(&ge_pool),
  };
  e_list_init(NULL, 0, E_VAR_AS_LIST(&l));

  for (u32 i = 0; i < e_argc; i++) {
    e_var arg = e_make_var_from_string(e_strdup(e_argv[i]));
    e_list_append(&arg, E_VAR_AS_LIST(&l));
  }

  return l;
}