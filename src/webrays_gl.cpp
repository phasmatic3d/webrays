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
#include "webrays_gl_shaders.h"
#include "webrays_queue.h"
#include "webrays_math.h"
#include "webrays_ads.h"
#include "webrays_tlas.h"

#include <algorithm>

#include <cstdio>
#include <cstdlib>
#include <cstring>

// For LoadLibrary and such
#ifdef WRAYS_WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#elif defined(WRAYS_LINUX) || defined(WRAYS_APPLE)
#include <dlfcn.h>
#endif

typedef struct
{
  GLuint handle;
  GLuint fbo;
  int    width;
  int    height;
} wr_buffer_2d;

typedef struct
{
  const void* gles_library;

  /* Uniforms */
  wr_binding intersection_bindings[8];

  /* Caches */
  wr_buffer_2d* isect_buffers_2d;
  wr_buffer_2d* occlusion_buffers_2d;

  wr_string_buffer scene_accessor_shader;
  wr_string_buffer shader_scratch;

  GLuint intersection_program;
  GLuint occlusion_program;

  GLuint screen_fill_vao;
  GLuint screen_fill_vbo;

  /* Intersection Buffer */
  GLuint intersections_FBO;
  GLuint intersections_texture_data;

  GLuint ubo_spheres;
  GLuint block_index_spheres;
  GLuint block_binding_spheres;

  GLuint instances_UBO;
  GLuint instances_block_index;
  GLuint instances_block_binding;

  GLuint tlas_texture;

  GLuint bounds_texture; // {min, max}
  GLuint scene_texture;
  GLuint indices_texture;

  GLint indices_texture_size;
  GLint scene_texture_size;
  GLint bounds_texture_size;

  wr_size binding_count;

  GLuint gpu_timers[5];
} wr_gl_context;

WR_INTERNAL wr_error
            wrays_gl_shader_create(GLuint* shader_handle, const char* shader_source,
                                   GLenum shader_type)
{
  *shader_handle = -1;
  GLint compiled = GL_FALSE;

  *shader_handle = glCreateShader(shader_type);
  glShaderSource(*shader_handle, 1, &shader_source, 0);
  glCompileShader(*shader_handle);

  glGetShaderiv(*shader_handle, GL_COMPILE_STATUS, &compiled);
  if (compiled == GL_FALSE) {
    GLint max_length = 0;
    glGetShaderiv(*shader_handle, GL_INFO_LOG_LENGTH, &max_length);

    // The max_length includes the NULL character
    char* log       = (char*)malloc(max_length + 1);
    log[max_length] = 0;
    glGetShaderInfoLog(*shader_handle, max_length, &max_length, &log[0]);
    printf("Shader Log %s\n", log);

    free(log);
  }

  return WR_SUCCESS;
}

WR_INTERNAL wr_error
            wrays_gl_program_create(GLuint* program_handle, GLuint vertex_shader,
                                    GLuint fragment_shader)
{
  *program_handle = glCreateProgram();
  glAttachShader(*program_handle, vertex_shader);
  glAttachShader(*program_handle, fragment_shader);
  glLinkProgram(*program_handle);
  GLint linked = GL_FALSE;

  glDetachShader(*program_handle, vertex_shader);
  glDetachShader(*program_handle, fragment_shader);

  // Check the program
  glGetProgramiv(*program_handle, GL_LINK_STATUS, &linked);
  if (linked == GL_FALSE) {
    GLint max_length;
    glGetProgramiv(*program_handle, GL_INFO_LOG_LENGTH, &max_length);

    char* log       = (char*)malloc(max_length + 1);
    log[max_length] = 0;

    glGetProgramInfoLog(*program_handle, max_length, NULL, log);
    printf("Program Log %s\n", log);
    free(log);

    glDeleteProgram(*program_handle);
    *program_handle = 0;
  }

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  return WR_SUCCESS;
}

wr_error
wrays_gl_ads_create(wr_handle handle, wr_handle ads_id,
                    wr_ads_descriptor* options, int options_count)
{
  wr_context*    webrays       = (wr_context*)handle;
  wr_gl_context* webrays_webgl = (wr_gl_context*)webrays->webgl;

  return WR_SUCCESS;
}

wr_error
wrays_gl_add_shape(wr_handle handle, wr_handle ads, float* positions,
                   int position_stride, float* normals, int normal_stride,
                   float* uvs, int uv_stride, int attr_count, int* faces,
                   int num_triangles, int* shape_id)
{

  wr_context*    webrays       = (wr_context*)handle;
  wr_gl_context* webrays_webgl = (wr_gl_context*)webrays->webgl;
  int            ads_id        = (int)(size_t)(ads);

  ADS* ads_impl = webrays->scene.blas_handles[ads_id];

  *shape_id = ads_impl->AddTriangularMesh(positions, position_stride, normals,
                                          normal_stride, uvs, uv_stride,
                                          attr_count, faces, num_triangles);

  return WR_SUCCESS;
}

wr_error
wrays_gl_init(wr_handle handle)
{
  wr_context* webrays = (wr_context*)handle;
  if (WR_NULL == webrays)
    return (wr_error) "Invalid WebRays context";
  wr_gl_context* webrays_webgl = (wr_gl_context*)malloc(sizeof(*webrays_webgl));
  if (WR_NULL == webrays_webgl)
    return (wr_error) "Failed to allocate WebGL WebRays context";

  void* gles_library = WR_NULL;
#ifndef WRAYS_EMSCRIPTEN
  gles_library = wrays_dlopen(WRAYS_GLES_LIBRARY);
  if (WR_NULL == gles_library)
    return (wr_error) "Failed to load " WRAYS_GLES_LIBRARY;

  wrCopyTexSubImage3D = (PFNGLCOPYTEXSUBIMAGE3DPROC)wrays_dlsym(
    gles_library, "glCopyTexSubImage3D");
  wrDrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC)wrays_dlsym(
    gles_library, "wrDrawRangeElements");
  wrTexImage3D = (PFNGLTEXIMAGE3DPROC)wrays_dlsym(gles_library, "glTexImage3D");
  wrTexSubImage3D =
    (PFNGLTEXSUBIMAGE3DPROC)wrays_dlsym(gles_library, "glTexSubImage3D");
  wrActiveTexture =
    (PFNGLACTIVETEXTUREPROC)wrays_dlsym(gles_library, "glActiveTexture");
  wrCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)wrays_dlsym(
    gles_library, "glCompressedTexImage2D");
  wrCompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3DPROC)wrays_dlsym(
    gles_library, "glCompressedTexImage3D");
  wrCompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)wrays_dlsym(
    gles_library, "glCompressedTexSubImage2D");
  wrCompressedTexSubImage3D = (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)wrays_dlsym(
    gles_library, "glCompressedTexSubImage3D");
  wrSampleCoverage =
    (PFNGLSAMPLECOVERAGEPROC)wrays_dlsym(gles_library, "glSampleCoverage");
  wrBlendColor = (PFNGLBLENDCOLORPROC)wrays_dlsym(gles_library, "glBlendColor");
  wrBlendEquation =
    (PFNGLBLENDEQUATIONPROC)wrays_dlsym(gles_library, "glBlendEquation");
  wrBlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC)wrays_dlsym(
    gles_library, "glBlendFuncSeparate");
  wrBeginQuery = (PFNGLBEGINQUERYPROC)wrays_dlsym(gles_library, "glBeginQuery");
  wrBindBuffer = (PFNGLBINDBUFFERPROC)wrays_dlsym(gles_library, "glBindBuffer");
  wrBufferData = (PFNGLBUFFERDATAPROC)wrays_dlsym(gles_library, "glBufferData");
  wrBufferSubData =
    (PFNGLBUFFERSUBDATAPROC)wrays_dlsym(gles_library, "glBufferSubData");
  wrDeleteBuffers =
    (PFNGLDELETEBUFFERSPROC)wrays_dlsym(gles_library, "glDeleteBuffers");
  wrDeleteQueries =
    (PFNGLDELETEQUERIESPROC)wrays_dlsym(gles_library, "glDeleteQueries");
  wrEndQuery   = (PFNGLENDQUERYPROC)wrays_dlsym(gles_library, "glEndQuery");
  wrGenBuffers = (PFNGLGENBUFFERSPROC)wrays_dlsym(gles_library, "glGenBuffers");
  wrGenQueries = (PFNGLGENQUERIESPROC)wrays_dlsym(gles_library, "glGenQueries");
  wrGetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC)wrays_dlsym(
    gles_library, "glGetBufferParameteriv");
  wrGetBufferPointerv = (PFNGLGETBUFFERPOINTERVPROC)wrays_dlsym(
    gles_library, "glGetBufferPointerv");
  wrGetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC)wrays_dlsym(
    gles_library, "glGetQueryObjectuiv");
  wrGetQueryiv = (PFNGLGETQUERYIVPROC)wrays_dlsym(gles_library, "glGetQueryiv");
  wrIsBuffer   = (PFNGLISBUFFERPROC)wrays_dlsym(gles_library, "glIsBuffer");
  wrIsQuery    = (PFNGLISQUERYPROC)wrays_dlsym(gles_library, "glIsQuery");
  wrUnmapBuffer =
    (PFNGLUNMAPBUFFERPROC)wrays_dlsym(gles_library, "glUnmapBuffer");
  wrAttachShader =
    (PFNGLATTACHSHADERPROC)wrays_dlsym(gles_library, "glAttachShader");
  wrBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)wrays_dlsym(
    gles_library, "glBindAttribLocation");
  wrBlendEquationSeparate = (PFNGLBLENDEQUATIONSEPARATEPROC)wrays_dlsym(
    gles_library, "glBlendEquationSeparate");
  wrCompileShader =
    (PFNGLCOMPILESHADERPROC)wrays_dlsym(gles_library, "glCompileShader");
  wrCreateProgram =
    (PFNGLCREATEPROGRAMPROC)wrays_dlsym(gles_library, "glCreateProgram");
  wrCreateShader =
    (PFNGLCREATESHADERPROC)wrays_dlsym(gles_library, "glCreateShader");
  wrDeleteProgram =
    (PFNGLDELETEPROGRAMPROC)wrays_dlsym(gles_library, "glDeleteProgram");
  wrDeleteShader =
    (PFNGLDELETESHADERPROC)wrays_dlsym(gles_library, "glDeleteShader");
  wrDetachShader =
    (PFNGLDETACHSHADERPROC)wrays_dlsym(gles_library, "glDetachShader");
  wrDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)wrays_dlsym(
    gles_library, "glDisableVertexAttribArray");
  wrDrawBuffers =
    (PFNGLDRAWBUFFERSPROC)wrays_dlsym(gles_library, "glDrawBuffers");
  wrEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)wrays_dlsym(
    gles_library, "glEnableVertexAttribArray");
  wrGetActiveAttrib =
    (PFNGLGETACTIVEATTRIBPROC)wrays_dlsym(gles_library, "glGetActiveAttrib");
  wrGetActiveUniform =
    (PFNGLGETACTIVEUNIFORMPROC)wrays_dlsym(gles_library, "glGetActiveUniform");
  wrGetAttachedShaders = (PFNGLGETATTACHEDSHADERSPROC)wrays_dlsym(
    gles_library, "glGetAttachedShaders");
  wrGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)wrays_dlsym(
    gles_library, "glGetAttribLocation");
  wrGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wrays_dlsym(
    gles_library, "glGetProgramInfoLog");
  wrGetProgramiv =
    (PFNGLGETPROGRAMIVPROC)wrays_dlsym(gles_library, "glGetProgramiv");
  wrGetShaderInfoLog =
    (PFNGLGETSHADERINFOLOGPROC)wrays_dlsym(gles_library, "glGetShaderInfoLog");
  wrGetShaderSource =
    (PFNGLGETSHADERSOURCEPROC)wrays_dlsym(gles_library, "glGetShaderSource");
  wrGetShaderiv =
    (PFNGLGETSHADERIVPROC)wrays_dlsym(gles_library, "glGetShaderiv");
  wrGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wrays_dlsym(
    gles_library, "glGetUniformLocation");
  wrGetUniformfv =
    (PFNGLGETUNIFORMFVPROC)wrays_dlsym(gles_library, "glGetUniformfv");
  wrGetUniformiv =
    (PFNGLGETUNIFORMIVPROC)wrays_dlsym(gles_library, "glGetUniformiv");
  wrGetVertexAttribPointerv = (PFNGLGETVERTEXATTRIBPOINTERVPROC)wrays_dlsym(
    gles_library, "glGetVertexAttribPointerv");
  wrGetVertexAttribfv = (PFNGLGETVERTEXATTRIBFVPROC)wrays_dlsym(
    gles_library, "glGetVertexAttribfv");
  wrGetVertexAttribiv = (PFNGLGETVERTEXATTRIBIVPROC)wrays_dlsym(
    gles_library, "glGetVertexAttribiv");
  wrIsProgram = (PFNGLISPROGRAMPROC)wrays_dlsym(gles_library, "glIsProgram");
  wrIsShader  = (PFNGLISSHADERPROC)wrays_dlsym(gles_library, "glIsShader");
  wrLinkProgram =
    (PFNGLLINKPROGRAMPROC)wrays_dlsym(gles_library, "glLinkProgram");
  wrShaderSource =
    (PFNGLSHADERSOURCEPROC)wrays_dlsym(gles_library, "glShaderSource");
  wrStencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATEPROC)wrays_dlsym(
    gles_library, "glStencilFuncSeparate");
  wrStencilMaskSeparate = (PFNGLSTENCILMASKSEPARATEPROC)wrays_dlsym(
    gles_library, "glStencilMaskSeparate");
  wrStencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC)wrays_dlsym(
    gles_library, "glStencilOpSeparate");
  wrUniform1f  = (PFNGLUNIFORM1FPROC)wrays_dlsym(gles_library, "glUniform1f");
  wrUniform1fv = (PFNGLUNIFORM1FVPROC)wrays_dlsym(gles_library, "glUniform1fv");
  wrUniform1i  = (PFNGLUNIFORM1IPROC)wrays_dlsym(gles_library, "glUniform1i");
  wrUniform1iv = (PFNGLUNIFORM1IVPROC)wrays_dlsym(gles_library, "glUniform1iv");
  wrUniform2f  = (PFNGLUNIFORM2FPROC)wrays_dlsym(gles_library, "glUniform2f");
  wrUniform2fv = (PFNGLUNIFORM2FVPROC)wrays_dlsym(gles_library, "glUniform2fv");
  wrUniform2i  = (PFNGLUNIFORM2IPROC)wrays_dlsym(gles_library, "glUniform2i");
  wrUniform2iv = (PFNGLUNIFORM2IVPROC)wrays_dlsym(gles_library, "glUniform2iv");
  wrUniform3f  = (PFNGLUNIFORM3FPROC)wrays_dlsym(gles_library, "glUniform3f");
  wrUniform3fv = (PFNGLUNIFORM3FVPROC)wrays_dlsym(gles_library, "glUniform3fv");
  wrUniform3i  = (PFNGLUNIFORM3IPROC)wrays_dlsym(gles_library, "glUniform3i");
  wrUniform3iv = (PFNGLUNIFORM3IVPROC)wrays_dlsym(gles_library, "glUniform3iv");
  wrUniform4f  = (PFNGLUNIFORM4FPROC)wrays_dlsym(gles_library, "glUniform4f");
  wrUniform4fv = (PFNGLUNIFORM4FVPROC)wrays_dlsym(gles_library, "glUniform4fv");
  wrUniform4i  = (PFNGLUNIFORM4IPROC)wrays_dlsym(gles_library, "glUniform4i");
  wrUniform4iv = (PFNGLUNIFORM4IVPROC)wrays_dlsym(gles_library, "glUniform4iv");
  wrUniformMatrix2fv =
    (PFNGLUNIFORMMATRIX2FVPROC)wrays_dlsym(gles_library, "glUniformMatrix2fv");
  wrUniformMatrix3fv =
    (PFNGLUNIFORMMATRIX3FVPROC)wrays_dlsym(gles_library, "glUniformMatrix3fv");
  wrUniformMatrix4fv =
    (PFNGLUNIFORMMATRIX4FVPROC)wrays_dlsym(gles_library, "glUniformMatrix4fv");
  wrUseProgram = (PFNGLUSEPROGRAMPROC)wrays_dlsym(gles_library, "glUseProgram");
  wrValidateProgram =
    (PFNGLVALIDATEPROGRAMPROC)wrays_dlsym(gles_library, "glValidateProgram");
  wrVertexAttrib1f =
    (PFNGLVERTEXATTRIB1FPROC)wrays_dlsym(gles_library, "glVertexAttrib1f");
  wrVertexAttrib1fv =
    (PFNGLVERTEXATTRIB1FVPROC)wrays_dlsym(gles_library, "glVertexAttrib1fv");
  wrVertexAttrib3f =
    (PFNGLVERTEXATTRIB3FPROC)wrays_dlsym(gles_library, "glVertexAttrib3f");
  wrVertexAttrib3fv =
    (PFNGLVERTEXATTRIB3FVPROC)wrays_dlsym(gles_library, "glVertexAttrib3fv");
  wrVertexAttrib4f =
    (PFNGLVERTEXATTRIB4FPROC)wrays_dlsym(gles_library, "glVertexAttrib4f");
  wrVertexAttrib4fv =
    (PFNGLVERTEXATTRIB4FVPROC)wrays_dlsym(gles_library, "glVertexAttrib4fv");
  wrVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)wrays_dlsym(
    gles_library, "glVertexAttribPointer");
  wrUniformMatrix2x3fv = (PFNGLUNIFORMMATRIX2X3FVPROC)wrays_dlsym(
    gles_library, "glUniformMatrix2x3fv");
  wrUniformMatrix2x4fv = (PFNGLUNIFORMMATRIX2X4FVPROC)wrays_dlsym(
    gles_library, "glUniformMatrix2x4fv");
  wrUniformMatrix3x2fv = (PFNGLUNIFORMMATRIX3X2FVPROC)wrays_dlsym(
    gles_library, "glUniformMatrix3x2fv");
  wrUniformMatrix3x4fv = (PFNGLUNIFORMMATRIX3X4FVPROC)wrays_dlsym(
    gles_library, "glUniformMatrix3x4fv");
  wrUniformMatrix4x2fv = (PFNGLUNIFORMMATRIX4X2FVPROC)wrays_dlsym(
    gles_library, "glUniformMatrix4x2fv");
  wrUniformMatrix4x3fv = (PFNGLUNIFORMMATRIX4X3FVPROC)wrays_dlsym(
    gles_library, "glUniformMatrix4x3fv");
  wrBeginTransformFeedback = (PFNGLBEGINTRANSFORMFEEDBACKPROC)wrays_dlsym(
    gles_library, "glBeginTransformFeedback");
  wrClearBufferfi =
    (PFNGLCLEARBUFFERFIPROC)wrays_dlsym(gles_library, "glClearBufferfi");
  wrClearBufferfv =
    (PFNGLCLEARBUFFERFVPROC)wrays_dlsym(gles_library, "glClearBufferfv");
  wrClearBufferiv =
    (PFNGLCLEARBUFFERIVPROC)wrays_dlsym(gles_library, "glClearBufferiv");
  wrClearBufferuiv =
    (PFNGLCLEARBUFFERUIVPROC)wrays_dlsym(gles_library, "glClearBufferuiv");
  wrEndTransformFeedback = (PFNGLENDTRANSFORMFEEDBACKPROC)wrays_dlsym(
    gles_library, "glEndTransformFeedback");
  wrGetFragDataLocation = (PFNGLGETFRAGDATALOCATIONPROC)wrays_dlsym(
    gles_library, "glGetFragDataLocation");
  wrGetStringi = (PFNGLGETSTRINGIPROC)wrays_dlsym(gles_library, "glGetStringi");
  wrGetTransformFeedbackVarying =
    (PFNGLGETTRANSFORMFEEDBACKVARYINGPROC)wrays_dlsym(
      gles_library, "glGetTransformFeedbackVarying");
  wrGetUniformuiv =
    (PFNGLGETUNIFORMUIVPROC)wrays_dlsym(gles_library, "glGetUniformuiv");
  wrGetVertexAttribIiv = (PFNGLGETVERTEXATTRIBIIVPROC)wrays_dlsym(
    gles_library, "glGetVertexAttribIiv");
  wrGetVertexAttribIuiv = (PFNGLGETVERTEXATTRIBIUIVPROC)wrays_dlsym(
    gles_library, "glGetVertexAttribIuiv");
  wrTransformFeedbackVaryings = (PFNGLTRANSFORMFEEDBACKVARYINGSPROC)wrays_dlsym(
    gles_library, "glTransformFeedbackVaryings");
  wrUniform1ui = (PFNGLUNIFORM1UIPROC)wrays_dlsym(gles_library, "glUniform1ui");
  wrUniform1uiv =
    (PFNGLUNIFORM1UIVPROC)wrays_dlsym(gles_library, "glUniform1uiv");
  wrUniform2ui = (PFNGLUNIFORM2UIPROC)wrays_dlsym(gles_library, "glUniform2ui");
  wrUniform2uiv =
    (PFNGLUNIFORM2UIVPROC)wrays_dlsym(gles_library, "glUniform2uiv");
  wrUniform3ui = (PFNGLUNIFORM3UIPROC)wrays_dlsym(gles_library, "glUniform3ui");
  wrUniform3uiv =
    (PFNGLUNIFORM3UIVPROC)wrays_dlsym(gles_library, "glUniform3uiv");
  wrUniform4ui = (PFNGLUNIFORM4UIPROC)wrays_dlsym(gles_library, "glUniform4ui");
  wrUniform4uiv =
    (PFNGLUNIFORM4UIVPROC)wrays_dlsym(gles_library, "glUniform4uiv");
  wrVertexAttribI4i =
    (PFNGLVERTEXATTRIBI4IPROC)wrays_dlsym(gles_library, "glVertexAttribI4i");
  wrVertexAttribI4iv =
    (PFNGLVERTEXATTRIBI4IVPROC)wrays_dlsym(gles_library, "glVertexAttribI4iv");
  wrVertexAttribI4ui =
    (PFNGLVERTEXATTRIBI4UIPROC)wrays_dlsym(gles_library, "glVertexAttribI4ui");
  wrVertexAttribI4uiv = (PFNGLVERTEXATTRIBI4UIVPROC)wrays_dlsym(
    gles_library, "glVertexAttribI4uiv");
  wrVertexAttribIPointer = (PFNGLVERTEXATTRIBIPOINTERPROC)wrays_dlsym(
    gles_library, "glVertexAttribIPointer");
  wrDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC)wrays_dlsym(
    gles_library, "glDrawArraysInstanced");
  wrDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDPROC)wrays_dlsym(
    gles_library, "glDrawElementsInstanced");
  wrGetBufferParameteri64v = (PFNGLGETBUFFERPARAMETERI64VPROC)wrays_dlsym(
    gles_library, "glGetBufferParameteri64v");
  wrGetInteger64i_v =
    (PFNGLGETINTEGER64I_VPROC)wrays_dlsym(gles_library, "glGetInteger64i_v");
  wrVertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISORPROC)wrays_dlsym(
    gles_library, "glVertexAttribDivisor");
  wrClearDepthf =
    (PFNGLCLEARDEPTHFPROC)wrays_dlsym(gles_library, "glClearDepthf");
  wrDepthRangef =
    (PFNGLDEPTHRANGEFPROC)wrays_dlsym(gles_library, "glDepthRangef");
  wrGetShaderPrecisionFormat = (PFNGLGETSHADERPRECISIONFORMATPROC)wrays_dlsym(
    gles_library, "glGetShaderPrecisionFormat");
  wrReleaseShaderCompiler = (PFNGLRELEASESHADERCOMPILERPROC)wrays_dlsym(
    gles_library, "glReleaseShaderCompiler");
  wrShaderBinary =
    (PFNGLSHADERBINARYPROC)wrays_dlsym(gles_library, "glShaderBinary");
  wrCopyBufferSubData = (PFNGLCOPYBUFFERSUBDATAPROC)wrays_dlsym(
    gles_library, "glCopyBufferSubData");
  wrViewport = (PFNGLVIEWPORTPROC)wrays_dlsym(gles_library, "glViewport");
  wrTexStorage3D =
    (PFNGLTEXSTORAGE3DPROC)wrays_dlsym(gles_library, "glTexStorage3D");
  wrTexParameteri =
    (PFNGLTEXPARAMETERIPROC)wrays_dlsym(gles_library, "glTexParameteri");
  wrGetIntegerv =
    (PFNGLGETINTEGERVPROC)wrays_dlsym(gles_library, "glGetIntegerv");
  wrGenVertexArrays =
    (PFNGLGENVERTEXARRAYSPROC)wrays_dlsym(gles_library, "glGenVertexArrays");
  wrIsTexture = (PFNGLISTEXTUREPROC)wrays_dlsym(gles_library, "glIsTexture");
  wrGenTextures =
    (PFNGLGENTEXTURESPROC)wrays_dlsym(gles_library, "glGenTextures");
  wrDeleteTextures =
    (PFNGLDELETETEXTURESPROC)wrays_dlsym(gles_library, "glDeleteTextures");
  wrBindTexture =
    (PFNGLBINDTEXTUREPROC)wrays_dlsym(gles_library, "glBindTexture");
  wrDrawArrays = (PFNGLDRAWARRAYSPROC)wrays_dlsym(gles_library, "glDrawArrays");
  wrBindVertexArray =
    (PFNGLBINDVERTEXARRAYPROC)wrays_dlsym(gles_library, "glBindVertexArray");
  wrGetError = (PFNGLGETERRORPROC)wrays_dlsym(gles_library, "glGetError");
  wrBindFramebuffer =
    (PFNGLBINDFRAMEBUFFERPROC)wrays_dlsym(gles_library, "glBindFramebuffer");
  wrBindRenderbuffer =
    (PFNGLBINDRENDERBUFFERPROC)wrays_dlsym(gles_library, "glBindRenderbuffer");
  wrBlitFramebuffer =
    (PFNGLBLITFRAMEBUFFERPROC)wrays_dlsym(gles_library, "glBlitFramebuffer");
  wrCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)wrays_dlsym(
    gles_library, "glCheckFramebufferStatus");
  wrDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)wrays_dlsym(
    gles_library, "glDeleteFramebuffers");
  wrDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)wrays_dlsym(
    gles_library, "glDeleteRenderbuffers");
  wrFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)wrays_dlsym(
    gles_library, "glFramebufferRenderbuffer");
  wrFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)wrays_dlsym(
    gles_library, "glFramebufferTexture2D");
  wrFramebufferTextureLayer = (PFNGLFRAMEBUFFERTEXTURELAYERPROC)wrays_dlsym(
    gles_library, "glFramebufferTextureLayer");
  wrGenFramebuffers =
    (PFNGLGENFRAMEBUFFERSPROC)wrays_dlsym(gles_library, "glGenFramebuffers");
  wrGenRenderbuffers =
    (PFNGLGENRENDERBUFFERSPROC)wrays_dlsym(gles_library, "glGenRenderbuffers");
  wrGenerateMipmap =
    (PFNGLGENERATEMIPMAPPROC)wrays_dlsym(gles_library, "glGenerateMipmap");
  wrGetFramebufferAttachmentParameteriv =
    (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)wrays_dlsym(
      gles_library, "glGetFramebufferAttachmentParameteriv");
  wrGetRenderbufferParameteriv =
    (PFNGLGETRENDERBUFFERPARAMETERIVPROC)wrays_dlsym(
      gles_library, "glGetRenderbufferParameteriv");
  wrIsFramebuffer =
    (PFNGLISFRAMEBUFFERPROC)wrays_dlsym(gles_library, "glIsFramebuffer");
  wrIsRenderbuffer =
    (PFNGLISRENDERBUFFERPROC)wrays_dlsym(gles_library, "glIsRenderbuffer");
  wrRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)wrays_dlsym(
    gles_library, "glRenderbufferStorage");
  wrRenderbufferStorageMultisample =
    (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)wrays_dlsym(
      gles_library, "glRenderbufferStorageMultisample");
#endif /* WRAYS_EMSCRIPTEN */

  memset(webrays_webgl, 0, sizeof(*webrays_webgl));

  wrays_queue_init(webrays_webgl->isect_buffers_2d, 1);
  wrays_queue_init(webrays_webgl->occlusion_buffers_2d, 1);

  webrays_webgl->gles_library = gles_library;
  webrays->webgl              = webrays_webgl;

  webrays_webgl->shader_scratch        = wr_string_buffer_create(1024);
  webrays_webgl->scene_accessor_shader = wr_string_buffer_create(1024);

  WR_GL_CHECK(glGenVertexArrays(1, &webrays_webgl->screen_fill_vao));
  glBindVertexArray(webrays_webgl->screen_fill_vao);
  float screen_fill_triangle[] = { -4.0f, -4.0f, 0.0f, 4.0f, -4.0f,
                                   0.0f,  0.0f,  4.0f, 0.0f };

  size_t size = sizeof(screen_fill_triangle);
  glGenBuffers(1, &webrays_webgl->screen_fill_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, webrays_webgl->screen_fill_vbo);
  glBufferData(GL_ARRAY_BUFFER, size, &screen_fill_triangle[0], GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, (GLvoid*)0);

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // GLint max_color_attachments = 0;
  // GLint max_draw_buffers = 0;
  // GLint max_texture_size = 0;
  // GLint max_texture_units = 0;
  // GLint max_combined_texture_units = 0;
  // glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments);
  // glGetIntegerv(GL_MAX_DRAW_BUFFERS, &max_draw_buffers);
  // glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
  // glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_texture_units);
  // glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,
  // &max_combined_texture_units);

  // printf("EXT %s\n", glGetString(GL_EXTENSIONS));

  // printf("GL_MAX_COLOR_ATTACHMENTS <-> %d\n", max_color_attachments);
  // printf("GL_MAX_DRAW_BUFFERS <-> %d\n", max_draw_buffers);
  // printf("GL_MAX_TEXTURE_SIZE <-> %d\n", max_texture_size);
  // printf("GL_MAX_TEXTURE_IMAGE_UNITS <-> %d\n", max_texture_units);
  // printf("GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS <-> %d\n",
  // max_combined_texture_units);

  // glGenQueries(5, webrays_webgl->gpu_timers);

  return WR_SUCCESS;
}

WR_INTERNAL wr_error
            wrays_gl_query_occlusion_2d(wr_handle handle, wr_handle ads,
                                        wr_handle* ray_buffers, wr_size ray_buffer_count,
                                        wr_handle occlusion, wr_size width, wr_size height)
{
  wr_context*    webrays       = (wr_context*)handle;
  wr_gl_context* webrays_webgl = (wr_gl_context*)webrays->webgl;
  GLuint         gl_occlusion  = (GLuint)(size_t)occlusion;

  /* This might be slow but it is robust and safe */
  GLuint  fbo          = 0;
  wr_size buffer_count = wrays_queue_size(webrays_webgl->occlusion_buffers_2d);
  wr_size buffer_index = 0;

  for (buffer_index = 0; buffer_index < buffer_count; ++buffer_index) {
    if (webrays_webgl->occlusion_buffers_2d[buffer_index].handle ==
        gl_occlusion) {
      fbo = webrays_webgl->occlusion_buffers_2d[buffer_index].fbo;
    }
  }

  if ((0 == fbo) && glIsTexture(gl_occlusion)) {
    glGenFramebuffers(1, &fbo);

    WR_GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fbo));
    WR_GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 0,
                                       GL_TEXTURE_2D, gl_occlusion, 0));

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
      return (wr_error) "Invalid occlusion buffer";
    }

    wr_buffer_2d texture_2d = { gl_occlusion, fbo, static_cast<int>(width),
                                static_cast<int>(height) };
    wrays_queue_push(webrays_webgl->occlusion_buffers_2d, texture_2d);
  }

  glBindVertexArray(webrays_webgl->screen_fill_vao);
  glViewport(0, 0, width, height);

  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 + 0 };
  glDrawBuffers(sizeof(drawBuffers) / sizeof(drawBuffers[0]), drawBuffers);

  glUseProgram(webrays_webgl->occlusion_program);

  GLuint index =
    glGetUniformLocation(webrays_webgl->occlusion_program, "wr_RayOrigins");
  glActiveTexture(GL_TEXTURE0 + 0);
  glBindTexture(GL_TEXTURE_2D, (GLuint)(size_t)ray_buffers[0]);
  glUniform1i(index, 0);
  index =
    glGetUniformLocation(webrays_webgl->occlusion_program, "wr_RayDirections");
  glActiveTexture(GL_TEXTURE0 + 1);
  glBindTexture(GL_TEXTURE_2D, (GLuint)(size_t)ray_buffers[1]);
  glUniform1i(index, 1);

  index = glGetUniformLocation(webrays_webgl->occlusion_program, "wr_ADS");
  glUniform1i(index, (int)(size_t)ads);

  int current_texture_unit = 1;
  for (wr_size i = 0; i < webrays_webgl->binding_count; ++i) {
    index = glGetUniformLocation(webrays_webgl->occlusion_program,
                                 webrays_webgl->intersection_bindings[i].name);
    if (webrays_webgl->intersection_bindings[i].type ==
        wr_binding_type::WR_BINDING_TYPE_GL_TEXTURE_2D_ARRAY) {
      ++current_texture_unit;
      glActiveTexture(GL_TEXTURE0 + current_texture_unit);
      glBindTexture(GL_TEXTURE_2D,
                    webrays_webgl->intersection_bindings[i].data.texture);
      glUniform1i(index, current_texture_unit);
    } else if (webrays_webgl->intersection_bindings[i].type ==
               wr_binding_type::WR_BINDING_TYPE_GL_TEXTURE_2D_ARRAY) {
      ++current_texture_unit;
      glActiveTexture(GL_TEXTURE0 + current_texture_unit);
      glBindTexture(GL_TEXTURE_2D_ARRAY,
                    webrays_webgl->intersection_bindings[i].data.texture);
      glUniform1i(index, current_texture_unit);
    }
  }

  WR_GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 3));

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  return WR_SUCCESS;
}

wr_error
wrays_gl_query_occlusion(wr_handle handle, wr_handle ads,
                         wr_handle* ray_buffers, wr_size ray_buffer_count,
                         wr_handle occlusion, wr_size* dimensions,
                         wr_size dimension_count)
{
  wr_context*    webrays       = (wr_context*)handle;
  wr_gl_context* webrays_webgl = (wr_gl_context*)webrays->webgl;

  switch (dimension_count) {
    case 1: break;
    case 2:
      return wrays_gl_query_occlusion_2d(handle, ads, ray_buffers,
                                         ray_buffer_count, occlusion,
                                         dimensions[0], dimensions[1]);
    default: break;
  }

  return WR_SUCCESS;
}

WR_INTERNAL wr_error
            wrays_gl_query_intersection_2d(wr_handle handle, wr_handle ads,
                                           wr_handle* ray_buffers, wr_size ray_buffer_count,
                                           wr_handle intersections, wr_size width,
                                           wr_size height)
{
  wr_context*    webrays          = (wr_context*)handle;
  wr_gl_context* webrays_webgl    = (wr_gl_context*)webrays->webgl;
  GLuint         gl_intersections = (GLuint)(size_t)intersections;

  /* This might be slow but it is robust and safe */
  GLuint  fbo          = 0;
  wr_size buffer_count = wrays_queue_size(webrays_webgl->isect_buffers_2d);
  wr_size buffer_index = 0;

  for (buffer_index = 0; buffer_index < buffer_count; ++buffer_index) {
    if (webrays_webgl->isect_buffers_2d[buffer_index].handle ==
        gl_intersections) {
      /* Check dims maybe */
      fbo = webrays_webgl->isect_buffers_2d[buffer_index].fbo;
    }
  }

  if ((0 == fbo) && glIsTexture(gl_intersections)) {
    glGenFramebuffers(1, &fbo);

    WR_GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fbo));
    WR_GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 0,
                                       GL_TEXTURE_2D, gl_intersections, 0));

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
      return (wr_error) "Invalid intersection buffer";
    }

    wr_buffer_2d texture_2d = { gl_intersections, fbo, static_cast<int>(width),
                                static_cast<int>(height) };
    wrays_queue_push(webrays_webgl->isect_buffers_2d, texture_2d);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glBindVertexArray(webrays_webgl->screen_fill_vao);
  glViewport(0, 0, width, height);

  GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 + 0 };
  glDrawBuffers(sizeof(drawBuffers) / sizeof(drawBuffers[0]), drawBuffers);

  glUseProgram(webrays_webgl->intersection_program);

  GLuint index =
    glGetUniformLocation(webrays_webgl->intersection_program, "wr_RayOrigins");
  glActiveTexture(GL_TEXTURE0 + 0);
  glBindTexture(GL_TEXTURE_2D, (GLuint)(size_t)ray_buffers[0]);
  glUniform1i(index, 0);
  index = glGetUniformLocation(webrays_webgl->intersection_program,
                               "wr_RayDirections");
  glActiveTexture(GL_TEXTURE0 + 1);
  glBindTexture(GL_TEXTURE_2D, (GLuint)(size_t)ray_buffers[1]);
  glUniform1i(index, 1);

  index = glGetUniformLocation(webrays_webgl->intersection_program, "wr_ADS");
  glUniform1i(index, (int)(size_t)ads);

  int current_texture_unit = 1;
  for (wr_size i = 0; i < webrays_webgl->binding_count; ++i) {
    index = glGetUniformLocation(webrays_webgl->intersection_program,
                                 webrays_webgl->intersection_bindings[i].name);
    if (webrays_webgl->intersection_bindings[i].type ==
        wr_binding_type::WR_BINDING_TYPE_GL_TEXTURE_2D) {
      ++current_texture_unit;
      glActiveTexture(GL_TEXTURE0 + current_texture_unit);
      glBindTexture(GL_TEXTURE_2D,
                    webrays_webgl->intersection_bindings[i].data.texture);
      glUniform1i(index, current_texture_unit);
    } else if (webrays_webgl->intersection_bindings[i].type ==
               wr_binding_type::WR_BINDING_TYPE_GL_TEXTURE_2D_ARRAY) {
      ++current_texture_unit;
      glActiveTexture(GL_TEXTURE0 + current_texture_unit);
      glBindTexture(GL_TEXTURE_2D_ARRAY,
                    webrays_webgl->intersection_bindings[i].data.texture);
      glUniform1i(index, current_texture_unit);
    }
  }

  /*WR_GL_CHECK(glBindBufferBase(GL_UNIFORM_BUFFER, 0,
  webrays_webgl->instances_UBO)); index =
  glGetUniformBlockIndex(webrays_webgl->intersection_program,
  "wr_InstancesUBO"); if (index != GL_INVALID_INDEX) {
          WR_GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER,
  webrays_webgl->instances_UBO));
          WR_GL_CHECK(glUniformBlockBinding(webrays_webgl->intersection_program,
  index, 0));
  }*/

  WR_GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 3));

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  return WR_SUCCESS;
}

wr_error
wrays_gl_query_intersection(wr_handle handle, wr_handle ads,
                            wr_handle* ray_buffers, wr_size ray_buffer_count,
                            wr_handle intersections, wr_size* dimensions,
                            wr_size dimension_count)
{
  wr_context*    webrays       = (wr_context*)handle;
  wr_gl_context* webrays_webgl = (wr_gl_context*)webrays->webgl;

  switch (dimension_count) {
    case 1: break;
    case 2:
      return wrays_gl_query_intersection_2d(handle, ads, ray_buffers,
                                            ray_buffer_count, intersections,
                                            dimensions[0], dimensions[1]);
    default: break;
  }

  return WR_SUCCESS;
}

const char*
wrays_gl_get_scene_accessor(wr_handle handle)
{
  wr_context*    webrays       = (wr_context*)handle;
  wr_gl_context* webrays_webgl = (wr_gl_context*)webrays->webgl;

  return wr_string_buffer_data(webrays_webgl->scene_accessor_shader);
}

const wr_binding*
wrays_gl_get_scene_accessor_bindings(wr_handle handle, wr_size* count)
{
  wr_context*    webrays       = (wr_context*)handle;
  wr_gl_context* webrays_webgl = (wr_gl_context*)webrays->webgl;

  *count = (wr_size)webrays_webgl->binding_count;

  return &webrays_webgl->intersection_bindings[0];
}

WR_INTERNAL unsigned int
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

WR_INTERNAL wr_error
            wrays_gl_ads_build(wr_handle handle)
{
  wr_context* webrays = (wr_context*)handle;
  if (WR_NULL == webrays)
    return (wr_error) "Invalid WebRays context";
  wr_gl_context* webrays_webgl = (wr_gl_context*)webrays->webgl;
  if (WR_NULL == webrays_webgl)
    return (wr_error) "Invalid WebGL WebRays context";

  // Build the BLASes...
  for (int i = 0; i < webrays->scene.blas_count; ++i) {
    ADS* ads = (ADS*)webrays->scene.blas_handles[i];
    if (ads != nullptr)
      ads->Build();
  }

  if (webrays->scene.blas_type == wr_blas_type::WR_BLAS_TYPE_SAH) {

    int bvh_nodes_texture_size = 0;
    int attrs_texture_size     = 0;
    int faces_texture_size     = 0;

    int bvh_nodes_width  = 0;
    int bvh_nodes_height = 0;
    int bvh_nodes_layers = webrays->scene.blas_count;

    int attrs_width  = 0;
    int attrs_height = 0;
    int attrs_layers = webrays->scene.blas_count * 2;

    int faces_width  = 0;
    int faces_height = 0;
    int faces_layers = webrays->scene.blas_count;

    int ads_index = 0;
    for (ads_index = 0; ads_index < webrays->scene.blas_count; ads_index++) {
      SAHBVH* sahbvh = (SAHBVH*)webrays->scene.blas_handles[ads_index];

      int num_pixels         = 2 * sahbvh->m_total_nodes;
      bvh_nodes_texture_size = wrays_maxi(
        bvh_nodes_texture_size,
        (int)wr_next_power_of_2(1 + (unsigned int)sqrtf((float)num_pixels)));

      bvh_nodes_width =
        wrays_maxi(bvh_nodes_width, (num_pixels < bvh_nodes_texture_size)
                                      ? num_pixels
                                      : bvh_nodes_texture_size);
      bvh_nodes_height = wrays_maxi(
        bvh_nodes_height, (num_pixels - 1) / bvh_nodes_texture_size + 1);

      num_pixels         = (int)sahbvh->m_vertex_data.size();
      attrs_texture_size = wrays_maxi(
        attrs_texture_size,
        (int)wr_next_power_of_2(1 + (unsigned int)sqrtf((float)num_pixels)));

      attrs_width = wrays_maxi(attrs_width, (num_pixels < attrs_texture_size)
                                              ? num_pixels
                                              : attrs_texture_size);
      attrs_height =
        wrays_maxi(attrs_height, (num_pixels - 1) / attrs_texture_size + 1);

      num_pixels         = (int)sahbvh->m_triangles.size();
      faces_texture_size = wrays_maxi(
        faces_texture_size,
        (int)wr_next_power_of_2(1 + (unsigned int)sqrtf((float)num_pixels)));

      faces_width = wrays_maxi(faces_width, (num_pixels < faces_texture_size)
                                              ? num_pixels
                                              : faces_texture_size);
      faces_height =
        wrays_maxi(faces_height, (num_pixels - 1) / faces_texture_size + 1);
    }

    // Allocate on the GPU
    if (glIsTexture(webrays_webgl->bounds_texture))
      glDeleteTextures(1, &webrays_webgl->bounds_texture);
    glGenTextures(1, &webrays_webgl->bounds_texture);

    glBindTexture(GL_TEXTURE_2D_ARRAY, webrays_webgl->bounds_texture);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA32F, bvh_nodes_width,
                   bvh_nodes_height, bvh_nodes_layers);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    if (glIsTexture(webrays_webgl->scene_texture))
      glDeleteTextures(1, &webrays_webgl->scene_texture);
    glGenTextures(1, &webrays_webgl->scene_texture);

    glBindTexture(GL_TEXTURE_2D_ARRAY, webrays_webgl->scene_texture);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA32F, attrs_width,
                   attrs_height, attrs_layers);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    if (glIsTexture(webrays_webgl->indices_texture))
      glDeleteTextures(1, &webrays_webgl->indices_texture);
    glGenTextures(1, &webrays_webgl->indices_texture);

    glBindTexture(GL_TEXTURE_2D_ARRAY, webrays_webgl->indices_texture);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA32I, faces_width,
                   faces_height, faces_layers);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    /* Populate GPU memory */
    for (ads_index = 0; ads_index < webrays->scene.blas_count; ads_index++) {
      SAHBVH* sahbvh = (SAHBVH*)webrays->scene.blas_handles[ads_index];

      glBindTexture(GL_TEXTURE_2D_ARRAY, webrays_webgl->bounds_texture);
      {
        // Reorder the bits
        struct GPU_NODE
        {
          vec3 minValue;
          union
          {
            int primitivesOffset;  // leaf
            int secondChildOffset; // interior
          };
          vec3     maxValue;
          uint16_t nPrimitives; // 0 -> interior node
          uint8_t  axis;        // interior node: xyz
          uint8_t  pad[1];      // ensure 32 byte total size
        };
        constexpr int node_size = sizeof(GPU_NODE);
        GPU_NODE* temp_data = new GPU_NODE[bvh_nodes_width * bvh_nodes_height];
        for (int i = 0; i < sahbvh->m_total_nodes; ++i) {
          temp_data[i].minValue = sahbvh->m_linear_nodes[i].bounds.min;
          temp_data[i].primitivesOffset =
            sahbvh->m_linear_nodes[i].primitivesOffset;
          temp_data[i].maxValue    = sahbvh->m_linear_nodes[i].bounds.max;
          temp_data[i].nPrimitives = sahbvh->m_linear_nodes[i].nPrimitives;
          temp_data[i].axis        = sahbvh->m_linear_nodes[i].axis;
        }
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, ads_index,
                        bvh_nodes_width, bvh_nodes_height, 1, GL_RGBA, GL_FLOAT,
                        (const void*)temp_data);
        delete[] temp_data;
      }

      glBindTexture(GL_TEXTURE_2D_ARRAY, webrays_webgl->scene_texture);
      int num_pixels = (int)sahbvh->m_vertex_data.size();
      if (attrs_width * attrs_height == num_pixels) { // no need to realocate
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, ads_index * 2 + 0,
                        attrs_width, attrs_height, 1, GL_RGBA, GL_FLOAT,
                        (const void*)sahbvh->m_vertex_data.data());
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, ads_index * 2 + 1,
                        attrs_width, attrs_height, 1, GL_RGBA, GL_FLOAT,
                        (const void*)sahbvh->m_normal_data.data());
      } else {
        unsigned char* temp_data =
          new unsigned char[attrs_width * attrs_height * 4 * 4];
        std::memcpy(temp_data, sahbvh->m_vertex_data.data(),
                    sahbvh->m_vertex_data.size() * 4 * 4);
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, ads_index * 2 + 0,
                        attrs_width, attrs_height, 1, GL_RGBA, GL_FLOAT,
                        (const void*)temp_data);
        std::memcpy(temp_data, sahbvh->m_normal_data.data(),
                    sahbvh->m_normal_data.size() * 4 * 4);
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, ads_index * 2 + 1,
                        attrs_width, attrs_height, 1, GL_RGBA, GL_FLOAT,
                        (const void*)temp_data);
        delete[] temp_data;
      }

      glBindTexture(GL_TEXTURE_2D_ARRAY, webrays_webgl->indices_texture);
      num_pixels = (int)sahbvh->m_triangles.size();

      if (faces_width * faces_height == num_pixels) // no need to realocate
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, ads_index, faces_width,
                        faces_height, 1, GL_RGBA_INTEGER, GL_INT,
                        (const void*)sahbvh->m_triangles.data());
      else {
        unsigned char* temp_data =
          new unsigned char[faces_width * faces_height * 4 * 4];
        std::memcpy(temp_data, sahbvh->m_triangles.data(),
                    sahbvh->m_triangles.size() * 4 * 4);
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, ads_index, faces_width,
                        faces_height, 1, GL_RGBA_INTEGER, GL_INT,
                        (const void*)temp_data);
        delete[] temp_data;
      }

      sahbvh->m_webgl_bindings[0] = { "wr_scene_vertices",
                                      WR_BINDING_TYPE_GL_TEXTURE_2D_ARRAY,
                                      { webrays_webgl->scene_texture } };
      sahbvh->m_webgl_bindings[1] = { "wr_scene_indices",
                                      WR_BINDING_TYPE_GL_TEXTURE_2D_ARRAY,
                                      { webrays_webgl->indices_texture } };
      sahbvh->m_webgl_bindings[2] = { "wr_bvh_nodes",
                                      WR_BINDING_TYPE_GL_TEXTURE_2D_ARRAY,
                                      { webrays_webgl->bounds_texture } };
    }

    webrays_webgl->bounds_texture_size  = bvh_nodes_texture_size;
    webrays_webgl->scene_texture_size   = attrs_texture_size;
    webrays_webgl->indices_texture_size = faces_texture_size;

    webrays_webgl->intersection_bindings[0] = {
      "wr_scene_vertices",
      WR_BINDING_TYPE_GL_TEXTURE_2D_ARRAY,
      { webrays_webgl->scene_texture }
    };
    webrays_webgl->intersection_bindings[1] = {
      "wr_scene_indices",
      WR_BINDING_TYPE_GL_TEXTURE_2D_ARRAY,
      { webrays_webgl->indices_texture }
    };
    webrays_webgl->intersection_bindings[2] = {
      "wr_bvh_nodes",
      WR_BINDING_TYPE_GL_TEXTURE_2D_ARRAY,
      { webrays_webgl->bounds_texture }
    };
    webrays_webgl->binding_count =
      (int)(sizeof(webrays_webgl->intersection_bindings) /
            sizeof(webrays_webgl->intersection_bindings[0]));

    for (ads_index = 0; ads_index < webrays->scene.blas_count; ads_index++) {
      SAHBVH* sahbvh = (SAHBVH*)webrays->scene.blas_handles[ads_index];
      sahbvh->m_node_texture_size   = webrays_webgl->bounds_texture_size;
      sahbvh->m_index_texture_size  = webrays_webgl->indices_texture_size;
      sahbvh->m_vertex_texture_size = webrays_webgl->scene_texture_size;
    }
  } else if (webrays->scene.blas_type == wr_blas_type::WR_BLAS_TYPE_WIDEBVH) {
    int bvh_nodes_texture_size = 0;
    int attrs_texture_size     = 0;
    int faces_texture_size     = 0;

    int       max_bvh_nodes_width  = 0;
    int       max_bvh_nodes_height = 0;
    const int bvh_nodes_layers     = webrays->scene.blas_count;

    int       max_attrs_width  = 0;
    int       max_attrs_height = 0;
    const int attrs_layers     = webrays->scene.blas_count * 2;

    int       max_faces_width  = 0;
    int       max_faces_height = 0;
    const int faces_layers     = webrays->scene.blas_count;

    int ads_index = 0;
    for (ads_index = 0; ads_index < webrays->scene.blas_count; ads_index++) {
      WideBVH* widebvh = (WideBVH*)webrays->scene.blas_handles[ads_index];

      int num_pixels = sizeof(wr_wide_bvh_node) * widebvh->m_total_nodes / 16;
      bvh_nodes_texture_size = wrays_maxi(
        bvh_nodes_texture_size,
        (int)wr_next_power_of_2(1 + (unsigned int)sqrtf((float)num_pixels)));

      max_bvh_nodes_width =
        wrays_maxi(max_bvh_nodes_width, (num_pixels < bvh_nodes_texture_size)
                                          ? num_pixels
                                          : bvh_nodes_texture_size);
      max_bvh_nodes_height = wrays_maxi(
        max_bvh_nodes_height, (num_pixels - 1) / bvh_nodes_texture_size + 1);

      num_pixels         = (int)widebvh->m_vertex_data.size();
      attrs_texture_size = wrays_maxi(
        attrs_texture_size,
        (int)wr_next_power_of_2(1 + (unsigned int)sqrtf((float)num_pixels)));

      max_attrs_width = wrays_maxi(
        max_attrs_width,
        (num_pixels < attrs_texture_size) ? num_pixels : attrs_texture_size);
      max_attrs_height =
        wrays_maxi(max_attrs_height, (num_pixels - 1) / attrs_texture_size + 1);

      num_pixels         = (int)widebvh->m_triangles.size();
      faces_texture_size = wrays_maxi(
        faces_texture_size,
        (int)wr_next_power_of_2(1 + (unsigned int)sqrtf((float)num_pixels)));

      max_faces_width = wrays_maxi(
        max_faces_width,
        (num_pixels < faces_texture_size) ? num_pixels : faces_texture_size);
      max_faces_height =
        wrays_maxi(max_faces_height, (num_pixels - 1) / faces_texture_size + 1);
    }

    // Allocate on the GPU
    if (glIsTexture(webrays_webgl->bounds_texture))
      glDeleteTextures(1, &webrays_webgl->bounds_texture);
    WR_GL_CHECK(glGenTextures(1, &webrays_webgl->bounds_texture));

    WR_GL_CHECK(
      glBindTexture(GL_TEXTURE_2D_ARRAY, webrays_webgl->bounds_texture));
    WR_GL_CHECK(glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA32F,
                               max_bvh_nodes_width, max_bvh_nodes_height,
                               bvh_nodes_layers));

    WR_GL_CHECK(
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    WR_GL_CHECK(
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    WR_GL_CHECK(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S,
                                GL_CLAMP_TO_EDGE));
    WR_GL_CHECK(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T,
                                GL_CLAMP_TO_EDGE));
    WR_GL_CHECK(glBindTexture(GL_TEXTURE_2D_ARRAY, 0));

    if (glIsTexture(webrays_webgl->scene_texture))
      glDeleteTextures(1, &webrays_webgl->scene_texture);
    WR_GL_CHECK(glGenTextures(1, &webrays_webgl->scene_texture));

    WR_GL_CHECK(
      glBindTexture(GL_TEXTURE_2D_ARRAY, webrays_webgl->scene_texture));
    WR_GL_CHECK(glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA32F,
                               max_attrs_width, max_attrs_height,
                               attrs_layers));

    WR_GL_CHECK(
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    WR_GL_CHECK(
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    WR_GL_CHECK(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S,
                                GL_CLAMP_TO_EDGE));
    WR_GL_CHECK(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T,
                                GL_CLAMP_TO_EDGE));
    WR_GL_CHECK(glBindTexture(GL_TEXTURE_2D_ARRAY, 0));

    if (glIsTexture(webrays_webgl->indices_texture))
      glDeleteTextures(1, &webrays_webgl->indices_texture);
    WR_GL_CHECK(glGenTextures(1, &webrays_webgl->indices_texture));

    WR_GL_CHECK(
      glBindTexture(GL_TEXTURE_2D_ARRAY, webrays_webgl->indices_texture));
    WR_GL_CHECK(glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA32I,
                               max_faces_width, max_faces_height,
                               faces_layers));

    WR_GL_CHECK(
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    WR_GL_CHECK(
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    WR_GL_CHECK(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S,
                                GL_CLAMP_TO_EDGE));
    WR_GL_CHECK(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T,
                                GL_CLAMP_TO_EDGE));
    WR_GL_CHECK(glBindTexture(GL_TEXTURE_2D_ARRAY, 0));

    /* Populate GPU memory */
    for (ads_index = 0; ads_index < webrays->scene.blas_count; ads_index++) {
      WideBVH* widebvh = (WideBVH*)webrays->scene.blas_handles[ads_index];

      glBindTexture(GL_TEXTURE_2D_ARRAY, webrays_webgl->bounds_texture);
      int num_pixels = (int)widebvh->m_total_nodes * sizeof(wr_wide_bvh_node) /
                       (4 * sizeof(float));
      if (max_bvh_nodes_width * max_bvh_nodes_height ==
          num_pixels) { // no need to realocate
        WR_GL_CHECK(glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, ads_index,
                                    max_bvh_nodes_width, max_bvh_nodes_height,
                                    1, GL_RGBA, GL_FLOAT,
                                    (const void*)widebvh->m_linear_nodes));
      } else {
        unsigned char* temp_data =
          new unsigned char[max_bvh_nodes_width * max_bvh_nodes_height * 4 *
                            sizeof(float)];
        std::memset(temp_data, 0,
                    max_bvh_nodes_width * max_bvh_nodes_height * 4 *
                      sizeof(float));
        std::memcpy(temp_data, widebvh->m_linear_nodes,
                    num_pixels * 4 * sizeof(float));
        WR_GL_CHECK(glTexSubImage3D(
          GL_TEXTURE_2D_ARRAY, 0, 0, 0, ads_index, max_bvh_nodes_width,
          max_bvh_nodes_height, 1, GL_RGBA, GL_FLOAT, (const void*)temp_data));
        delete[] temp_data;
      }

      WR_GL_CHECK(
        glBindTexture(GL_TEXTURE_2D_ARRAY, webrays_webgl->scene_texture));
      num_pixels = (int)widebvh->m_vertex_data.size();
      if (max_attrs_width * max_attrs_height ==
          num_pixels) { // no need to realocate
        WR_GL_CHECK(glTexSubImage3D(
          GL_TEXTURE_2D_ARRAY, 0, 0, 0, ads_index * 2 + 0, max_attrs_width,
          max_attrs_height, 1, GL_RGBA, GL_FLOAT,
          (const void*)widebvh->m_vertex_data.data()));
        WR_GL_CHECK(glTexSubImage3D(
          GL_TEXTURE_2D_ARRAY, 0, 0, 0, ads_index * 2 + 1, max_attrs_width,
          max_attrs_height, 1, GL_RGBA, GL_FLOAT,
          (const void*)widebvh->m_normal_data.data()));
      } else {
        unsigned char* temp_data =
          new unsigned char[max_attrs_width * max_attrs_height * 4 * 4];
        std::memset(temp_data, 0, max_attrs_width * max_attrs_height * 4 * 4);
        std::memcpy(temp_data, widebvh->m_vertex_data.data(),
                    widebvh->m_vertex_data.size() * 4 * 4);
        WR_GL_CHECK(glTexSubImage3D(
          GL_TEXTURE_2D_ARRAY, 0, 0, 0, ads_index * 2 + 0, max_attrs_width,
          max_attrs_height, 1, GL_RGBA, GL_FLOAT, (const void*)temp_data));
        std::memcpy(temp_data, widebvh->m_normal_data.data(),
                    widebvh->m_normal_data.size() * 4 * 4);
        WR_GL_CHECK(glTexSubImage3D(
          GL_TEXTURE_2D_ARRAY, 0, 0, 0, ads_index * 2 + 1, max_attrs_width,
          max_attrs_height, 1, GL_RGBA, GL_FLOAT, (const void*)temp_data));
        delete[] temp_data;
      }

      WR_GL_CHECK(
        glBindTexture(GL_TEXTURE_2D_ARRAY, webrays_webgl->indices_texture));
      num_pixels = (int)widebvh->m_triangles.size();

      if (max_faces_width * max_faces_height ==
          num_pixels) // no need to realocate
        WR_GL_CHECK(glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, ads_index,
                                    max_faces_width, max_faces_height, 1,
                                    GL_RGBA_INTEGER, GL_INT,
                                    (const void*)widebvh->m_triangles.data()));
      else {
        unsigned char* temp_data =
          new unsigned char[max_faces_width * max_faces_height * 4 * 4];
        std::memset(temp_data, 0, max_faces_width * max_faces_height * 4 * 4);
        std::memcpy(temp_data, widebvh->m_triangles.data(),
                    widebvh->m_triangles.size() * 4 * 4);
        WR_GL_CHECK(glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, ads_index,
                                    max_faces_width, max_faces_height, 1,
                                    GL_RGBA_INTEGER, GL_INT,
                                    (const void*)temp_data));
        delete[] temp_data;
      }

      widebvh->m_webgl_bindings[0]   = { "wr_scene_vertices",
                                       WR_BINDING_TYPE_GL_TEXTURE_2D_ARRAY,
                                       { webrays_webgl->scene_texture } };
      widebvh->m_webgl_bindings[1]   = { "wr_scene_indices",
                                       WR_BINDING_TYPE_GL_TEXTURE_2D_ARRAY,
                                       { webrays_webgl->indices_texture } };
      widebvh->m_webgl_bindings[2]   = { "wr_bvh_nodes",
                                       WR_BINDING_TYPE_GL_TEXTURE_2D_ARRAY,
                                       { webrays_webgl->bounds_texture } };
      widebvh->m_webgl_binding_count = 3;
      if (0 != webrays_webgl->tlas_texture) {
        widebvh->m_webgl_bindings[3]   = { "wr_scene_instances",
                                         WR_BINDING_TYPE_GL_TEXTURE_2D_ARRAY,
                                         { webrays_webgl->tlas_texture } };
        widebvh->m_webgl_binding_count = 4;
      }
    }

    webrays_webgl->bounds_texture_size  = bvh_nodes_texture_size;
    webrays_webgl->scene_texture_size   = attrs_texture_size;
    webrays_webgl->indices_texture_size = faces_texture_size;

    webrays_webgl->intersection_bindings[0] = {
      "wr_scene_vertices",
      WR_BINDING_TYPE_GL_TEXTURE_2D_ARRAY,
      { webrays_webgl->scene_texture }
    };
    webrays_webgl->intersection_bindings[1] = {
      "wr_scene_indices",
      WR_BINDING_TYPE_GL_TEXTURE_2D_ARRAY,
      { webrays_webgl->indices_texture }
    };
    webrays_webgl->intersection_bindings[2] = {
      "wr_bvh_nodes",
      WR_BINDING_TYPE_GL_TEXTURE_2D_ARRAY,
      { webrays_webgl->bounds_texture }
    };
    webrays_webgl->binding_count = 3;
    if (0 != webrays_webgl->tlas_texture) {
      webrays_webgl->intersection_bindings[3] = {
        "wr_scene_instances",
        WR_BINDING_TYPE_GL_TEXTURE_2D_ARRAY,
        { webrays_webgl->tlas_texture }
      };
      webrays_webgl->binding_count = 4;
    }

    /* Update all ADS to have the same dimensions for scene textures */
    for (ads_index = 0; ads_index < webrays->scene.blas_count; ads_index++) {
      WideBVH* sahbvh = (WideBVH*)webrays->scene.blas_handles[ads_index];
      sahbvh->m_node_texture_size     = webrays_webgl->bounds_texture_size;
      sahbvh->m_index_texture_size    = webrays_webgl->indices_texture_size;
      sahbvh->m_vertex_texture_size   = webrays_webgl->scene_texture_size;
      sahbvh->m_instance_texture_size = 0;
    }
  }

  return WR_SUCCESS;
}

wr_error
wrays_gl_update(wr_handle handle, wr_update_flags* flags)
{
  wr_context* webrays = (wr_context*)handle;
  if (WR_NULL == webrays)
    return (wr_error) "Invalid WebRays context";
  wr_gl_context* webrays_webgl = (wr_gl_context*)webrays->webgl;
  if (WR_NULL == webrays_webgl)
    return (wr_error) "Invalid WebGL WebRays context";

  int tlas_node_count[WR_MAX_TLAS_COUNT] = { 0 };
  int max_allocation_per_tlas            = 0;
  int tlas_texture_width                 = 0;
  for (int i = 0; i < webrays->scene.tlas_count; i++) {
    max_allocation_per_tlas =
      std::max(max_allocation_per_tlas,
               (int)webrays->scene.tlas_handles[i]->m_instances.size());
    tlas_node_count[i] =
      (int)webrays->scene.tlas_handles[i]->m_instances.size();
  }

  const int MAX_TEXTURE_WIDTH = 512;
  const int num_pixels =
    max_allocation_per_tlas * webrays->scene.tlas_count * sizeof(Instance) / 16;
  tlas_texture_width       = num_pixels % MAX_TEXTURE_WIDTH;
  const int texture_height = (num_pixels - 1) / MAX_TEXTURE_WIDTH + 1;

  if (webrays->update_flags & WR_UPDATE_FLAG_INSTANCE_ADD) {
    if (glIsTexture(webrays_webgl->tlas_texture))
      glDeleteTextures(1, &webrays_webgl->tlas_texture);

    WR_GL_CHECK(glGenTextures(1, &webrays_webgl->tlas_texture));
    WR_GL_CHECK(
      glBindTexture(GL_TEXTURE_2D_ARRAY, webrays_webgl->tlas_texture));
    WR_GL_CHECK(glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA32F,
                               tlas_texture_width, texture_height,
                               webrays->scene.tlas_count));
    WR_GL_CHECK(
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    WR_GL_CHECK(
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    WR_GL_CHECK(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S,
                                GL_CLAMP_TO_EDGE));
    WR_GL_CHECK(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T,
                                GL_CLAMP_TO_EDGE));
    WR_GL_CHECK(glBindTexture(GL_TEXTURE_2D_ARRAY, 0));

    *flags = (wr_update_flags)(WR_UPDATE_FLAG_ACCESSOR_BINDINGS |
                               WR_UPDATE_FLAG_ACCESSOR_CODE);
  }

  if (max_allocation_per_tlas > 0 &&
      (webrays->update_flags &
       (WR_UPDATE_FLAG_INSTANCE_UPDATE | WR_UPDATE_FLAG_INSTANCE_ADD))) {
    static_assert(sizeof(Instance) == 64,
                  "Instance size is not 64 bytes (mat4)");
    Instance* data =
      new Instance[max_allocation_per_tlas * webrays->scene.tlas_count];
    std::memset(data, 0,
                max_allocation_per_tlas * webrays->scene.tlas_count *
                  sizeof(Instance));
    // fill the TLAS Texture
    for (int i = 0; i < webrays->scene.tlas_count; i++) {
      std::memcpy(&data[max_allocation_per_tlas * i],
                  webrays->scene.tlas_handles[i]->m_instances.data(),
                  sizeof(Instance) *
                    webrays->scene.tlas_handles[i]->m_instances.size());
    }

    WR_GL_CHECK(
      glBindTexture(GL_TEXTURE_2D_ARRAY, webrays_webgl->tlas_texture));
    WR_GL_CHECK(glTexSubImage3D(
      GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, tlas_texture_width, texture_height,
      webrays->scene.tlas_count, GL_RGBA, GL_FLOAT, (const void*)data));

    webrays->update_flags = (wr_update_flags)(
      webrays->update_flags &
      ~(WR_UPDATE_FLAG_INSTANCE_UPDATE | WR_UPDATE_FLAG_INSTANCE_ADD));

    delete[] data;
  }

  if (!(webrays->update_flags &
        (WR_UPDATE_FLAG_ACCESSOR_BINDINGS | WR_UPDATE_FLAG_ACCESSOR_CODE))) {
    return WR_SUCCESS;
  }

  wrays_gl_ads_build(handle);

  *flags                = (wr_update_flags)(WR_UPDATE_FLAG_ACCESSOR_BINDINGS |
                             WR_UPDATE_FLAG_ACCESSOR_CODE);
  webrays->update_flags = (wr_update_flags)(
    webrays->update_flags &
    ~(WR_UPDATE_FLAG_ACCESSOR_BINDINGS | WR_UPDATE_FLAG_ACCESSOR_CODE));

  // get the first ADS in order to get the shader code (ALL ADS should have the
  // same shader code)
  ADS* ads                     = (ADS*)webrays->scene.blas_handles[0];
  ads->m_instance_texture_size = tlas_texture_width;
  /*if (webrays->scene.blas_count == 2) { // TODO: Maybe it should be converted
  to a FOR LOOP ads = (ADS*)webrays->scene.blas_handles[1];
          ads->m_instance_texture_size = tlas_texture_width;
  }*/
  ads = (ADS*)webrays->scene.blas_handles[0];

  GLuint vertex_shader;
  GLuint fragment_shader;

  /* Manage Programs */
  if (glIsProgram(webrays_webgl->intersection_program))
    glDeleteProgram(webrays_webgl->intersection_program);
  if (glIsProgram(webrays_webgl->occlusion_program))
    glDeleteProgram(webrays_webgl->occlusion_program);

  static const int node_upper_bound = 100000;

  char tlas_node_count_str[128];
  static_assert(WR_MAX_TLAS_COUNT == 8, "TLAS size is hard coded as 8");
  sprintf(
    tlas_node_count_str,
    "const int WR_TLAS_NODE_COUNT[8] = int[](%d, %d, %d, %d, %d, %d, %d, %d);",
    tlas_node_count[0], tlas_node_count[1], tlas_node_count[2],
    tlas_node_count[3], tlas_node_count[4], tlas_node_count[5],
    tlas_node_count[6], tlas_node_count[7]);
  char tlas_texture_size_str[64];
  sprintf(tlas_texture_size_str, "#define WR_TLAS_TEXTURE_SIZE %d\n",
          tlas_texture_width);

  /* Stitch intersection program */
  wr_string_buffer_appendln(webrays_webgl->shader_scratch, "#version 300 es");
  wr_string_buffer_appendln(webrays_webgl->shader_scratch,
                            "precision highp float;");
  wr_string_buffer_appendln(webrays_webgl->shader_scratch,
                            "precision highp int;");
  wr_string_buffer_appendln(webrays_webgl->shader_scratch,
                            "precision highp isampler2DArray;");
  wr_string_buffer_appendln(webrays_webgl->shader_scratch,
                            "precision highp sampler2DArray;");
  wr_string_buffer_appendln(webrays_webgl->shader_scratch, tlas_node_count_str);
  wr_string_buffer_appendln(webrays_webgl->shader_scratch,
                            tlas_texture_size_str);
  wr_string_buffer_appendln(webrays_webgl->shader_scratch,
                            ads->GetIntersectionCode());
  wr_string_buffer_appendln(webrays_webgl->shader_scratch,
                            wr_intersection_fragment_shader);

  wrays_gl_shader_create(&vertex_shader, wr_screen_fill_vertex_shader,
                         GL_VERTEX_SHADER);
  wrays_gl_shader_create(&fragment_shader,
                         wr_string_buffer_data(webrays_webgl->shader_scratch),
                         GL_FRAGMENT_SHADER);
  wrays_gl_program_create(&webrays_webgl->intersection_program, vertex_shader,
                          fragment_shader);

  wr_string_buffer_clear(webrays_webgl->shader_scratch);

  /* Stitch occlusion program */
  wr_string_buffer_appendln(webrays_webgl->shader_scratch, "#version 300 es");
  wr_string_buffer_appendln(webrays_webgl->shader_scratch,
                            "precision highp float;");
  wr_string_buffer_appendln(webrays_webgl->shader_scratch,
                            "precision highp int;");
  wr_string_buffer_appendln(webrays_webgl->shader_scratch,
                            "precision highp isampler2DArray;");
  wr_string_buffer_appendln(webrays_webgl->shader_scratch,
                            "precision highp sampler2DArray;");
  wr_string_buffer_appendln(webrays_webgl->shader_scratch, tlas_node_count_str);
  wr_string_buffer_appendln(webrays_webgl->shader_scratch,
                            tlas_texture_size_str);
  wr_string_buffer_appendln(webrays_webgl->shader_scratch,
                            ads->GetIntersectionCode());
  wr_string_buffer_appendln(webrays_webgl->shader_scratch,
                            wr_occlusion_fragment_shader);

  // rg_string_buffer_pretty_print(webrays_webgl->shader_scratch);

  wrays_gl_shader_create(&vertex_shader, wr_screen_fill_vertex_shader,
                         GL_VERTEX_SHADER);
  wrays_gl_shader_create(&fragment_shader,
                         wr_string_buffer_data(webrays_webgl->shader_scratch),
                         GL_FRAGMENT_SHADER);
  wrays_gl_program_create(&webrays_webgl->occlusion_program, vertex_shader,
                          fragment_shader);

  wr_string_buffer_clear(webrays_webgl->shader_scratch);

  // wr_string_buffer_appendln(webrays_webgl->scene_accessor_shader, "#version
  // 300 es");
  wr_string_buffer_appendln(webrays_webgl->scene_accessor_shader,
                            "precision highp float;");
  wr_string_buffer_appendln(webrays_webgl->scene_accessor_shader,
                            "precision highp int;");
  wr_string_buffer_appendln(webrays_webgl->scene_accessor_shader,
                            "precision highp isampler2DArray;");
  wr_string_buffer_appendln(webrays_webgl->scene_accessor_shader,
                            "precision highp sampler2DArray;");
  wr_string_buffer_appendln(webrays_webgl->scene_accessor_shader,
                            tlas_node_count_str);
  wr_string_buffer_appendln(webrays_webgl->scene_accessor_shader,
                            tlas_texture_size_str);
  wr_string_buffer_appendln(webrays_webgl->scene_accessor_shader,
                            ads->GetIntersectionCode());

  return WR_SUCCESS;
}

wr_error
_wrays_gl_internal_get_bound_texture_2d_handle(wr_handle     handle,
                                               unsigned int* texture)
{
  GLint id;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &id);

  return WR_SUCCESS;
}

wr_error
_wrays_gl_internal_get_intersection_kernel(wr_handle     handle,
                                           unsigned int* program)
{
  wr_context* webrays = (wr_context*)handle;
  if (WR_NULL == webrays)
    return (wr_error) "Invalid WebRays context";
  wr_gl_context* webrays_webgl = (wr_gl_context*)webrays->webgl;
  if (WR_NULL == webrays_webgl)
    return (wr_error) "Invalid WebGL WebRays context";

  *program = webrays_webgl->intersection_program;

  return WR_SUCCESS;
}

wr_error
_wrays_gl_internal_get_occlusion_kernel(wr_handle handle, unsigned int* program)
{
  wr_context* webrays = (wr_context*)handle;
  if (WR_NULL == webrays)
    return (wr_error) "Invalid WebRays context";
  wr_gl_context* webrays_webgl = (wr_gl_context*)webrays->webgl;
  if (WR_NULL == webrays_webgl)
    return (wr_error) "Invalid WebGL WebRays context";

  *program = webrays_webgl->occlusion_program;

  return WR_SUCCESS;
}

WR_INTERNAL wr_error
            wrays_gl_ray_buffer_requirements_2d(wr_handle       handle,
                                                wr_buffer_info* buffer_info, wr_size width,
                                                wr_size height)
{
  wr_context* webrays = (wr_context*)handle;
  if (WR_NULL == webrays)
    return (wr_error) "Invalid WebRays context";
  wr_gl_context* webrays_webgl = (wr_gl_context*)webrays->webgl;
  if (WR_NULL == webrays_webgl)
    return (wr_error) "Invalid WebGL WebRays context";

  buffer_info->type = WR_BUFFER_TYPE_TEXTURE_2D;

  buffer_info->data.as_texture_2d.width           = width;
  buffer_info->data.as_texture_2d.height          = height;
  buffer_info->data.as_texture_2d.internal_format = GL_RGBA32F;
  buffer_info->data.as_texture_2d.target          = GL_TEXTURE_2D;

  return WR_SUCCESS;
}

wr_error
wrays_gl_ray_buffer_requirements(wr_handle handle, wr_buffer_info* buffer_info,
                                 wr_size* dimensions, wr_size dimension_count)
{
  switch (dimension_count) {
    case 1: return (wr_error) "Not Implemented!";
    case 2:
      return wrays_gl_ray_buffer_requirements_2d(handle, buffer_info,
                                                 dimensions[0], dimensions[1]);
    default: return (wr_error) "Not Implemented!";
  }
  return WR_SUCCESS;
}

WR_INTERNAL wr_error
            wrays_gl_intersection_buffer_requirements_2d(wr_handle       handle,
                                                         wr_buffer_info* buffer_info,
                                                         wr_size width, wr_size height)
{
  wr_context* webrays = (wr_context*)handle;
  if (WR_NULL == webrays)
    return (wr_error) "Invalid WebRays context";
  wr_gl_context* webrays_webgl = (wr_gl_context*)webrays->webgl;
  if (WR_NULL == webrays_webgl)
    return (wr_error) "Invalid WebGL WebRays context";

  buffer_info->type = WR_BUFFER_TYPE_TEXTURE_2D;

  buffer_info->data.as_texture_2d.width           = width;
  buffer_info->data.as_texture_2d.height          = height;
  buffer_info->data.as_texture_2d.internal_format = GL_RGBA32I;
  buffer_info->data.as_texture_2d.target          = GL_TEXTURE_2D;

  return WR_SUCCESS;
}

wr_error
wrays_gl_intersection_buffer_requirements(wr_handle       handle,
                                          wr_buffer_info* buffer_info,
                                          wr_size*        dimensions,
                                          wr_size         dimension_count)
{
  switch (dimension_count) {
    case 1: return (wr_error) "Not Implemented!";
    case 2:
      return wrays_gl_intersection_buffer_requirements_2d(
        handle, buffer_info, dimensions[0], dimensions[1]);
    default: return (wr_error) "Not Implemented!";
  }
  return WR_SUCCESS;
}

WR_INTERNAL wr_error
            wrays_gl_occlusion_buffer_requirements_2d(wr_handle       handle,
                                                      wr_buffer_info* buffer_info,
                                                      wr_size width, wr_size height)
{
  wr_context* webrays = (wr_context*)handle;
  if (WR_NULL == webrays)
    return (wr_error) "Invalid WebRays context";
  wr_gl_context* webrays_webgl = (wr_gl_context*)webrays->webgl;
  if (WR_NULL == webrays_webgl)
    return (wr_error) "Invalid WebGL WebRays context";

  buffer_info->type = WR_BUFFER_TYPE_TEXTURE_2D;

  buffer_info->data.as_texture_2d.width           = width;
  buffer_info->data.as_texture_2d.height          = height;
  buffer_info->data.as_texture_2d.internal_format = GL_R32I;
  buffer_info->data.as_texture_2d.target          = GL_TEXTURE_2D;

  return WR_SUCCESS;
}

wr_error
wrays_gl_occlusion_buffer_requirements(wr_handle       handle,
                                       wr_buffer_info* buffer_info,
                                       wr_size*        dimensions,
                                       wr_size         dimension_count)
{
  switch (dimension_count) {
    case 1: return (wr_error) "Not Implemented!";
    case 2:
      return wrays_gl_occlusion_buffer_requirements_2d(
        handle, buffer_info, dimensions[0], dimensions[1]);
    default: return (wr_error) "Not Implemented!";
  }
  return WR_SUCCESS;
}

#ifndef WRAYS_EMSCRIPTEN
PFNGLCOPYTEXSUBIMAGE3DPROC wrCopyTexSubImage3D = WR_NULL;
PFNGLDRAWRANGEELEMENTSPROC wrDrawRangeElements = WR_NULL;
PFNGLTEXIMAGE3DPROC        wrTexImage3D        = WR_NULL;
PFNGLTEXSUBIMAGE3DPROC     wrTexSubImage3D     = WR_NULL;

PFNGLACTIVETEXTUREPROC           wrActiveTexture           = WR_NULL;
PFNGLCOMPRESSEDTEXIMAGE2DPROC    wrCompressedTexImage2D    = WR_NULL;
PFNGLCOMPRESSEDTEXIMAGE3DPROC    wrCompressedTexImage3D    = WR_NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC wrCompressedTexSubImage2D = WR_NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC wrCompressedTexSubImage3D = WR_NULL;
PFNGLSAMPLECOVERAGEPROC          wrSampleCoverage          = WR_NULL;

PFNGLBLENDCOLORPROC        wrBlendColor        = WR_NULL;
PFNGLBLENDEQUATIONPROC     wrBlendEquation     = WR_NULL;
PFNGLBLENDFUNCSEPARATEPROC wrBlendFuncSeparate = WR_NULL;

PFNGLBEGINQUERYPROC           wrBeginQuery           = WR_NULL;
PFNGLBINDBUFFERPROC           wrBindBuffer           = WR_NULL;
PFNGLBUFFERDATAPROC           wrBufferData           = WR_NULL;
PFNGLBUFFERSUBDATAPROC        wrBufferSubData        = WR_NULL;
PFNGLDELETEBUFFERSPROC        wrDeleteBuffers        = WR_NULL;
PFNGLDELETEQUERIESPROC        wrDeleteQueries        = WR_NULL;
PFNGLENDQUERYPROC             wrEndQuery             = WR_NULL;
PFNGLGENBUFFERSPROC           wrGenBuffers           = WR_NULL;
PFNGLGENQUERIESPROC           wrGenQueries           = WR_NULL;
PFNGLGETBUFFERPARAMETERIVPROC wrGetBufferParameteriv = WR_NULL;
PFNGLGETBUFFERPOINTERVPROC    wrGetBufferPointerv    = WR_NULL;
PFNGLGETQUERYOBJECTUIVPROC    wrGetQueryObjectuiv    = WR_NULL;
PFNGLGETQUERYIVPROC           wrGetQueryiv           = WR_NULL;
PFNGLISBUFFERPROC             wrIsBuffer             = WR_NULL;
PFNGLISQUERYPROC              wrIsQuery              = WR_NULL;
PFNGLUNMAPBUFFERPROC          wrUnmapBuffer          = WR_NULL;

PFNGLATTACHSHADERPROC             wrAttachShader             = WR_NULL;
PFNGLBINDATTRIBLOCATIONPROC       wrBindAttribLocation       = WR_NULL;
PFNGLBLENDEQUATIONSEPARATEPROC    wrBlendEquationSeparate    = WR_NULL;
PFNGLCOMPILESHADERPROC            wrCompileShader            = WR_NULL;
PFNGLCREATEPROGRAMPROC            wrCreateProgram            = WR_NULL;
PFNGLCREATESHADERPROC             wrCreateShader             = WR_NULL;
PFNGLDELETEPROGRAMPROC            wrDeleteProgram            = WR_NULL;
PFNGLDELETESHADERPROC             wrDeleteShader             = WR_NULL;
PFNGLDETACHSHADERPROC             wrDetachShader             = WR_NULL;
PFNGLDISABLEVERTEXATTRIBARRAYPROC wrDisableVertexAttribArray = WR_NULL;
PFNGLDRAWBUFFERSPROC              wrDrawBuffers              = WR_NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC  wrEnableVertexAttribArray  = WR_NULL;
PFNGLGETACTIVEATTRIBPROC          wrGetActiveAttrib          = WR_NULL;
PFNGLGETACTIVEUNIFORMPROC         wrGetActiveUniform         = WR_NULL;
PFNGLGETATTACHEDSHADERSPROC       wrGetAttachedShaders       = WR_NULL;
PFNGLGETATTRIBLOCATIONPROC        wrGetAttribLocation        = WR_NULL;
PFNGLGETPROGRAMINFOLOGPROC        wrGetProgramInfoLog        = WR_NULL;
PFNGLGETPROGRAMIVPROC             wrGetProgramiv             = WR_NULL;
PFNGLGETSHADERINFOLOGPROC         wrGetShaderInfoLog         = WR_NULL;
PFNGLGETSHADERSOURCEPROC          wrGetShaderSource          = WR_NULL;
PFNGLGETSHADERIVPROC              wrGetShaderiv              = WR_NULL;
PFNGLGETUNIFORMLOCATIONPROC       wrGetUniformLocation       = WR_NULL;
PFNGLGETUNIFORMFVPROC             wrGetUniformfv             = WR_NULL;
PFNGLGETUNIFORMIVPROC             wrGetUniformiv             = WR_NULL;
PFNGLGETVERTEXATTRIBPOINTERVPROC  wrGetVertexAttribPointerv  = WR_NULL;
PFNGLGETVERTEXATTRIBFVPROC        wrGetVertexAttribfv        = WR_NULL;
PFNGLGETVERTEXATTRIBIVPROC        wrGetVertexAttribiv        = WR_NULL;
PFNGLISPROGRAMPROC                wrIsProgram                = WR_NULL;
PFNGLISSHADERPROC                 wrIsShader                 = WR_NULL;
PFNGLLINKPROGRAMPROC              wrLinkProgram              = WR_NULL;
PFNGLSHADERSOURCEPROC             wrShaderSource             = WR_NULL;
PFNGLSTENCILFUNCSEPARATEPROC      wrStencilFuncSeparate      = WR_NULL;
PFNGLSTENCILMASKSEPARATEPROC      wrStencilMaskSeparate      = WR_NULL;
PFNGLSTENCILOPSEPARATEPROC        wrStencilOpSeparate        = WR_NULL;
PFNGLUNIFORM1FPROC                wrUniform1f                = WR_NULL;
PFNGLUNIFORM1FVPROC               wrUniform1fv               = WR_NULL;
PFNGLUNIFORM1IPROC                wrUniform1i                = WR_NULL;
PFNGLUNIFORM1IVPROC               wrUniform1iv               = WR_NULL;
PFNGLUNIFORM2FPROC                wrUniform2f                = WR_NULL;
PFNGLUNIFORM2FVPROC               wrUniform2fv               = WR_NULL;
PFNGLUNIFORM2IPROC                wrUniform2i                = WR_NULL;
PFNGLUNIFORM2IVPROC               wrUniform2iv               = WR_NULL;
PFNGLUNIFORM3FPROC                wrUniform3f                = WR_NULL;
PFNGLUNIFORM3FVPROC               wrUniform3fv               = WR_NULL;
PFNGLUNIFORM3IPROC                wrUniform3i                = WR_NULL;
PFNGLUNIFORM3IVPROC               wrUniform3iv               = WR_NULL;
PFNGLUNIFORM4FPROC                wrUniform4f                = WR_NULL;
PFNGLUNIFORM4FVPROC               wrUniform4fv               = WR_NULL;
PFNGLUNIFORM4IPROC                wrUniform4i                = WR_NULL;
PFNGLUNIFORM4IVPROC               wrUniform4iv               = WR_NULL;
PFNGLUNIFORMMATRIX2FVPROC         wrUniformMatrix2fv         = WR_NULL;
PFNGLUNIFORMMATRIX3FVPROC         wrUniformMatrix3fv         = WR_NULL;
PFNGLUNIFORMMATRIX4FVPROC         wrUniformMatrix4fv         = WR_NULL;
PFNGLUSEPROGRAMPROC               wrUseProgram               = WR_NULL;
PFNGLVALIDATEPROGRAMPROC          wrValidateProgram          = WR_NULL;
PFNGLVERTEXATTRIB1FPROC           wrVertexAttrib1f           = WR_NULL;
PFNGLVERTEXATTRIB1FVPROC          wrVertexAttrib1fv          = WR_NULL;
PFNGLVERTEXATTRIB3FPROC           wrVertexAttrib3f           = WR_NULL;
PFNGLVERTEXATTRIB3FVPROC          wrVertexAttrib3fv          = WR_NULL;
PFNGLVERTEXATTRIB4FPROC           wrVertexAttrib4f           = WR_NULL;
PFNGLVERTEXATTRIB4FVPROC          wrVertexAttrib4fv          = WR_NULL;
PFNGLVERTEXATTRIBPOINTERPROC      wrVertexAttribPointer      = WR_NULL;

PFNGLUNIFORMMATRIX2X3FVPROC wrUniformMatrix2x3fv = WR_NULL;
PFNGLUNIFORMMATRIX2X4FVPROC wrUniformMatrix2x4fv = WR_NULL;
PFNGLUNIFORMMATRIX3X2FVPROC wrUniformMatrix3x2fv = WR_NULL;
PFNGLUNIFORMMATRIX3X4FVPROC wrUniformMatrix3x4fv = WR_NULL;
PFNGLUNIFORMMATRIX4X2FVPROC wrUniformMatrix4x2fv = WR_NULL;
PFNGLUNIFORMMATRIX4X3FVPROC wrUniformMatrix4x3fv = WR_NULL;

PFNGLBEGINTRANSFORMFEEDBACKPROC      wrBeginTransformFeedback      = WR_NULL;
PFNGLCLEARBUFFERFIPROC               wrClearBufferfi               = WR_NULL;
PFNGLCLEARBUFFERFVPROC               wrClearBufferfv               = WR_NULL;
PFNGLCLEARBUFFERIVPROC               wrClearBufferiv               = WR_NULL;
PFNGLCLEARBUFFERUIVPROC              wrClearBufferuiv              = WR_NULL;
PFNGLENDTRANSFORMFEEDBACKPROC        wrEndTransformFeedback        = WR_NULL;
PFNGLGETFRAGDATALOCATIONPROC         wrGetFragDataLocation         = WR_NULL;
PFNGLGETSTRINGIPROC                  wrGetStringi                  = WR_NULL;
PFNGLGETTRANSFORMFEEDBACKVARYINGPROC wrGetTransformFeedbackVarying = WR_NULL;
PFNGLGETUNIFORMUIVPROC               wrGetUniformuiv               = WR_NULL;
PFNGLGETVERTEXATTRIBIIVPROC          wrGetVertexAttribIiv          = WR_NULL;
PFNGLGETVERTEXATTRIBIUIVPROC         wrGetVertexAttribIuiv         = WR_NULL;
PFNGLTRANSFORMFEEDBACKVARYINGSPROC   wrTransformFeedbackVaryings   = WR_NULL;
PFNGLUNIFORM1UIPROC                  wrUniform1ui                  = WR_NULL;
PFNGLUNIFORM1UIVPROC                 wrUniform1uiv                 = WR_NULL;
PFNGLUNIFORM2UIPROC                  wrUniform2ui                  = WR_NULL;
PFNGLUNIFORM2UIVPROC                 wrUniform2uiv                 = WR_NULL;
PFNGLUNIFORM3UIPROC                  wrUniform3ui                  = WR_NULL;
PFNGLUNIFORM3UIVPROC                 wrUniform3uiv                 = WR_NULL;
PFNGLUNIFORM4UIPROC                  wrUniform4ui                  = WR_NULL;
PFNGLUNIFORM4UIVPROC                 wrUniform4uiv                 = WR_NULL;
PFNGLVERTEXATTRIBI4IPROC             wrVertexAttribI4i             = WR_NULL;
PFNGLVERTEXATTRIBI4IVPROC            wrVertexAttribI4iv            = WR_NULL;
PFNGLVERTEXATTRIBI4UIPROC            wrVertexAttribI4ui            = WR_NULL;
PFNGLVERTEXATTRIBI4UIVPROC           wrVertexAttribI4uiv           = WR_NULL;
PFNGLVERTEXATTRIBIPOINTERPROC        wrVertexAttribIPointer        = WR_NULL;

PFNGLDRAWARRAYSINSTANCEDPROC   wrDrawArraysInstanced   = WR_NULL;
PFNGLDRAWELEMENTSINSTANCEDPROC wrDrawElementsInstanced = WR_NULL;

PFNGLGETBUFFERPARAMETERI64VPROC wrGetBufferParameteri64v = WR_NULL;
PFNGLGETINTEGER64I_VPROC        wrGetInteger64i_v        = WR_NULL;

PFNGLVERTEXATTRIBDIVISORPROC wrVertexAttribDivisor = WR_NULL;

PFNGLCLEARDEPTHFPROC              wrClearDepthf              = WR_NULL;
PFNGLDEPTHRANGEFPROC              wrDepthRangef              = WR_NULL;
PFNGLGETSHADERPRECISIONFORMATPROC wrGetShaderPrecisionFormat = WR_NULL;
PFNGLRELEASESHADERCOMPILERPROC    wrReleaseShaderCompiler    = WR_NULL;
PFNGLSHADERBINARYPROC             wrShaderBinary             = WR_NULL;

PFNGLCOPYBUFFERSUBDATAPROC wrCopyBufferSubData = WR_NULL;

PFNGLVIEWPORTPROC        wrViewport        = WR_NULL;
PFNGLTEXSTORAGE3DPROC    wrTexStorage3D    = WR_NULL;
PFNGLTEXPARAMETERIPROC   wrTexParameteri   = WR_NULL;
PFNGLGETINTEGERVPROC     wrGetIntegerv     = WR_NULL;
PFNGLGENVERTEXARRAYSPROC wrGenVertexArrays = WR_NULL;
PFNGLISTEXTUREPROC       wrIsTexture       = WR_NULL;
PFNGLGENTEXTURESPROC     wrGenTextures     = WR_NULL;
PFNGLDELETETEXTURESPROC  wrDeleteTextures  = WR_NULL;
PFNGLBINDTEXTUREPROC     wrBindTexture     = WR_NULL;
PFNGLDRAWARRAYSPROC      wrDrawArrays      = WR_NULL;
PFNGLBINDVERTEXARRAYPROC wrBindVertexArray = WR_NULL;
PFNGLGETERRORPROC        wrGetError        = WR_NULL;

PFNGLBINDFRAMEBUFFERPROC         wrBindFramebuffer         = WR_NULL;
PFNGLBINDRENDERBUFFERPROC        wrBindRenderbuffer        = WR_NULL;
PFNGLBLITFRAMEBUFFERPROC         wrBlitFramebuffer         = WR_NULL;
PFNGLCHECKFRAMEBUFFERSTATUSPROC  wrCheckFramebufferStatus  = WR_NULL;
PFNGLDELETEFRAMEBUFFERSPROC      wrDeleteFramebuffers      = WR_NULL;
PFNGLDELETERENDERBUFFERSPROC     wrDeleteRenderbuffers     = WR_NULL;
PFNGLFRAMEBUFFERRENDERBUFFERPROC wrFramebufferRenderbuffer = WR_NULL;
PFNGLFRAMEBUFFERTEXTURE2DPROC    wrFramebufferTexture2D    = WR_NULL;
PFNGLFRAMEBUFFERTEXTURELAYERPROC wrFramebufferTextureLayer = WR_NULL;
PFNGLGENFRAMEBUFFERSPROC         wrGenFramebuffers         = WR_NULL;
PFNGLGENRENDERBUFFERSPROC        wrGenRenderbuffers        = WR_NULL;
PFNGLGENERATEMIPMAPPROC          wrGenerateMipmap          = WR_NULL;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC
wrGetFramebufferAttachmentParameteriv                                = WR_NULL;
PFNGLGETRENDERBUFFERPARAMETERIVPROC     wrGetRenderbufferParameteriv = WR_NULL;
PFNGLISFRAMEBUFFERPROC                  wrIsFramebuffer              = WR_NULL;
PFNGLISRENDERBUFFERPROC                 wrIsRenderbuffer             = WR_NULL;
PFNGLRENDERBUFFERSTORAGEPROC            wrRenderbufferStorage        = WR_NULL;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC wrRenderbufferStorageMultisample =
  WR_NULL;
#endif /* WRAYS_EMSCRIPTEN */
