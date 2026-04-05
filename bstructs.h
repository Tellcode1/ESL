#ifndef E_BUILTIN_STRUCT_H
#define E_BUILTIN_STRUCT_H

#include "stdafx.h"

typedef struct e_builtin_struct {
  const char*  name;
  const char** fields;
  u32          fields_count;
} e_builtin_struct;

static const e_builtin_struct eb_structs[] = {
  {
      .name         = "vec2",
      .fields       = (const char*[]){ "x", "y" },
      .fields_count = 2,
  },
  {
      .name         = "vec3",
      .fields       = (const char*[]){ "x", "y", "z" },
      .fields_count = 3,
  },
  {
      .name         = "vec4",
      .fields       = (const char*[]){ "x", "y", "z", "w" },
      .fields_count = 4,
  },

  {
      .name         = "mat3",
      .fields       = (const char*[]){ "x", "y", "z" },
      .fields_count = 3,
  },
  {
      .name         = "mat4",
      .fields       = (const char*[]){ "x", "y", "z", "w" },
      .fields_count = 4,
  },
};

#endif // E_BUILTIN_STRUCT_H