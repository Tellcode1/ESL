#ifndef E_BUILTIN_VARIABLES_H
#define E_BUILTIN_VARIABLES_H

#include "var.h"

#include <math.h>

/**
 * Builtin variables structure.
 * All variables within are loaded by the compiler ONCE at
 * start of initialization.
 * Any occurence of these variables are replaced with their values.
 */
typedef struct e_builtin_var {
  const char* name;
  e_vartype   type;
  e_varval    value;
} e_builtin_var;

/**
 * List is not checked for conflicting entries!
 * All variables must have constant, compile time (literal) values.
 */
static const e_builtin_var eb_vars[] = {
  { .name = "math::PI", .type = E_VARTYPE_FLOAT, .value = { .f = M_PI } },
  { .name = "math::PIby2", .type = E_VARTYPE_FLOAT, .value = { .f = M_PI_2 } },
  { .name = "math::PIby4", .type = E_VARTYPE_FLOAT, .value = { .f = M_PI_4 } },
  { .name = "math::PIx2", .type = E_VARTYPE_FLOAT, .value = { .f = M_PI * 2.0 } },
  { .name = "math::tau", .type = E_VARTYPE_FLOAT, .value = { .f = M_PI * 2.0 } },
  { .name = "math::phi", .type = E_VARTYPE_FLOAT, .value = { .f = 1.6180339887498948 } }, // golden ratio
  { .name = "math::e", .type = E_VARTYPE_FLOAT, .value = { .f = M_E } },
  { .name = "math::root2", .type = E_VARTYPE_FLOAT, .value = { .f = M_SQRT2 } },
  { .name = "math::root3", .type = E_VARTYPE_FLOAT, .value = { .f = 1.7320508075688772 } },
  { .name = "math::root5", .type = E_VARTYPE_FLOAT, .value = { .f = 2.2360679774997896 } },
  { .name = "math::ln2", .type = E_VARTYPE_FLOAT, .value = { .f = M_LN2 } },
  { .name = "math::ln10", .type = E_VARTYPE_FLOAT, .value = { .f = M_LN10 } },
};

#endif // E_BUILTIN_VARIABLES_H