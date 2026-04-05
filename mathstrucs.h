#ifndef E_MATH_STRUCTURES_H
#define E_MATH_STRUCTURES_H

/* inline */
typedef struct e_vec2 {
  double x, y;
} e_vec2;
typedef struct e_vec3 {
  double x, y, z;
} e_vec3;
typedef struct e_vec4 {
  double x, y, z, w;
} e_vec4;

/* Ref counted because they are bigly big */
typedef struct e_mat3 {
  double m[3][3];
} e_mat3;
typedef struct e_mat4 {
  double m[4][4];
} e_mat4;

#endif // E_MATH_STRUCTURES_H
