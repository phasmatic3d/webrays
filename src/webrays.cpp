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

#include "webrays_context.h"
#include "webrays_gl.h"
#include "webrays_cpu.h"
#include "webrays_ads.h"
#include "webrays_tlas.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "webrays_queue.h"

#define WR_MATH_IMPLEMENTATION
#include "webrays_math.h"

void
wrays_version(int* major, int* minor)
{
  *major = WR_VERSION_MAJOR;
  *minor = WR_VERSION_MINOR;
}

char const*
wrays_version_string()
{
  return WR_VERSION_STRING;
}

const char*
wrays_error_string(wr_handle handle, wr_error error)
{
  return (const char*)error;
}

wr_handle
wrays_init(wr_backend_type backend_type, wr_handle data)
{
  wr_context* webrays = (wr_context*)malloc(sizeof(*webrays));
  memset(webrays, 0, sizeof(*webrays));

  webrays->backend_type = backend_type;

  switch (webrays->backend_type) {
    case WR_BACKEND_TYPE_GLES: wrays_gl_init(webrays); break;
    case WR_BACKEND_TYPE_CPU: wrays_cpu_init(webrays); break;
    default: break;
  }

  return webrays;
}

#define WR_MAX_NUMBER_OF_BLAS_REACHED                                          \
  ((wr_error) "You cannot create more than " WR_XSTR(WR_MAX_BLAS_COUNT) " BLA" \
                                                                        "S")
#define WR_MAX_NUMBER_OF_TLAS_REACHED                                          \
  ((wr_error) "You cannot create more than " WR_XSTR(WR_MAX_TLAS_COUNT) " TLA" \
                                                                        "S")
#define WR_INVALID_OPTIONS ((wr_error) "You provided invalid options.")
wr_error
wrays_create_ads(wr_handle handle, wr_handle* ads, wr_ads_descriptor* options,
                 int options_count)
{
  wr_context* webrays = (wr_context*)handle;

  // Get the type of the ADS
  wr_ads_type ads_type = wr_ads_type::WR_ADS_TYPE_BLAS;
  if (options != nullptr && options_count > 0) {
    for (int i = 0; i < options_count; ++i) {
      if (strncmp(options[i].key, "type", 4) == 0) {
        ads_type = (strncmp(options[i].value, "BLAS", 4) == 0)
                     ? WR_ADS_TYPE_BLAS
                     : (strncmp(options[i].value, "TLAS", 4) == 0)
                         ? WR_ADS_TYPE_TLAS
                         : WR_ADS_TYPE_BLAS;
      }
    }
  }

  // the type of the ADS
  if (ads_type == WR_ADS_TYPE_BLAS) {
    if (WR_MAX_BLAS_COUNT == webrays->scene.blas_count)
      return WR_MAX_NUMBER_OF_BLAS_REACHED;

    int ads_id = webrays->scene.blas_count++;
    *ads       = (wr_handle)((intptr_t)ads_id);

    webrays->scene.blas_type = WR_BLAS_TYPE_WIDEBVH;
    // webrays->scene.blas_handles[(int)*ads] = new SAHBVH();
    webrays->scene.blas_handles[ads_id] = new WideBVH();

    switch (webrays->backend_type) {
      case WR_BACKEND_TYPE_GLES:
        return wrays_gl_ads_create(webrays, *ads, options, options_count);
      default: break;
    }
  } else // type TLAS
  {
    if (WR_MAX_TLAS_COUNT == webrays->scene.tlas_count)
      return WR_MAX_NUMBER_OF_TLAS_REACHED;

    int ads_id = webrays->scene.tlas_count++;
    *ads       = WR_INT2PTR(ads_id | WR_TLAS_ID_MASK);

    webrays->scene.tlas_handles[ads_id] = new TLAS();
  }

  webrays->update_flags =
    (wr_update_flags)(webrays->update_flags | WR_UPDATE_FLAG_ACCESSOR_BINDINGS |
                      WR_UPDATE_FLAG_ACCESSOR_CODE);

  return WR_SUCCESS;
}

#define WR_INVALID_POSITION_BUFFER ((wr_error) "Invalid position buffer")
#define WR_INVALID_INDEX_BUFFER ((wr_error) "Invalid index buffer")
#define WR_INVALID_ADS_HANDLE ((wr_error) "Invalid ADS handle")
wr_error
wrays_add_shape(wr_handle handle, wr_handle ads, float* positions,
                int position_stride, float* normals, int normal_stride,
                float* uvs, int uv_stride, int num_vertices, int* indices,
                int num_triangles, int* shape_id)
{
  wr_context* webrays = (wr_context*)handle;
  wr_error    error   = WR_SUCCESS;

  int ads_id = WR_PTR2INT(ads);

  if ((ads_id & WR_TLAS_ID_MASK) == WR_TLAS_ID_MASK)
    return WR_INVALID_ADS_HANDLE;

  *shape_id = -1;

  if (WR_NULL == positions) {
    return WR_INVALID_POSITION_BUFFER;
  }

  if (WR_NULL == indices) {
    return WR_INVALID_INDEX_BUFFER;
  }

  switch (webrays->backend_type) {
    case WR_BACKEND_TYPE_GLES:
      error = wrays_gl_add_shape(
        handle, ads, positions, position_stride, normals, normal_stride, uvs,
        uv_stride, num_vertices, indices, num_triangles, shape_id);
      break;
    case WR_BACKEND_TYPE_CPU:
      error = wrays_cpu_add_shape(
        handle, ads, positions, position_stride, normals, normal_stride, uvs,
        uv_stride, num_vertices, indices, num_triangles, shape_id);
      break;
    default: break;
  }

  webrays->needs_update = 1;
  webrays->update_flags =
    (wr_update_flags)(webrays->update_flags | WR_UPDATE_FLAG_ACCESSOR_CODE |
                      WR_UPDATE_FLAG_ACCESSOR_BINDINGS);

  return error;
}

#define WR_INVALID_BLAS_HANDLE ((wr_error) "Invalid BLAS handle")
#define WR_INVALID_TLAS_HANDLE ((wr_error) "Invalid TLAS handle")
#define WR_INVALID_TRANSFORM_BUFFER ((wr_error) "Invalid transformation matrix")
wr_error
wrays_add_instance(wr_handle handle, wr_handle tlas, wr_handle blas,
                   float* transformation, int* instance_id)
{
  wr_context* webrays = (wr_context*)handle;

  int tlas_id = WR_PTR2INT(tlas) & ~WR_TLAS_ID_MASK;
  int blas_id = WR_PTR2INT(blas);

  if (tlas_id < 0 || tlas_id >= webrays->scene.tlas_count ||
      webrays->scene.tlas_handles[tlas_id] == nullptr)
    return WR_INVALID_TLAS_HANDLE;
  if (blas_id < 0 || blas_id >= webrays->scene.blas_count ||
      webrays->scene.blas_handles[blas_id] == nullptr)
    return WR_INVALID_BLAS_HANDLE;
  if (transformation == nullptr)
    return WR_INVALID_TRANSFORM_BUFFER;

  Instance inst;
  inst.blas_offset = blas_id;
  std::memcpy(inst.transform, transformation, 12 * sizeof(float));

  *instance_id = (int)webrays->scene.tlas_handles[tlas_id]->m_instances.size();
  webrays->scene.tlas_handles[tlas_id]->m_instances.push_back(inst);

  webrays->needs_update = 1;
  webrays->update_flags =
    (wr_update_flags)(webrays->update_flags | WR_UPDATE_FLAG_INSTANCE_ADD);

  return WR_SUCCESS;
}

#define WR_INVALID_INSTANCE_HANDLE ((wr_error) "Invalid InstanceID")
wr_error
wrays_update_instance(wr_handle handle, wr_handle tlas, int instance_id,
                      float* transformation)
{
  wr_context* webrays = (wr_context*)handle;

  int tlas_id = WR_PTR2INT(tlas) & ~WR_TLAS_ID_MASK;

  if (tlas_id < 0 || tlas_id >= webrays->scene.tlas_count ||
      webrays->scene.tlas_handles[tlas_id] == nullptr)
    return WR_INVALID_TLAS_HANDLE;

  const auto ads = webrays->scene.tlas_handles[tlas_id];
  if (instance_id < 0 || instance_id >= ads->m_instances.size())
    return WR_INVALID_INSTANCE_HANDLE;

  if (transformation == nullptr)
    return WR_INVALID_TRANSFORM_BUFFER;

  std::memcpy(ads->m_instances[instance_id].transform, transformation,
              12 * sizeof(float));

  webrays->needs_update = 1;
  webrays->update_flags =
    (wr_update_flags)(webrays->update_flags | WR_UPDATE_FLAG_INSTANCE_UPDATE);

  return WR_SUCCESS;
}

const char*
wrays_get_scene_accessor(wr_handle handle)
{
  wr_context* webrays = (wr_context*)handle;

  switch (webrays->backend_type) {
    case WR_BACKEND_TYPE_GLES: return wrays_gl_get_scene_accessor(webrays);
    case WR_BACKEND_TYPE_CPU: return wrays_cpu_get_scene_accessor(webrays);
    default: break;
  }

  return WR_NULL;
}

wr_error
wrays_ray_buffer_requirements(wr_handle handle, wr_buffer_info* buffer_info,
                              wr_size* dimensions, wr_size dimension_count)
{
  wr_context* webrays = (wr_context*)handle;

  switch (webrays->backend_type) {
    case WR_BACKEND_TYPE_GLES:
      return wrays_gl_ray_buffer_requirements(webrays, buffer_info, dimensions,
                                              dimension_count);
    default: break;
  }

  return WR_SUCCESS;
}

wr_error
wrays_occlusion_buffer_requirements(wr_handle       handle,
                                    wr_buffer_info* buffer_info,
                                    wr_size*        dimensions,
                                    wr_size         dimension_count)
{
  wr_context* webrays = (wr_context*)handle;

  switch (webrays->backend_type) {
    case WR_BACKEND_TYPE_GLES:
      return wrays_gl_occlusion_buffer_requirements(
        webrays, buffer_info, dimensions, dimension_count);
    default: break;
  }

  return WR_SUCCESS;
}

wr_error
wrays_intersection_buffer_requirements(wr_handle       handle,
                                       wr_buffer_info* buffer_info,
                                       wr_size*        dimensions,
                                       wr_size         dimension_count)
{
  wr_context* webrays = (wr_context*)handle;

  switch (webrays->backend_type) {
    case WR_BACKEND_TYPE_GLES:
      return wrays_gl_intersection_buffer_requirements(
        webrays, buffer_info, dimensions, dimension_count);
    default: break;
  }

  return WR_SUCCESS;
}

const wr_binding*
wrays_get_scene_accessor_bindings(wr_handle handle, wr_size* count)
{
  wr_context* webrays = (wr_context*)handle;

  switch (webrays->backend_type) {
    case WR_BACKEND_TYPE_GLES:
      return wrays_gl_get_scene_accessor_bindings(webrays, count);
    case WR_BACKEND_TYPE_CPU:
      return wrays_cpu_get_scene_accessor_bindings(webrays, count);
    default: break;
  }

  return WR_NULL;
}

wr_error
wrays_query_intersection(wr_handle handle, wr_handle ads,
                         wr_handle* ray_buffers, wr_size ray_buffer_count,
                         wr_handle intersections, wr_size* dimensions,
                         wr_size dimension_count)
{
  wr_context* webrays = (wr_context*)handle;

  switch (webrays->backend_type) {
    case WR_BACKEND_TYPE_GLES:
      wrays_gl_query_intersection(webrays, ads, ray_buffers, ray_buffer_count,
                                  intersections, dimensions, dimension_count);
    default: break;
  }

  return WR_SUCCESS;
}

wr_error
wrays_query_occlusion(wr_handle handle, wr_handle ads, wr_handle* ray_buffers,
                      wr_size ray_buffer_count, wr_handle occlusion,
                      wr_size* dimensions, wr_size dimension_count)
{
  wr_context* webrays = (wr_context*)handle;

  switch (webrays->backend_type) {
    case WR_BACKEND_TYPE_GLES:
      wrays_gl_query_occlusion(webrays, ads, ray_buffers, ray_buffer_count,
                               occlusion, dimensions, dimension_count);
    default: break;
  }

  return WR_SUCCESS;
}

wr_error
wrays_update(wr_handle handle, wr_update_flags* flags)
{
  wr_context* webrays = (wr_context*)handle;
  wr_error    error   = WR_SUCCESS;

  *flags = WR_UPDATE_FLAG_NONE;

  if (webrays->needs_update == 0)
    return WR_SUCCESS;

  switch (webrays->backend_type) {
    case WR_BACKEND_TYPE_GLES: error = wrays_gl_update(handle, flags); break;
    case WR_BACKEND_TYPE_CPU: error = wrays_cpu_update(handle); break;
    default: break;
  }

  webrays->needs_update = 0;

  return error;
}

wr_error
wrays_destroy(wr_handle webrays)
{
  return WR_SUCCESS;
}

/* Clean Up API */
wr_error
wrays_ray_buffer_destroy(wr_handle handle, wr_handle buffer)
{
  return WR_SUCCESS;
}

wr_error
wrays_intersection_buffer_destroy(wr_handle handle, wr_handle buffer)
{
  return WR_SUCCESS;
}

wr_error
wrays_occlusion_buffer_destroy(wr_handle handle, wr_handle buffer)
{
  return WR_SUCCESS;
}

wr_error
wrays_ads_destroy(wr_handle handle, wr_handle ads)
{
  return WR_SUCCESS;
}

#if defined(__EMSCRIPTEN__)

wr_error
_wrays_internal_get_intersection_kernel(wr_handle handle, unsigned int* program)
{
  wr_context* webrays = (wr_context*)handle;
  wr_error    error   = WR_SUCCESS;

  if (WR_NULL == webrays)
    return (wr_error) "Invalid WebRays context";

  switch (webrays->backend_type) {
    case WR_BACKEND_TYPE_GLES:
      error = _wrays_gl_internal_get_intersection_kernel(handle, program);
      break;
    default: break;
  }

  return error;
}

wr_error
_wrays_internal_get_occlusion_kernel(wr_handle handle, unsigned int* program)
{
  wr_context* webrays = (wr_context*)handle;
  wr_error    error   = WR_SUCCESS;

  if (WR_NULL == webrays)
    return (wr_error) "Invalid WebRays context";

  switch (webrays->backend_type) {
    case WR_BACKEND_TYPE_GLES:
      error = _wrays_gl_internal_get_occlusion_kernel(handle, program);
      break;
    default: break;
  }

  return error;
}

#endif