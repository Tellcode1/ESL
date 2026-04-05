#include "bfunc.h"

e_var
eb_vec2(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type     = E_VARTYPE_VEC2,
                  .val.vec2 = {
                      .x = evar_to_float(args[0]),
                      .y = evar_to_float(args[1]),
                  } };
}

e_var
eb_vec3(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type     = E_VARTYPE_VEC3,
                  .val.vec3 = {
                      .x = evar_to_float(args[0]),
                      .y = evar_to_float(args[1]),
                      .z = evar_to_float(args[2]),
                  } };
}

e_var
eb_vec4(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type     = E_VARTYPE_VEC4,
                  .val.vec4 = {
                      .x = evar_to_float(args[0]),
                      .y = evar_to_float(args[1]),
                      .z = evar_to_float(args[2]),
                      .w = evar_to_float(args[3]),
                  } };
}

e_var
eb_mat3(e_var* args, u32 nargs)
{
}

e_var
eb_mat4(e_var* args, u32 nargs)
{
}
