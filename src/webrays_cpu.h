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

#ifndef _WRAYS_CPU_H_
#define _WRAYS_CPU_H_

#include "webrays/webrays.h"

const char*
wrays_cpu_get_scene_accessor(wr_handle handle);
const wr_binding*
wrays_cpu_get_scene_accessor_bindings(wr_handle handle, wr_size* count);

wr_error
wrays_cpu_init(wr_handle handle);
wr_error
wrays_cpu_update(wr_handle handle);

wr_error
wrays_cpu_add_shape(wr_handle handle, wr_handle ads, float* positions,
                    int position_stride, float* normals, int normal_stride,
                    float* uvs, int uv_stride, int attr_count, int* faces,
                    int num_triangles, int* shape_id);

wr_error
wrays_cpu_ads_create(wr_handle handle, wr_handle ads,
                     wr_ads_descriptor* descriptor);

#endif /* _WRAYS_CPU_H_ */
