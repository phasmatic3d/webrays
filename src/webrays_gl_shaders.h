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

#ifndef _WRAYS_GL_SHADERS_H_
#define _WRAYS_GL_SHADERS_H_

static char const* const wr_brute_force_scene_accessor_uniform_blocks[] = {
  "wr_Vertices",
  "wr_Normals",
  "wr_Faces",
};

static char const* const wr_brute_force_scene_accessor_string =
  R"glsl(
layout (std140) uniform wr_VerticesUBO
{
    vec4 wr_Vertices[wr_AttributeCount];
};

layout (std140) uniform wr_NormalsUBO
{
    vec4 wr_Normals[wr_AttributeCount];
};

layout (std140) uniform wr_FacesUBO
{
    ivec4 wr_Faces[wr_FaceCount];
};

vec3 rg_fast_intersect_triangle(vec3 direction, vec3 origin, vec3 v1, vec3 v2, vec3 v3, float t_max)
{
  vec3 e1 = v2 - v1;
  vec3 e2 = v3 - v1;
  vec3 s1 = cross(direction, e2);

#define USE_SAFE_MATH
#ifdef USE_SAFE_MATH
  float invd = 1.0 / dot(s1, e1);
#else
  float invd = native_recip(dot(s1, e1));
#endif

  vec3 d = origin - v1;
  float b1 = dot(d, s1) * invd;
  vec3 s2 = cross(d, e1);
  float b2 = dot(direction, s2) * invd;
  float temp = dot(e2, s2) * invd;
  //return vec3(b1, b2, temp);
  if (b1 < 0.0 || b1 > 1.0 || b2 < 0.0 || b1 + b2 > 1.0 || temp < 0.0 || temp > t_max)
  {
    return vec3(0, 0, t_max);
  }
  else
  {
    return vec3(b1, b2, temp);
  }
}

ivec4 wr_query_intersection(vec3 origin, vec3 direction, float t_max) {
  ivec4 intersection = ivec4(-1, 0, 0, 0);
  float min_distance = t_max;
  for (int face_index = 0; face_index < wr_FaceCount; ++face_index) {
    ivec4 face = wr_Faces[face_index];
    vec3 res = rg_fast_intersect_triangle(direction, origin, wr_Vertices[face.x].xyz, wr_Vertices[face.y].xyz, wr_Vertices[face.z].xyz, min_distance);
    if (res.z < min_distance) {
      min_distance = res.z;
      intersection = ivec4(face_index, floatBitsToInt(res.xy), floatBitsToInt(res.z));
    }
  }

  return intersection;
}

bool wr_query_occlusion(vec3 direction, vec3 origin, float t_max) {
  float min_distance = t_max;
  for (int face_index = 0; face_index < wr_FaceCount; ++face_index) {
    ivec4 face = wr_Faces[face_index];
    vec3 res = rg_fast_intersect_triangle(direction, origin, wr_Vertices[face.x].xyz, wr_Vertices[face.y].xyz, wr_Vertices[face.z].xyz, min_distance);
    if (res.z < min_distance) {
      return true;
    }
  }

  return false;
}

vec2
wr_GetBaryCoords(ivec4 intersection) {
  return intBitsToFloat(intersection.yz);
}

ivec4
wr_GetFace(ivec4 intersection) {
  return wr_Faces[intersection.x];
}

vec3
wr_GetNormal(ivec4 intersection) {
  ivec4 face = wr_Faces[intersection.x];
  return wr_Normals[face.x].xyz;
}

float
wr_GetHitDistance(ivec4 intersection) {
  return intBitsToFloat(intersection.w); 
}

)glsl";

static char const* const wr_intersection_fragment_shader =
  R"glsl(
layout(location = 0) out ivec4 wr_Intersection;

uniform sampler2D wr_RayOrigins;
uniform sampler2D wr_RayDirections;

uniform int wr_ADS;

void main() {
  vec4 ray_direction = texelFetch(wr_RayDirections, ivec2(gl_FragCoord.xy), 0);
  vec4 ray_origin = texelFetch(wr_RayOrigins, ivec2(gl_FragCoord.xy), 0);
  
  if (0.0 == ray_direction.w) return;
  ray_origin.xyz = ray_origin.xyz + ray_origin.w * ray_direction.xyz;

  wr_Intersection = wr_query_intersection(wr_ADS, ray_origin.xyz, ray_direction.xyz, ray_direction.w);
}
)glsl";

static char const* const wr_occlusion_fragment_shader =
  R"glsl(
layout(location = 0) out int wr_Occlusion;

uniform sampler2D wr_RayOrigins;
uniform sampler2D wr_RayDirections;

uniform int wr_ADS;

void main() {
  vec4 ray_direction = texelFetch(wr_RayDirections, ivec2(gl_FragCoord.xy), 0);
  vec4 ray_origin = texelFetch(wr_RayOrigins, ivec2(gl_FragCoord.xy), 0);
  
  if (0.0 == ray_direction.w) return;
  ray_origin.xyz = ray_origin.xyz + ray_origin.w * ray_direction.xyz;

  wr_Occlusion = wr_query_occlusion(wr_ADS, ray_origin.xyz, ray_direction.xyz, ray_direction.w) ? 1 : 0;
}
)glsl";

static char const* const wr_screen_fill_vertex_shader =
  R"glsl(#version 300 es
precision highp float;

layout(location = 0) in vec3 wr_VertexPosition;

const vec3 screen_fill_triangle[3] = vec3[3](
  vec3(-4.0f, -4.0f, 0.0f),
  vec3( 4.0f, -4.0f, 0.0f),
  vec3( 0.0f,  4.0f, 0.0f)
);

void main(){
  gl_Position = vec4(wr_VertexPosition, 1.0f);

  /* In-shader */
  //vec2 uvs = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
  //gl_Position = vec4(uvs * 2.0f - 1.0f, 0.0f, 1.0f);
  //gl_Position = vec4(screen_fill_triangle[gl_VertexID], 1.0);
}

)glsl";

#endif /* _WRAYS_GL_SHADERS_H_ */
