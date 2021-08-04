/* Copyright 2021 Phasmatic
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _WRAYS_MATH_H_
#define _WRAYS_MATH_H_

#include <cmath>
#include <cstdint>

#define WR_PI_F 3.14159265359f
#define WR_2PI_F 6.28318530718f
#define WR_TWO_PI_F WR_2PI_F
#define WR_3PI_F 9.42477796077f
#define WR_4PI_F 12.5663706144f

#define WR_INV_PI_F 0.31830988618f
#define WR_INV_2PI_F 0.15915494309f
#define WR_INV_3PI_F 0.10610329539f
#define WR_INV_4PI_F 0.07957747154f

#define WR_PI_OVER_2_F 1.57079632679f
#define WR_PI_OVER_3_F 1.0471975512f
#define WR_PI_OVER_4_F 0.78539816339f

#define WR_EPSILON FLT_EPSILON
#define WR_EPSILON_F FLT_EPSILON
#define WR_MAX_F FLT_MAX
#define WR_LOWEST_F -WR_MAX_F

#define WR_MAX_UINT 0xFFFFFFFF

#define WR_RADIANS(angleInDegrees)                                             \
  ((angleInDegrees)*0.01745329251994329576923690768489f)
#define WR_DEGREES(angleInRadians)                                             \
  ((angleInRadians)*57.295779513082320876798154814105f)

typedef union
{
  struct
  {
    float x, y;
  };
  struct
  {
    float r, g;
  };
  struct
  {
    float u, v;
  };
  float at[2];
} vec2;

typedef union
{
  struct
  {
    float x, y, z;
  };
  struct
  {
    float r, g, b;
  };
  struct
  {
    float u, v, s;
  };
  float at[3];
} vec3;

typedef union
{
  struct
  {
    float x, y, z, w;
  };
  struct
  {
    float r, g, b, a;
  };
  struct
  {
    float u, v, s, t;
  };
  float at[4];
} vec4;

typedef struct
{
  int         major;
  int         minor;
  const char* description;
  vec3        luda;
} wrays_version_t;

typedef union
{
  struct
  {
    double_t x, y, z;
  };
  struct
  {
    double_t r, g, b;
  };
  struct
  {
    double_t u, v, s;
  };
  double_t at[3];
} dvec3;

typedef union
{
  struct
  {
    int32_t x, y, z;
  };
  struct
  {
    int32_t r, g, b;
  };
  int32_t at[3];
} ivec3;

typedef union
{
  struct
  {
    int32_t x, y, z, w;
  };
  int32_t at[4];
} ivec4;

typedef union
{
  struct
  {
    uint32_t x, y, z;
  };
  struct
  {
    uint32_t r, g, b;
  };
  uint32_t at[3];
} uvec3;

typedef union
{
  struct
  {
    uint32_t x, y;
  };
  struct
  {
    uint32_t r, g;
  };
  uint32_t at[2];
} uvec2;

typedef union
{
  struct
  {
    uint32_t x, y, z, w;
  };
  struct
  {
    uint32_t r, g, b, a;
  };
  uint32_t at[4];
} uvec4;

typedef union
{
  struct
  {
    int32_t x, y;
  };
  struct
  {
    int32_t r, g;
  };
  int32_t at[2];
} ivec2;

typedef union
{
  struct
  {
    int16_t x, y;
  };
  struct
  {
    int16_t r, g;
  };
  int16_t at[2];
} svec2;

typedef union
{
  struct
  {
    int16_t x, y, z;
  };
  struct
  {
    int16_t r, g, b;
  };
  int16_t at[3];
} svec3;

typedef union
{
  struct
  {
    int8_t x, y;
  };
  struct
  {
    int8_t r, g;
  };
  int8_t at[2];
} bvec2;

typedef union
{
  struct
  {
    int8_t x, y, z;
  };
  struct
  {
    int8_t r, g, b;
  };
  int8_t at[3];
} bvec3;

typedef union
{
  struct
  {
    int8_t x, y, z, w;
  };
  struct
  {
    int8_t r, g, b, a;
  };
  int8_t at[4];
} bvec4;

typedef union
{
  struct
  {
    uint8_t x, y, z, w;
  };
  struct
  {
    uint8_t r, g, b, a;
  };
  uint8_t at[4];
} ubvec4;

typedef union
{
  vec3  row[3];
  float at[9];
  float cell[3][3];
} mat3;

typedef union
{
  vec4  row[4];
  float at[16];
  float cell[4][4];
} mat4;

vec3
wrays_vec3_max(vec3 a, vec3 b);
vec3
wrays_vec3_min(vec3 a, vec3 b);
vec4
wrays_vec4_max(vec4 a, vec4 b);
vec4
wrays_vec4_min(vec4 a, vec4 b);
vec3
wrays_vec3_mul(vec3 a, vec3 b);
vec2
wrays_vec2_add(vec2 a, vec2 b);
vec3
wrays_vec3_add(vec3 a, vec3 b);
vec4
wrays_vec4_add(vec4 a, vec4 b);
vec4
wrays_vec4_div(vec4 a, vec4 b);
vec3
wrays_vec3_sub(vec3 a, vec3 b);
vec4
wrays_vec4_sub_vec3(vec4 a, vec3 b);
vec4
wrays_vec4_add_vec3(vec4 a, vec3 b);
vec4
wrays_vec4_scale(vec4 vector, vec4 scale);
vec3
wrays_vec3_scale(vec3 vector, vec3 scale);
vec3
wrays_vec3_scalef(vec3 vector, float scale);
vec2
wrays_vec2_scalef(vec2 vector, float scale);
float
wrays_vec3_length2(vec3 vector);
float
wrays_vec3_length(vec3 vector);
float
wrays_vec2_length2(vec2 vector);
float
wrays_vec2_length(vec2 vector);
vec3
wrays_vec3_cross(vec3 vector_a, vec3 vector_b);
vec3
wrays_vec3_normalize(vec3 vector);
vec3
wrays_vec3_sub(vec3 a, vec3 b);
float
wrays_vec3_dot(vec3 a, vec3 b);
float
wrays_vec4_dot(vec4 a, vec4 b);
mat4
wrays_mat4_identity();
vec3
wrays_vec4_to_vec3(vec4 vector);
vec4
wrays_vec3_to_vec4(vec3 vector);
vec3
wrays_vec3_negate(vec3 vector);
vec4
wrays_vec4_negate(vec4 vector);
void
wrays_mat4_translate_in_place(mat4* m, float x, float y, float z);
mat4
wrays_mat4_look_at(vec3 eye, vec3 center, vec3 up);
mat4
wrays_mat4_identity();
mat4
wrays_mat4_scale(vec3 scale);
mat4
wrays_mat4_scalef(float x, float y, float z);
vec4
wrays_vec4_scalef(vec4 vector, float scale);
mat4
wrays_mat4_translatef(float x, float y, float z);
mat4
wrays_mat4_translate(vec3 vec);
mat4
wrays_mat4_rotate(vec3 vec, float angle);
mat4
wrays_mat4_mul(const mat4 m1, const mat4 m2);
void
wrays_mat4_frustum(mat4* m, float l, float r, float b, float t, float n,
                   float f);
void
wrays_mat4_ortho(mat4* m, float l, float r, float b, float t, float n, float f);
mat4
wrays_mat4_perspective(float y_fov, float aspect, float n, float f);
mat4
wrays_mat4_inverse(mat4 m);
vec3
wrays_mat4_mul_vec3(mat4 m, vec3 v);
vec4
wrays_mat4_mul_vec4(mat4 m, vec4 v);
mat4
wrays_mat4_transpose(mat4 m);
void
wrays_mat4_invert(mat4* m);
vec4
wrays_mat4_col(mat4 m, int i);
float
wrays_minf(float x, float y);
float
wrays_maxf(float x, float y);
int
wrays_mini(int x, int y);
int
wrays_maxi(int x, int y);
unsigned int
wrays_minu(unsigned int x, unsigned int y);
unsigned int
wrays_maxu(unsigned int x, unsigned int y);

#define WR_DUMP_VEC3(vec)                                                      \
  {                                                                            \
    printf(#vec " { %f %f %f }\n", (vec).x, (vec).y, (vec).z);                 \
  }
#define WR_DUMP_VEC4(vec)                                                      \
  {                                                                            \
    printf(#vec " { %f %f %f %f }\n", (vec).x, (vec).y, (vec).z, (vec).w);     \
  }
#define WR_DUMP_MAT4(mat)                                                      \
  {                                                                            \
    printf(#mat " { { %f %f %f %f }\n    , { %f %f %f %f }\n    , { %f %f %f " \
                "%f }\n    , { %f %f %f %f }\n}\n",                            \
           (mat).at[0], (mat).at[1], (mat).at[2], (mat).at[3], (mat).at[4],    \
           (mat).at[5], (mat).at[6], (mat).at[7], (mat).at[8], (mat).at[9],    \
           (mat).at[10], (mat).at[11], (mat).at[12], (mat).at[13],             \
           (mat).at[14], (mat).at[15]);                                        \
  }

#endif

#ifdef WR_MATH_IMPLEMENTATION

float
wrays_minf(float x, float y)
{
  return x < y ? x : y;
}

float
wrays_maxf(float x, float y)
{
  return x > y ? x : y;
}

int
wrays_mini(int x, int y)
{
  return x < y ? x : y;
}

int
wrays_maxi(int x, int y)
{
  return x > y ? x : y;
}

unsigned int
wrays_minu(unsigned int x, unsigned int y)
{
  return x < y ? x : y;
}

unsigned int
wrays_maxu(unsigned int x, unsigned int y)
{
  return x > y ? x : y;
}

vec4
wrays_vec4_max(vec4 a, vec4 b)
{
  vec4 vector;

  vector.x = a.x > b.x ? a.x : b.x;
  vector.y = a.y > b.y ? a.y : b.y;
  vector.z = a.z > b.z ? a.z : b.z;
  vector.w = a.w > b.w ? a.w : b.w;

  return vector;
}

vec4
wrays_vec4_min(vec4 a, vec4 b)
{
  vec4 vector;

  vector.x = a.x < b.x ? a.x : b.x;
  vector.y = a.y < b.y ? a.y : b.y;
  vector.z = a.z < b.z ? a.z : b.z;
  vector.w = a.w < b.w ? a.w : b.w;

  return vector;
}

vec3
wrays_vec3_max(vec3 a, vec3 b)
{
  vec3 vector;

  vector.x = a.x > b.x ? a.x : b.x;
  vector.y = a.y > b.y ? a.y : b.y;
  vector.z = a.z > b.z ? a.z : b.z;

  return vector;
}

vec3
wrays_vec3_min(vec3 a, vec3 b)
{
  vec3 vector;

  vector.x = a.x < b.x ? a.x : b.x;
  vector.y = a.y < b.y ? a.y : b.y;
  vector.z = a.z < b.z ? a.z : b.z;

  return vector;
}

vec3
wrays_mat4_mul_vec3(mat4 m, vec3 v)
{
  vec4 vec;
  vec4 vector = { v.x, v.y, v.z, 1.0f };
  int  i, j;
  for (j = 0; j < 4; ++j) {
    vec.at[j] = 0.f;
    for (i = 0; i < 4; ++i)
      vec.at[j] += m.cell[i][j] * vector.at[i];
  }

  vec3 ret = { vec.x, vec.y, vec.z };
  return ret;
}

vec4
wrays_mat4_mul_vec4(mat4 m, vec4 v)
{
  vec4 vec;
  int  i, j;
  for (j = 0; j < 4; ++j) {
    vec.at[j] = 0.f;
    for (i = 0; i < 4; ++i)
      vec.at[j] += m.cell[i][j] * v.at[i];
  }

  return vec;
}

mat4
wrays_mat4_transpose(mat4 m)
{
  mat4 tran;
  int  i, j;
  for (j = 0; j < 4; ++j)
    for (i = 0; i < 4; ++i)
      tran.cell[i][j] = m.cell[j][i];

  return tran;
}

void
wrays_mat4_invert(mat4* m)
{
  *m = wrays_mat4_inverse(*m);
}

vec4
wrays_mat4_col(mat4 m, int i)
{
  int  k;
  vec4 col;
  for (k = 0; k < 4; ++k)
    col.at[k] = m.cell[k][i];

  return col;
}

mat4
wrays_mat4_perspective(float y_fov, float aspect, float n, float f)
{
  /* NOTE: Degrees are an unhandy unit to work with.
   * linmath.h uses radians for everything! */
  mat4 m;
  y_fov         = WR_RADIANS(y_fov);
  float const a = 1.f / tanf(y_fov / 2.f);

  m.cell[0][0] = a / aspect;
  m.cell[0][1] = 0.f;
  m.cell[0][2] = 0.f;
  m.cell[0][3] = 0.f;

  m.cell[1][0] = 0.f;
  m.cell[1][1] = a;
  m.cell[1][2] = 0.f;
  m.cell[1][3] = 0.f;

  m.cell[2][0] = 0.f;
  m.cell[2][1] = 0.f;
  m.cell[2][2] = -((f + n) / (f - n));
  m.cell[2][3] = -1.f;

  m.cell[3][0] = 0.f;
  m.cell[3][1] = 0.f;
  m.cell[3][2] = -((2.f * f * n) / (f - n));
  m.cell[3][3] = 0.f;

  return m;
}

mat4
wrays_mat4_inverse(mat4 m)
{
  mat4 inv;

  float s[6];
  float c[6];
  s[0] = m.cell[0][0] * m.cell[1][1] - m.cell[1][0] * m.cell[0][1];
  s[1] = m.cell[0][0] * m.cell[1][2] - m.cell[1][0] * m.cell[0][2];
  s[2] = m.cell[0][0] * m.cell[1][3] - m.cell[1][0] * m.cell[0][3];
  s[3] = m.cell[0][1] * m.cell[1][2] - m.cell[1][1] * m.cell[0][2];
  s[4] = m.cell[0][1] * m.cell[1][3] - m.cell[1][1] * m.cell[0][3];
  s[5] = m.cell[0][2] * m.cell[1][3] - m.cell[1][2] * m.cell[0][3];

  c[0] = m.cell[2][0] * m.cell[3][1] - m.cell[3][0] * m.cell[2][1];
  c[1] = m.cell[2][0] * m.cell[3][2] - m.cell[3][0] * m.cell[2][2];
  c[2] = m.cell[2][0] * m.cell[3][3] - m.cell[3][0] * m.cell[2][3];
  c[3] = m.cell[2][1] * m.cell[3][2] - m.cell[3][1] * m.cell[2][2];
  c[4] = m.cell[2][1] * m.cell[3][3] - m.cell[3][1] * m.cell[2][3];
  c[5] = m.cell[2][2] * m.cell[3][3] - m.cell[3][2] * m.cell[2][3];

  /* Assumes it is invertible */
  float idet = 1.0f / (s[0] * c[5] - s[1] * c[4] + s[2] * c[3] + s[3] * c[2] -
                       s[4] * c[1] + s[5] * c[0]);

  inv.cell[0][0] =
    (m.cell[1][1] * c[5] - m.cell[1][2] * c[4] + m.cell[1][3] * c[3]) * idet;
  inv.cell[0][1] =
    (-m.cell[0][1] * c[5] + m.cell[0][2] * c[4] - m.cell[0][3] * c[3]) * idet;
  inv.cell[0][2] =
    (m.cell[3][1] * s[5] - m.cell[3][2] * s[4] + m.cell[3][3] * s[3]) * idet;
  inv.cell[0][3] =
    (-m.cell[2][1] * s[5] + m.cell[2][2] * s[4] - m.cell[2][3] * s[3]) * idet;

  inv.cell[1][0] =
    (-m.cell[1][0] * c[5] + m.cell[1][2] * c[2] - m.cell[1][3] * c[1]) * idet;
  inv.cell[1][1] =
    (m.cell[0][0] * c[5] - m.cell[0][2] * c[2] + m.cell[0][3] * c[1]) * idet;
  inv.cell[1][2] =
    (-m.cell[3][0] * s[5] + m.cell[3][2] * s[2] - m.cell[3][3] * s[1]) * idet;
  inv.cell[1][3] =
    (m.cell[2][0] * s[5] - m.cell[2][2] * s[2] + m.cell[2][3] * s[1]) * idet;

  inv.cell[2][0] =
    (m.cell[1][0] * c[4] - m.cell[1][1] * c[2] + m.cell[1][3] * c[0]) * idet;
  inv.cell[2][1] =
    (-m.cell[0][0] * c[4] + m.cell[0][1] * c[2] - m.cell[0][3] * c[0]) * idet;
  inv.cell[2][2] =
    (m.cell[3][0] * s[4] - m.cell[3][1] * s[2] + m.cell[3][3] * s[0]) * idet;
  inv.cell[2][3] =
    (-m.cell[2][0] * s[4] + m.cell[2][1] * s[2] - m.cell[2][3] * s[0]) * idet;

  inv.cell[3][0] =
    (-m.cell[1][0] * c[3] + m.cell[1][1] * c[1] - m.cell[1][2] * c[0]) * idet;
  inv.cell[3][1] =
    (m.cell[0][0] * c[3] - m.cell[0][1] * c[1] + m.cell[0][2] * c[0]) * idet;
  inv.cell[3][2] =
    (-m.cell[3][0] * s[3] + m.cell[3][1] * s[1] - m.cell[3][2] * s[0]) * idet;
  inv.cell[3][3] =
    (m.cell[2][0] * s[3] - m.cell[2][1] * s[1] + m.cell[2][2] * s[0]) * idet;

  return inv;
}

mat4
wrays_mat4_mul(const mat4 m1, const mat4 m2)
{
  mat4 m;

  m.at[0] = m1.at[0] * m2.at[0] + m1.at[4] * m2.at[1] + m1.at[8] * m2.at[2] +
            m1.at[12] * m2.at[3];
  m.at[1] = m1.at[1] * m2.at[0] + m1.at[5] * m2.at[1] + m1.at[9] * m2.at[2] +
            m1.at[13] * m2.at[3];
  m.at[2] = m1.at[2] * m2.at[0] + m1.at[6] * m2.at[1] + m1.at[10] * m2.at[2] +
            m1.at[14] * m2.at[3];
  m.at[3] = m1.at[3] * m2.at[0] + m1.at[7] * m2.at[1] + m1.at[11] * m2.at[2] +
            m1.at[15] * m2.at[3];

  m.at[4] = m1.at[0] * m2.at[4] + m1.at[4] * m2.at[5] + m1.at[8] * m2.at[6] +
            m1.at[12] * m2.at[7];
  m.at[5] = m1.at[1] * m2.at[4] + m1.at[5] * m2.at[5] + m1.at[9] * m2.at[6] +
            m1.at[13] * m2.at[7];
  m.at[6] = m1.at[2] * m2.at[4] + m1.at[6] * m2.at[5] + m1.at[10] * m2.at[6] +
            m1.at[14] * m2.at[7];
  m.at[7] = m1.at[3] * m2.at[4] + m1.at[7] * m2.at[5] + m1.at[11] * m2.at[6] +
            m1.at[15] * m2.at[7];

  m.at[8] = m1.at[0] * m2.at[8] + m1.at[4] * m2.at[9] + m1.at[8] * m2.at[10] +
            m1.at[12] * m2.at[11];
  m.at[9] = m1.at[1] * m2.at[8] + m1.at[5] * m2.at[9] + m1.at[9] * m2.at[10] +
            m1.at[13] * m2.at[11];
  m.at[10] = m1.at[2] * m2.at[8] + m1.at[6] * m2.at[9] + m1.at[10] * m2.at[10] +
             m1.at[14] * m2.at[11];
  m.at[11] = m1.at[3] * m2.at[8] + m1.at[7] * m2.at[9] + m1.at[11] * m2.at[10] +
             m1.at[15] * m2.at[11];

  m.at[12] = m1.at[0] * m2.at[12] + m1.at[4] * m2.at[13] +
             m1.at[8] * m2.at[14] + m1.at[12] * m2.at[15];
  m.at[13] = m1.at[1] * m2.at[12] + m1.at[5] * m2.at[13] +
             m1.at[9] * m2.at[14] + m1.at[13] * m2.at[15];
  m.at[14] = m1.at[2] * m2.at[12] + m1.at[6] * m2.at[13] +
             m1.at[10] * m2.at[14] + m1.at[14] * m2.at[15];
  m.at[15] = m1.at[3] * m2.at[12] + m1.at[7] * m2.at[13] +
             m1.at[11] * m2.at[14] + m1.at[15] * m2.at[15];

  return m;
}

void
wrays_mat4_frustum(mat4* m, float l, float r, float b, float t, float n,
                   float f)
{
  m->cell[0][0] = 2.f * n / (r - l);
  m->cell[0][1] = m->cell[0][2] = m->cell[0][3] = 0.f;

  m->cell[1][1] = 2.f * n / (t - b);
  m->cell[1][0] = m->cell[1][2] = m->cell[1][3] = 0.f;

  m->cell[2][0] = (r + l) / (r - l);
  m->cell[2][1] = (t + b) / (t - b);
  m->cell[2][2] = -(f + n) / (f - n);
  m->cell[2][3] = -1.f;

  m->cell[3][2] = -2.f * (f * n) / (f - n);
  m->cell[3][0] = m->cell[3][1] = m->cell[3][3] = 0.f;
}

void
wrays_mat4_ortho(mat4* m, float l, float r, float b, float t, float n, float f)
{
  m->cell[0][0] = 2.f / (r - l);
  m->cell[0][1] = m->cell[0][2] = m->cell[0][3] = 0.f;

  m->cell[1][1] = 2.f / (t - b);
  m->cell[1][0] = m->cell[1][2] = m->cell[1][3] = 0.f;

  m->cell[2][2] = -2.f / (f - n);
  m->cell[2][0] = m->cell[2][1] = m->cell[2][3] = 0.f;

  m->cell[3][0] = -(r + l) / (r - l);
  m->cell[3][1] = -(t + b) / (t - b);
  m->cell[3][2] = -(f + n) / (f - n);
  m->cell[3][3] = 1.f;
}

mat4
wrays_mat4_scale(vec3 scale)
{
  mat4 m   = wrays_mat4_identity();
  m.row[0] = wrays_vec4_scalef(m.row[0], scale.x);
  m.row[1] = wrays_vec4_scalef(m.row[1], scale.y);
  m.row[2] = wrays_vec4_scalef(m.row[2], scale.z);
  return m;
}

mat4
wrays_mat4_scalef(float x, float y, float z)
{
  vec3 scale = { x, y, z };
  return wrays_mat4_scale(scale);
}

mat4
wrays_mat4_translatef(float x, float y, float z)
{
  mat4 m       = wrays_mat4_identity();
  m.cell[3][0] = x;
  m.cell[3][1] = y;
  m.cell[3][2] = z;
  return m;
}

mat4
wrays_mat4_translate(vec3 vec)
{
  return wrays_mat4_translatef(vec.x, vec.y, vec.z);
}

mat4
wrays_mat4_rotate(vec3 vec, float angle)
{
  vec3  v       = wrays_vec3_normalize(vec);
  float radians = angle;
  float cost    = cosf(radians);
  float sint    = sinf(radians);
  mat4  mat     = wrays_mat4_identity();

  /* https://en.wikipedia.org/wiki/Rotation_matrix#Basic_rotations */
  mat.cell[0][0] = cost + v.x * v.x * (1.0f - cost);
  mat.cell[0][1] = v.x * v.y * (1.0f - cost) - v.z * sint;
  mat.cell[0][2] = v.x * v.z * (1.0f - cost) + v.y * sint;

  mat.cell[1][0] = v.y * v.x * (1.0f - cost) + v.z * sint;
  mat.cell[1][1] = cost + v.y * v.y * (1.0f - cost);
  mat.cell[1][2] = v.y * v.z * (1.0f - cost) - v.x * sint;

  mat.cell[2][0] = v.z * v.x * (1.0f - cost) - v.y * sint;
  mat.cell[2][1] = v.z * v.y * (1.0f - cost) + v.x * sint;
  mat.cell[2][2] = cost + v.z * v.z * (1.0f - cost);

  return mat;
}

mat4
wrays_mat4_identity()
{
  mat4 mat  = { 0 };
  mat.at[0] = mat.at[5] = mat.at[10] = mat.at[15] = 1.0f;
  return mat;
}

vec3
wrays_vec3_mul(vec3 a, vec3 b)
{
  vec3 ret;
  ret.x = a.x * b.x;
  ret.y = a.y * b.y;
  ret.z = a.z * b.z;

  return ret;
}

vec3
wrays_vec3_scalef(vec3 vector, float scale)
{
  vector.x = vector.x * scale;
  vector.y = vector.y * scale;
  vector.z = vector.z * scale;
  return vector;
}

vec2
wrays_vec2_scalef(vec2 vector, float scale)
{
  vector.x = vector.x * scale;
  vector.y = vector.y * scale;
  return vector;
}

vec3
wrays_vec3_scale(vec3 vector, vec3 scale)
{
  vector.x = vector.x * scale.x;
  vector.y = vector.y * scale.y;
  vector.z = vector.z * scale.z;
  return vector;
}

vec4
wrays_vec4_scalef(vec4 vector, float scale)
{
  vector.x = vector.x * scale;
  vector.y = vector.y * scale;
  vector.z = vector.z * scale;
  vector.w = vector.w * scale;
  return vector;
}

vec4
wrays_vec4_scale(vec4 vector, vec4 scale)
{
  vector.x = vector.x * scale.x;
  vector.y = vector.y * scale.y;
  vector.z = vector.z * scale.z;
  vector.w = vector.w * scale.w;
  return vector;
}

vec4
wrays_vec4_add(vec4 a, vec4 b)
{
  vec4 ret;
  ret.x = a.x + b.x;
  ret.y = a.y + b.y;
  ret.z = a.z + b.z;
  ret.w = a.w + b.w;
  return ret;
}

vec4
wrays_vec4_div(vec4 a, vec4 b)
{
  vec4 ret;
  ret.x = a.x / b.x;
  ret.y = a.y / b.y;
  ret.z = a.z / b.z;
  ret.w = a.w / b.w;
  return ret;
}

vec3
wrays_vec3_add(vec3 a, vec3 b)
{
  vec3 ret;
  ret.x = a.x + b.x;
  ret.y = a.y + b.y;
  ret.z = a.z + b.z;
  return ret;
}

vec2
wrays_vec2_add(vec2 a, vec2 b)
{
  vec2 ret;
  ret.x = a.x + b.x;
  ret.y = a.y + b.y;
  return ret;
}

float
wrays_vec3_length2(vec3 vector)
{
  return (vector.x * vector.x) + (vector.y * vector.y) + (vector.z * vector.z);
}

float
wrays_vec3_length(vec3 vector)
{
  return sqrtf(wrays_vec3_length2(vector));
}

float
wrays_vec2_length2(vec2 vector)
{
  return (vector.x * vector.x) + (vector.y * vector.y);
}

float
wrays_vec2_length(vec2 vector)
{
  return sqrtf(wrays_vec2_length2(vector));
}

vec3
wrays_vec3_cross(vec3 vector_a, vec3 vector_b)
{
  vec3 ret = { vector_a.y * vector_b.z - vector_a.z * vector_b.y,
               vector_a.z * vector_b.x - vector_a.x * vector_b.z,
               vector_a.x * vector_b.y - vector_a.y * vector_b.x };

  return ret;
}

vec3
wrays_vec3_normalize(vec3 vector)
{
  float length = wrays_vec3_length(vector);
  vec3  ret    = { vector.x / length, vector.y / length, vector.z / length };
  return ret;
}

vec4
wrays_vec4_add_vec3(vec4 a, vec3 b)
{
  vec4 ret;
  ret.x = a.x + b.x;
  ret.y = a.y + b.y;
  ret.z = a.z + b.z;
  ret.w = a.w;
  return ret;
}

vec4
wrays_vec4_sub_vec3(vec4 a, vec3 b)
{
  vec4 ret;
  ret.x = a.x - b.x;
  ret.y = a.y - b.y;
  ret.z = a.z - b.z;
  ret.w = a.w;
  return ret;
}

vec3
wrays_vec3_sub(vec3 a, vec3 b)
{
  vec3 ret;
  ret.x = a.x - b.x;
  ret.y = a.y - b.y;
  ret.z = a.z - b.z;

  return ret;
}

float
wrays_vec3_dot(vec3 a, vec3 b)
{
  float p = 0.f;
  int   i;
  for (i = 0; i < 3; ++i)
    p += b.at[i] * a.at[i];
  return p;
}

float
wrays_vec4_dot(vec4 a, vec4 b)
{
  float p = 0.f;
  int   i;
  for (i = 0; i < 4; ++i)
    p += b.at[i] * a.at[i];
  return p;
}

vec3
wrays_vec4_to_vec3(vec4 vector)
{
  vec3 vector3 = { vector.x, vector.y, vector.z };
  return vector3;
}

vec4
wrays_vec3_to_vec4(vec3 vector)
{
  vec4 vector4 = { vector.x, vector.y, vector.z, 0 };
  return vector4;
}

vec3
wrays_vec3_negate(vec3 vector)
{
  vec3 vector_neg = { -vector.x, -vector.y, -vector.z };
  return vector_neg;
}

vec4
wrays_vec4_negate(vec4 vector)
{
  vec4 vector_neg = { -vector.x, -vector.y, -vector.z, -vector.w };
  return vector_neg;
}

void
wrays_mat4_translate_in_place(mat4* m, float x, float y, float z)
{
  vec4 t = { x, y, z, 0 };
  int  i;
  for (i = 0; i < 4; ++i) {
    vec4 row = m->row[i];
    m->cell[3][i] += wrays_vec4_dot(row, t);
  }
}

mat4
wrays_mat4_look_at(vec3 eye, vec3 center, vec3 up)
{
  vec3 forward = wrays_vec3_normalize(wrays_vec3_sub(eye, center));
  vec3 right   = wrays_vec3_normalize(wrays_vec3_cross(up, forward));
  vec3 up2     = wrays_vec3_cross(forward, right);

  mat4 camToWorld = wrays_mat4_identity();

  camToWorld.cell[0][0] = right.x;
  camToWorld.cell[0][1] = right.y;
  camToWorld.cell[0][2] = right.z;
  camToWorld.cell[1][0] = up2.x;
  camToWorld.cell[1][1] = up2.y;
  camToWorld.cell[1][2] = up2.z;
  camToWorld.cell[2][0] = forward.x;
  camToWorld.cell[2][1] = forward.y;
  camToWorld.cell[2][2] = forward.z;

  camToWorld.cell[3][0] = eye.x;
  camToWorld.cell[3][1] = eye.y;
  camToWorld.cell[3][2] = eye.z;

  return camToWorld;
}

#endif /* _WRAYS_MATH_H_ */