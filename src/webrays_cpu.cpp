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
#include "webrays_cpu.h"
#include "webrays_queue.h"
#include "webrays_math.h"
#include "webrays_ads.h"

#include <cstring> // memset

typedef struct
{
  wr_binding intersection_bindings[3];
  int        binding_count;
} wr_cpu_context;

wr_error
wrays_cpu_init(wr_handle handle)
{
  wr_context* webrays = (wr_context*)handle;
  if (WR_NULL == webrays)
    return (wr_error) "Invalid WebRays context";
  wr_cpu_context* webrays_cpu = (wr_cpu_context*)malloc(sizeof(*webrays_cpu));
  if (WR_NULL == webrays_cpu)
    return (wr_error) "Failed to allocate CPU WebRays context";

  memset(webrays_cpu, 0, sizeof(*webrays_cpu));

  webrays->cpu = webrays_cpu;

  return WR_SUCCESS;
}

wr_error
wrays_cpu_add_shape(wr_handle handle, wr_handle ads_id, float* positions,
                    int position_stride, float* normals, int normal_stride,
                    float* uvs, int uv_stride, int attr_count, int* faces,
                    int num_triangles, int* shape_id)
{

  wr_context* webrays = (wr_context*)handle;
  if (WR_NULL == webrays)
    return WR_NULL;
  wr_cpu_context* webrays_cpu = (wr_cpu_context*)webrays->cpu;
  if (WR_NULL == webrays_cpu)
    return WR_NULL;

  ADS* ads = webrays->scene.blas_handles[(int)(size_t)ads_id];

  *shape_id =
    ads->AddTriangularMesh(positions, position_stride, normals, normal_stride,
                           uvs, uv_stride, attr_count, faces, num_triangles);

  return WR_SUCCESS;
}

wr_error
wrays_cpu_ads_create(wr_handle handle, wr_handle ads,
                     wr_ads_descriptor* descriptor)
{

  return WR_SUCCESS;
}

const char*
wrays_cpu_get_scene_accessor(wr_handle handle)
{

  return WR_NULL;
}

const wr_binding*
wrays_cpu_get_scene_accessor_bindings(wr_handle handle, wr_size* count)
{
  wr_context* webrays = (wr_context*)handle;
  if (WR_NULL == webrays)
    return WR_NULL;
  wr_cpu_context* webrays_cpu = (wr_cpu_context*)webrays->cpu;
  if (WR_NULL == webrays_cpu)
    return WR_NULL;

  *count = webrays_cpu->binding_count;

  return &webrays_cpu->intersection_bindings[0];
}

WR_INTERNAL wr_error
            wrays_cpu_ads_build(wr_handle handle)
{
  wr_context* webrays = (wr_context*)handle;
  if (WR_NULL == webrays)
    return (wr_error) "Invalid WebRays context";
  wr_cpu_context* webrays_cpu = (wr_cpu_context*)webrays->cpu;
  if (WR_NULL == webrays_cpu)
    return (wr_error) "Invalid CPU WebRays context";

  ADS* ads = (ADS*)webrays->scene.blas_handles[0];

  ads->Build();

  return WR_SUCCESS;
}

wr_error
wrays_cpu_update(wr_handle handle)
{
  wr_context* webrays = (wr_context*)handle;
  if (WR_NULL == webrays)
    return (wr_error) "Invalid WebRays context";
  wr_cpu_context* webrays_cpu = (wr_cpu_context*)webrays->cpu;
  if (WR_NULL == webrays_cpu)
    return (wr_error) "Invalid CPU WebRays context";

  wrays_cpu_ads_build(handle);

  SAHBVH* ads = (SAHBVH*)webrays->scene.blas_handles[0];

  webrays_cpu->intersection_bindings[0] = { "wr_scene_vertices",
                                            WR_BINDING_TYPE_CPU_BUFFER, 0 };
  webrays_cpu->intersection_bindings[1] = { "wr_scene_indices",
                                            WR_BINDING_TYPE_CPU_BUFFER, 0 };
  webrays_cpu->intersection_bindings[2] = { "wr_bvh_nodes",
                                            WR_BINDING_TYPE_CPU_BUFFER, 0 };
  webrays_cpu->intersection_bindings[0].data.cpu_buffer.buffer =
    ads->m_vertex_data.data();
  webrays_cpu->intersection_bindings[0].data.cpu_buffer.size =
    (unsigned int)ads->m_vertex_data.size();
  webrays_cpu->intersection_bindings[1].data.cpu_buffer.buffer =
    ads->m_triangles.data();
  webrays_cpu->intersection_bindings[1].data.cpu_buffer.size =
    (unsigned int)ads->m_triangles.size();
  webrays_cpu->intersection_bindings[2].data.cpu_buffer.buffer =
    ads->m_linear_nodes;
  webrays_cpu->intersection_bindings[2].data.cpu_buffer.size =
    (unsigned int)ads->m_total_nodes;
  webrays_cpu->binding_count =
    (int)(sizeof(webrays_cpu->intersection_bindings) /
          sizeof(webrays_cpu->intersection_bindings[0]));

  return WR_SUCCESS;
}