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

#ifndef _WRAYS_H_
#define _WRAYS_H_

#define WEBRAYS_PROTOTYPE_API

#if defined(__EMSCRIPTEN__)
#define WRAYS_EMSCRIPTEN 1

#include <emscripten.h>
#include <stddef.h>

#define WRAYS_API EMSCRIPTEN_KEEPALIVE

#elif defined(_WIN32)
#define WRAYS_WIN32 1

#ifdef WRAYS_BUILD
#define WRAYS_API __declspec(dllexport)
#else
#define WRAYS_API __declspec(dllimport)
#endif

#elif defined(__linux__)

#define WRAYS_LINUX 1
#define WRAYS_API

#elif __APPLE__

#define WRAYS_APPLE 1
#define WRAYS_API

#else

#define WRAYS_UNKNOWN_PLATFORM 1
#define WRAYS_API

/* other */

#endif /* __EMSCRIPTEN__ */

#define WR_XSTR(a) WR_STR(a)
#define WR_STR(a) #a
#define WR_VERSION_MINOR 8
#define WR_VERSION_MAJOR 0
#define WR_VERSION_STRING                                                      \
  "WebRays Version " WR_XSTR(WR_VERSION_MAJOR) "." WR_XSTR(WR_VERSION_MINOR)

#define WR_INTERNAL static
#define WR_SUCCESS ((wr_error)0)
#ifdef __cplusplus
#define WR_NULL nullptr
#else
#define WR_NULL ((void*)0)
#endif
#define WR_TRUE (1)
#define WR_FALSE (0)
#define WR_FLAG(x) (1 << (x))

#ifdef __cplusplus
extern "C"
{
#endif

  typedef void*         wr_handle;
  typedef unsigned char wr_byte;
  typedef unsigned int  wr_uint;
  typedef unsigned int  wr_size;
  typedef int           wr_bool;
  typedef const void*   wr_error;
  typedef void*         wr_string_buffer;

  typedef enum
  {
    WR_BACKEND_TYPE_NONE,
    WR_BACKEND_TYPE_GLES,
    WR_BACKEND_TYPE_GL,
    WR_BACKEND_TYPE_VULKAN,
    WR_BACKEND_TYPE_WEBGPU,
    WR_BACKEND_TYPE_CPU,
    WR_BACKEND_TYPE_MAX
  } wr_backend_type;

  typedef enum
  {
    WR_BINDING_TYPE_UNKNOWN,
    WR_BINDING_TYPE_GL_UNIFORM_BLOCK,
    WR_BINDING_TYPE_GL_TEXTURE_2D,
    WR_BINDING_TYPE_GL_TEXTURE_2D_ARRAY,
    WR_BINDING_TYPE_GL_STORAGE_BUFFER,
    WR_BINDING_TYPE_CPU_BUFFER,
    WR_BINDING_TYPE_MAX,
  } wr_binding_type;

  typedef enum
  {
    WR_PRECISION_TYPE_UNKNOWN,
    WR_PRECISION_TYPE_LOW,
    WR_PRECISION_TYPE_MEDIUM, /* RGBA16F */
    WR_PRECISION_TYPE_HIGH,   /* RGBA32F */
    WR_PRECISION_TYPE_MAX
  } wr_precision_type;

  typedef enum
  {
    WR_UPDATE_FLAG_NONE              = 0,
    WR_UPDATE_FLAG_ACCESSOR_BINDINGS = WR_FLAG(0),
    WR_UPDATE_FLAG_ACCESSOR_CODE     = WR_FLAG(1),
    WR_UPDATE_FLAG_INSTANCE_UPDATE   = WR_FLAG(2),
    WR_UPDATE_FLAG_INSTANCE_ADD      = WR_FLAG(3),
    WR_UPDATE_FLAG_MAX
  } wr_update_flags;

  typedef struct
  {
    const void*  buffer;
    unsigned int size;
  } wr_binding_cpu_buffer;

  typedef union
  {
    unsigned int          ubo;
    unsigned int          texture;
    const void*           buffer;
    wr_binding_cpu_buffer cpu_buffer;
  } wr_binding_data;

  typedef struct
  {
    const char*     name;
    wr_binding_type type;
    wr_binding_data data;
  } wr_binding;

  typedef enum
  {
    WR_BUFFER_TYPE_UNKNOWN,
    WR_BUFFER_TYPE_TEXTURE_1D,
    WR_BUFFER_TYPE_TEXTURE_2D,
    WR_BUFFER_TYPE_SSBO,
    WR_BUFFER_TYPE_MAX
  } wr_buffer_type;

  typedef enum
  {
    WR_ADS_TYPE_UNKNOWN,
    WR_ADS_TYPE_BLAS,
    WR_ADS_TYPE_TLAS,
    WR_ADS_TYPE_MAX
  } wr_ads_type;

  typedef struct
  {
    unsigned int target;
    unsigned int internal_format;
    int          width;
    int          height;
  } wr_buffer_texture_2d_data;

  typedef union
  {
    wr_buffer_texture_2d_data as_texture_2d;
  } wr_buffer_data;

  typedef struct
  {
    wr_buffer_type type;
    wr_buffer_data data;
  } wr_buffer_info;

  typedef struct
  {
    const char* key;
    const char* value;
  } wr_ads_descriptor;

  WRAYS_API void
  wrays_version(int* major, int* minor);
  WRAYS_API char const*
  wrays_version_string();

  //
  // wrays_init
  // Create a webrays instance
  //
  // Parameters:
  // - backend_type, specify the backend type
  //
  // Returns a handle to the created instance.
  WRAYS_API wr_handle
            wrays_init(wr_backend_type backend_type, wr_handle);

  //
  // wrays_update
  // Updates and build the ADSs
  //
  // Parameters:
  // - handle, webrays instance handle
  // - flags, flags that specify the update operation that was performed
  //
  // Returns WR_SUCCESS if operation was successful. Otherwise an error string.
  WRAYS_API wr_error
            wrays_update(wr_handle handle, wr_update_flags* flags);

  WRAYS_API wr_error
            wrays_destroy(wr_handle handle);

  //
  // wrays_query_intersection
  // Query the ADS for ray intersections
  //
  // Parameters:
  // - handle, webrays instance handle
  // - ads, ads handle
  // - ray_buffers, a handle to an array that holds ray (position/direction)
  // information
  // - ray_buffer_count, size of the ray_buffers array
  // - intersections, the returned intersections
  // - dimensions, the dimensions of ray_buffers and intersections
  // - dimension_count, size of the dimensions array
  //
  // Returns a handle to the created instance.
  WRAYS_API wr_error
            wrays_query_intersection(wr_handle handle, wr_handle ads,
                                     wr_handle* ray_buffers, wr_size ray_buffer_count,
                                     wr_handle intersections, wr_size* dimensions,
                                     wr_size dimension_count);

  //
  // wrays_query_occlusion
  // Query the ADS for ray occlusions
  //
  // Parameters:
  // - handle, webrays instance handle
  // - ads, ads handle
  // - ray_buffers, a handle to an array that holds ray (position/direction)
  // information
  // - ray_buffer_count, size of the ray_buffers array
  // - occlusion, the returned occlusions
  // - dimensions, the dimensions of ray_buffers and intersections
  // - dimension_count, size of the dimensions array
  //
  // Returns a handle to the created instance.
  WRAYS_API wr_error
            wrays_query_occlusion(wr_handle handle, wr_handle ads, wr_handle* ray_buffers,
                                  wr_size ray_buffer_count, wr_handle occlusion,
                                  wr_size* dimensions, wr_size dimension_count);

  //
  // wrays_ads_create
  // Create an Acceleration Data Structure (ADS) based on the provided options.
  // There are two supported types of ADS:
  // - Bottom Level Acceleration Data Structure (BLAS)
  // - Top Level Acceleration Data Structure (TLAS)
  // If no options are provided, then the generated ADS is of type BLAS.
  //
  // Parameters:
  // - handle, webrays instance handle
  // - ads, the returned ads handle (int)
  // - options, an array of {key, value} strings. Available options are {"type"
  // : "BLAS" || "TLAS" }
  // - options_count, the number of descriptors
  //
  // Returns WR_SUCCESS if operation was successful. Otherwise an error string.
  WRAYS_API wr_error
            wrays_create_ads(wr_handle handle, wr_handle* ads, wr_ads_descriptor* options,
                             int options_count);

  //
  // wrays_add_shape
  // Add a triangular shape to the BLAS
  //
  // Parameters:
  // - handle, webrays instance handle
  // - ads, the id of the BLAS
  // - positions, position buffer (each position is of type float[3])
  // - position_stride, stride of positions on the position buffer
  // - normals, normals buffer (each normal is of type float[3])
  // - normal_stride, stride of normals on the normals buffer
  // - uvs, uv buffer (each uv is of type float[2]). Array can be NULL
  // - uv_stride, stride of uvs on the uvs buffer
  // - num_vertices, number of attributes
  // - indices, faces are always ivec4(v0, v1, v2, materialOffset)
  // - num_triangles, number of triangles
  // - shape_id, tthe returned id of the created shape
  //
  // Returns WR_SUCCESS if operation was successful. Otherwise an error string.
  WRAYS_API wr_error
            wrays_add_shape(wr_handle handle, wr_handle ads, float* positions,
                            int position_stride, float* normals, int normal_stride,
                            float* uvs, int uv_stride, int num_vertices, int* indices,
                            int num_triangles, int* shape_id);

  //
  // wrays_add_instance
  // Creates an instance of the given BLAS
  //
  // Parameters:
  // - handle, webrays instance handle
  // - tlas, the handle of the TLAS that the instance will be attached to
  // - blas, the handle of the BLAS to be instantiated
  // - transformation, a 4x3 transformation matrix (float[12] in column major)
  // - instance_id, the returned id of the created instance
  //
  // Returns WR_SUCCESS if operation was successful. Otherwise an error string.
  WRAYS_API wr_error
            wrays_add_instance(wr_handle handle, wr_handle tlas, wr_handle blas,
                               float* transformation, int* instance_id);

  //
  // wrays_update_instance
  // Update the transformation matrix of the specified instance
  //
  // Parameters:
  // - handle, webrays instance handle
  // - tlas, the handle of the TLAS that the instance belongs to
  // - instance_id, the id of the instance
  // - transformation, a 4x3 transformation matrix (float[12] in column major)
  //
  // Returns WR_SUCCESS if operation was successful. Otherwise an error string.
  WRAYS_API wr_error
            wrays_update_instance(wr_handle handle, wr_handle tlas, int instance_id,
                                  float* transformation);

  //
  // wrays_error_string
  // Get human friendly error message of the provided error
  //
  // Parameters:
  // - handle, webrays instance handle
  // - error,  webrays error code
  //
  // Returns a string representation of the error code
  WRAYS_API const char*
  wrays_error_string(wr_handle handle, wr_error error);

  /* Accessor API */

  //
  // wrays_get_scene_accessor
  // Get accessor code to properly access the data structures
  //
  // Parameters:
  // - handle, webrays instance handle
  //
  // Returns a string representation of the accessor code
  WRAYS_API const char*
  wrays_get_scene_accessor(wr_handle handle);

  //
  // wrays_get_scene_accessor_bindings
  // Get the bindings of the data structures
  //
  // Parameters:
  // - handle, webrays instance handle
  // - count, the number of bindings
  //
  // Returns bindings array
  WRAYS_API const wr_binding*
                  wrays_get_scene_accessor_bindings(wr_handle handle, wr_size* count);

  /* Buffer requirements API */
  WRAYS_API wr_error
            wrays_ray_buffer_requirements(wr_handle handle, wr_buffer_info* buffer_info,
                                          wr_size* dimensions, wr_size dimension_count);
  WRAYS_API wr_error
            wrays_occlusion_buffer_requirements(wr_handle       handle,
                                                wr_buffer_info* buffer_info,
                                                wr_size*        dimensions,
                                                wr_size         dimension_count);
  WRAYS_API wr_error
            wrays_intersection_buffer_requirements(wr_handle       handle,
                                                   wr_buffer_info* buffer_info,
                                                   wr_size*        dimensions,
                                                   wr_size         dimension_count);

  /* Clean Up API */
  WRAYS_API wr_error
            wrays_ray_buffer_destroy(wr_handle handle, wr_handle buffer);
  WRAYS_API wr_error
            wrays_intersection_buffer_destroy(wr_handle handle, wr_handle buffer);
  WRAYS_API wr_error
            wrays_occlusion_buffer_destroy(wr_handle handle, wr_handle buffer);
  WRAYS_API wr_error
            wrays_ads_destroy(wr_handle handle, wr_handle ads);

  WRAYS_API void*
  _wrays_internal_get_bvh_nodes(wr_handle handle, int* node_count);

#if defined(__EMSCRIPTEN__)

  /* Internal API exposed only to help the Javascript version */
  WRAYS_API wr_error
            _wrays_internal_get_intersection_kernel(wr_handle     handle,
                                                    unsigned int* program);

  WRAYS_API wr_error
            _wrays_internal_get_occlusion_kernel(wr_handle handle, unsigned int* program);

#endif

#ifdef __cplusplus
}
#endif

#endif /* _WRAYS_H_ */