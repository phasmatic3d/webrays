#version 300 es
precision highp float;

layout(location = 0) out vec4 accumulation_OUT;
layout(location = 1) out vec4 origin_OUT;
layout(location = 2) out vec4 direction_OUT;
layout(location = 3) out vec4 payload_OUT;

uniform ivec4 tile;
uniform ivec2 frame;
uniform uvec2 seed;
/* Camera */
uniform vec3  camera_pos;
uniform vec3  camera_up;
uniform vec3  camera_front;
uniform vec3  camera_right;
uniform float camera_vfov;

uniform mat4 ViewInvMatrix;
uniform mat4 ProjectionInvMatrix;

/* For random4f */
#include "util.inc.glsl"

#define USE_INVERSE_MATRIX_PROJECTION
void main() {
  vec4 tilef = vec4(tile);
  vec2 framef = vec2(frame);
  vec2 pixel = tilef.xy * tilef.zw + gl_FragCoord.xy;
  vec2 pixel_norm = pixel / framef;

  uint counter = uint(pixel.x) + uint(pixel.y) * uint(frame.x) + 1u;
  vec4 randoms = random4f(counter, seed.x, seed.y);

  /* Jitter pixel coordinates */
  pixel_norm = pixel_norm + randoms.xy / framef;

#ifdef USE_INVERSE_MATRIX_PROJECTION
  vec4 pndc = vec4(2.0 * pixel_norm - 1.0, 0.0, 1.0);
  vec4 pecs = ProjectionInvMatrix * pndc;
  pecs /= pecs.w;
  
  vec4 direction_ecs = normalize(vec4(pecs.xyz, 0.0));
  vec3 ray_origin = vec3(ViewInvMatrix * vec4(pecs.xyz, 1.0));
  vec3 ray_direction = normalize(vec3(ViewInvMatrix * direction_ecs));
#else
  float tanFOV2 = tan(radians(camera_vfov) / 2.0);    
  vec3 cx = tanFOV2 * (framef.x / framef.y) * normalize(cross(camera_front, camera_up));
  vec3 cy = tanFOV2 * normalize(cross(cx, camera_front));
    
  vec3 ray_origin = camera_pos;
  vec3 ray_direction = normalize(2.0 * (pixel_norm.x - 0.5) * cx + 2.0 * (pixel_norm.y - 0.5) * cy + camera_front);
#endif

  direction_OUT = vec4(ray_direction, 10000.0);
  origin_OUT = vec4(camera_pos, 0.0);
  accumulation_OUT = vec4(0, 0, 0, 0);
  payload_OUT = vec4(1,1,1,0); // throughput
}