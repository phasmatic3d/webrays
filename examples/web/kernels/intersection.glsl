#line 0
precision highp sampler2D;
precision highp sampler2DArray;
precision highp isampler2D;

layout(location = 0) out vec4 accumulation_OUT;
layout(location = 1) out vec4 origin_OUT;
layout(location = 2) out vec4 direction_OUT;
layout(location = 3) out vec4 payload_OUT;

uniform int   ads; 
uniform int   sample_index;
uniform int   depth;
uniform ivec4 tile;
uniform ivec2 frame;
uniform uvec2 seed;

uniform sampler2D ray_origins;
uniform sampler2D ray_accumulations;
uniform sampler2D ray_directions;
uniform sampler2D ray_payloads;
uniform isampler2D intersections;

uniform sampler2D materialBuffer;
uniform sampler2D lightBuffer;
uniform int light_count; 
uniform sampler2DArray texturesBuffer;

#include "common.inc.glsl"
#include "reflection.inc.glsl"
#include "util.inc.glsl"
#include "bxdf.inc.glsl"
#include "light.inc.glsl"

void terminate_ray() {
  direction_OUT    = vec4(0.0);
  origin_OUT       = vec4(0.0);
}

void main() {
  /* Reconstruct pixel coordinates from tile coordinates */
  vec4 tilef = vec4(tile);
  vec2 framef = vec2(frame);
  vec2 pixel = tilef.xy * tilef.zw + gl_FragCoord.xy;
  
  vec4  direction_IN = texelFetch(ray_directions, ivec2(gl_FragCoord.xy), 0);
  vec4  origin_IN = texelFetch(ray_origins, ivec2(gl_FragCoord.xy), 0);
  vec4  payload_IN = texelFetch(ray_payloads, ivec2(gl_FragCoord.xy), 0);
  vec4  accumulation_IN = texelFetch(ray_accumulations, ivec2(gl_FragCoord.xy), 0);
  ivec4 intersection = texelFetch(intersections, ivec2(gl_FragCoord.xy), 0);
  
  vec3 wo = -direction_IN.xyz;
  vec3 throughput = payload_IN.xyz;
  
  if (direction_IN.w == 0.0)  {
    accumulation_OUT = accumulation_IN;
    payload_OUT      = vec4(0.0);
    terminate_ray();
    return;
  }

  if (!wr_IsValidIntersection(intersection) && depth == 0)  {
    accumulation_OUT = vec4(0);
    payload_OUT      = vec4(0);
    terminate_ray();
    return;
  }

  // Generate random numbers   
  uint counter = uint(pixel.x) + uint(pixel.y) * uint(frame.x) + 1u;
  vec4 random_lights = random4f((uint(sample_index) * 2u + 0u) * counter, seed.x, seed.y);
  vec4 random_bxdfs  = random4f((uint(sample_index) * 2u + 1u) * counter, seed.x, seed.y);

  vec3 origin         = wr_GetInterpolatedPosition(ads, intersection);
  origin = origin_IN.xyz + direction_IN.xyz * wr_GetHitDistance(intersection);
  
  ivec4 face          = wr_GetFace(ads, intersection);
  vec2 uv             = wr_GetInterpolatedTexCoords(ads, intersection);
  vec3 geom_normal    = wr_GetGeomNormal(ads, intersection);
  vec3 shading_normal = normalize(wr_GetInterpolatedNormal(ads, intersection));
  shading_normal      = faceforward(shading_normal, direction_IN.xyz, shading_normal);

  vec4 type_base_color   = texelFetch(materialBuffer, ivec2(0,face.w), 0);
  vec3 textureProperties = texelFetch(materialBuffer, ivec2(2,face.w), 0).rgb; // a == -1, for now

  int  basecolorTextureIndex = int(textureProperties.r);
  int  metallicTextureIndex = int(textureProperties.g);
  int  normalTextureIndex = int(textureProperties.b);

  type_base_color.gba = (basecolorTextureIndex == -1) ? type_base_color.gba : texture(texturesBuffer, vec3(uv.xy, basecolorTextureIndex)).rgb;
  vec4 normal_emission = (normalTextureIndex == -1) ? vec4(1,0,0,0) : texture(texturesBuffer, vec3(uv.xy, normalTextureIndex));
  vec4 metal_refl_gloss = (metallicTextureIndex == -1) ? vec4(0,0,0.06,0.8) : texture(texturesBuffer, vec3(uv.xy, metallicTextureIndex));

  vec3 v0  = wr_GetPosition(ads, intersection, face.x);
  vec3 v1  = wr_GetPosition(ads, intersection, face.y);
  vec3 v2  = wr_GetPosition(ads, intersection, face.z);
  vec2 uv0 = wr_GetTexCoords(ads, intersection, face.x);
  vec2 uv1 = wr_GetTexCoords(ads, intersection, face.y);
  vec2 uv2 = wr_GetTexCoords(ads, intersection, face.z);
  
  vec3 deltaPos1 = v1 - v0;
  vec3 deltaPos2 = v2 - v0;

  // UV delta
  vec2 deltaUV1 = uv1 - uv0;
  vec2 deltaUV2 = uv2 - uv0;

  /* TODO: Normal mapping needs revisiting */
  vec3 tangent = normalize(deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y);
  vec3 bitangent = normalize(deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x);

  vec3 NN = normal_emission.rgb;
  NN = NN * 2.0 - 1.0;
  vec3 N = mat3(tangent, bitangent, shading_normal) * normalize(NN);
  shading_normal = N;

  event surface;
  surface.basis = create_onb(shading_normal);
  surface.base_color = (basecolorTextureIndex == -1) ? type_base_color.gba : texture(texturesBuffer, vec3(uv.xy, basecolorTextureIndex)).rgb;
  surface.type = int(type_base_color.x);
  surface.shading_normal = shading_normal;
  surface.position = origin;
 
  /* Estimate Direct Lighting */
  float light_selection_pdf;
  wr_light light = Lights_sample(random_lights.x, light_selection_pdf);

  float light_pdf;
  float light_distance;
  vec3  wi;
  vec3  Li = Light_sample(surface, light, random_lights.yz, wi, light_pdf, light_distance);
  vec3  Ld = vec3(0);
  vec3  offset_oriign = origin + geom_normal * 0.05;
  
  if ( light_pdf > 0.0 && !is_zero(Li)) {
    float scattering_pdf = BxDF_pdf(surface, wo, wi);
    vec3 BxDF = BxDF_eval(surface, wo, wi);

    if(scattering_pdf > 0.0 && !is_zero(BxDF)) {
      float NdL = max(0.001, abs(dot(shading_normal, wi)));
      bool occluded = wr_QueryOcclusion(ads, offset_oriign, wi, light_distance - 0.05);

      if(!occluded) {
        Ld += (throughput * BxDF * NdL * Li) / (light_pdf * light_selection_pdf);
      }
    }
  }
  
#define RR_BOUNCES 3
  if (depth > RR_BOUNCES) {
			float t_probability = min(0.95, luminance(throughput));
			if (t_probability < random_lights.w) {
        accumulation_OUT = accumulation_IN;
        payload_OUT = vec4(0.0); 
        terminate_ray();
        return;
      }
			else {
        throughput /= t_probability;
      }
		}

  /* Next Event */
  float scattering_pdf;
  vec3 BxDF = BxDF_sample(surface, wo, random_bxdfs.xy, wi, scattering_pdf);
  if(scattering_pdf == 0.0 || is_zero(BxDF)) {
    accumulation_OUT = accumulation_IN;
    payload_OUT = vec4(0.0); 
    terminate_ray();
    return;
  }

  float NdL = max(0.001, abs(dot(shading_normal, wi)));

  accumulation_OUT = vec4(accumulation_IN.xyz + Ld, 1.0);
  direction_OUT = vec4(wi, 10000.0);
  origin_OUT = vec4(offset_oriign, 0.0);
  payload_OUT = vec4((throughput * BxDF * NdL) / max(0.001, scattering_pdf), 0); // throughput
}