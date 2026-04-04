#include "bfunc.h"

#include "bvar.h"
#include "pool.h"
#include "var.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

FILE*
file_from_var(const e_var* v)
{
  FILE* f = nullptr;
  if (v->type == E_VARTYPE_INT) {
    if (v->val.i == EB_IO_STDOUT) {
      f = stdout;
    } else if (v->val.i == EB_IO_STDIN) {
      f = stdin;
    } else if (v->val.i == EB_IO_STDERR) {
      f = stderr;
    } else {
      f = (FILE*)v->val.generic_ptr;
    }
  }
  return f;
}

e_var
var_from_file(FILE* f)
{
  e_var yay           = { .type = E_VARTYPE_INT };
  yay.val.generic_ptr = (void*)f;
  return yay;
}

e_var
eb_io_read(e_var* args, u32 nargs)
{
  (void)nargs;

  FILE* f = file_from_var(&args[0]);
  if (!f) return (e_var){ .type = E_VARTYPE_NULL };

  int   nbytes = args[1].val.i;
  char* s      = malloc(nbytes + 1);
  memset(s, 0, nbytes + 1);

  size_t nread = fread(s, 1, nbytes, f);
  s[nread]     = 0;

  e_var v = {
    .type  = E_VARTYPE_STRING,
    .val.s = e_refdobj_pool_acquire(&ge_pool),
  };
  E_VAR_AS_STRING(&v)->s = s;

  return v;
}

e_var
eb_io_write(e_var* args, u32 nargs)
{
  (void)nargs;

  FILE* f = file_from_var(&args[0]);
  if (!f) return (e_var){ .type = E_VARTYPE_NULL };

  char* s = E_VAR_AS_STRING(&args[1])->s;
  fwrite(s, 1, strlen(s), f);

  return (e_var){ .type = E_VARTYPE_NULL };
}

e_var
eb_io_seek(e_var* args, u32 nargs)
{
  (void)nargs;

  FILE* f = file_from_var(&args[0]);
  if (!f) return (e_var){ .type = E_VARTYPE_NULL };

  int offset      = args[1].val.i;
  int relative_to = args[2].val.i;

  int c_rel = 0;
  switch (relative_to) {
    case EB_IO_REL_TO_START: c_rel = SEEK_SET; break;
    case EB_IO_REL_TO_CURR: c_rel = SEEK_CUR; break;
    case EB_IO_REL_TO_END: c_rel = SEEK_END; break;
  }

  fseek(f, offset, c_rel);

  return (e_var){ .type = E_VARTYPE_NULL };
}

e_var
eb_io_ptell(e_var* args, u32 nargs)
{
  (void)nargs;

  FILE* f = file_from_var(&args[0]);
  if (!f) return (e_var){ .type = E_VARTYPE_NULL };

  return (e_var){ .type = E_VARTYPE_INT, .val.i = (int)ftell(f) };
}

e_var
eb_io_readln(e_var* args, u32 nargs)
{
  (void)args;
  (void)nargs;

  FILE* f = file_from_var(&args[0]);
  if (!f) return (e_var){ .type = E_VARTYPE_NULL };

  return e_make_var_from_string(e_read_full_line(f));
}

e_var
eb_io_println(e_var* args, u32 nargs)
{
  FILE* f = file_from_var(&args[0]);
  if (!f) return (e_var){ .type = E_VARTYPE_NULL };

  for (u32 i = 1; i < nargs; i++) { e_var_print(&args[i], f); }
  fputc('\n', f);

  return (e_var){ .type = E_VARTYPE_NULL };
}

e_var
eb_io_print(e_var* args, u32 nargs)
{
  FILE* f = file_from_var(&args[0]);
  if (!f) return (e_var){ .type = E_VARTYPE_NULL };

  for (u32 i = 1; i < nargs; i++) { e_var_print(&args[i], f); }

  return (e_var){ .type = E_VARTYPE_NULL };
}

e_var
eb_io_getc(e_var* args, u32 nargs)
{
  (void)args;
  (void)nargs;

  FILE* f = file_from_var(&args[0]);
  if (!f) return (e_var){ .type = E_VARTYPE_NULL };

  int i = fgetc(f);

  return (e_var){
    .type  = i < 0 ? E_VARTYPE_NULL : E_VARTYPE_CHAR,
    .val.c = (char)i,
  };
}

e_var
eb_io_putc(e_var* args, u32 nargs)
{
  (void)args;
  (void)nargs;

  FILE* f = file_from_var(&args[0]);
  if (!f) return (e_var){ .type = E_VARTYPE_NULL };

  int ch = evar_to_int(args[1]);
  fputc(ch, f);

  return (e_var){
    .type = E_VARTYPE_NULL,
  };
}

e_var
eb_io_at_eof(e_var* args, u32 nargs)
{
  (void)nargs;
  FILE* f = file_from_var(&args[0]);
  if (!f) return (e_var){ .type = E_VARTYPE_NULL };

  return (e_var){ .type = E_VARTYPE_BOOL, .val.b = (bool)feof(f) };
}

e_var
eb_io_open(e_var* args, u32 nargs)
{
  (void)nargs;
  const char* path = E_VAR_AS_STRING(&args[0])->s;
  const char* mode = E_VAR_AS_STRING(&args[1])->s;

  FILE* f = fopen(path, mode);
  if (!f) { return (e_var){ .type = E_VARTYPE_NULL }; }

  return var_from_file(f);
}

e_var
eb_io_close(e_var* args, u32 nargs)
{
  (void)nargs;
  FILE* f = file_from_var(&args[0]);
  if (!f) return (e_var){ .type = E_VARTYPE_NULL };

  fclose(f);
  return (e_var){ .type = E_VARTYPE_NULL };
}

e_var
eb_io_error(e_var* args, u32 nargs)
{
  (void)args;
  (void)nargs;
  return e_make_var_from_string(strdup(strerror(errno)));
}
