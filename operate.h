#ifndef E_OPERATE_H
#define E_OPERATE_H

#include "bc.h"
#include "bfunc.h"
#include "var.h"

#define BINOP(l, r, op)                                                                                                                              \
  (l.type == E_VARTYPE_FLOAT || r.type == E_VARTYPE_FLOAT) ? (e_var){ .type = E_VARTYPE_FLOAT, .val.f = (double)l.val.f op(double) r.val.f }         \
                                                           : (e_var)                                                                                 \
  { .type = E_VARTYPE_INT, .val.i = l.val.i op r.val.i }
#define BOOLEAN_BINOP(l, r, op)                                                                                                                      \
  (l.type == E_VARTYPE_FLOAT || r.type == E_VARTYPE_FLOAT) ? (e_var){ .type = E_VARTYPE_BOOL, .val.b = (double)l.val.f op(double) r.val.f }          \
                                                           : (e_var)                                                                                 \
  { .type = E_VARTYPE_BOOL, .val.b = l.val.i op r.val.i }

static inline bool
is_float(e_var v)
{ return v.type == E_VARTYPE_FLOAT; }

#define COERCE_BINOP(l, r, op)                                                                                                                       \
  (is_float(l) || is_float(r)) ? (e_var){ .type = E_VARTYPE_FLOAT, .val.f = evar_to_float(l) op evar_to_float(r) } : (e_var)                         \
  { .type = E_VARTYPE_INT, .val.i = evar_to_int(l) op evar_to_int(r) }

#define COERCE_BOOLEAN_BINOP(l, r, op)                                                                                                               \
  (is_float(l) || is_float(r)) ? (e_var){ .type = E_VARTYPE_BOOL, .val.b = evar_to_float(l) op evar_to_float(r) } : (e_var)                          \
  { .type = E_VARTYPE_BOOL, .val.b = evar_to_int(l) op evar_to_int(r) }

static inline bool
is_vector(e_var v)
{ return (bool)(v.type == E_VARTYPE_VEC2 || v.type == E_VARTYPE_VEC3 || v.type == E_VARTYPE_VEC4); }

static inline bool
is_scalar(e_var v)
{ return (bool)(v.type == E_VARTYPE_INT || v.type == E_VARTYPE_FLOAT || v.type == E_VARTYPE_CHAR || v.type == E_VARTYPE_BOOL); }

static inline e_var
e_make_vec4(double x, double y, double z, double w)
{
  e_var out    = { 0 };
  out.type     = E_VARTYPE_VEC4;
  out.val.vec4 = (e_vec4){ .x = x, .y = y, .z = z, .w = w };
  return out;
}

static inline e_var
e_make_vec3(double x, double y, double z)
{
  e_var out    = { 0 };
  out.type     = E_VARTYPE_VEC3;
  out.val.vec3 = (e_vec3){ .x = x, .y = y, .z = z };
  return out;
}

static inline e_var
e_make_vec2(double x, double y)
{
  e_var out    = { 0 };
  out.type     = E_VARTYPE_VEC2;
  out.val.vec4 = (e_vec4){ .x = x, .y = y };
  return out;
}

static inline e_var
v4_operate(e_var l, e_var r, e_opcode op)
{
  if (l.type != E_VARTYPE_VEC4 && r.type != E_VARTYPE_VEC4) return (e_var){ 0 };

  e_vec4 lv = { 0 };
  e_vec4 rv = { 0 };

  bool lvec = l.type == E_VARTYPE_VEC4;
  bool rvec = r.type == E_VARTYPE_VEC4;

  if (lvec) lv = l.val.vec4;
  if (rvec) rv = r.val.vec4;

  switch (op) {
    case E_OPCODE_ADD:
      if (lvec && rvec) return e_make_vec4(lv.x + rv.x, lv.y + rv.y, lv.z + rv.z, lv.w + rv.w);
      break;

    case E_OPCODE_SUB:
      if (lvec && rvec) return e_make_vec4(lv.x - rv.x, lv.y - rv.y, lv.z - rv.z, lv.w - rv.w);
      break;

    case E_OPCODE_MUL:
      if (lvec && is_scalar(r)) {
        double s = evar_to_float(r);
        return e_make_vec4(lv.x * s, lv.y * s, lv.z * s, lv.w * s);
      }
      if (rvec && is_scalar(l)) {
        double s = evar_to_float(l);
        return e_make_vec4(rv.x * s, rv.y * s, rv.z * s, rv.w * s);
      }
      break;

    case E_OPCODE_DIV:
      if (lvec && is_scalar(r)) {
        double s = evar_to_float(r);
        return e_make_vec4(lv.x / s, lv.y / s, lv.z / s, lv.w / s);
      }
      break;

    case E_OPCODE_NEG:
      if (rvec) return e_make_vec4(-rv.x, -rv.y, -rv.z, -rv.w);
      break;

    default: break;
  }

  return (e_var){ 0 };
}

static inline e_var
v3_operate(e_var l, e_var r, e_opcode op)
{
  if (l.type != E_VARTYPE_VEC3 && r.type != E_VARTYPE_VEC3) return (e_var){ .type = E_VARTYPE_NULL };

  e_vec3 lv = { 0 };
  e_vec3 rv = { 0 };

  bool lvec = l.type == E_VARTYPE_VEC3;
  bool rvec = r.type == E_VARTYPE_VEC3;

  if (lvec) lv = l.val.vec3;
  if (rvec) rv = r.val.vec3;

  switch (op) {
    case E_OPCODE_ADD:
      if (lvec && rvec) return e_make_vec3(lv.x + rv.x, lv.y + rv.y, lv.z + rv.z);
      break;

    case E_OPCODE_SUB:
      if (lvec && rvec) return e_make_vec3(lv.x - rv.x, lv.y - rv.y, lv.z - rv.z);
      break;

    case E_OPCODE_MUL:
      if (lvec && is_scalar(r)) {
        double s = evar_to_float(r);
        return e_make_vec3(lv.x * s, lv.y * s, lv.z * s);
      }
      if (rvec && is_scalar(l)) {
        double s = evar_to_float(l);
        return e_make_vec3(rv.x * s, rv.y * s, rv.z * s);
      }
      break;

    case E_OPCODE_DIV:
      if (lvec && is_scalar(r)) {
        double s = evar_to_float(r);
        return e_make_vec3(lv.x / s, lv.y / s, lv.z / s);
      }
      break;

    case E_OPCODE_NEG:
      if (rvec) return e_make_vec3(-rv.x, -rv.y, -rv.z);
      break;

    default: break;
  }

  return (e_var){ .type = E_VARTYPE_NULL };
}

static inline e_var
v2_operate(e_var l, e_var r, e_opcode op)
{
  if (l.type != E_VARTYPE_VEC2 && r.type != E_VARTYPE_VEC2) return (e_var){ .type = E_VARTYPE_NULL };

  e_vec3 lv = { 0 };
  e_vec3 rv = { 0 };

  bool lhs_is_vec = l.type == E_VARTYPE_VEC2;
  bool rhs_is_vec = r.type == E_VARTYPE_VEC2;

  if (lhs_is_vec) lv = l.val.vec3;
  if (rhs_is_vec) rv = r.val.vec3;

  switch (op) {
    case E_OPCODE_ADD:
      if (lhs_is_vec && rhs_is_vec) return e_make_vec2(lv.x + rv.x, lv.y + rv.y);
      break;

    case E_OPCODE_SUB:
      if (lhs_is_vec && rhs_is_vec) return e_make_vec2(lv.x - rv.x, lv.y - rv.y);
      break;

    case E_OPCODE_MUL:
      if (lhs_is_vec && is_scalar(r)) {
        double s = evar_to_float(r);
        return e_make_vec2(lv.x * s, lv.y * s);
      }
      if (rhs_is_vec && is_scalar(l)) {
        double s = evar_to_float(l);
        return e_make_vec2(rv.x * s, rv.y * s);
      }
      break;

    case E_OPCODE_DIV:
      if (lhs_is_vec && is_scalar(r)) {
        double s = evar_to_float(r);
        return e_make_vec2(lv.x / s, lv.y / s);
      }
      break;

    case E_OPCODE_NEG:
      if (rhs_is_vec) return e_make_vec2(-rv.x, -rv.y);
      break;

    default: break;
  }

  return (e_var){ .type = E_VARTYPE_NULL };
}

static inline e_var
vector_operate(e_var l, e_var r, e_opcode op)
{
  if (l.type == E_VARTYPE_VEC4 || r.type == E_VARTYPE_VEC4) { return v4_operate(l, r, op); }
  if (l.type == E_VARTYPE_VEC3 || r.type == E_VARTYPE_VEC3) { return v3_operate(l, r, op); }
  if (l.type == E_VARTYPE_VEC2 || r.type == E_VARTYPE_VEC2) { return v2_operate(l, r, op); }
  return (e_var){ .type = E_VARTYPE_NULL };
}

static inline e_var
operate(e_var l, e_var r, e_opcode op)
{
  if (is_vector(l) || is_vector(r)) { return vector_operate(l, r, op); }

  if (op == E_OPCODE_NOT) return (e_var){ .type = E_VARTYPE_BOOL, .val.b = (bool)!evar_to_bool(r) };

  switch (op) {
    case E_OPCODE_ADD: return COERCE_BINOP(l, r, +);
    case E_OPCODE_SUB: return COERCE_BINOP(l, r, -);
    case E_OPCODE_MUL: return COERCE_BINOP(l, r, *);
    case E_OPCODE_DIV: return COERCE_BINOP(l, r, /);
    case E_OPCODE_MOD: return (e_var){ .type = E_VARTYPE_INT, .val.i = evar_to_int(l) % evar_to_int(r) };
    case E_OPCODE_EQL: return (e_var){ .type = E_VARTYPE_BOOL, .val.b = e_var_equal(&l, &r) };
    case E_OPCODE_NEQ: return (e_var){ .type = E_VARTYPE_BOOL, .val.b = (bool)!e_var_equal(&l, &r) };
    case E_OPCODE_LT: return COERCE_BOOLEAN_BINOP(l, r, <);
    case E_OPCODE_LTE: return COERCE_BOOLEAN_BINOP(l, r, <=);
    case E_OPCODE_GT: return COERCE_BOOLEAN_BINOP(l, r, >);
    case E_OPCODE_GTE: return COERCE_BOOLEAN_BINOP(l, r, >=);
    case E_OPCODE_AND: return COERCE_BOOLEAN_BINOP(l, r, &&);
    case E_OPCODE_OR: return COERCE_BOOLEAN_BINOP(l, r, ||);
    case E_OPCODE_NEG:
      switch (r.type) {
        case E_VARTYPE_INT: return (e_var){ .type = E_VARTYPE_INT, .val.i = -r.val.i };
        case E_VARTYPE_CHAR: return (e_var){ .type = E_VARTYPE_CHAR, .val.c = (char)-(int)r.val.c };
        case E_VARTYPE_FLOAT: return (e_var){ .type = E_VARTYPE_FLOAT, .val.f = -r.val.f };
        default: break;
      }
    case E_OPCODE_BNOT: return (e_var){ .type = E_VARTYPE_INT, .val.i = ~evar_to_int(r) };
    case E_OPCODE_BAND: return (e_var){ .type = E_VARTYPE_INT, .val.i = evar_to_int(l) & evar_to_int(r) };
    case E_OPCODE_BOR: return (e_var){ .type = E_VARTYPE_INT, .val.i = evar_to_int(l) | evar_to_int(r) };
    case E_OPCODE_XOR: return (e_var){ .type = E_VARTYPE_INT, .val.i = evar_to_int(l) ^ evar_to_int(r) };
    default: break;
  }
  return (e_var){ 0 };
}

#endif // E_OPERATE_H