#include "bfunc.h"

#include "operate.h"
#include "pool.h"
#include "var.h"

#include <assert.h>

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
  (void)nargs;

  e_var m = {
    .type     = E_VARTYPE_MAT3,
    .val.mat3 = e_refdobj_pool_acquire(&ge_pool),
  };
  for (u32 i = 0; i < 3; i++) {
    memcpy(E_VAR_AS_MAT3(&m)->m[i], &args[i].val.vec3, sizeof(e_vec3));
    assert(args[i].type == E_VARTYPE_VEC3);
  }
  return m;
}

e_var
eb_mat4(e_var* args, u32 nargs)
{
  (void)nargs;

  e_var m = {
    .type     = E_VARTYPE_MAT4,
    .val.mat4 = e_refdobj_pool_acquire(&ge_pool),
  };
  for (u32 i = 0; i < 4; i++) {
    memcpy(E_VAR_AS_MAT4(&m)->m[i], &args[i].val.vec4, sizeof(e_vec4));
    assert(args[i].type == E_VARTYPE_VEC4);
  }
  return m;
}

e_var
evector_zero_extend(const e_var* v)
{
  if (v->type == E_VARTYPE_VEC2) { return e_make_vec4(v->val.vec2.x, v->val.vec2.y, 0.0, 0.0); }
  if (v->type == E_VARTYPE_VEC3) { return e_make_vec4(v->val.vec3.x, v->val.vec3.y, v->val.vec3.z, 0.0); }
  if (v->type == E_VARTYPE_VEC4) { return e_make_vec4(v->val.vec4.x, v->val.vec4.y, v->val.vec4.z, v->val.vec4.w); }
  return (e_var){ .type = E_VARTYPE_NULL };
}
