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

#ifndef _WRAYS_CONTEXT_H_
#define _WRAYS_CONTEXT_H_

#include "webrays/webrays.h"

#if defined(_DEBUG) && WR_WIN32
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#define WR_FREE(x)                                                             \
  if (WR_NULL != x) {                                                          \
    free((x));                                                                 \
    (x = WR_NULL);                                                             \
  }
#define WR_PTR2INT(p) ((int)(unsigned long long)(p))
#define WR_INT2PTR(i) ((void*)(unsigned long long)(i))

#define WR_TLAS_ID_MASK 0x80000000
#define WR_MAX_TLAS_COUNT 8
#define WR_MAX_BLAS_COUNT 256

typedef enum
{
  WR_BLAS_TYPE_UNKNOWN,
  WR_BLAS_TYPE_SAH,
  WR_BLAS_TYPE_WIDEBVH,
  WR_BLAS_TYPE_MAX
} wr_blas_type;

typedef struct
{
  class ADS*   blas_handles[WR_MAX_BLAS_COUNT];
  int          blas_count;
  wr_blas_type blas_type;

  class TLAS* tlas_handles[WR_MAX_TLAS_COUNT];
  int         tlas_count;

} wr_scene, wr_ads;

typedef struct
{
  wr_scene  scene;
  wr_handle ads[WR_MAX_BLAS_COUNT];

  wr_handle       webgl;
  wr_handle       cpu;
  wr_backend_type backend_type;

  /* Properties */
  int bvh_count;

  unsigned int sphere_count;
  unsigned int triangle_count;

  int             needs_update;
  wr_update_flags update_flags;
} wr_context;

#ifdef WRAYS_WIN32

#define wrays_dlopen(path) LoadLibrary((char*)path)
#define wrays_dlsym(handle, symbol)                                            \
  (void*)GetProcAddress((HMODULE)handle, symbol)
#define wrays_dlclose(handle) (FreeLibrary((HMODULE)handle) ? 0 : -1)
#define WRAYS_GLES_LIBRARY "libGLESv2.dll"

#elif WRAYS_EMSCRIPTEN

#define wrays_dlopen(path) WR_NULL
#define wrays_dlsym(handle, symbol) WR_NULL
#define wrays_dlclose(handle)
#define WRAYS_GLES_LIBRARY ""

#else

#define wrays_dlopen(path) dlopen((char*)path, RTLD_NOW | RTLD_GLOBAL)
#define wrays_dlsym(handle, symbol) dlsym(handle, symbol)
#define wrays_dlclose(handle) dlclose(handle)
#if WRAYS_LINUX
#define WRAYS_GLES_LIBRARY "libGLESv2.so"
#elif WRAYS_APPLE
#define WRAYS_GLES_LIBRARY "libGLESv2.dylib"
#else
#endif /* WRAYS_LINUX */

#endif

/* Internal API */
wr_string_buffer
wr_string_buffer_create(wr_size reserve);
wr_error
wr_string_buffer_destroy(wr_string_buffer*);
wr_error
wr_string_buffer_append(wr_string_buffer string_buffer, const char* data);
wr_error
wr_string_buffer_prepend(wr_string_buffer string_buffer, const char* data);
wr_error
wr_string_buffer_append_range(wr_string_buffer string_buffer, const char* begin,
                              const char* end);
wr_error
wr_string_buffer_appendln(wr_string_buffer string_buffer, const char* data);
wr_error
wr_string_buffer_appendsp(wr_string_buffer string_buffer, const char* data);
wr_error
wr_string_buffer_clear(wr_string_buffer string_buffer);
wr_error
wr_string_buffer_pop_back(wr_string_buffer string_buffer);
char
wr_string_buffer_back(wr_string_buffer string_buffer);
const char*
wr_string_buffer_data(wr_string_buffer string_buffer);
void
wr_string_buffer_pretty_print(wr_string_buffer string_buffer);

#endif /* _WRAYS_CONTEXT_H_ */