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

#include <vector>
#include <algorithm>
#include <string>
#include <list>
#include <cfloat>
#include <cstring> // memset

#include "webrays_ads.h"

/* @see https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2 */
static unsigned int
wr_next_power_of_2(unsigned int number)
{
  number--;
  number |= number >> 1;
  number |= number >> 2;
  number |= number >> 4;
  number |= number >> 8;
  number |= number >> 16;
  number++;

  return (number > 15) ? number : 16;
}

static char const* const g_copysign_func =
  "float copysignf(float x, float y) { return "
  "intBitsToFloat((floatBitsToInt(x) & 0x7fffffff) | (floatBitsToInt(y) & "
  "0x80000000)); }\n";

static char const* const g_ray_triangle_intersection_func =
  R"glsl(vec3 wr_fast_intersect_triangle(vec3 direction, vec3 origin, vec3 v1, vec3 v2, vec3 v3, float t_max)
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
})glsl";

static char const* const g_linear_nodes_ray_intersect_fragment_shader =
  R"glsl(

uniform sampler2DArray wr_scene_vertices;
uniform isampler2DArray wr_scene_indices;

vec2
wr_GetBaryCoords(ivec4 intersection) {
	return intBitsToFloat(intersection.yz);
}

ivec4
wr_GetFace(ivec4 intersection) {
	return texelFetch(wr_scene_indices, ivec3(intersection.x % WR_PRIMITIVE_TEXTURE_SIZE, intersection.x / WR_PRIMITIVE_TEXTURE_SIZE, 0), 0);
}

vec3
wr_GetNormal(ivec4 intersection) {
	//ivec4 face = wr_Faces[intersection.x];
	//return wr_Normals[face.x].xyz;
	return vec3(1,0,0);
}

float
wr_GetHitDistance(ivec4 intersection) {
	return intBitsToFloat(intersection.w);
}

bool wr_query_occlusion(vec3 direction, vec3 origin, float t_max) {
	float min_distance = t_max;
	for (int primitive_index = 0; primitive_index < wr_TriangleCount; ++primitive_index) {
		ivec4 indices = texelFetch(wr_scene_indices, ivec3(primitive_index % WR_PRIMITIVE_TEXTURE_SIZE, primitive_index / WR_PRIMITIVE_TEXTURE_SIZE, 0), 0);
		vec3 v0 = texelFetch(wr_scene_vertices, ivec3(indices.x % WR_SCENE_TEXTURE_SIZE, indices.x / WR_SCENE_TEXTURE_SIZE, 0), 0).xyz;
  		vec3 v1 = texelFetch(wr_scene_vertices, ivec3(indices.y % WR_SCENE_TEXTURE_SIZE, indices.y / WR_SCENE_TEXTURE_SIZE, 0), 0).xyz;
  		vec3 v2 = texelFetch(wr_scene_vertices, ivec3(indices.z % WR_SCENE_TEXTURE_SIZE, indices.z / WR_SCENE_TEXTURE_SIZE, 0), 0).xyz;
		vec3 ret = wr_fast_intersect_triangle(direction, origin, v0, v1, v2, min_distance);
		if (ret.z < min_distance) {
			return true;
		}
	}
	return false;
}

ivec4
wr_query_intersection(vec3 ray_origin, vec3 ray_direction, float tmax) {
  float min_distance = tmax;
  ivec4 min_intersection_point = ivec4(-1,0,0,floatBitsToInt(tmax));
  bool hit = false;
#if wr_TriangleCount
  bool found = false;
  for(int primitive_index = 0; primitive_index < wr_TriangleCount; ++primitive_index) {
    ivec4 indices = texelFetch(wr_scene_indices, ivec3(primitive_index % WR_PRIMITIVE_TEXTURE_SIZE, primitive_index / WR_PRIMITIVE_TEXTURE_SIZE, 0), 0);
    vec3 v0 = texelFetch(wr_scene_vertices, ivec3(indices.x % WR_SCENE_TEXTURE_SIZE, indices.x / WR_SCENE_TEXTURE_SIZE, 0), 0).xyz;
  	vec3 v1 = texelFetch(wr_scene_vertices, ivec3(indices.y % WR_SCENE_TEXTURE_SIZE, indices.y / WR_SCENE_TEXTURE_SIZE, 0), 0).xyz;
  	vec3 v2 = texelFetch(wr_scene_vertices, ivec3(indices.z % WR_SCENE_TEXTURE_SIZE, indices.z / WR_SCENE_TEXTURE_SIZE, 0), 0).xyz;
	vec3 ret = wr_fast_intersect_triangle(ray_direction, ray_origin, v0, v1, v2, min_distance);
	if(ret.z < min_distance)
	{
        min_distance = ret.z;
  	    min_intersection_point = ivec4(primitive_index, floatBitsToInt(ret.xy), floatBitsToInt(ret.z));
	}
  }
#endif /* wr_TriangleCount */

#if wr_SphereCount
	for(int i = 0; i < wr_SphereCount; i++ ) {
		vec4 sphere = spheres.sphere[i]; /* {center_x, center_y, center_z, radius} */

		float dist = rg_fast_intersect_sphere(sphere.xyz, sphere.w, ray_direction, ray_origin, min_distance);

		if(dist < min_distance)
		{
			min_distance = dist;
			vec3 point_on_sphere = ray_origin + ray_direction * dist;
			vec3 normal = normalize(point_on_sphere - sphere.xyz);
			vec2 spher = vec2(atan(normal.y, normal.x), acos(normal.z));

			min_intersection_point = ivec4(wr_TriangleCount + i, floatBitsToInt(spher), floatBitsToInt(dist));
		}
	}
#endif /* wr_SphereCount */

  return min_intersection_point;
}
)glsl";

static char const* const g_ray_sahbvh_intersect_fragment_shader =
  R"glsl(

#if wr_InstanceCount
layout (std140) uniform wr_InstancesUBO
{
    mat4 instances[wr_InstanceCount];
};
#endif

vec2
wr_GetBaryCoords(ivec4 intersection) {
  return intBitsToFloat(intersection.yz);
}

vec3
wr_GetBaryCoords3D(ivec4 intersection) {
  vec3 barys;
  barys.yz = intBitsToFloat(intersection.yz);
  barys.x = 1.0 - barys.y - barys.z;
  return barys;
}

float
wr_GetHitDistance(ivec4 intersection) {
	return intBitsToFloat(intersection.w);
}

vec3
wr_GetInterpolatedNormal(int ads, ivec4 intersection) {
	ivec4 face = wr_GetFace(ads, intersection);
	vec3 b  = wr_GetBaryCoords3D(intersection);
	vec3 n0 = wr_GetNormal(ads, face.x);
  	vec3 n1 = wr_GetNormal(ads, face.y);
  	vec3 n2 = wr_GetNormal(ads, face.z);
		  
	vec3 normal = normalize(n0 * b.x + n1 * b.y + n2 * b.z);

#if wr_InstanceCount
    mat4 transform = transpose(instances[intersection.w]);
    return vec3( transform * vec4(normal, 0.0) );
#else
    return normal;
#endif
}

vec3
wr_GetInterpolatedPosition(int ads, ivec4 intersection) {
	ivec4 face = wr_GetFace(ads, intersection);
	vec3 b  = wr_GetBaryCoords3D(intersection);
	vec3 v0 = wr_GetPositionBLAS(ads, face.x);
  	vec3 v1 = wr_GetPositionBLAS(ads, face.y);
  	vec3 v2 = wr_GetPositionBLAS(ads, face.z);
	
	vec3 position = v0 * b.x + v1 * b.y + v2 * b.z;
#if wr_InstanceCount
    mat4 transform = transpose(instances[intersection.w]);
    return vec3( transform * vec4(position, 1.0) );
#else
    return position;
#endif
}

vec2
wr_GetInterpolatedTexCoords(int ads, ivec4 intersection) {
	ivec4 face = wr_GetFace(ads, intersection);
	vec3 b     = wr_GetBaryCoords3D(intersection);
	 
	vec2 t0    = wr_GetTexCoords(ads, face.x);		  
	vec2 t1    = wr_GetTexCoords(ads, face.y);		  
	vec2 t2    = wr_GetTexCoords(ads, face.z);		  

    return t0 * b.x + t1 * b.y + t2 * b.z;
}

vec3
wr_GetGeomNormal(int ads, ivec4 intersection) {
	ivec4 face = wr_GetFace(ads, intersection);
	
	vec3 v0 = wr_GetPosition(ads, face.x);
  	vec3 v1 = wr_GetPosition(ads, face.y);
  	vec3 v2 = wr_GetPosition(ads, face.z);
	
	vec3 normal = normalize(cross(v1 - v0, v2 - v0));
#if wr_InstanceCount
    mat4 transform = transpose(instances[intersection.w]);
    return vec3( transform * vec4(normal, 0.0) );
#else
    return normal;
#endif
}

void
wr_Swapf(inout float a, inout float b) {
  float temp = a;
  a = b;
  b = temp;
}

float
wr_BoundsIntersect(vec3 vmin, vec3 vmax, vec3 rpos, vec3 dirfrac, float tmax)
{
  float t0 = 0.0, t1 = tmax;
  for (int i = 0; i < 3; ++i) {
    float tNear = (vmin[i] - rpos[i]) * dirfrac[i];
    float tFar  = (vmax[i] - rpos[i]) * dirfrac[i];
    if (dirfrac[i] < 0.0) wr_Swapf(tNear, tFar);
    t0 = tNear > t0 ? tNear : t0;
    t1 = tFar  < t1 ? tFar  : t1;
    if (t0 > t1) return -1.0;
  }

  return 1.0;
}

ivec4
wr_query_shape_intersection(int ads, vec3 ray_origin, vec3 ray_direction, float tmax) {
  float min_distance = tmax;
  ivec4 min_intersection_point = ivec4(-1,0,0,floatBitsToInt(tmax));
  bool hit = false;

#if wr_TriangleCount && wr_BVHNodeCount
  vec3 invDir = vec3(1.0 / ray_direction.x, 1.0 / ray_direction.y, 1.0 / ray_direction.z);
  bvec3 dirIsNeg = bvec3(invDir.x < 0.0, invDir.y < 0.0, invDir.z < 0.0);
  int toVisitOffset = 0, currentNodeIndex = 0;
  int nodesToVisit[WR_TRAVERSE_STACK_SIZE];
  bool found = false;
  for(int loop = 0; loop < wr_BVHNodeCount; ++loop) {
  //while(true) {
    vec4 bound_packed_min = wr_GetPackedBoundMin(ads, currentNodeIndex);
    vec4 bound_packed_max = wr_GetPackedBoundMax(ads, currentNodeIndex);
    vec3 bound_min = bound_packed_min.xyz;
    vec3 bound_max = bound_packed_max.xyz;
	ivec2 node_info = ivec2(floatBitsToInt(bound_packed_min.w), floatBitsToInt(bound_packed_max.w));
	int node_offset = node_info.x;
    int nPrimitives = (node_info.y & 0x0000FFFF);
	int axis = (node_info.y & 0x00FF0000) >> 16;
    found = wr_BoundsIntersect(bound_min, bound_max, ray_origin, invDir, min_distance) > 0.0;
    if (found) {
      if (nPrimitives > 0 ) {
        for (int i = 0; i < nPrimitives; i++ ) {
          int primitve_index = node_offset + i;
          ivec4 indices = wr_GetIndices(ads, primitve_index);
    	  vec3 v0 = wr_GetPosition(ads, indices.x);
    	  vec3 v1 = wr_GetPosition(ads, indices.y);
    	  vec3 v2 = wr_GetPosition(ads, indices.z);
		  vec3 ret = wr_fast_intersect_triangle(ray_direction, ray_origin, v0, v1, v2, min_distance);
		  if(ret.z < min_distance)
		  {
            min_distance = ret.z;
  			min_intersection_point = ivec4(primitve_index, floatBitsToInt(ret.xy), floatBitsToInt(ret.z));
		  }
        }
        if (toVisitOffset == 0)
          break;
          currentNodeIndex = nodesToVisit[--toVisitOffset];
      } else {
        if (dirIsNeg[axis]) {
          nodesToVisit[toVisitOffset++] = currentNodeIndex + 1;
          currentNodeIndex = node_offset;
        } else {
          nodesToVisit[toVisitOffset++] = node_offset;
          currentNodeIndex = currentNodeIndex + 1;
        }
      }
    } else {
      if (toVisitOffset == 0)
        break;
      currentNodeIndex = nodesToVisit[--toVisitOffset];
    }
  }
#endif /* wr_TriangleCount */

#if wr_SphereCount
	for(int i = 0; i < wr_SphereCount; i++ ) {
		vec4 sphere = spheres.sphere[i]; /* {center_x, center_y, center_z, radius} */

		float dist = rg_fast_intersect_sphere(sphere.xyz, sphere.w, ray_direction, ray_origin, min_distance);

		if(dist < min_distance)
		{
			min_distance = dist;
			vec3 point_on_sphere = ray_origin + ray_direction * dist;
			vec3 normal = normalize(point_on_sphere - sphere.xyz);
			vec2 spher = vec2(atan(normal.y, normal.x), acos(normal.z));

			min_intersection_point = ivec4(wr_TriangleCount + i, floatBitsToInt(spher), floatBitsToInt(dist));
		}
	}
#endif /* wr_SphereCount */

  return min_intersection_point;
}

ivec4
wr_query_intersection(int ads, vec3 ray_origin, vec3 ray_direction, float tmax) {
#if wr_InstanceCount
  float min_distance = tmax;
  ivec4 min_intersection_point = ivec4(-1,0,0,floatBitsToInt(tmax));
  for (int i = 0; i < wr_InstanceCount; ++i) {
	mat4 transform = inverse(transpose(instances[i]));
	vec3 ray_origin2 = vec3( transform * vec4(ray_origin, 1.0) );
	vec3 ray_direction2 = vec3( transform * vec4(ray_direction, 0.0) );
	ivec4 intersection = wr_query_shape_intersection(ads, ray_origin2, ray_direction2, min_distance);
    if(intBitsToFloat(intersection.w) < min_distance)
	{
      min_distance = intBitsToFloat(intersection.w);
  	  min_intersection_point.xyz = intersection.xyz;
	  min_intersection_point.w = i;
	}
  }

  return min_intersection_point;
#else

  return wr_query_shape_intersection(ads, ray_origin, ray_direction, tmax);

#endif
}

bool
wr_query_occlusion(int ads, vec3 ray_origin, vec3 ray_direction, float tMax) {
  float min_distance = tMax;

#if wr_TriangleCount && wr_BVHNodeCount
  vec3 invDir = vec3(1.0 / ray_direction.x, 1.0 / ray_direction.y, 1.0 / ray_direction.z);
  bvec3 dirIsNeg = bvec3(invDir.x < 0.0, invDir.y < 0.0, invDir.z < 0.0);
  int toVisitOffset = 0, currentNodeIndex = 0;
  int nodesToVisit[WR_TRAVERSE_STACK_SIZE];
  bool found = false;
  for(int loop = 0; loop < wr_BVHNodeCount; ++loop) {
  //while(true) {
	vec4 bound_packed_min = wr_GetPackedBoundMin(ads, currentNodeIndex);
    vec4 bound_packed_max = wr_GetPackedBoundMax(ads, currentNodeIndex);
    vec3 bound_min = bound_packed_min.xyz;
    vec3 bound_max = bound_packed_max.xyz;        
    ivec2 node_info = ivec2(floatBitsToInt(bound_packed_min.w), floatBitsToInt(bound_packed_max.w));

	int node_offset = node_info.x;
    int nPrimitives = (node_info.y & 0x0000FFFF);
    int axis = (node_info.y & 0x00FF0000) >> 16;
    found = wr_BoundsIntersect(bound_min, bound_max, ray_origin, invDir, tMax) > 0.0;
    if (found) {
      if (nPrimitives > 0 ) {
        for (int i = 0; i < nPrimitives; i++ ) {
          int primitve_index = node_offset + i;
          ivec4 indices = wr_GetIndices(ads, primitve_index);
    	  vec3 v0 = wr_GetPosition(ads, indices.x);
    	  vec3 v1 = wr_GetPosition(ads, indices.y);
    	  vec3 v2 = wr_GetPosition(ads, indices.z);
	      vec3 ret = wr_fast_intersect_triangle(ray_direction, ray_origin, v0, v1, v2, tMax);
	      if( ret.z < tMax ) { 
			return true;
	      }
        }
        if (toVisitOffset == 0)
          break;
        currentNodeIndex = nodesToVisit[--toVisitOffset];
      } else {
        if (dirIsNeg[axis]) {
          nodesToVisit[toVisitOffset++] = currentNodeIndex + 1;
          currentNodeIndex = node_offset;
        } else {
          nodesToVisit[toVisitOffset++] = node_offset;
          currentNodeIndex = currentNodeIndex + 1;
        }
      }
    } else {
      if (toVisitOffset == 0)
        break;
      currentNodeIndex = nodesToVisit[--toVisitOffset];
    }
  }
#endif /* wr_TriangleCount */

#if wr_SphereCount
  /* Special handling for spheres */
  for(int i = 0; i < wr_SphereCount; i++ ) {
	vec4 sphere = spheres.sphere[i]; /* {center_x, center_y, center_z, radius} */

	float dist = rg_fast_intersect_sphere(sphere.xyz, sphere.w, ray_direction, ray_origin, WR_RAY_MAX_DISTANCE);

	if(dist < tMax)
	{
	  return true;
	}
  }
#endif /* wr_SphereCount */

  return false;
}
)glsl";

static char const* const g_widebvh_get_instance_transfrom =
  R"glsl(

int wr_GetBLASID(int ads, int instance) { 
  int ads_id = wr_GetAdsID(ads);

  int b = 4 * instance + 3; 
  return floatBitsToInt(texelFetch(wr_scene_instances, ivec3(b % WR_INSTANCE_TEXTURE_SIZE, 0, ads_id), 0).r); 
}

mat4 wr_GetObjectTranform(int ads, int instance) { 
#if wr_InstanceCount
  int ads_id = wr_GetAdsID(ads);
  int b = 4 * instance; 
  vec4 col0 = texelFetch(wr_scene_instances, ivec3((b + 0) % WR_INSTANCE_TEXTURE_SIZE, 0, ads_id), 0); 
  vec4 col1 = texelFetch(wr_scene_instances, ivec3((b + 1) % WR_INSTANCE_TEXTURE_SIZE, 0, ads_id), 0); 
  vec4 col2 = texelFetch(wr_scene_instances, ivec3((b + 2) % WR_INSTANCE_TEXTURE_SIZE, 0, ads_id), 0); 

  return mat4(vec4(col0.xyz, 0.0), vec4(col1.xyz, 0.0), vec4(col2.xyz, 0.0), vec4(col0.w, col1.w, col2.w, 1.0));
#else
  return mat4(1.0);
#endif
}

mat4 wr_GetNormalTranform(int ads, int instance) { 
#if wr_InstanceCount
  int ads_id = wr_GetAdsID(ads);
  int b = 4 * instance; 
  vec4 col0 = texelFetch(wr_scene_instances, ivec3((b + 0) % WR_INSTANCE_TEXTURE_SIZE, (b + 0) / WR_INSTANCE_TEXTURE_SIZE, ads_id), 0); 
  vec4 col1 = texelFetch(wr_scene_instances, ivec3((b + 1) % WR_INSTANCE_TEXTURE_SIZE, (b + 1) / WR_INSTANCE_TEXTURE_SIZE, ads_id), 0); 
  vec4 col2 = texelFetch(wr_scene_instances, ivec3((b + 2) % WR_INSTANCE_TEXTURE_SIZE, (b + 2) / WR_INSTANCE_TEXTURE_SIZE, ads_id), 0); 

  return transpose(inverse(mat4(vec4(col0.xyz, 0.0), vec4(col1.xyz, 0.0), vec4(col2.xyz, 0.0), vec4(col0.w, col1.w, col2.w, 1.0f))));
#else
  return mat4(1.0);
#endif
}

)glsl";

static char const* const g_widebvh_get_ads_id =
  R"glsl(

bool wr_IsValidIntersection(ivec4 intersection) {
  return !(intersection.x < 0);
}

int wr_GetAdsID(int ads) {
#if wr_InstanceCount
	return (WR_IS_TLAS(ads)) ? (ads & ~WR_TLAS_ID_MASK) : ads;
#else
	return ads;
#endif
}
)glsl";

static char const* const g_widebvh_attribute_accessors =
  R"glsl(
int wr_GetTriangleID(int ads, ivec4 intersection) {
#if wr_InstanceCount
  return ( WR_IS_TLAS(ads) ) ? (((1 << WR_INSTANCE_TRIANGLE_SPLIT_BIT) - 1) & intersection.x) : intersection.x;
#else
  return intersection.x;
#endif
}

int wr_GetInstanceID(int ads, ivec4 intersection) {
#if wr_InstanceCount
  return (intersection.x >> WR_INSTANCE_TRIANGLE_SPLIT_BIT); 
#else
  return -1;
#endif
}

ivec4 wr_GetFace(int ads, ivec4 intersection) {
	int ads_id = wr_GetAdsID(ads);
#if wr_InstanceCount
	int instance = wr_GetInstanceID(ads, intersection);
  int blas_id  =  WR_IS_TLAS(ads) ? wr_GetBLASID(ads, instance) : ads_id;
#else
  int blas_id  = ads_id;
#endif
  return texelFetch(wr_scene_indices, ivec3(wr_GetTriangleID(ads, intersection) % WR_PRIMITIVE_TEXTURE_SIZE, wr_GetTriangleID(ads, intersection) / WR_PRIMITIVE_TEXTURE_SIZE, blas_id), 0);
}

ivec4 wr_GetIndicesBLAS(int blas, int i) { 
  return texelFetch(wr_scene_indices, ivec3(i % WR_PRIMITIVE_TEXTURE_SIZE, i / WR_PRIMITIVE_TEXTURE_SIZE, blas), 0); 
}

vec3 wr_GetNormalBLAS(int blas, int i) { 
  return texelFetch(wr_scene_vertices, ivec3(i % WR_SCENE_TEXTURE_SIZE, i / WR_SCENE_TEXTURE_SIZE, blas * 2 + 1), 0).xyz; 
}

vec3 wr_GetPositionBLAS(int blas, int i) {
  return texelFetch(wr_scene_vertices, ivec3(i % WR_SCENE_TEXTURE_SIZE, i / WR_SCENE_TEXTURE_SIZE, blas * 2 + 0), 0).xyz; 
}

vec2 wr_GetTexCoordsBLAS(int blas, int i) {
  return vec2(texelFetch(wr_scene_vertices, ivec3(i % WR_SCENE_TEXTURE_SIZE, i / WR_SCENE_TEXTURE_SIZE, blas * 2 + 0), 0).w, texelFetch(wr_scene_vertices, ivec3(i % WR_SCENE_TEXTURE_SIZE, i / WR_SCENE_TEXTURE_SIZE, blas * 2 + 1), 0).w); 
}

vec2 wr_GetBaryCoords(ivec4 intersection) {
  return intBitsToFloat(intersection.yz); 
}

vec3 wr_GetBaryCoords3D(ivec4 intersection) {
  vec3 barys; barys.yz = intBitsToFloat(intersection.yz); barys.x = 1.0 - barys.y - barys.z; return barys; 
}

float wr_GetHitDistance(ivec4 intersection) {
  return intBitsToFloat(intersection.w); 
}

ivec4 wr_GetIndices(int ads, ivec4 intersection, int i) { 
  int ads_id = wr_GetAdsID(ads);
#if wr_InstanceCount
	int instance = wr_GetInstanceID(ads, intersection);
  int blas_id  =  WR_IS_TLAS(ads) ? wr_GetBLASID(ads, instance) : ads_id;
#else
  int blas_id  = ads_id;
#endif
  return wr_GetIndicesBLAS(blas_id, i);
}

vec3 wr_GetNormal(int ads, ivec4 intersection, int i) { 
  int ads_id = wr_GetAdsID(ads);
#if wr_InstanceCount
	int instance = wr_GetInstanceID(ads, intersection);
  int blas_id  =  WR_IS_TLAS(ads) ? wr_GetBLASID(ads, instance) : ads_id;
#else
  int blas_id  = ads_id;
#endif
  vec3 normal = wr_GetNormalBLAS(blas_id, i);
	normal = normalize(normal);
#if wr_InstanceCount
	if ( WR_IS_TLAS(ads) )
	  return normalize(wr_TransformDirectionFromObjectToWorldSpace(ads, instance, normal));
	else
    return normal;
#else  
    return normal;
#endif
}

vec3 wr_GetPosition(int ads, ivec4 intersection, int i) {
  int ads_id = wr_GetAdsID(ads);
#if wr_InstanceCount
	int instance = wr_GetInstanceID(ads, intersection);
  int blas_id  =  WR_IS_TLAS(ads) ? wr_GetBLASID(ads, instance) : ads_id;
#else
  int blas_id  = ads_id;
#endif
  vec3 position = wr_GetPositionBLAS(blas_id, i);
#if wr_InstanceCount
	if ( WR_IS_TLAS(ads) )
	  return wr_TransformPositionFromObjectToWorldSpace(ads, instance, position);
	else
    return position;
#else  
    return position;
#endif
}

vec2 wr_GetTexCoords(int ads, ivec4 intersection, int i) {
  int ads_id = wr_GetAdsID(ads);
#if wr_InstanceCount
	int instance = wr_GetInstanceID(ads, intersection);
  int blas_id  =  WR_IS_TLAS(ads) ? wr_GetBLASID(ads, instance) : ads_id;
#else
  int blas_id  = ads_id;
#endif
  return wr_GetTexCoordsBLAS(blas_id, i);
}

vec3
wr_GetInterpolatedNormal(int ads, ivec4 intersection) {
	int ads_id = wr_GetAdsID(ads);
#if wr_InstanceCount
	int instance = wr_GetInstanceID(ads, intersection);
    int blas_id  =  WR_IS_TLAS(ads) ? wr_GetBLASID(ads, instance) : ads_id;
#else
    int blas_id  = ads_id;
#endif
	ivec4 face = wr_GetIndicesBLAS(blas_id, wr_GetTriangleID(ads, intersection));
	vec3 b  = wr_GetBaryCoords3D(intersection);
	vec3 n0 = wr_GetNormalBLAS(blas_id, face.x);
	vec3 n1 = wr_GetNormalBLAS(blas_id, face.y);
	vec3 n2 = wr_GetNormalBLAS(blas_id, face.z);
	vec3 normal = n0 * b.x + n1 * b.y + n2 * b.z;
	normal = normalize(normal);
#if wr_InstanceCount
	if ( WR_IS_TLAS(ads) )
	  return normalize(wr_TransformDirectionFromObjectToWorldSpace(ads, instance, normal));
	else
      return normal;
#else  
    return normal;
#endif
}
vec3
wr_GetInterpolatedPosition(int ads, ivec4 intersection) {
	int ads_id = wr_GetAdsID(ads);
#if wr_InstanceCount
	int instance = wr_GetInstanceID(ads, intersection);
    int blas_id  =  WR_IS_TLAS(ads) ? wr_GetBLASID(ads, instance) : ads_id;
#else
    int blas_id  = ads_id;
#endif
	ivec4 face = wr_GetIndicesBLAS(blas_id, wr_GetTriangleID(ads, intersection));
	vec3 b  = wr_GetBaryCoords3D(intersection);
	vec3 v0 = wr_GetPositionBLAS(blas_id, face.x);
  	vec3 v1 = wr_GetPositionBLAS(blas_id, face.y);
  	vec3 v2 = wr_GetPositionBLAS(blas_id, face.z);	
	vec3 position = v0 * b.x + v1 * b.y + v2 * b.z;
#if wr_InstanceCount
	if ( WR_IS_TLAS(ads) )
	  return wr_TransformPositionFromObjectToWorldSpace(ads, instance, position);
	else
	  return position;
#else  
    return position;
#endif
}
vec2
wr_GetInterpolatedTexCoords(int ads, ivec4 intersection) {
	int ads_id = wr_GetAdsID(ads);
#if wr_InstanceCount
	int instance = wr_GetInstanceID(ads, intersection);
    int blas_id  =  WR_IS_TLAS(ads) ? wr_GetBLASID(ads, instance) : ads_id;
#else
    int blas_id  = ads_id;
#endif
	ivec4 face = wr_GetIndicesBLAS(blas_id, wr_GetTriangleID(ads, intersection));
	vec3 b     = wr_GetBaryCoords3D(intersection);	 
	vec2 t0    = wr_GetTexCoordsBLAS(blas_id, face.x);		  
	vec2 t1    = wr_GetTexCoordsBLAS(blas_id, face.y);		  
	vec2 t2    = wr_GetTexCoordsBLAS(blas_id, face.z);
    return t0 * b.x + t1 * b.y + t2 * b.z;
}

vec3
wr_GetGeomNormal(int ads, ivec4 intersection) {
	int ads_id = wr_GetAdsID(ads);
#if wr_InstanceCount
	int instance = wr_GetInstanceID(ads, intersection);
    int blas_id  = WR_IS_TLAS(ads) ? wr_GetBLASID(ads, instance) : ads_id;
#else
    int blas_id  = ads_id;
#endif

	ivec4 face = wr_GetIndicesBLAS(blas_id, wr_GetTriangleID(ads, intersection));
	vec3 v0 = wr_GetPositionBLAS(blas_id, face.x);
  	vec3 v1 = wr_GetPositionBLAS(blas_id, face.y);
  	vec3 v2 = wr_GetPositionBLAS(blas_id, face.z);	
	vec3 normal = cross(v1 - v0, v2 - v0);
	normal = normalize(normal);
#if wr_InstanceCount
	if ( WR_IS_TLAS(ads) )
	  return normalize(wr_TransformDirectionFromObjectToWorldSpace(ads, instance, normal));
	else
      return normal;
#else  
    return normal;
#endif
}
)glsl";

static char const* const g_ray_widebvh_intersect_fragment_shader =
  R"glsl(
void
wr_Swapf(inout float a, inout float b) {
  float temp = a;
  a = b;
  b = temp;
}

float fmax3(float a, float b, float c) { return max(a, max(b, c)); }
float fmin3(float a, float b, float c) { return min(a, min(b, c)); }

void intersectChildren(int ads, int node_index, inout ivec2 G, inout ivec2 Gt, const in vec3 ray_orig, const in vec3 ray_dir, float ray_tMax)
{
	vec4 p = wr_GetPackedPosExyzMask(ads, node_index);
	ivec4 node_data1 = wr_GetPackedNodeTriangleBaseIndexMeta(ads, node_index);
	ivec4 node_data2 = wr_GetPackedChildBBOX0(ads, node_index);
	ivec4 node_data3 = wr_GetPackedChildBBOX1(ads, node_index);
	ivec4 node_data4 = wr_GetPackedChildBBOX2(ads, node_index);

	int exyzmask = floatBitsToInt(p.w);
	int ex = exyzmask & 0xFF;
	int ey = (exyzmask >> 8) & 0xFF;
	int ez = (exyzmask >> 16) & 0xFF;
		
	int imask = (exyzmask >> 24) & 0xFF;

	int child_node_base_index = node_data1.x; // NEED cast to uint
	int triangle_base_index = node_data1.y; // NEED cast to uint
	int meta0 = node_data1.z; // NEED cast to uint
	int meta1 = node_data1.w; // NEED cast to uint

	int childBBOX[12];
	childBBOX[0] = (node_data2.x); // NEED cast to uint
	childBBOX[1] = (node_data2.y); // NEED cast to uint
	childBBOX[2] = (node_data2.z); // NEED cast to uint
	childBBOX[3] = (node_data2.w); // NEED cast to uint
	childBBOX[4] = (node_data3.x); // NEED cast to uint
	childBBOX[5] = (node_data3.y); // NEED cast to uint
	childBBOX[6] = (node_data3.z); // NEED cast to uint
	childBBOX[7] = (node_data3.w); // NEED cast to uint
	childBBOX[8] = (node_data4.x); // NEED cast to uint
	childBBOX[9] = (node_data4.y); // NEED cast to uint
	childBBOX[10] = (node_data4.z); // NEED cast to uint
	childBBOX[11] = (node_data4.w); // NEED cast to uint
	
	int hitmask = 0;
	const float ooeps = 1e-40;
	float idirx = 1.0f / (abs(ray_dir.x) > ooeps ? ray_dir.x : copysignf(ooeps, ray_dir.x)); // inverse ray direction
	float idiry = 1.0f / (abs(ray_dir.y) > ooeps ? ray_dir.y : copysignf(ooeps, ray_dir.y)); // inverse ray direction
	float idirz = 1.0f / (abs(ray_dir.z) > ooeps ? ray_dir.z : copysignf(ooeps, ray_dir.z)); // inverse ray direction

	float adjusted_idirx = intBitsToFloat((ex) << 23) * idirx;
	float adjusted_idiry = intBitsToFloat((ey) << 23) * idiry;
	float adjusted_idirz = intBitsToFloat((ez) << 23) * idirz;
	float origx = (p.x - ray_orig.x) * idirx;
	float origy = (p.y - ray_orig.y) * idiry;
	float origz = (p.z - ray_orig.z) * idirz;

	int swizzledLox = (ray_dir.x < 0.0) ? childBBOX[6] : childBBOX[0];
	int swizzledHix = (ray_dir.x < 0.0) ? childBBOX[0] : childBBOX[6];
	int swizzledLoy = (ray_dir.y < 0.0) ? childBBOX[8] : childBBOX[2];
	int swizzledHiy = (ray_dir.y < 0.0) ? childBBOX[2] : childBBOX[8];
	int swizzledLoz = (ray_dir.z < 0.0) ? childBBOX[10] : childBBOX[4];
	int swizzledHiz = (ray_dir.z < 0.0) ? childBBOX[4] : childBBOX[10];

	float tminx[4];
	float tminy[4];
	float tminz[4];
	float tmaxx[4];
	float tmaxy[4];
	float tmaxz[4];
	tminx[0] = float((swizzledLox >> 0) & 0xFF)* adjusted_idirx + origx;
	tminx[1] = float((swizzledLox >> 8) & 0xFF)* adjusted_idirx + origx;
	tminx[2] = float((swizzledLox >> 16) & 0xFF)* adjusted_idirx + origx;
	tminx[3] = float((swizzledLox >> 24) & 0xFF)* adjusted_idirx + origx;
	tminy[0] = float((swizzledLoy >> 0) & 0xFF)* adjusted_idiry + origy;
	tminy[1] = float((swizzledLoy >> 8) & 0xFF)* adjusted_idiry + origy;
	tminy[2] = float((swizzledLoy >> 16) & 0xFF)* adjusted_idiry + origy;
	tminy[3] = float((swizzledLoy >> 24) & 0xFF)* adjusted_idiry + origy;
	tminz[0] = float((swizzledLoz >> 0) & 0xFF)* adjusted_idirz + origz;
	tminz[1] = float((swizzledLoz >> 8) & 0xFF)* adjusted_idirz + origz;
	tminz[2] = float((swizzledLoz >> 16) & 0xFF)* adjusted_idirz + origz;
	tminz[3] = float((swizzledLoz >> 24) & 0xFF)* adjusted_idirz + origz;
	tmaxx[0] = float((swizzledHix >> 0) & 0xFF)* adjusted_idirx + origx;
	tmaxx[1] = float((swizzledHix >> 8) & 0xFF)* adjusted_idirx + origx;
	tmaxx[2] = float((swizzledHix >> 16) & 0xFF)* adjusted_idirx + origx;
	tmaxx[3] = float((swizzledHix >> 24) & 0xFF)* adjusted_idirx + origx;
	tmaxy[0] = float((swizzledHiy >> 0) & 0xFF)* adjusted_idiry + origy;
	tmaxy[1] = float((swizzledHiy >> 8) & 0xFF)* adjusted_idiry + origy;
	tmaxy[2] = float((swizzledHiy >> 16) & 0xFF)* adjusted_idiry + origy;
	tmaxy[3] = float((swizzledHiy >> 24) & 0xFF)* adjusted_idiry + origy;
	tmaxz[0] = float((swizzledHiz >> 0) & 0xFF)* adjusted_idirz + origz;
	tmaxz[1] = float((swizzledHiz >> 8) & 0xFF)* adjusted_idirz + origz;
	tmaxz[2] = float((swizzledHiz >> 16) & 0xFF)* adjusted_idirz + origz;
	tmaxz[3] = float((swizzledHiz >> 24) & 0xFF)* adjusted_idirz + origz;

	float tmin = 0.0;
	float tmax = ray_tMax;
	for (int childIndex = 0; childIndex < 4; childIndex++)
	{
		// Use VMIN, VMAX to compute the slabs
		float cmin = max(fmax3(tminx[childIndex], tminy[childIndex], tminz[childIndex]), tmin);
		float cmax = min(fmin3(tmaxx[childIndex], tmaxy[childIndex], tmaxz[childIndex]), tmax);

		bool intersected = cmin <= cmax;
		if (!intersected)
			continue;

		int childMeta = meta0 >> (8 * (childIndex % 4));
		childMeta = childMeta & 0xFF;

		hitmask = hitmask | ((childMeta >> 5) << (childMeta & 31));
	}

	// The remaining 4 nodes
	{
		int swizzledLox = (ray_dir.x < 0.0) ? childBBOX[7] : childBBOX[1];
		int swizzledHix = (ray_dir.x < 0.0) ? childBBOX[1] : childBBOX[7];
		int swizzledLoy = (ray_dir.y < 0.0) ? childBBOX[9] : childBBOX[3];
		int swizzledHiy = (ray_dir.y < 0.0) ? childBBOX[3] : childBBOX[9];
		int swizzledLoz = (ray_dir.z < 0.0) ? childBBOX[11] : childBBOX[5];
		int swizzledHiz = (ray_dir.z < 0.0) ? childBBOX[5] : childBBOX[11];

		float tminx[4];
		float tminy[4];
		float tminz[4];
		float tmaxx[4];
		float tmaxy[4];
		float tmaxz[4];
		tminx[0] = float((swizzledLox >> 0) & 0xFF)* adjusted_idirx + origx;
		tminx[1] = float((swizzledLox >> 8) & 0xFF)* adjusted_idirx + origx;
		tminx[2] = float((swizzledLox >> 16) & 0xFF)* adjusted_idirx + origx;
		tminx[3] = float((swizzledLox >> 24) & 0xFF)* adjusted_idirx + origx;
		tminy[0] = float((swizzledLoy >> 0) & 0xFF)* adjusted_idiry + origy;
		tminy[1] = float((swizzledLoy >> 8) & 0xFF)* adjusted_idiry + origy;
		tminy[2] = float((swizzledLoy >> 16) & 0xFF)* adjusted_idiry + origy;
		tminy[3] = float((swizzledLoy >> 24) & 0xFF)* adjusted_idiry + origy;
		tminz[0] = float((swizzledLoz >> 0) & 0xFF)* adjusted_idirz + origz;
		tminz[1] = float((swizzledLoz >> 8) & 0xFF)* adjusted_idirz + origz;
		tminz[2] = float((swizzledLoz >> 16) & 0xFF)* adjusted_idirz + origz;
		tminz[3] = float((swizzledLoz >> 24) & 0xFF)* adjusted_idirz + origz;
		tmaxx[0] = float((swizzledHix >> 0) & 0xFF)* adjusted_idirx + origx;
		tmaxx[1] = float((swizzledHix >> 8) & 0xFF)* adjusted_idirx + origx;
		tmaxx[2] = float((swizzledHix >> 16) & 0xFF)* adjusted_idirx + origx;
		tmaxx[3] = float((swizzledHix >> 24) & 0xFF)* adjusted_idirx + origx;
		tmaxy[0] = float((swizzledHiy >> 0) & 0xFF)* adjusted_idiry + origy;
		tmaxy[1] = float((swizzledHiy >> 8) & 0xFF)* adjusted_idiry + origy;
		tmaxy[2] = float((swizzledHiy >> 16) & 0xFF)* adjusted_idiry + origy;
		tmaxy[3] = float((swizzledHiy >> 24) & 0xFF)* adjusted_idiry + origy;
		tmaxz[0] = float((swizzledHiz >> 0) & 0xFF)* adjusted_idirz + origz;
		tmaxz[1] = float((swizzledHiz >> 8) & 0xFF)* adjusted_idirz + origz;
		tmaxz[2] = float((swizzledHiz >> 16) & 0xFF)* adjusted_idirz + origz;
		tmaxz[3] = float((swizzledHiz >> 24) & 0xFF)* adjusted_idirz + origz;

		float tmin = 0.0;
		float tmax = ray_tMax;
		for (int childIndex = 0; childIndex < 4; childIndex++)
		{
			// Use VMIN, VMAX to compute the slabs
			float cmin = max(fmax3(tminx[childIndex], tminy[childIndex], tminz[childIndex]), tmin);
			float cmax = min(fmin3(tmaxx[childIndex], tmaxy[childIndex], tmaxz[childIndex]), tmax);

			bool intersected = cmin <= cmax;
			if (!intersected)
				continue;

			int childMeta = meta1 >> (8 * (childIndex % 4));
			childMeta = childMeta & 0xFF;
			int prefix = (childMeta >> 5);
			int postfix = (childMeta) & 31;
			hitmask = hitmask | (prefix << postfix);
		}
	}
	G.x = child_node_base_index;
	G.y = (hitmask & 0xFF000000) | imask;
	Gt.y = hitmask & 0x00FFFFFF;
	Gt.x = triangle_base_index;
}

#define wr_QueryShapeIntersection wr_query_shape_intersection
ivec4
wr_query_shape_intersection(int ads, vec3 ray_origin, vec3 ray_direction, float tmax) {
  float min_distance = tmax;
  ivec4 min_intersection_point = ivec4(-1,0,0,floatBitsToInt(tmax));
  bool hit = false;
#if wr_TriangleCount && wr_BVHNodeCount
  int toVisitOffset = 0, currentNodeIndex = 0;
  ivec2 nodesToVisit[WR_TRAVERSE_STACK_SIZE];
  nodesToVisit[0] = ivec2(0, 0x80000000);
  
  ivec2 nodeGroup = ivec2(0, 0x1000000);
  ivec2 triangleGroup = ivec2(0,0);

  for(int loop = 0; loop < wr_BVHNodeCount; ++loop) {
  //while(true) {
    if (uint(nodeGroup.y) > 0x00FFFFFFu)//if ((nodeGroup.y & 0xFF000000) != 0) //if (nodeGroup.y > 0x00FFFFFF) // it represents a node group
    {
	  int n = 0;
	  for (int i = 24; i < 32; ++i)
	  {
	    if ((nodeGroup.y & (1 << i)) != 0)
		{
		  n = i - 24;
		  break;
        }
	  }
      nodeGroup.y = nodeGroup.y & (~(1 << (n + 24)));
      if(((nodeGroup.y >> 24) & 0xFFFF) != 0)
	  {
		// push the remaining nodes to the stack
		nodesToVisit[++toVisitOffset] = nodeGroup;
	  }

	  intersectChildren(ads, nodeGroup.x + n, nodeGroup, triangleGroup, ray_origin, ray_direction, min_distance);
	}
    else
	{
	  triangleGroup = nodeGroup; 
	  nodeGroup = ivec2( 0,0 );
	}

	int triangle_hits = triangleGroup.y; 
	int relative_index_of_triangle = 0;
	while (triangle_hits > 0) 
	{
		ivec4 indices = wr_GetIndicesBLAS(ads, triangleGroup.x + relative_index_of_triangle);
		float u, v;
		vec3 v0 = wr_GetPositionBLAS(ads, indices.x);
    	vec3 v1 = wr_GetPositionBLAS(ads, indices.y);
    	vec3 v2 = wr_GetPositionBLAS(ads, indices.z);
	    vec3 ret = wr_fast_intersect_triangle(ray_direction, ray_origin, v0, v1, v2, min_distance);
	    if( ret.z < min_distance ) { 
			min_intersection_point = ivec4(triangleGroup.x + relative_index_of_triangle, floatBitsToInt(ret.xy), floatBitsToInt(ret.z));
			min_distance = ret.z;
	    }		
		relative_index_of_triangle++;
		triangle_hits = triangle_hits >> 1;
	}
	if ((nodeGroup.y & 0xFF000000) == 0) 
	{
		if (toVisitOffset == 0) 
			break;
		nodeGroup = nodesToVisit[toVisitOffset--]; 
	}
  }
 
#endif /* wr_TriangleCount */

#if wr_SphereCount
	for(int i = 0; i < wr_SphereCount; i++ ) {
		vec4 sphere = spheres.sphere[i]; /* {center_x, center_y, center_z, radius} */

		float dist = rg_fast_intersect_sphere(sphere.xyz, sphere.w, ray_direction, ray_origin, min_distance);

		if(dist < min_distance)
		{
			min_distance = dist;
			vec3 point_on_sphere = ray_origin + ray_direction * dist;
			vec3 normal = normalize(point_on_sphere - sphere.xyz);
			vec2 spher = vec2(atan(normal.y, normal.x), acos(normal.z));

			min_intersection_point = ivec4(wr_TriangleCount + i, floatBitsToInt(spher), floatBitsToInt(dist));
		}
	}
#endif /* wr_SphereCount */

  return min_intersection_point;
}

#define wr_QueryIntersection wr_query_intersection
ivec4
wr_query_intersection(int ads, vec3 ray_origin, vec3 ray_direction, float tmax) {
#if wr_InstanceCount
  if ( WR_IS_TLAS(ads) ) {
    float min_distance = tmax;
    ivec4 min_intersection_point = ivec4(-1,0,0,floatBitsToInt(tmax));
    for (int i = 0; i < wr_InstanceCount; ++i) {
	  vec3 world_ray_origin = wr_GetWorldRayOrigin(ads, i, ray_origin);
	  vec3 world_ray_direction = wr_GetWorldRayDirection(ads, i, ray_direction);
	  ivec4 intersection = wr_QueryShapeIntersection( wr_GetBLASID(ads, i), world_ray_origin, world_ray_direction, min_distance);
      if(intBitsToFloat(intersection.w) < min_distance) {
        min_distance = intBitsToFloat(intersection.w);
  	    min_intersection_point = intersection;
  	    min_intersection_point.x = wr_GetPackedInstanceTriangleID(min_intersection_point.x, i);
	  }
    }

    return min_intersection_point;
  } else {
    return wr_query_shape_intersection(ads, ray_origin, ray_direction, tmax);
  }
#else
  return wr_query_shape_intersection(ads, ray_origin, ray_direction, tmax);
#endif
}

bool
wr_query_shape_occlusion(int ads, vec3 ray_origin, vec3 ray_direction, float tMax) {
  
#if wr_TriangleCount && wr_BVHNodeCount
  int toVisitOffset = 0, currentNodeIndex = 0;
  ivec2 nodesToVisit[WR_TRAVERSE_STACK_SIZE];
  nodesToVisit[0] = ivec2(0, 0x80000000);
  bool hit = false;

  ivec2 nodeGroup = ivec2(0, 0x1000000);
  ivec2 triangleGroup = ivec2(0,0);

  for(int loop = 0; loop < wr_BVHNodeCount; ++loop) {
  //while(true) {
    if (uint(nodeGroup.y) > 0x00FFFFFFu) //if (nodeGroup.y > 0x00FFFFFF) // it represents a node group
    {
      // 6: extract the node n using the octant traversal order. Also removes 7: 
	  //int n = closestInternalNode(nodeGroup, oct);
	  int n = 0;
	  for (int i = 24; i < 32; ++i)
	  {
	    if ((nodeGroup.y & (1 << i)) != 0)
		{
		  n = i - 24;
		  break;
        }
	  }
      nodeGroup.y = nodeGroup.y & (~(1 << (n + 24)));
      if(((nodeGroup.y >> 24) & 0xFFFF) != 0)
	  {
		// push the remaining nodes to the stack
		nodesToVisit[++toVisitOffset] = nodeGroup;
	  }

	  // 9: intersect with all children of G (returns G and Gt) (G contains only internal nodes and Gt contains triangle nodes. A WideNode can contain both)
	  intersectChildren(ads, nodeGroup.x + n, nodeGroup, triangleGroup, ray_origin, ray_direction, tMax);
	}
    else
	{
	  triangleGroup = nodeGroup; // 11: Gt = G
	  nodeGroup = ivec2( 0,0 ); // 12: G = 0
	}

	int triangle_hits = triangleGroup.y; // we could use hits & 0xFFFFFF but pad will always be zero
	int relative_index_of_triangle = 0;
	while (triangle_hits > 0) // 14: 
	{
		//19: t = GetNextTriangle(Gt)
		ivec4 indices = wr_GetIndicesBLAS(ads, triangleGroup.x + relative_index_of_triangle);

		//20: Gt = Gt / t; // no need to remove since we check for all triangles here

		//21: IntersectTriangle(t, r);
		float u, v;
		//float tHit = tMax;
		vec3 v0 = wr_GetPositionBLAS(ads, indices.x);
    	vec3 v1 = wr_GetPositionBLAS(ads, indices.y);
    	vec3 v2 = wr_GetPositionBLAS(ads, indices.z);
	    vec3 ret = wr_fast_intersect_triangle(ray_direction, ray_origin, v0, v1, v2, tMax);
	    if( ret.z < tMax ) { 
			return true;
	    }
		
		relative_index_of_triangle++;
		triangle_hits = triangle_hits >> 1;
	}
	if ((nodeGroup.y & 0xFF000000) == 0) // 23: G == 0
	{
		if (toVisitOffset == 0) //24: S == 0
			break;
		nodeGroup = nodesToVisit[toVisitOffset--]; //25: G <- StackPop
	}
  }
#endif /* wr_TriangleCount */

#if wr_SphereCount
  /* Special handling for spheres */
  for(int i = 0; i < wr_SphereCount; i++ ) {
	vec4 sphere = spheres.sphere[i]; /* {center_x, center_y, center_z, radius} */

	float dist = rg_fast_intersect_sphere(sphere.xyz, sphere.w, ray_direction, ray_origin, WR_RAY_MAX_DISTANCE);

	if(dist < tMax)
	{
	  return true;
	}
  }
#endif /* wr_SphereCount */

  return false;
}

#define wr_QueryOcclusion wr_query_occlusion
bool
wr_query_occlusion(int ads, vec3 ray_origin, vec3 ray_direction, float tmax) {
#if wr_InstanceCount
  if ( WR_IS_TLAS(ads) ) {
    for (int i = 0; i < wr_InstanceCount; ++i) {
	  vec3 world_ray_origin = wr_GetWorldRayOrigin(ads, i, ray_origin);
	  vec3 world_ray_direction = wr_GetWorldRayDirection(ads, i, ray_direction);
	  bool occluded = wr_query_shape_occlusion( wr_GetBLASID(ads, i), world_ray_origin, world_ray_direction, tmax);
      if(occluded) {
        return true;
	  }
    }

    return false;
  } else {
    return wr_query_shape_occlusion(ads, ray_origin, ray_direction, tmax);
  }
#else
  return wr_query_shape_occlusion(ads, ray_origin, ray_direction, tmax);
#endif
}

)glsl";

typedef enum
{
  RG_3D_OBJECT_TYPE_NONE,
  RG_3D_OBJECT_TYPE_SPHERE,
  RG_3D_OBJECT_TYPE_CUBE,
  RG_3D_OBJECT_TYPE_QUAD,
  RG_3D_OBJECT_TYPE_TRIANGLE_LIST,
  RG_3D_OBJECT_TYPE_FILE,
  RG_3D_OBJECT_TYPE_URL,
  RG_3D_OBJECT_TYPE_MAX
} rg_3d_object_type;

#define MAX_MATERIAL_COUNT 8

typedef struct
{
  unsigned char* data; /* Not kept alive after pushing to the GPU */
  int            width;
  int            height;
  int            channels;
} wr_texture;

typedef struct
{
  float cx, cy, cz, radius;
} wr_sphere;

typedef struct
{
  vec4 properties[MAX_MATERIAL_COUNT];
} wr_material;

struct wr_3d_mesh
{
  vec3*  vertices;
  vec2*  uvs;
  vec3*  normals;
  ivec4* faces;
  size_t face_count;
  size_t attr_count;
};

struct wr_3d_object
{
  rg_3d_object_type type;
  union
  {
    wr_3d_mesh as_mesh;
    wr_sphere  as_sphere;
  };
  std::vector<wr_material> materials;
};

static wr_bounds
wr_bounds_union(const wr_bounds& b1, const wr_bounds& b2)
{
  return { vec3{ wrays_minf(b1.min.x, b2.min.x), wrays_minf(b1.min.y, b2.min.y),
                 wrays_minf(b1.min.z, b2.min.z) },
           vec3{ wrays_maxf(b1.max.x, b2.max.x), wrays_maxf(b1.max.y, b2.max.y),
                 wrays_maxf(b1.max.z, b2.max.z) } };
}

static wr_bounds
wr_bounds_union_point(const wr_bounds& b, const vec3& p)
{
  return { vec3{ wrays_minf(b.min.x, p.x), wrays_minf(b.min.y, p.y),
                 wrays_minf(b.min.z, p.z) },
           vec3{ wrays_maxf(b.max.x, p.x), wrays_maxf(b.max.y, p.y),
                 wrays_maxf(b.max.z, p.z) } };
}

static vec3
wr_bounds_offset(const wr_bounds& bounds, const vec3 p)
{
  vec3 o = wrays_vec3_sub(p, bounds.min);
  if (bounds.max.x > bounds.min.x)
    o.x /= bounds.max.x - bounds.min.x;
  if (bounds.max.y > bounds.min.y)
    o.y /= bounds.max.y - bounds.min.y;
  if (bounds.max.z > bounds.min.z)
    o.z /= bounds.max.z - bounds.min.z;
  return o;
}

static vec3
wr_bounds_diagonal(const wr_bounds& bounds)
{
  return wrays_vec3_sub(bounds.max, bounds.min);
}

static float
wr_bounds_surface_area(const wr_bounds& bounds)
{
  vec3 d = wr_bounds_diagonal(bounds);
  return 2.0f * (d.x * d.y + d.x * d.z + d.y * d.z);
}

static wr_bounds
wr_triangle_bounds(vec3 v1, vec3 v2, vec3 v3)
{
  wr_bounds bounds = { wrays_vec3_min(wrays_vec3_min(v1, v2), v3),
                       wrays_vec3_max(wrays_vec3_max(v1, v2), v3) };
  return bounds;
}

static int
wr_bounds_maximum_extent(const wr_bounds& bounds)
{
  vec3 d = wr_bounds_diagonal(bounds);
  if (d.x > d.y && d.x > d.z)
    return 0;
  else if (d.y > d.z)
    return 1;
  else
    return 2;
}

struct wr_primitive
{
  int       index;
  wr_bounds bounds;
  vec3      centroid;

  wr_primitive() {}
  wr_primitive(int index, wr_bounds bounds)
    : index(index)
    , bounds(bounds)
    , centroid(wrays_vec3_scalef(wrays_vec3_add(bounds.max, bounds.min), 0.5f))
  {}
};

struct bvh_node
{
  wr_bounds bounds;
  bvh_node* children[2];
  int       splitAxis, firstPrimOffset, nPrimitives;

  void
  init_leaf(int first, int n, const wr_bounds& b)
  {
    firstPrimOffset = first;
    nPrimitives     = n;
    bounds          = b;
    children[0] = children[1] = nullptr;
  }

  void
  init_interior(int axis, bvh_node* c0, bvh_node* c1)
  {
    children[0] = c0;
    children[1] = c1;
    bounds      = wr_bounds_union(c0->bounds, c1->bounds);
    splitAxis   = axis;
    nPrimitives = 0;
  }
};

bvh_node*
rg_build_bvh_recursive(std::vector<wr_primitive>& primitiveInfo, int start,
                       int end, std::vector<ivec4>& orderedPrims,
                       int* total_nodes, const std::vector<ivec4>& m_triangles);
int
flattenBVHTree(bvh_node* node, int* offset, wr_linear_bvh_node* nodes);

SAHBVH::SAHBVH()
  : maxPrimsInNode(5)
  , m_shape_id_generator(0)
  , m_material_id_generator(0)
{
  std::memset(&m_webgl_textures, 0, sizeof(m_webgl_textures));
  m_need_update         = true;
  m_linear_nodes        = nullptr;
  intersection_code     = nullptr;
  m_total_nodes         = 0;
  m_vertex_texture_size = 0;
  m_index_texture_size  = 0;
  m_node_texture_size   = 0;
  m_webgl_binding_count = 3;
}

SAHBVH::~SAHBVH()
{
  delete[] intersection_code;
  delete[] m_linear_nodes;
}

int
SAHBVH::AddSphere(float* position, float radius, int materialID)
{
  return 0;
};

int
SAHBVH::AddTriangularMesh(float* positions, int position_stride, float* normals,
                          int normal_stride, float* uvs, int uv_stride,
                          int num_vertices, int* indices, int num_triangles)
{
  int          ID = ++m_shape_id_generator;
  TriangleMesh mesh;
  mesh.ID            = ID;
  mesh.num_vertices  = num_vertices;
  mesh.num_triangles = 0;

  mesh.vertex_offset   = (int)m_vertex_data.size();
  mesh.triangle_offset = (int)m_triangles.size();

  m_normal_data.resize(m_normal_data.size() + num_vertices); // (XYZU)
  m_vertex_data.resize(m_vertex_data.size() + num_vertices); // (XYZV)
  for (int i = 0; i < mesh.num_vertices; ++i) {
    float tx = (WR_NULL != uvs) ? uvs[i * uv_stride + 0] : 0.0f;
    float ty = (WR_NULL != uvs) ? uvs[i * uv_stride + 1] : 0.0f;
    float vx = positions[i * position_stride + 0];
    float vy = positions[i * position_stride + 1];
    float vz = positions[i * position_stride + 2];
    float nx = (WR_NULL != normals) ? normals[i * normal_stride + 0] : 0.0f;
    float ny = (WR_NULL != normals) ? normals[i * normal_stride + 1] : 0.0f;
    float nz = (WR_NULL != normals) ? normals[i * normal_stride + 2] : 0.0f;
    m_vertex_data[mesh.vertex_offset + i] = { vx, vy, vz, tx };
    m_normal_data[mesh.vertex_offset + i] = { nx, ny, nz, ty };
  }
  // std::memcpy(&m_vertex_data[mesh.vertex_offset], positions, 4 * num_vertices
  // * sizeof(float));

  mesh.num_triangles = num_triangles;
  m_triangles.resize(m_triangles.size() + mesh.num_triangles);
  int materialID = m_material_id_generator++;
  for (int i = 0; i < mesh.num_triangles; ++i) {
    m_triangles[i + mesh.triangle_offset] = {
      indices[4 * i + 0] + mesh.vertex_offset,
      indices[4 * i + 1] + mesh.vertex_offset,
      indices[4 * i + 2] + mesh.vertex_offset, indices[4 * i + 3]
    };
  }

  m_need_update = true;

  return ID;
}

bool
SAHBVH::Build()
{
  if (m_triangles.size() == 0) {
    return true;
  }

  if (!m_need_update) {
    return true;
  }

  std::vector<wr_primitive> primitiveInfo(m_triangles.size());
  for (size_t i = 0; i < m_triangles.size(); ++i) {
    vec3 v0          = { m_vertex_data[m_triangles[i].x].x,
                m_vertex_data[m_triangles[i].x].y,
                m_vertex_data[m_triangles[i].x].z };
    vec3 v1          = { m_vertex_data[m_triangles[i].y].x,
                m_vertex_data[m_triangles[i].y].y,
                m_vertex_data[m_triangles[i].y].z };
    vec3 v2          = { m_vertex_data[m_triangles[i].z].x,
                m_vertex_data[m_triangles[i].z].y,
                m_vertex_data[m_triangles[i].z].z };
    primitiveInfo[i] = { (int)i, wr_triangle_bounds(v0, v1, v2) };
  }

  m_total_nodes = 0;
  std::vector<ivec4> orderedPrim;
  orderedPrim.reserve(m_triangles.size());
  bvh_node* root =
    rg_build_bvh_recursive(primitiveInfo, 0, (int)m_triangles.size(),
                           orderedPrim, &m_total_nodes, m_triangles);

  m_triangles.swap(orderedPrim);
  // primitiveInfo.clear(); primitiveInfo.shrink_to_fit();

  m_linear_nodes = new wr_linear_bvh_node[m_total_nodes];
  int offset     = 0;
  flattenBVHTree(root, &offset, m_linear_nodes);

#if 0
	// Upload to the GPU
	constexpr int byte_size = sizeof(wr_linear_bvh_node);
	if (m_webgl_textures.bvh_nodes != 0)
		glDeleteTextures(1, &m_webgl_textures.bvh_nodes);
	glGenTextures(1, &m_webgl_textures.bvh_nodes);

	int num_pixels = 2 * m_total_nodes;
	m_node_texture_size = (int)wr_next_power_of_2(1 + (unsigned int)sqrtf((float)num_pixels));
	
	int width = (num_pixels < m_node_texture_size) ? num_pixels : m_node_texture_size;
	int height = (num_pixels - 1) / m_node_texture_size + 1;

	glBindTexture(GL_TEXTURE_2D_ARRAY, m_webgl_textures.bvh_nodes);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA32F, width, height, 1);
	{
		// Reorder the bits
		struct GPU_NODE {
			vec3 minValue;
			union
			{
				int primitivesOffset;  // leaf
				int secondChildOffset; // interior
			};
			vec3 maxValue;
			uint16_t nPrimitives; // 0 -> interior node
			uint8_t  axis;        // interior node: xyz
			uint8_t  pad[1];      // ensure 32 byte total size
		};
		constexpr int node_size = sizeof(GPU_NODE);
		GPU_NODE* temp_data = new GPU_NODE[width * height];
		for (int i = 0; i < m_total_nodes; ++i)
		{
			temp_data[i].minValue = m_linear_nodes[i].bounds.min;
			temp_data[i].primitivesOffset = m_linear_nodes[i].primitivesOffset;
			temp_data[i].maxValue = m_linear_nodes[i].bounds.max;
			temp_data[i].nPrimitives = m_linear_nodes[i].nPrimitives;
			temp_data[i].axis = m_linear_nodes[i].axis;
		}
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, width, height, 1, GL_RGBA, GL_FLOAT, (const void*)temp_data);
		delete[] temp_data;
	}
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	if (m_webgl_textures.scene_vertices != 0)
		glDeleteTextures(1, &m_webgl_textures.scene_vertices);
	glGenTextures(1, &m_webgl_textures.scene_vertices);

	num_pixels = (int)m_vertex_data.size();
	m_vertex_texture_size = (int)wr_next_power_of_2(1 + (unsigned int)sqrtf((float)num_pixels));

	width = (num_pixels < m_vertex_texture_size) ? num_pixels : m_vertex_texture_size;
	height = (num_pixels - 1) / m_vertex_texture_size + 1;
	glBindTexture(GL_TEXTURE_2D_ARRAY, m_webgl_textures.scene_vertices);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA32F, width, height, 2);
	if (width * height == num_pixels) {// no need to realocate
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, width, height, 1, GL_RGBA, GL_FLOAT, (const void*)m_vertex_data.data());
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 1, width, height, 1, GL_RGBA, GL_FLOAT, (const void*)m_normal_data.data());
	}
	else
	{
		unsigned char* temp_data = new unsigned char[width * height * 4 * 4];
		std::memcpy(temp_data, m_vertex_data.data(), m_vertex_data.size() * 4 * 4);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, width, height, 1, GL_RGBA, GL_FLOAT, (const void*)temp_data);
		std::memcpy(temp_data, m_normal_data.data(), m_normal_data.size() * 4 * 4);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 1, width, height, 1, GL_RGBA, GL_FLOAT, (const void*)temp_data);
		//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_FLOAT, (const void*)temp_data);
		delete[] temp_data;
	}
	/*glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, width, height);
    if (width * height == num_pixels) // no need to realocate
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_FLOAT, m_vertex_data.data());
	else
	{
        unsigned char* temp_data = new unsigned char[width * height * 4 * 4];
        std::memcpy(temp_data, m_vertex_data.data(), m_vertex_data.size() * 4 * 4);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_FLOAT, (const void*)temp_data);
        delete[] temp_data;
	}*/
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	if (m_webgl_textures.scene_indices != 0)
		glDeleteTextures(1, &m_webgl_textures.scene_indices);
	glGenTextures(1, &m_webgl_textures.scene_indices);

	num_pixels = m_triangles.size();
	m_index_texture_size = (int)wr_next_power_of_2(1 + (unsigned int)sqrtf((float)num_pixels));
	width = (num_pixels < m_index_texture_size) ? num_pixels : m_index_texture_size;
	height = (num_pixels - 1) / m_index_texture_size + 1;

	glBindTexture(GL_TEXTURE_2D_ARRAY, m_webgl_textures.scene_indices);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA32I, width, height, 1);

    if (width * height == num_pixels) // no need to realocate
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, width, height, 1, GL_RGBA_INTEGER, GL_INT, (const void*)m_triangles.data());
	else
	{
        unsigned char* temp_data = new unsigned char[width * height * 4 * 4];
        std::memcpy(temp_data, m_triangles.data(), m_triangles.size() * 4 * 4);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, width, height, 1, GL_RGBA_INTEGER, GL_INT, (const void*)temp_data);
        delete[] temp_data;
	}
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	
	m_webgl_bindings[0] = { "wr_scene_vertices", WR_BINDING_TYPE_TEXTURE_2D_ARRAY, {m_webgl_textures.scene_vertices} };
	m_webgl_bindings[1] = { "wr_scene_indices", WR_BINDING_TYPE_TEXTURE_2D_ARRAY, {m_webgl_textures.scene_indices} };
	m_webgl_bindings[2] = { "wr_bvh_nodes", WR_BINDING_TYPE_TEXTURE_2D_ARRAY, {m_webgl_textures.bvh_nodes} };
#endif

  m_need_update = false;

  return true;
}

const char*
SAHBVH::GetIntersectionCode()
{
  delete[] intersection_code;

  std::string str;
  str += "#define WR_PRIMITIVE_TEXTURE_SIZE " +
         std::to_string(m_index_texture_size) + "\n";
  str += "#define WR_TRAVERSE_STACK_SIZE " + std::to_string(32) + "\n";
  str += "#define WR_NODES_TEXTURE_SIZE " +
         std::to_string(m_node_texture_size) + "\n";
  str += "#define WR_SCENE_TEXTURE_SIZE " +
         std::to_string(m_vertex_texture_size) + "\n";
  str += "#define wr_InstanceCount " + std::to_string(0) + "\n";
  str += "#define wr_SphereCount " + std::to_string(0) + "\n";
  str +=
    "#define wr_TriangleCount " + std::to_string(m_triangles.size()) + "\n";
  str += "#define wr_BVHNodeCount " + std::to_string(m_total_nodes) + "\n";
  str += "#define WR_RAY_MAX_DISTANCE 1.e27\n";

  str += "uniform sampler2DArray wr_scene_vertices;\n";
  str += "uniform isampler2DArray wr_scene_indices;\n";
  str += "uniform sampler2DArray wr_bvh_nodes;\n";

  /* Maybe we need this for instancing */
  // str += "const ivec2 wr_Counts[8] = ivec2[]( ivec2(" +
  // std::to_string(m_triangles.size()) + ", " + std::to_string(m_total_nodes) +
  // "), ivec2(0,0), ivec2(0,0), ivec2(0,0), ivec2(0,0), ivec2(0,0), ivec2(0,0),
  // ivec2(0,0) );\n";

  str += "ivec4 wr_GetFace(int ads, ivec4 intersection) { return "
         "texelFetch(wr_scene_indices, ivec3(intersection.x % "
         "WR_PRIMITIVE_TEXTURE_SIZE, intersection.x / "
         "WR_PRIMITIVE_TEXTURE_SIZE, ads), 0); }\n";
  str += "ivec4 wr_GetIndices(int ads, int i) { return "
         "texelFetch(wr_scene_indices, ivec3(i % WR_PRIMITIVE_TEXTURE_SIZE, i "
         "/ WR_PRIMITIVE_TEXTURE_SIZE, ads), 0); }\n";
  str += "vec3 wr_GetNormal(int ads, int i) { return "
         "texelFetch(wr_scene_vertices, ivec3(i % WR_SCENE_TEXTURE_SIZE, i / "
         "WR_SCENE_TEXTURE_SIZE, ads * 2 + 1), 0).xyz; }\n";
  str += "vec3 wr_GetPosition(int ads, int i) { return "
         "texelFetch(wr_scene_vertices, ivec3(i % WR_SCENE_TEXTURE_SIZE, i / "
         "WR_SCENE_TEXTURE_SIZE, ads * 2 + 0), 0).xyz; }\n";
  str += "vec2 wr_GetTexCoords(int ads, int i) { return "
         "vec2(texelFetch(wr_scene_vertices, ivec3(i % WR_SCENE_TEXTURE_SIZE, "
         "i / WR_SCENE_TEXTURE_SIZE, ads * 2 + 0), 0).w, "
         "texelFetch(wr_scene_vertices, ivec3(i % WR_SCENE_TEXTURE_SIZE, i / "
         "WR_SCENE_TEXTURE_SIZE, ads * 2 + 1), 0).w); }\n";
  str += "vec4 wr_GetPackedBoundMin(int ads, int i) { int b = 2 * i + 0; "
         "return texelFetch(wr_bvh_nodes, ivec3(b % WR_NODES_TEXTURE_SIZE, b / "
         "WR_NODES_TEXTURE_SIZE, ads), 0); }\n";
  str += "vec4 wr_GetPackedBoundMax(int ads, int i) { int b = 2 * i + 1; "
         "return texelFetch(wr_bvh_nodes, ivec3(b % WR_NODES_TEXTURE_SIZE, b / "
         "WR_NODES_TEXTURE_SIZE, ads), 0); }\n";
  str += g_copysign_func;
  str += g_ray_triangle_intersection_func;
  str += g_ray_sahbvh_intersect_fragment_shader;

  intersection_code             = new char[str.size() + 1];
  intersection_code[str.size()] = 0;
  std::memcpy(intersection_code, str.data(), str.size() * sizeof(char));

  return intersection_code;
}

int
flattenBVHTree(bvh_node* node, int* offset, wr_linear_bvh_node* nodes)
{
  wr_linear_bvh_node* linearNode = &nodes[*offset];
  linearNode->bounds             = node->bounds;
  int myOffset                   = (*offset)++;

  if (node->nPrimitives > 0) {
    linearNode->primitivesOffset = node->firstPrimOffset;
    linearNode->nPrimitives      = node->nPrimitives;
  } else {
    linearNode->axis        = node->splitAxis;
    linearNode->nPrimitives = 0;
    flattenBVHTree(node->children[0], offset, nodes);
    linearNode->secondChildOffset =
      flattenBVHTree(node->children[1], offset, nodes);
  }
  return myOffset;
}

bvh_node*
rg_build_bvh_recursive(std::vector<wr_primitive>& primitiveInfo, int start,
                       int end, std::vector<ivec4>& orderedPrims,
                       int* total_nodes, const std::vector<ivec4>& m_triangles)
{
  // use a memory arena
  // webrays->scene.bvh_node_arena.push_back({});
  // bvh_node* node =
  // &webrays->scene.bvh_node_arena[webrays->scene.bvh_node_arena.size() - 1];
  bvh_node* node = new bvh_node();
  (*total_nodes)++;
  wr_bounds bounds;
  for (int i = start; i < end; ++i)
    bounds = wr_bounds_union(bounds, primitiveInfo[i].bounds);

  int nPrimitives = end - start;

  if (nPrimitives == 1) {
    int firstPrimOffset = (int)orderedPrims.size();
    for (int i = start; i < end; ++i) {
      int primNum = primitiveInfo[i].index;
      orderedPrims.push_back(m_triangles[primNum]);
    }
    node->init_leaf(firstPrimOffset, nPrimitives, bounds);

    return node;
  } else {
    wr_bounds centroid_bounds;
    for (int i = start; i < end; ++i)
      centroid_bounds =
        wr_bounds_union_point(centroid_bounds, primitiveInfo[i].centroid);

    int dim = wr_bounds_maximum_extent(centroid_bounds);

    int mid = (start + end) / 2;
    // if (centroid_bounds.max[dim] == centroid_bounds.min[dim]) {
    if (std::abs(centroid_bounds.max.at[dim] - centroid_bounds.min.at[dim]) <
        0.01f) {
      int firstPrimOffset = (int)orderedPrims.size();
      for (int i = start; i < end; ++i) {
        int primNum = primitiveInfo[i].index;
        orderedPrims.push_back(m_triangles[primNum]);
      }
      node->init_leaf(firstPrimOffset, nPrimitives, bounds);

      return node;
    } else {
      if (nPrimitives <= 2) {
        mid = (start + end) / 2;
        std::nth_element(&primitiveInfo[start], &primitiveInfo[mid],
                         &primitiveInfo[end - 1] + 1,
                         [dim](const wr_primitive& a, const wr_primitive& b) {
                           return a.centroid.at[dim] < b.centroid.at[dim];
                         });
      } else {
        constexpr int nBuckets       = 64;
        constexpr int maxPrimsInNode = 3;

        struct BucketInfo
        {
          int       count = 0;
          wr_bounds bounds;
        };
        BucketInfo buckets[nBuckets];
        for (int i = start; i < end; ++i) {
          int b = int(nBuckets * wr_bounds_offset(centroid_bounds,
                                                  primitiveInfo[i].centroid)
                                   .at[dim]);
          if (b == nBuckets)
            b = nBuckets - 1;
          buckets[b].count++;
          buckets[b].bounds =
            wr_bounds_union(buckets[b].bounds, primitiveInfo[i].bounds);
        }
        float cost[nBuckets - 1];
        for (int i = 0; i < nBuckets - 1; ++i) {
          wr_bounds b0, b1;
          int       count0 = 0, count1 = 0;
          for (int j = 0; j <= i; ++j) {
            b0 = wr_bounds_union(b0, buckets[j].bounds);
            count0 += buckets[j].count;
          }
          for (int j = i + 1; j < nBuckets; ++j) {
            b1 = wr_bounds_union(b1, buckets[j].bounds);
            count1 += buckets[j].count;
          }
          /*cost[i] =
            .125f + (count0 * rg_bounds_surface_area(b0) +
            count1 * rg_bounds_surface_area(b1))
            / rg_bounds_surface_area(bounds);*/
          cost[i] = 1.0f + (count0 * wr_bounds_surface_area(b0) +
                            count1 * wr_bounds_surface_area(b1)) /
                             wr_bounds_surface_area(bounds);
        }

        float minCost            = cost[0];
        int   minCostSplitBucket = 0;
        for (int i = 1; i < nBuckets - 1; ++i) {
          if (cost[i] < minCost) {
            minCost            = cost[i];
            minCostSplitBucket = i;
          }
        }

        float leafCost = (float)nPrimitives;
        if (nPrimitives > maxPrimsInNode || minCost < leafCost) {
          wr_primitive* pmid = std::partition(
            &primitiveInfo[start], &primitiveInfo[end - 1] + 1,
            [=](const wr_primitive& pi) {
              int b =
                int(nBuckets *
                    wr_bounds_offset(centroid_bounds, pi.centroid).at[dim]);
              if (b == nBuckets)
                b = nBuckets - 1;
              return b <= minCostSplitBucket;
            });
          mid = int(pmid - &primitiveInfo[0]);
        } else {
          int firstPrimOffset = int(orderedPrims.size());
          for (int i = start; i < end; ++i) {
            int primNum = primitiveInfo[i].index;
            orderedPrims.push_back(m_triangles[primNum]);
          }

          node->init_leaf(firstPrimOffset, nPrimitives, bounds);

          return node;
        }
      }
      bvh_node* c0 = rg_build_bvh_recursive(
        primitiveInfo, start, mid, orderedPrims, total_nodes, m_triangles);
      bvh_node* c1 = rg_build_bvh_recursive(
        primitiveInfo, mid, end, orderedPrims, total_nodes, m_triangles);
      node->init_interior(dim, c0, c1);
    }
  }

  return node;
}

bvh_node*
rg_build_bvh_recursive_1prim(std::vector<wr_primitive>& primitiveInfo,
                             int start, int end,
                             std::vector<ivec4>& orderedPrims, int* total_nodes,
                             const std::vector<ivec4>& m_triangles)
{
  constexpr int maxPrimsInNode = 1;

  // use a memory arena
  // webrays->scene.bvh_node_arena.push_back({});
  // bvh_node* node =
  // &webrays->scene.bvh_node_arena[webrays->scene.bvh_node_arena.size() - 1];
  bvh_node* node = new bvh_node();
  (*total_nodes)++;
  wr_bounds bounds;
  for (int i = start; i < end; ++i)
    bounds = wr_bounds_union(bounds, primitiveInfo[i].bounds);

  int nPrimitives = end - start;

  if (nPrimitives == maxPrimsInNode) {
    int firstPrimOffset = (int)orderedPrims.size();
    int primNum         = primitiveInfo[start].index;
    orderedPrims.push_back(m_triangles[primNum]);
    node->init_leaf(firstPrimOffset, nPrimitives, bounds);

    return node;
  } else {
    wr_bounds centroid_bounds;
    for (int i = start; i < end; ++i)
      centroid_bounds =
        wr_bounds_union_point(centroid_bounds, primitiveInfo[i].centroid);

    int dim = wr_bounds_maximum_extent(centroid_bounds);

    int mid = (start + end) / 2;
    if (centroid_bounds.max.at[dim] == centroid_bounds.min.at[dim]) {
      mid = (start + end) / 2;
      std::nth_element(&primitiveInfo[start], &primitiveInfo[mid],
                       &primitiveInfo[end - 1] + 1,
                       [dim](const wr_primitive& a, const wr_primitive& b) {
                         return a.centroid.at[dim] < b.centroid.at[dim];
                       });
    } else {
      if (nPrimitives <= 2) {
        mid = (start + end) / 2;
        std::nth_element(&primitiveInfo[start], &primitiveInfo[mid],
                         &primitiveInfo[end - 1] + 1,
                         [dim](const wr_primitive& a, const wr_primitive& b) {
                           return a.centroid.at[dim] < b.centroid.at[dim];
                         });
      } else {
        constexpr int nBuckets = 64;

        struct BucketInfo
        {
          int       count = 0;
          wr_bounds bounds;
        };
        BucketInfo buckets[nBuckets];
        for (int i = start; i < end; ++i) {
          int b = int(nBuckets * wr_bounds_offset(centroid_bounds,
                                                  primitiveInfo[i].centroid)
                                   .at[dim]);
          if (b == nBuckets)
            b = nBuckets - 1;
          buckets[b].count++;
          buckets[b].bounds =
            wr_bounds_union(buckets[b].bounds, primitiveInfo[i].bounds);
        }
        float cost[nBuckets - 1];
        for (int i = 0; i < nBuckets - 1; ++i) {
          wr_bounds b0, b1;
          int       count0 = 0, count1 = 0;
          for (int j = 0; j <= i; ++j) {
            b0 = wr_bounds_union(b0, buckets[j].bounds);
            count0 += buckets[j].count;
          }
          for (int j = i + 1; j < nBuckets; ++j) {
            b1 = wr_bounds_union(b1, buckets[j].bounds);
            count1 += buckets[j].count;
          }
          /*cost[i] =
            .125f + (count0 * rg_bounds_surface_area(b0) +
            count1 * rg_bounds_surface_area(b1))
            / rg_bounds_surface_area(bounds);*/
          cost[i] = 1.0f + (count0 * wr_bounds_surface_area(b0) +
                            count1 * wr_bounds_surface_area(b1)) /
                             wr_bounds_surface_area(bounds);
        }

        float minCost            = cost[0];
        int   minCostSplitBucket = 0;
        for (int i = 1; i < nBuckets - 1; ++i) {
          if (cost[i] < minCost) {
            minCost            = cost[i];
            minCostSplitBucket = i;
          }
        }

        float leafCost = (float)nPrimitives;
        if (nPrimitives > maxPrimsInNode || minCost < leafCost) {
          wr_primitive* pmid = std::partition(
            &primitiveInfo[start], &primitiveInfo[end - 1] + 1,
            [=](const wr_primitive& pi) {
              int b =
                int(nBuckets *
                    wr_bounds_offset(centroid_bounds, pi.centroid).at[dim]);
              if (b == nBuckets)
                b = nBuckets - 1;
              return b <= minCostSplitBucket;
            });
          mid = int(pmid - &primitiveInfo[0]);
        } else {
          int firstPrimOffset = int(orderedPrims.size());
          for (int i = start; i < end; ++i) {
            int primNum = primitiveInfo[i].index;
            orderedPrims.push_back(m_triangles[primNum]);
          }

          node->init_leaf(firstPrimOffset, nPrimitives, bounds);

          return node;
        }
      }
    }
    bvh_node* c0 = rg_build_bvh_recursive_1prim(
      primitiveInfo, start, mid, orderedPrims, total_nodes, m_triangles);
    bvh_node* c1 = rg_build_bvh_recursive_1prim(
      primitiveInfo, mid, end, orderedPrims, total_nodes, m_triangles);
    node->init_interior(dim, c0, c1);
  }

  return node;
}

LinearNodes::LinearNodes()
  : intersection_code(nullptr)
{
  m_webgl_binding_count = 2;
  m_vertex_texture_size = 0;
  m_index_texture_size  = 0;
}

LinearNodes::~LinearNodes()
{
  delete[] intersection_code;
}

int
LinearNodes::AddSphere(float* position, float radius, int materialID)
{
  return 0;
}

int
LinearNodes::AddTriangularMesh(float* positions, int position_stride,
                               float* normals, int normal_stride, float* uvs,
                               int uv_stride, int num_vertices, int* indices,
                               int num_triangles)
{
  int          ID = ++m_shape_id_generator;
  TriangleMesh mesh;
  mesh.ID            = ID;
  mesh.num_vertices  = num_vertices;
  mesh.num_triangles = 0;

  mesh.vertex_offset   = (int)m_vertex_data.size();
  mesh.triangle_offset = (int)m_triangles.size();

  m_vertex_data.resize(m_vertex_data.size() + num_vertices); // (XYZW)
  std::memcpy(&m_vertex_data[mesh.vertex_offset], positions,
              4 * num_vertices * sizeof(float));

  mesh.num_triangles = num_triangles;
  m_triangles.resize(m_triangles.size() + mesh.num_triangles);
  int materialID = m_material_id_generator++;
  for (int i = 0; i < mesh.num_triangles; ++i) {
    m_triangles[i + mesh.triangle_offset] = {
      indices[4 * i + 0] + mesh.vertex_offset,
      indices[4 * i + 1] + mesh.vertex_offset,
      indices[4 * i + 2] + mesh.vertex_offset, indices[4 * i + 3]
    };
  }

  m_need_update = true;

  return ID;
}

bool
LinearNodes::Build()
{
#if 0
	if (m_triangles.size() == 0)
		return true;

	if (!m_need_update)
		return true;
	
	// Upload to the GPU
	if (m_webgl_textures.scene_vertices != 0)
		glDeleteTextures(1, &m_webgl_textures.scene_vertices);
	glGenTextures(1, &m_webgl_textures.scene_vertices);

	int num_pixels = m_vertex_data.size();
	m_vertex_texture_size = (int)wr_next_power_of_2(1 + (unsigned int)sqrtf((float)num_pixels));
	int width = (num_pixels < m_vertex_texture_size) ? num_pixels : m_vertex_texture_size;
	int height = (num_pixels - 1) / m_vertex_texture_size + 1;

	glBindTexture(GL_TEXTURE_2D_ARRAY, m_webgl_textures.scene_vertices);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA32F, width, height, 2);
	if (width * height == num_pixels) { // no need to realocate
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, width, height, 1, GL_RGBA, GL_FLOAT, (const void*)m_vertex_data.data());
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 1, width, height, 1, GL_RGBA, GL_FLOAT, (const void*)m_normal_data.data());
	}
	else
	{
		unsigned char* temp_data = new unsigned char[width * height * 4 * 4];
		std::memcpy(temp_data, m_vertex_data.data(), m_vertex_data.size() * 4 * 4);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, width, height, 1, GL_RGBA, GL_FLOAT, (const void*)temp_data);
		std::memcpy(temp_data, m_normal_data.data(), m_normal_data.size() * 4 * 4);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 1, width, height, 1, GL_RGBA, GL_FLOAT, (const void*)temp_data);
		delete[] temp_data;
	}
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);


	if (m_webgl_textures.scene_indices != 0)
		glDeleteTextures(1, &m_webgl_textures.scene_indices);
	glGenTextures(1, &m_webgl_textures.scene_indices);

	num_pixels = m_triangles.size();
	m_index_texture_size = (int)wr_next_power_of_2(1 + (unsigned int)sqrtf((float)num_pixels));
	width = (num_pixels < m_index_texture_size) ? num_pixels : m_index_texture_size;
	height = (num_pixels - 1) / m_index_texture_size + 1;

	glBindTexture(GL_TEXTURE_2D_ARRAY, m_webgl_textures.scene_indices);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA32I, width, height, 1);
	if (width * height == num_pixels) // no need to realocate
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, width, height, 1, GL_RGBA_INTEGER, GL_INT, (const void*)m_triangles.data());
	else
	{
		unsigned char* temp_data = new unsigned char[width * height * 4 * 4];
		std::memcpy(temp_data, m_triangles.data(), m_triangles.size() * 4 * 4);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, width, height, 1, GL_RGBA_INTEGER, GL_INT, (const void*)temp_data);
		delete[] temp_data;
	}
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	m_webgl_bindings[0] = { "wr_scene_vertices", WR_BINDING_TYPE_GL_TEXTURE_2D_ARRAY, {m_webgl_textures.scene_vertices} };
	m_webgl_bindings[1] = { "wr_scene_indices", WR_BINDING_TYPE_GL_TEXTURE_2D_ARRAY, {m_webgl_textures.scene_indices} };

	m_need_update = false;
#endif
  return true;
}

const char*
LinearNodes::GetIntersectionCode()
{
  delete[] intersection_code;

  std::string str;
  str += "#define WR_PRIMITIVE_TEXTURE_SIZE " +
         std::to_string(m_index_texture_size) + "\n";
  str += "#define WR_SCENE_TEXTURE_SIZE " +
         std::to_string(m_vertex_texture_size) + "\n";
  str += "#define wr_SphereCount " + std::to_string(0) + "\n";
  str +=
    "#define wr_TriangleCount " + std::to_string(m_triangles.size()) + "\n";
  str += "#define WR_RAY_MAX_DISTANCE 1.e27\n";
  str += g_ray_triangle_intersection_func;
  str += g_linear_nodes_ray_intersect_fragment_shader;

  intersection_code             = new char[str.size() + 1];
  intersection_code[str.size()] = 0;
  std::memcpy(intersection_code, str.data(), str.size() * sizeof(char));

  return intersection_code;
}

WideBVH::WideBVH()
  : maxPrimsInNode(5)
  , m_shape_id_generator(0)
{
  std::memset(&m_webgl_textures, 0, sizeof(m_webgl_textures));
  m_need_update         = true;
  m_linear_nodes        = nullptr;
  intersection_code     = nullptr;
  m_total_nodes         = 0;
  m_vertex_texture_size = 0;
  m_index_texture_size  = 0;
  m_node_texture_size   = 0;
}

WideBVH::~WideBVH()
{
  delete[] intersection_code;
  delete[] m_linear_nodes;
}

int
WideBVH::AddSphere(float* position, float radius, int materialID)
{
  return 0;
};

int
WideBVH::AddTriangularMesh(float* positions, int position_stride,
                           float* normals, int normal_stride, float* uvs,
                           int uv_stride, int num_vertices, int* indices,
                           int num_triangles)
{
  int          ID = ++m_shape_id_generator;
  TriangleMesh mesh;
  mesh.ID            = ID;
  mesh.num_vertices  = num_vertices;
  mesh.num_triangles = 0;

  mesh.vertex_offset   = (int)m_vertex_data.size();
  mesh.triangle_offset = (int)m_triangles.size();

  m_vertex_data.resize(m_vertex_data.size() + num_vertices); // (XYZU)
  m_normal_data.resize(m_normal_data.size() + num_vertices); // (XYZV)
  for (int i = 0; i < mesh.num_vertices; ++i) {
    float vx = positions[i * position_stride + 0];
    float vy = positions[i * position_stride + 1];
    float vz = positions[i * position_stride + 2];
    float nx = (WR_NULL != normals) ? normals[i * normal_stride + 0] : 0.0f;
    float ny = (WR_NULL != normals) ? normals[i * normal_stride + 1] : 0.0f;
    float nz = (WR_NULL != normals) ? normals[i * normal_stride + 2] : 0.0f;
    float tx = (WR_NULL != uvs) ? uvs[i * uv_stride + 0] : 0.0f;
    float ty = (WR_NULL != uvs) ? uvs[i * uv_stride + 1] : 0.0f;
    m_vertex_data[mesh.vertex_offset + i] = { vx, vy, vz, tx };
    m_normal_data[mesh.vertex_offset + i] = { nx, ny, nz, ty };
  }

  mesh.num_triangles = num_triangles;
  m_triangles.resize(m_triangles.size() + mesh.num_triangles);
  for (int i = 0; i < mesh.num_triangles; ++i) {
    m_triangles[i + mesh.triangle_offset] = {
      indices[4 * i + 0] + mesh.vertex_offset,
      indices[4 * i + 1] + mesh.vertex_offset,
      indices[4 * i + 2] + mesh.vertex_offset, indices[4 * i + 3]
    };
  }

  m_need_update = true;

  return ID;
}

bool
WideBVH::Build()
{
  if (m_triangles.size() == 0) {
    return true;
  }

  if (!m_need_update) {
    return true;
  }

  std::vector<wr_primitive> primitiveInfo(m_triangles.size());
  for (size_t i = 0; i < m_triangles.size(); ++i) {
    vec3 v0          = { m_vertex_data[m_triangles[i].x].x,
                m_vertex_data[m_triangles[i].x].y,
                m_vertex_data[m_triangles[i].x].z };
    vec3 v1          = { m_vertex_data[m_triangles[i].y].x,
                m_vertex_data[m_triangles[i].y].y,
                m_vertex_data[m_triangles[i].y].z };
    vec3 v2          = { m_vertex_data[m_triangles[i].z].x,
                m_vertex_data[m_triangles[i].z].y,
                m_vertex_data[m_triangles[i].z].z };
    primitiveInfo[i] = { (int)i, wr_triangle_bounds(v0, v1, v2) };
  }

  m_total_nodes = 0;
  std::vector<ivec4> orderedPrim;
  orderedPrim.reserve(m_triangles.size());
  bvh_node* root =
    rg_build_bvh_recursive_1prim(primitiveInfo, 0, (int)primitiveInfo.size(),
                                 orderedPrim, &m_total_nodes, m_triangles);
  m_triangles.swap(orderedPrim);

  float rootSurfaceArea = wr_bounds_surface_area(root->bounds);

  int   Nw = 16;
  int   Nd = 4;
  float Rt = 0.2f;

  const float rayNodeTestCost     = 1.f;
  const float rayTriangleTestCost = 0.3f;
  const int   Pmax                = 3;

  struct Cost
  {
    float     cost[8];      // [1 7]
    int       selection[8]; // 0 leaf, 1 distribute, 2 internal
    int       distribute0[8];
    int       distribute1[8];
    int       numPrimitives;
    int       firstPrimitiveOffset;
    bvh_node* node;
    Cost*     children[2];
  };

  auto buildRec = [rayTriangleTestCost, rayNodeTestCost,
                   Pmax](bvh_node* node, Cost* cost, float rootSurfaceArea,
                         auto&& buildRec) -> void {
    const float An = wr_bounds_surface_area(node->bounds) / rootSurfaceArea;
    if (node->children[0] == nullptr && node->children[1] == nullptr) // Leaf
    {
      cost->children[0] = cost->children[1] = nullptr;
      cost->numPrimitives                   = node->nPrimitives;
      cost->firstPrimitiveOffset            = node->firstPrimOffset;
      for (int i = 0; i < 7; ++i) {
        cost->cost[i]      = An * rayTriangleTestCost;
        cost->selection[i] = 0;
      }
    } else {
      // initialize cost
      cost->children[0]       = new Cost();
      cost->children[0]->node = node->children[0];
      buildRec(node->children[0], cost->children[0], rootSurfaceArea, buildRec);

      cost->children[1]       = new Cost();
      cost->children[1]->node = node->children[1];
      buildRec(node->children[1], cost->children[1], rootSurfaceArea, buildRec);

      cost->numPrimitives =
        cost->children[0]->numPrimitives + cost->children[1]->numPrimitives;
      cost->firstPrimitiveOffset =
        std::min(cost->children[0]->firstPrimitiveOffset,
                 cost->children[1]
                   ->firstPrimitiveOffset); // they are in continous memory so
                                            // they will be neighbors
      // Compute cost

      // Cost of creating a leaf node
      float costLeaf = (cost->numPrimitives <= Pmax)
                         ? An * cost->numPrimitives * rayTriangleTestCost
                         : std::numeric_limits<float>::infinity();

      // Compute the last element first
      char distribute0 = -1;
      char distribute1 = -1;

      float costDistribute = FLT_MAX;
      for (int k = 0; k < 7; ++k) {
        float costos =
          cost->children[0]->cost[k] + cost->children[1]->cost[6 - k];
        if (costos < costDistribute) {
          costDistribute = costos;
          distribute0    = k;
          distribute1    = 6 - k;
        }
      }

      float costInternal = costDistribute + An * rayNodeTestCost;

      if (costLeaf <= costInternal) {
        cost->cost[0]      = costLeaf;
        cost->selection[0] = 0;
      } else {
        cost->cost[0]      = costInternal;
        cost->selection[0] = 2;
      }
      cost->distribute0[0] = distribute0;
      cost->distribute1[0] = distribute1;

      //[1..7]
      for (int j = 1; j < 7; ++j) {
        float costDistribute = cost->cost[j - 1];

        distribute0 = -1;
        distribute1 = -1;

        for (int k = 0; k < j; ++k) {
          float costos =
            cost->children[0]->cost[k] + cost->children[1]->cost[j - k - 1];
          if (costos < costDistribute) {
            costDistribute = costos;
            distribute0    = k;
            distribute1    = j - k - 1;
          }
        }

        cost->cost[j] = costDistribute;

        if (distribute0 != -1) {
          cost->selection[j]   = 1; // distribute
          cost->distribute0[j] = distribute0;
          cost->distribute1[j] = distribute1;
        } else {
          cost->selection[j]   = cost->selection[j - 1];
          cost->distribute0[j] = cost->distribute0[j - 1];
          cost->distribute1[j] = cost->distribute1[j - 1];
        }
      }
    }
  };

  auto Fetch8Nodes = [](Cost* node, std::vector<Cost*>& rnodes, int size,
                        auto&& Fetch8Nodes) {
    if (node->numPrimitives <= 3) {
      rnodes.push_back(node);
      return;
    }
    int distribute0 = node->distribute0[size];
    int distribute1 = node->distribute1[size];

    if (node->children[0]->selection[distribute0] == 1) // distribute
      Fetch8Nodes(node->children[0], rnodes, distribute0, Fetch8Nodes);
    else
      rnodes.push_back(node->children[0]);

    if (node->children[1]->selection[distribute1] == 1) // distribute
      Fetch8Nodes(node->children[1], rnodes, distribute1, Fetch8Nodes);
    else
      rnodes.push_back(node->children[1]);
  };

  auto CountNodes2 = [](bvh_node* node, auto&& CountNodes2) -> int {
    if (node->children[0] != nullptr)
      return CountNodes2(node->children[0], CountNodes2) +
             CountNodes2(node->children[1], CountNodes2) + 1;
    else
      return 1;
  };

  auto flattenNext = [&](Cost* cost, int curOffset, int* offset,
                         std::vector<ivec4>& orderedPrims,
                         auto&&              flattenNext) -> void {
    wr_wide_bvh_node* wnode = &m_linear_nodes[curOffset];
    wnode->px               = cost->node->bounds.min.x;
    wnode->py               = cost->node->bounds.min.y;
    wnode->pz               = cost->node->bounds.min.z;

    float ex2 = exp2f(ceilf(
      log2f((cost->node->bounds.max.x - cost->node->bounds.min.x) / (255.f))));
    float ey2 = exp2f(ceilf(
      log2f((cost->node->bounds.max.y - cost->node->bounds.min.y) / (255.f))));
    float ez2 = exp2f(ceilf(
      log2f((cost->node->bounds.max.z - cost->node->bounds.min.z) / (255.f))));

    int u_ex, u_ey, u_ez;
    memcpy(&u_ex, &ex2, 4);
    memcpy(&u_ey, &ey2, 4);
    memcpy(&u_ez, &ez2, 4);

    wnode->ex = u_ex >> 23;
    wnode->ey = u_ey >> 23;
    wnode->ez = u_ez >> 23;

    // printf("Node %d ", curOffset);
    // printBBOX(cost->node->bounds.min, cost->node->bounds.max);

    wnode->imask                 = 0;
    wnode->child_node_base_index = *offset;
    wnode->triangle_base_index   = (int)orderedPrims.size();
    int triangle_offset          = 0;
    int node_offset              = 0;
    wnode->meta[0]               = 0;
    wnode->meta[1]               = 0;
    std::memset(wnode->childBBOX, 0, 12 * sizeof(int));

    ///
    std::vector<Cost*> rnodes;
    Fetch8Nodes(cost, rnodes, 0, Fetch8Nodes);

    for (int child = 0; child < rnodes.size(); ++child) {
      int qlox = (int)floorf(
        (rnodes[child]->node->bounds.min.x - cost->node->bounds.min.x) / ex2);
      int qloy = (int)floorf(
        (rnodes[child]->node->bounds.min.y - cost->node->bounds.min.y) / ey2);
      int qloz = (int)floorf(
        (rnodes[child]->node->bounds.min.z - cost->node->bounds.min.z) / ez2);

      int qhix = (int)ceilf(
        (rnodes[child]->node->bounds.max.x - cost->node->bounds.min.x) / ex2);
      int qhiy = (int)ceilf(
        (rnodes[child]->node->bounds.max.y - cost->node->bounds.min.y) / ey2);
      int qhiz = (int)ceilf(
        (rnodes[child]->node->bounds.max.z - cost->node->bounds.min.z) / ez2);

      // printf("Node %d.%d \n", curOffset, child);
      // printBBOX(rnodes[child]->node->bounds.pMin,
      // rnodes[child]->node->bounds.pMax);

      int childBBOX_index     = child / 4;
      int childBBOX_bit_index = child % 4;
      childBBOX_bit_index     = childBBOX_bit_index * 8;

      wnode->childBBOX[childBBOX_index] =
        wnode->childBBOX[childBBOX_index] | (qlox << childBBOX_bit_index);
      wnode->childBBOX[childBBOX_index + 2] =
        wnode->childBBOX[childBBOX_index + 2] | (qloy << childBBOX_bit_index);
      wnode->childBBOX[childBBOX_index + 4] =
        wnode->childBBOX[childBBOX_index + 4] | (qloz << childBBOX_bit_index);

      wnode->childBBOX[childBBOX_index + 6] =
        wnode->childBBOX[childBBOX_index + 6] | (qhix << childBBOX_bit_index);
      wnode->childBBOX[childBBOX_index + 8] =
        wnode->childBBOX[childBBOX_index + 8] | (qhiy << childBBOX_bit_index);
      wnode->childBBOX[childBBOX_index + 10] =
        wnode->childBBOX[childBBOX_index + 10] | (qhiz << childBBOX_bit_index);

      // check if the child is a leaf
      if (rnodes[child]->selection[0] == 0) {
        int meta = triangle_offset++;
        if (rnodes[child]->numPrimitives >= 1) {
          orderedPrims.push_back(
            m_triangles[rnodes[child]->firstPrimitiveOffset + 0]);
          meta = meta | (1 << 5);
        }
        if (rnodes[child]->numPrimitives >= 2) {
          orderedPrims.push_back(
            m_triangles[rnodes[child]->firstPrimitiveOffset + 1]);
          meta = meta | (3 << 5);
          triangle_offset++;
        }
        if (rnodes[child]->numPrimitives >= 3) {
          orderedPrims.push_back(
            m_triangles[rnodes[child]->firstPrimitiveOffset + 2]);
          meta = meta | (7 << 5);
          triangle_offset++;
        }
        if (rnodes[child]->numPrimitives >= 4)
          printf("ERRRRRRORRRRRRRRRRRRRRRRR\n");
        wnode->meta[childBBOX_index] =
          wnode->meta[childBBOX_index] | (meta << (childBBOX_bit_index));
      } else {
        wnode->imask = wnode->imask | 1 << child;
        int meta     = 32;
        meta         = 56; // 001 11xxx
        meta += node_offset++;
        (*offset)++;

        // packed offset count
        wnode->meta[childBBOX_index] =
          wnode->meta[childBBOX_index] | (meta << (childBBOX_bit_index));
      }
    }

    node_offset = 0;
    for (int child = 0; child < 8; ++child) {
      if ((wnode->imask & 1 << child) != 0)
        flattenNext(rnodes[child],
                    wnode->child_node_base_index + (node_offset++), offset,
                    orderedPrims, flattenNext);
    }
  };

  auto collapseRecNext = [&](bvh_node* node, Cost* cost) {
    int total_nodes = CountNodes2(node, CountNodes2);
    int offset      = 1;
    m_linear_nodes  = new wr_wide_bvh_node[total_nodes + 1];
    // std::memset(nodes, 0, total_nodes * sizeof(WideNode));
    std::vector<ivec4> orderedPrims;
    orderedPrims.reserve(m_triangles.size());
    { // the root should contain only one child
      wr_wide_bvh_node* wnode = &m_linear_nodes[0];
      wnode->px               = node->bounds.min.x;
      wnode->py               = node->bounds.min.y;
      wnode->pz               = node->bounds.min.z;

      float ex2 = exp2f(ceilf(log2f(
        (cost->node->bounds.max.x - cost->node->bounds.min.x) / (255.f))));
      float ey2 = exp2f(ceilf(log2f(
        (cost->node->bounds.max.y - cost->node->bounds.min.y) / (255.f))));
      float ez2 = exp2f(ceilf(log2f(
        (cost->node->bounds.max.z - cost->node->bounds.min.z) / (255.f))));

      int u_ex, u_ey, u_ez;
      memcpy(&u_ex, &ex2, 4);
      memcpy(&u_ey, &ey2, 4);
      memcpy(&u_ez, &ez2, 4);

      wnode->ex = u_ex >> 23;
      wnode->ey = u_ey >> 23;
      wnode->ez = u_ez >> 23;

      wnode->imask                 = 0;
      wnode->child_node_base_index = 1;
      wnode->triangle_base_index   = (int)orderedPrims.size();
      int node_offset              = 0;
      wnode->meta[0]               = 0;
      wnode->meta[1]               = 0;
      std::memset(wnode->childBBOX, 0, 12 * sizeof(int));

      int qlox = (int)floorf((node->bounds.min.x - node->bounds.min.x) / ex2);
      int qloy = (int)floorf((node->bounds.min.y - node->bounds.min.y) / ey2);
      int qloz = (int)floorf((node->bounds.min.z - node->bounds.min.z) / ez2);

      int qhix = (int)ceilf((node->bounds.max.x - node->bounds.min.x) / ex2);
      int qhiy = (int)ceilf((node->bounds.max.y - node->bounds.min.y) / ey2);
      int qhiz = (int)ceilf((node->bounds.max.z - node->bounds.min.z) / ez2);

      int childBBOX_index     = 0 / 4;
      int childBBOX_bit_index = 0 % 4;
      childBBOX_bit_index     = childBBOX_bit_index * 8;

      wnode->childBBOX[childBBOX_index] =
        wnode->childBBOX[childBBOX_index] | (qlox << childBBOX_bit_index);
      wnode->childBBOX[childBBOX_index + 2] =
        wnode->childBBOX[childBBOX_index + 2] | (qloy << childBBOX_bit_index);
      wnode->childBBOX[childBBOX_index + 4] =
        wnode->childBBOX[childBBOX_index + 4] | (qloz << childBBOX_bit_index);

      wnode->childBBOX[childBBOX_index + 6] =
        wnode->childBBOX[childBBOX_index + 6] | (qhix << childBBOX_bit_index);
      wnode->childBBOX[childBBOX_index + 8] =
        wnode->childBBOX[childBBOX_index + 8] | (qhiy << childBBOX_bit_index);
      wnode->childBBOX[childBBOX_index + 10] =
        wnode->childBBOX[childBBOX_index + 10] | (qhiz << childBBOX_bit_index);

      if (node->nPrimitives != 0) // Leaf
      {
        char meta = 0;
        for (int p = 0; p < node->nPrimitives; ++p) {
          orderedPrims.push_back(m_triangles[node->firstPrimOffset + p]);
          meta = meta | (1 << (5 + p));
        }
        wnode->meta[childBBOX_index] =
          wnode->meta[childBBOX_index] | (meta << (childBBOX_bit_index));
      } else {
        // It is an internal node
        wnode->imask = wnode->imask | 1 << 0;
        int meta     = 32;
        meta         = 56; // 001 11xxx
        meta += node_offset++;
        offset++; // no need since it starts from 1

        // packed offset count
        wnode->meta[childBBOX_index] =
          wnode->meta[childBBOX_index] | (meta << (childBBOX_bit_index));
      }
    }
    flattenNext(cost, 1, &offset, orderedPrims, flattenNext);
    m_triangles.swap(orderedPrims);
    m_total_nodes = offset + 1;
  };

  /// Trivial
  auto flattenTrivial = [&](Cost* cost, int curOffset, int* offset,
                            std::vector<ivec4>& orderedPrims,
                            auto&&              flattenTrivial) -> void {
    wr_wide_bvh_node* wnode = &m_linear_nodes[curOffset];
    wnode->px               = cost->node->bounds.min.x;
    wnode->py               = cost->node->bounds.min.y;
    wnode->pz               = cost->node->bounds.min.z;

    float ex2 = exp2f(ceilf(
      log2f((cost->node->bounds.max.x - cost->node->bounds.min.x) / (255.f))));
    float ey2 = exp2f(ceilf(
      log2f((cost->node->bounds.max.y - cost->node->bounds.min.y) / (255.f))));
    float ez2 = exp2f(ceilf(
      log2f((cost->node->bounds.max.z - cost->node->bounds.min.z) / (255.f))));

    int u_ex, u_ey, u_ez;
    memcpy(&u_ex, &ex2, 4);
    memcpy(&u_ey, &ey2, 4);
    memcpy(&u_ez, &ez2, 4);

    wnode->ex = u_ex >> 23;
    wnode->ey = u_ey >> 23;
    wnode->ez = u_ez >> 23;

    // printf("Node %d ", curOffset);
    // printBBOX(cost->node->bounds.min, cost->node->bounds.max);

    wnode->imask                 = 0;
    wnode->child_node_base_index = *offset;
    wnode->triangle_base_index   = (int)orderedPrims.size();
    int triangle_offset          = 0;
    int node_offset              = 0;
    wnode->meta[0]               = 0;
    wnode->meta[1]               = 0;
    std::memset(wnode->childBBOX, 0, 12 * sizeof(int));

    ///
    std::list<Cost*> snodes;
    snodes.push_back(cost);
    int pop_counter = 0;
    while (snodes.size() < 7 && pop_counter < 8) {
      Cost* top = snodes.front();
      snodes.pop_front();

      // If leaf
      if (top->children[0] == nullptr && top->children[1] == nullptr) {
        snodes.push_back(top);
        pop_counter++;
        continue;
      }

      // Else Expand
      if (top->children[0] != nullptr)
        snodes.push_back(top->children[0]);
      if (top->children[1] != nullptr)
        snodes.push_back(top->children[1]);
      pop_counter = 0;
    }
    std::vector<Cost*> rnodes(snodes.begin(), snodes.end());

    for (size_t child = 0; child < rnodes.size(); ++child) {
      int qlox = (int)floorf(
        (rnodes[child]->node->bounds.min.x - cost->node->bounds.min.x) / ex2);
      int qloy = (int)floorf(
        (rnodes[child]->node->bounds.min.y - cost->node->bounds.min.y) / ey2);
      int qloz = (int)floorf(
        (rnodes[child]->node->bounds.min.z - cost->node->bounds.min.z) / ez2);

      int qhix = (int)ceilf(
        (rnodes[child]->node->bounds.max.x - cost->node->bounds.min.x) / ex2);
      int qhiy = (int)ceilf(
        (rnodes[child]->node->bounds.max.y - cost->node->bounds.min.y) / ey2);
      int qhiz = (int)ceilf(
        (rnodes[child]->node->bounds.max.z - cost->node->bounds.min.z) / ez2);

      // printf("Node %d.%d (%d)\n", curOffset, child,
      // rnodes[child]->numPrimitives);
      // printBBOX(rnodes[child]->node->bounds.min,
      // rnodes[child]->node->bounds.max);

      int childBBOX_index     = child / 4;
      int childBBOX_bit_index = child % 4;
      childBBOX_bit_index     = childBBOX_bit_index * 8;

      wnode->childBBOX[childBBOX_index] =
        wnode->childBBOX[childBBOX_index] | (qlox << childBBOX_bit_index);
      wnode->childBBOX[childBBOX_index + 2] =
        wnode->childBBOX[childBBOX_index + 2] | (qloy << childBBOX_bit_index);
      wnode->childBBOX[childBBOX_index + 4] =
        wnode->childBBOX[childBBOX_index + 4] | (qloz << childBBOX_bit_index);

      wnode->childBBOX[childBBOX_index + 6] =
        wnode->childBBOX[childBBOX_index + 6] | (qhix << childBBOX_bit_index);
      wnode->childBBOX[childBBOX_index + 8] =
        wnode->childBBOX[childBBOX_index + 8] | (qhiy << childBBOX_bit_index);
      wnode->childBBOX[childBBOX_index + 10] =
        wnode->childBBOX[childBBOX_index + 10] | (qhiz << childBBOX_bit_index);

      // check if the child is a leaf
      if (rnodes[child]->numPrimitives <= 3) {
        int meta = triangle_offset++;
        if (rnodes[child]->numPrimitives >= 1) {
          orderedPrims.push_back(
            m_triangles[rnodes[child]->firstPrimitiveOffset + 0]);
          meta = meta | (1 << 5);
        }
        if (rnodes[child]->numPrimitives >= 2) {
          orderedPrims.push_back(
            m_triangles[rnodes[child]->firstPrimitiveOffset + 1]);
          meta = meta | (3 << 5);
          triangle_offset++;
        }
        if (rnodes[child]->numPrimitives >= 3) {
          orderedPrims.push_back(
            m_triangles[rnodes[child]->firstPrimitiveOffset + 2]);
          meta = meta | (7 << 5);
          triangle_offset++;
        }

        wnode->meta[childBBOX_index] =
          wnode->meta[childBBOX_index] | (meta << (childBBOX_bit_index));
      } else {
        wnode->imask = wnode->imask | 1 << child;
        int meta     = 32;
        meta         = 56; // 001 11xxx
        meta += node_offset++;
        (*offset)++;

        // packed offset count
        wnode->meta[childBBOX_index] =
          wnode->meta[childBBOX_index] | (meta << (childBBOX_bit_index));
      }
    }

    node_offset = 0;
    for (int child = 0; child < 8; ++child) {
      if ((wnode->imask & 1 << child) != 0)
        flattenTrivial(rnodes[child],
                       wnode->child_node_base_index + (node_offset++), offset,
                       orderedPrims, flattenTrivial);
    }
  };

  auto collapseRecTrivial = [&](bvh_node* node, Cost* cost) {
    int total_nodes = CountNodes2(node, CountNodes2);
    int offset      = 1;
    m_linear_nodes  = new wr_wide_bvh_node[total_nodes + 1];
    std::memset(m_linear_nodes, 0, total_nodes * sizeof(wr_wide_bvh_node));
    std::vector<ivec4> orderedPrims;
    orderedPrims.reserve(m_triangles.size());
    { // the root should contain only one child
      wr_wide_bvh_node* wnode = &m_linear_nodes[0];
      wnode->px               = node->bounds.min.x;
      wnode->py               = node->bounds.min.y;
      wnode->pz               = node->bounds.min.z;

      float ex2 = exp2f(ceilf(log2f(
        (cost->node->bounds.max.x - cost->node->bounds.min.x) / (255.f))));
      float ey2 = exp2f(ceilf(log2f(
        (cost->node->bounds.max.y - cost->node->bounds.min.y) / (255.f))));
      float ez2 = exp2f(ceilf(log2f(
        (cost->node->bounds.max.z - cost->node->bounds.min.z) / (255.f))));

      int u_ex, u_ey, u_ez;
      memcpy(&u_ex, &ex2, 4);
      memcpy(&u_ey, &ey2, 4);
      memcpy(&u_ez, &ez2, 4);

      wnode->ex = u_ex >> 23;
      wnode->ey = u_ey >> 23;
      wnode->ez = u_ez >> 23;

      wnode->imask                 = 0;
      wnode->child_node_base_index = 1;
      wnode->triangle_base_index   = (int)orderedPrims.size();
      int node_offset              = 0;
      wnode->meta[0]               = 0;
      wnode->meta[1]               = 0;
      std::memset(wnode->childBBOX, 0, 12 * sizeof(int));

      int qlox = (int)floorf((node->bounds.min.x - node->bounds.min.x) / ex2);
      int qloy = (int)floorf((node->bounds.min.y - node->bounds.min.y) / ey2);
      int qloz = (int)floorf((node->bounds.min.z - node->bounds.min.z) / ez2);

      int qhix = (int)ceilf((node->bounds.max.x - node->bounds.min.x) / ex2);
      int qhiy = (int)ceilf((node->bounds.max.y - node->bounds.min.y) / ey2);
      int qhiz = (int)ceilf((node->bounds.max.z - node->bounds.min.z) / ez2);

      int childBBOX_index     = 0 / 4;
      int childBBOX_bit_index = 0 % 4;
      childBBOX_bit_index     = childBBOX_bit_index * 8;

      wnode->childBBOX[childBBOX_index] =
        wnode->childBBOX[childBBOX_index] | (qlox << childBBOX_bit_index);
      wnode->childBBOX[childBBOX_index + 2] =
        wnode->childBBOX[childBBOX_index + 2] | (qloy << childBBOX_bit_index);
      wnode->childBBOX[childBBOX_index + 4] =
        wnode->childBBOX[childBBOX_index + 4] | (qloz << childBBOX_bit_index);

      wnode->childBBOX[childBBOX_index + 6] =
        wnode->childBBOX[childBBOX_index + 6] | (qhix << childBBOX_bit_index);
      wnode->childBBOX[childBBOX_index + 8] =
        wnode->childBBOX[childBBOX_index + 8] | (qhiy << childBBOX_bit_index);
      wnode->childBBOX[childBBOX_index + 10] =
        wnode->childBBOX[childBBOX_index + 10] | (qhiz << childBBOX_bit_index);

      if (node->nPrimitives != 0) // Leaf
      {
        char meta = 0;
        for (int p = 0; p < node->nPrimitives; ++p) {
          orderedPrims.push_back(m_triangles[node->firstPrimOffset + p]);
          meta = meta | (1 << (5 + p));
        }
        wnode->meta[childBBOX_index] =
          wnode->meta[childBBOX_index] | (meta << (childBBOX_bit_index));
      } else {
        // It is an internal node
        wnode->imask = wnode->imask | 1 << 0;
        int meta     = 32;
        meta         = 56; // 001 11xxx
        meta += node_offset++;
        offset++; // no need since it starts from 1

        // packed offset count
        wnode->meta[childBBOX_index] =
          wnode->meta[childBBOX_index] | (meta << (childBBOX_bit_index));
      }
    }
    flattenTrivial(cost, 1, &offset, orderedPrims, flattenTrivial);
    m_total_nodes = offset + 1;
    m_triangles.swap(orderedPrims);
  };

  Cost cost;
  cost.node = root;
  buildRec(root, &cost, rootSurfaceArea, buildRec);
  collapseRecNext(root, &cost);
  // collapseRecTrivial(root, &cost);

  m_need_update = false;

  return true;
}

const char*
WideBVH::GetIntersectionCode()
{
  delete[] intersection_code;

  std::string str;
  str += "#define WR_PRIMITIVE_TEXTURE_SIZE " +
         std::to_string(m_index_texture_size) + "\n";
  str += "#define WR_TRAVERSE_STACK_SIZE " + std::to_string(16) + "\n";
  str += "#define WR_NODES_TEXTURE_SIZE " +
         std::to_string(m_node_texture_size) + "\n";
  str += "#define WR_SCENE_TEXTURE_SIZE " +
         std::to_string(m_vertex_texture_size) + "\n";
  str += "#define WR_INSTANCE_TEXTURE_SIZE " +
         std::to_string(m_instance_texture_size) + "\n";
  str += "#define WR_INSTANCE_TRIANGLE_SPLIT_BIT " + std::to_string(24) + "\n";
  str += "#define WR_TLAS_ID_MASK " + std::to_string(WR_TLAS_ID_MASK) + "\n";

  int instance_count = m_instance_texture_size / 4;
  str += "#define wr_InstanceCount " + std::to_string(instance_count) + "\n";
  str += "#define wr_SphereCount " + std::to_string(0) + "\n";
  str +=
    "#define wr_TriangleCount " + std::to_string(m_triangles.size()) + "\n";
  str += "#define wr_BVHNodeCount " + std::to_string(m_total_nodes) + "\n";
  str += "#define WR_RAY_MAX_DISTANCE 1.e27\n";
  str += "#define WR_IS_TLAS(x) (((x) & WR_TLAS_ID_MASK) == WR_TLAS_ID_MASK)\n";

  str += "uniform sampler2DArray wr_scene_vertices;\n";
  str += "uniform sampler2DArray wr_scene_instances;\n";
  str += "uniform isampler2DArray wr_scene_indices;\n";
  str += "uniform sampler2DArray wr_bvh_nodes;\n";

  /* Maybe we need this for instancing */
  // str += "const ivec2 wr_Counts[8] = ivec2[]( ivec2(" +
  // std::to_string(m_triangles.size()) + ", " + std::to_string(m_total_nodes) +
  // "), ivec2(0,0), ivec2(0,0), ivec2(0,0), ivec2(0,0), ivec2(0,0), ivec2(0,0),
  // ivec2(0,0) );\n";

  if (instance_count) {
    // str += "layout(std140) uniform wr_InstancesUBO { mat4
    // instances[wr_InstanceCount]; };";
  }
  str += g_widebvh_get_ads_id;
  str += g_widebvh_get_instance_transfrom;

  /* Instancing */
  if (instance_count) {
    str +=
      "int wr_GetPackedInstanceTriangleID(int triangle, int instance) { return "
      "( ( instance << WR_INSTANCE_TRIANGLE_SPLIT_BIT) | triangle ); }\n";
    str += "vec3 wr_GetWorldRayOrigin(int ads, int instance, vec3 origin) { "
           "return vec3(inverse(wr_GetObjectTranform(ads, instance)) * "
           "vec4(origin, 1.0)); }\n";
    str += "vec3 wr_GetWorldRayDirection(int ads, int instance, vec3 "
           "direction) { return vec3(inverse(wr_GetNormalTranform(ads, "
           "instance)) * vec4(direction, 0.0)); }\n";
    str += "vec3 wr_TransformDirectionFromObjectToWorldSpace(int ads, int "
           "instance, vec3 direction) { return vec3(wr_GetNormalTranform(ads, "
           "instance) * vec4(direction, 0.0));	}";
    str += "vec3 wr_TransformPositionFromObjectToWorldSpace(int ads, int "
           "instance, vec3 position) { return vec3(wr_GetObjectTranform(ads, "
           "instance) * vec4(position, 1.0)); }";
  }

  str += "vec4 wr_GetPackedPosExyzMask(int ads, int i) { int b = 5 * i + 0; "
         "return texelFetch(wr_bvh_nodes, ivec3(b % WR_NODES_TEXTURE_SIZE, b / "
         "WR_NODES_TEXTURE_SIZE, ads), 0); }\n";
  str += "ivec4 wr_GetPackedNodeTriangleBaseIndexMeta(int ads, int i) { int b "
         "= 5 * i + 1; return floatBitsToInt(texelFetch(wr_bvh_nodes, ivec3(b "
         "% WR_NODES_TEXTURE_SIZE, b / WR_NODES_TEXTURE_SIZE, ads), 0)); }\n";
  str += "ivec4 wr_GetPackedChildBBOX0(int ads, int i) { int b = 5 * i + 2; "
         "return floatBitsToInt(texelFetch(wr_bvh_nodes, ivec3(b % "
         "WR_NODES_TEXTURE_SIZE, b / WR_NODES_TEXTURE_SIZE, ads), 0)); }\n";
  str += "ivec4 wr_GetPackedChildBBOX1(int ads, int i) { int b = 5 * i + 3; "
         "return floatBitsToInt(texelFetch(wr_bvh_nodes, ivec3(b % "
         "WR_NODES_TEXTURE_SIZE, b / WR_NODES_TEXTURE_SIZE, ads), 0)); }\n";
  str += "ivec4 wr_GetPackedChildBBOX2(int ads, int i) { int b = 5 * i + 4; "
         "return floatBitsToInt(texelFetch(wr_bvh_nodes, ivec3(b % "
         "WR_NODES_TEXTURE_SIZE, b / WR_NODES_TEXTURE_SIZE, ads), 0)); }\n";
  str += g_copysign_func;
  str += g_ray_triangle_intersection_func;

  str += g_widebvh_attribute_accessors;
  str += g_ray_widebvh_intersect_fragment_shader;

  intersection_code             = new char[str.size() + 1];
  intersection_code[str.size()] = 0;
  std::memcpy(intersection_code, str.data(), str.size() * sizeof(char));

  return intersection_code;
}
