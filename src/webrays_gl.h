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

#ifndef _WRAYS_GL_H_
#define _WRAYS_GL_H_

//#define USE_TIMERS

#include "webrays/webrays.h"

#ifdef __cplusplus
extern "C"
{
#endif
  /* Embeded GL Version 3 */
#if defined(__EMSCRIPTEN__)
#else
#define GL_GLEXT_PROTOTYPES 0
#define GL_GLES_PROTOTYPES 0
#define GL_APIENTRY
#endif
#include <GLES3/gl3.h>
#ifdef USE_TIMERS
#include <GLES2/gl2ext.h>
#endif
#ifdef __cplusplus
}
#endif

#define WR_XSTR(a) WR_STR(a)
#define WR_STR(a) #a
#if defined(__EMSCRIPTEN__) || defined(NDEBUG)
#define WR_GL_CHECK(expr) expr
#define WR_GL_CHECK_AT(expr, line)
#else
#define WR_GL_CHECK(expr) WR_GL_CHECK_AT(expr, __LINE__)
#define WR_GL_CHECK_AT(expr, line)                                             \
  do {                                                                         \
    expr;                                                                      \
    GLenum err;                                                                \
    if ((err = glGetError()) != GL_NO_ERROR) {                                 \
      switch (err) {                                                           \
        case GL_INVALID_ENUM:                                                  \
          return (wr_error)(                                                   \
            #expr " failed with GL_INVALID_ENUM at line:" WR_XSTR(line));      \
        case GL_INVALID_VALUE:                                                 \
          return (wr_error)(                                                   \
            #expr " failed with GL_INVALID_VALUE at line:" WR_XSTR(line));     \
        case GL_INVALID_OPERATION:                                             \
          return (wr_error)(                                                   \
            #expr " failed with INVALID_OPERATION at line:" WR_XSTR(line));    \
        case GL_INVALID_FRAMEBUFFER_OPERATION:                                 \
          return (wr_error)(                                                   \
            #expr                                                              \
            " failed with INVALID_FRAMEBUFFER_OPERATION at line:" WR_XSTR(     \
              line));                                                          \
        case GL_OUT_OF_MEMORY:                                                 \
          return (wr_error)(                                                   \
            #expr " failed with OUT_OF_MEMORY at line:" WR_XSTR(line));        \
      }                                                                        \
    }                                                                          \
  } while (0)
#endif

wr_error
_wrays_gl_internal_get_bound_texture_2d_handle(wr_handle     handle,
                                               unsigned int* texture);
wr_error
_wrays_gl_internal_get_intersection_kernel(wr_handle     handle,
                                           unsigned int* program);
wr_error
_wrays_gl_internal_get_occlusion_kernel(wr_handle     handle,
                                        unsigned int* program);

#ifdef WEBRAYS_PROTOTYPE_API
wr_error
wrays_gl_init(wr_handle handle);
wr_error
wrays_gl_update(wr_handle handle, wr_update_flags* flags);
const char*
wrays_gl_get_scene_accessor(wr_handle handle);
const wr_binding*
wrays_gl_get_scene_accessor_bindings(wr_handle handle, wr_size* count);
wr_error
wrays_gl_query_occlusion(wr_handle handle, wr_handle ads,
                         wr_handle* ray_buffers, wr_size ray_buffer_count,
                         wr_handle occlusion, wr_size* dimensions,
                         wr_size dimension_count);
wr_error
wrays_gl_query_intersection(wr_handle handle, wr_handle ads,
                            wr_handle* ray_buffers, wr_size ray_buffer_count,
                            wr_handle intersections, wr_size* dimensions,
                            wr_size dimension_count);

wr_error
wrays_gl_add_shape(wr_handle handle, wr_handle ads, float* positions,
                   int position_stride, float* normals, int normal_stride,
                   float* uvs, int uv_stride, int attr_count, int* faces,
                   int num_triangles, int* shape_id);

wr_error
wrays_gl_ray_buffer_destroy(wr_handle handle, wr_handle buffer);
wr_error
wrays_gl_intersection_buffer_destroy(wr_handle handle, wr_handle buffer);
wr_error
wrays_gl_occlusion_buffer_destroy(wr_handle handle, wr_handle buffer);

wr_error
wrays_gl_ray_buffer_requirements(wr_handle handle, wr_buffer_info* buffer_info,
                                 wr_size* dimensions, wr_size dimension_count);

wr_error
wrays_gl_intersection_buffer_requirements(wr_handle       handle,
                                          wr_buffer_info* buffer_info,
                                          wr_size*        dimensions,
                                          wr_size         dimension_count);

wr_error
wrays_gl_occlusion_buffer_requirements(wr_handle       handle,
                                       wr_buffer_info* buffer_info,
                                       wr_size*        dimensions,
                                       wr_size         dimension_count);

wr_error
wrays_gl_ads_create(wr_handle handle, wr_handle ads_id,
                    wr_ads_descriptor* options, int options_count);

#endif

#ifndef WRAYS_EMSCRIPTEN
#define WR_FUN_EXPORT extern
WR_FUN_EXPORT PFNGLCOPYTEXSUBIMAGE3DPROC wrCopyTexSubImage3D;
WR_FUN_EXPORT PFNGLDRAWRANGEELEMENTSPROC wrDrawRangeElements;
WR_FUN_EXPORT PFNGLTEXIMAGE3DPROC wrTexImage3D;
WR_FUN_EXPORT PFNGLTEXSUBIMAGE3DPROC wrTexSubImage3D;

WR_FUN_EXPORT PFNGLACTIVETEXTUREPROC wrActiveTexture;
WR_FUN_EXPORT PFNGLCOMPRESSEDTEXIMAGE2DPROC wrCompressedTexImage2D;
WR_FUN_EXPORT PFNGLCOMPRESSEDTEXIMAGE3DPROC wrCompressedTexImage3D;
WR_FUN_EXPORT PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC wrCompressedTexSubImage2D;
WR_FUN_EXPORT PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC wrCompressedTexSubImage3D;
WR_FUN_EXPORT PFNGLSAMPLECOVERAGEPROC wrSampleCoverage;

WR_FUN_EXPORT PFNGLBLENDCOLORPROC wrBlendColor;
WR_FUN_EXPORT PFNGLBLENDEQUATIONPROC wrBlendEquation;
WR_FUN_EXPORT PFNGLBLENDFUNCSEPARATEPROC wrBlendFuncSeparate;

WR_FUN_EXPORT PFNGLBEGINQUERYPROC wrBeginQuery;
WR_FUN_EXPORT PFNGLBINDBUFFERPROC wrBindBuffer;
WR_FUN_EXPORT PFNGLBUFFERDATAPROC wrBufferData;
WR_FUN_EXPORT PFNGLBUFFERSUBDATAPROC wrBufferSubData;
WR_FUN_EXPORT PFNGLDELETEBUFFERSPROC wrDeleteBuffers;
WR_FUN_EXPORT PFNGLDELETEQUERIESPROC wrDeleteQueries;
WR_FUN_EXPORT PFNGLENDQUERYPROC wrEndQuery;
WR_FUN_EXPORT PFNGLGENBUFFERSPROC wrGenBuffers;
WR_FUN_EXPORT PFNGLGENQUERIESPROC wrGenQueries;
WR_FUN_EXPORT PFNGLGETBUFFERPARAMETERIVPROC wrGetBufferParameteriv;
WR_FUN_EXPORT PFNGLGETBUFFERPOINTERVPROC wrGetBufferPointerv;
WR_FUN_EXPORT PFNGLGETQUERYOBJECTUIVPROC wrGetQueryObjectuiv;
WR_FUN_EXPORT PFNGLGETQUERYIVPROC wrGetQueryiv;
WR_FUN_EXPORT PFNGLISBUFFERPROC wrIsBuffer;
WR_FUN_EXPORT PFNGLISQUERYPROC wrIsQuery;
WR_FUN_EXPORT PFNGLUNMAPBUFFERPROC wrUnmapBuffer;

WR_FUN_EXPORT PFNGLATTACHSHADERPROC wrAttachShader;
WR_FUN_EXPORT PFNGLBINDATTRIBLOCATIONPROC wrBindAttribLocation;
WR_FUN_EXPORT PFNGLBLENDEQUATIONSEPARATEPROC wrBlendEquationSeparate;
WR_FUN_EXPORT PFNGLCOMPILESHADERPROC wrCompileShader;
WR_FUN_EXPORT PFNGLCREATEPROGRAMPROC wrCreateProgram;
WR_FUN_EXPORT PFNGLCREATESHADERPROC wrCreateShader;
WR_FUN_EXPORT PFNGLDELETEPROGRAMPROC wrDeleteProgram;
WR_FUN_EXPORT PFNGLDELETESHADERPROC wrDeleteShader;
WR_FUN_EXPORT PFNGLDETACHSHADERPROC wrDetachShader;
WR_FUN_EXPORT PFNGLDISABLEVERTEXATTRIBARRAYPROC wrDisableVertexAttribArray;
WR_FUN_EXPORT PFNGLDRAWBUFFERSPROC wrDrawBuffers;
WR_FUN_EXPORT PFNGLENABLEVERTEXATTRIBARRAYPROC wrEnableVertexAttribArray;
WR_FUN_EXPORT PFNGLGETACTIVEATTRIBPROC wrGetActiveAttrib;
WR_FUN_EXPORT PFNGLGETACTIVEUNIFORMPROC wrGetActiveUniform;
WR_FUN_EXPORT PFNGLGETATTACHEDSHADERSPROC wrGetAttachedShaders;
WR_FUN_EXPORT PFNGLGETATTRIBLOCATIONPROC wrGetAttribLocation;
WR_FUN_EXPORT PFNGLGETPROGRAMINFOLOGPROC wrGetProgramInfoLog;
WR_FUN_EXPORT PFNGLGETPROGRAMIVPROC wrGetProgramiv;
WR_FUN_EXPORT PFNGLGETSHADERINFOLOGPROC wrGetShaderInfoLog;
WR_FUN_EXPORT PFNGLGETSHADERSOURCEPROC wrGetShaderSource;
WR_FUN_EXPORT PFNGLGETSHADERIVPROC wrGetShaderiv;
WR_FUN_EXPORT PFNGLGETUNIFORMLOCATIONPROC wrGetUniformLocation;
WR_FUN_EXPORT PFNGLGETUNIFORMFVPROC wrGetUniformfv;
WR_FUN_EXPORT PFNGLGETUNIFORMIVPROC wrGetUniformiv;
WR_FUN_EXPORT PFNGLGETVERTEXATTRIBPOINTERVPROC wrGetVertexAttribPointerv;
WR_FUN_EXPORT PFNGLGETVERTEXATTRIBFVPROC wrGetVertexAttribfv;
WR_FUN_EXPORT PFNGLGETVERTEXATTRIBIVPROC wrGetVertexAttribiv;
WR_FUN_EXPORT PFNGLISPROGRAMPROC wrIsProgram;
WR_FUN_EXPORT PFNGLISSHADERPROC wrIsShader;
WR_FUN_EXPORT PFNGLLINKPROGRAMPROC wrLinkProgram;
WR_FUN_EXPORT PFNGLSHADERSOURCEPROC wrShaderSource;
WR_FUN_EXPORT PFNGLSTENCILFUNCSEPARATEPROC wrStencilFuncSeparate;
WR_FUN_EXPORT PFNGLSTENCILMASKSEPARATEPROC wrStencilMaskSeparate;
WR_FUN_EXPORT PFNGLSTENCILOPSEPARATEPROC wrStencilOpSeparate;
WR_FUN_EXPORT PFNGLUNIFORM1FPROC wrUniform1f;
WR_FUN_EXPORT PFNGLUNIFORM1FVPROC wrUniform1fv;
WR_FUN_EXPORT PFNGLUNIFORM1IPROC wrUniform1i;
WR_FUN_EXPORT PFNGLUNIFORM1IVPROC wrUniform1iv;
WR_FUN_EXPORT PFNGLUNIFORM2FPROC wrUniform2f;
WR_FUN_EXPORT PFNGLUNIFORM2FVPROC wrUniform2fv;
WR_FUN_EXPORT PFNGLUNIFORM2IPROC wrUniform2i;
WR_FUN_EXPORT PFNGLUNIFORM2IVPROC wrUniform2iv;
WR_FUN_EXPORT PFNGLUNIFORM3FPROC wrUniform3f;
WR_FUN_EXPORT PFNGLUNIFORM3FVPROC wrUniform3fv;
WR_FUN_EXPORT PFNGLUNIFORM3IPROC wrUniform3i;
WR_FUN_EXPORT PFNGLUNIFORM3IVPROC wrUniform3iv;
WR_FUN_EXPORT PFNGLUNIFORM4FPROC wrUniform4f;
WR_FUN_EXPORT PFNGLUNIFORM4FVPROC wrUniform4fv;
WR_FUN_EXPORT PFNGLUNIFORM4IPROC wrUniform4i;
WR_FUN_EXPORT PFNGLUNIFORM4IVPROC wrUniform4iv;
WR_FUN_EXPORT PFNGLUNIFORMMATRIX2FVPROC wrUniformMatrix2fv;
WR_FUN_EXPORT PFNGLUNIFORMMATRIX3FVPROC wrUniformMatrix3fv;
WR_FUN_EXPORT PFNGLUNIFORMMATRIX4FVPROC wrUniformMatrix4fv;
WR_FUN_EXPORT PFNGLUSEPROGRAMPROC wrUseProgram;
WR_FUN_EXPORT PFNGLVALIDATEPROGRAMPROC wrValidateProgram;
WR_FUN_EXPORT PFNGLVERTEXATTRIB1FPROC wrVertexAttrib1f;
WR_FUN_EXPORT PFNGLVERTEXATTRIB1FVPROC wrVertexAttrib1fv;
WR_FUN_EXPORT PFNGLVERTEXATTRIB3FPROC wrVertexAttrib3f;
WR_FUN_EXPORT PFNGLVERTEXATTRIB3FVPROC wrVertexAttrib3fv;
WR_FUN_EXPORT PFNGLVERTEXATTRIB4FPROC wrVertexAttrib4f;
WR_FUN_EXPORT PFNGLVERTEXATTRIB4FVPROC wrVertexAttrib4fv;
WR_FUN_EXPORT PFNGLVERTEXATTRIBPOINTERPROC wrVertexAttribPointer;

WR_FUN_EXPORT PFNGLUNIFORMMATRIX2X3FVPROC wrUniformMatrix2x3fv;
WR_FUN_EXPORT PFNGLUNIFORMMATRIX2X4FVPROC wrUniformMatrix2x4fv;
WR_FUN_EXPORT PFNGLUNIFORMMATRIX3X2FVPROC wrUniformMatrix3x2fv;
WR_FUN_EXPORT PFNGLUNIFORMMATRIX3X4FVPROC wrUniformMatrix3x4fv;
WR_FUN_EXPORT PFNGLUNIFORMMATRIX4X2FVPROC wrUniformMatrix4x2fv;
WR_FUN_EXPORT PFNGLUNIFORMMATRIX4X3FVPROC wrUniformMatrix4x3fv;

WR_FUN_EXPORT PFNGLBEGINTRANSFORMFEEDBACKPROC wrBeginTransformFeedback;
WR_FUN_EXPORT PFNGLCLEARBUFFERFIPROC wrClearBufferfi;
WR_FUN_EXPORT PFNGLCLEARBUFFERFVPROC wrClearBufferfv;
WR_FUN_EXPORT PFNGLCLEARBUFFERIVPROC wrClearBufferiv;
WR_FUN_EXPORT PFNGLCLEARBUFFERUIVPROC wrClearBufferuiv;
WR_FUN_EXPORT PFNGLENDTRANSFORMFEEDBACKPROC wrEndTransformFeedback;
WR_FUN_EXPORT PFNGLGETFRAGDATALOCATIONPROC wrGetFragDataLocation;
WR_FUN_EXPORT PFNGLGETSTRINGIPROC wrGetStringi;
WR_FUN_EXPORT                     PFNGLGETTRANSFORMFEEDBACKVARYINGPROC
                                  wrGetTransformFeedbackVarying;
WR_FUN_EXPORT PFNGLGETUNIFORMUIVPROC wrGetUniformuiv;
WR_FUN_EXPORT PFNGLGETVERTEXATTRIBIIVPROC wrGetVertexAttribIiv;
WR_FUN_EXPORT PFNGLGETVERTEXATTRIBIUIVPROC wrGetVertexAttribIuiv;
WR_FUN_EXPORT PFNGLTRANSFORMFEEDBACKVARYINGSPROC wrTransformFeedbackVaryings;
WR_FUN_EXPORT PFNGLUNIFORM1UIPROC wrUniform1ui;
WR_FUN_EXPORT PFNGLUNIFORM1UIVPROC wrUniform1uiv;
WR_FUN_EXPORT PFNGLUNIFORM2UIPROC wrUniform2ui;
WR_FUN_EXPORT PFNGLUNIFORM2UIVPROC wrUniform2uiv;
WR_FUN_EXPORT PFNGLUNIFORM3UIPROC wrUniform3ui;
WR_FUN_EXPORT PFNGLUNIFORM3UIVPROC wrUniform3uiv;
WR_FUN_EXPORT PFNGLUNIFORM4UIPROC wrUniform4ui;
WR_FUN_EXPORT PFNGLUNIFORM4UIVPROC wrUniform4uiv;
WR_FUN_EXPORT PFNGLVERTEXATTRIBI4IPROC wrVertexAttribI4i;
WR_FUN_EXPORT PFNGLVERTEXATTRIBI4IVPROC wrVertexAttribI4iv;
WR_FUN_EXPORT PFNGLVERTEXATTRIBI4UIPROC wrVertexAttribI4ui;
WR_FUN_EXPORT PFNGLVERTEXATTRIBI4UIVPROC wrVertexAttribI4uiv;
WR_FUN_EXPORT PFNGLVERTEXATTRIBIPOINTERPROC wrVertexAttribIPointer;

WR_FUN_EXPORT PFNGLDRAWARRAYSINSTANCEDPROC wrDrawArraysInstanced;
WR_FUN_EXPORT PFNGLDRAWELEMENTSINSTANCEDPROC wrDrawElementsInstanced;

WR_FUN_EXPORT PFNGLGETBUFFERPARAMETERI64VPROC wrGetBufferParameteri64v;
WR_FUN_EXPORT PFNGLGETINTEGER64I_VPROC wrGetInteger64i_v;

WR_FUN_EXPORT PFNGLVERTEXATTRIBDIVISORPROC wrVertexAttribDivisor;

WR_FUN_EXPORT PFNGLCLEARDEPTHFPROC wrClearDepthf;
WR_FUN_EXPORT PFNGLDEPTHRANGEFPROC wrDepthRangef;
WR_FUN_EXPORT PFNGLGETSHADERPRECISIONFORMATPROC wrGetShaderPrecisionFormat;
WR_FUN_EXPORT PFNGLRELEASESHADERCOMPILERPROC wrReleaseShaderCompiler;
WR_FUN_EXPORT PFNGLSHADERBINARYPROC wrShaderBinary;

WR_FUN_EXPORT PFNGLCOPYBUFFERSUBDATAPROC wrCopyBufferSubData;

WR_FUN_EXPORT PFNGLVIEWPORTPROC wrViewport;
WR_FUN_EXPORT PFNGLTEXSTORAGE3DPROC wrTexStorage3D;
WR_FUN_EXPORT PFNGLTEXPARAMETERIPROC wrTexParameteri;
WR_FUN_EXPORT PFNGLGETINTEGERVPROC wrGetIntegerv;
WR_FUN_EXPORT PFNGLGENVERTEXARRAYSPROC wrGenVertexArrays;
WR_FUN_EXPORT PFNGLISTEXTUREPROC wrIsTexture;
WR_FUN_EXPORT PFNGLGENTEXTURESPROC wrGenTextures;
WR_FUN_EXPORT PFNGLDELETETEXTURESPROC wrDeleteTextures;
WR_FUN_EXPORT PFNGLBINDTEXTUREPROC wrBindTexture;
WR_FUN_EXPORT PFNGLDRAWARRAYSPROC wrDrawArrays;
WR_FUN_EXPORT PFNGLBINDVERTEXARRAYPROC wrBindVertexArray;
WR_FUN_EXPORT PFNGLGETERRORPROC wrGetError;

WR_FUN_EXPORT PFNGLBINDFRAMEBUFFERPROC wrBindFramebuffer;
WR_FUN_EXPORT PFNGLBINDRENDERBUFFERPROC wrBindRenderbuffer;
WR_FUN_EXPORT PFNGLBLITFRAMEBUFFERPROC wrBlitFramebuffer;
WR_FUN_EXPORT PFNGLCHECKFRAMEBUFFERSTATUSPROC wrCheckFramebufferStatus;
WR_FUN_EXPORT PFNGLDELETEFRAMEBUFFERSPROC wrDeleteFramebuffers;
WR_FUN_EXPORT PFNGLDELETERENDERBUFFERSPROC wrDeleteRenderbuffers;
WR_FUN_EXPORT PFNGLFRAMEBUFFERRENDERBUFFERPROC wrFramebufferRenderbuffer;
WR_FUN_EXPORT PFNGLFRAMEBUFFERTEXTURE2DPROC wrFramebufferTexture2D;
WR_FUN_EXPORT PFNGLFRAMEBUFFERTEXTURELAYERPROC wrFramebufferTextureLayer;
WR_FUN_EXPORT PFNGLGENFRAMEBUFFERSPROC wrGenFramebuffers;
WR_FUN_EXPORT PFNGLGENRENDERBUFFERSPROC wrGenRenderbuffers;
WR_FUN_EXPORT PFNGLGENERATEMIPMAPPROC wrGenerateMipmap;
WR_FUN_EXPORT PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC
              wrGetFramebufferAttachmentParameteriv;
WR_FUN_EXPORT PFNGLGETRENDERBUFFERPARAMETERIVPROC wrGetRenderbufferParameteriv;
WR_FUN_EXPORT PFNGLISFRAMEBUFFERPROC wrIsFramebuffer;
WR_FUN_EXPORT PFNGLISRENDERBUFFERPROC wrIsRenderbuffer;
WR_FUN_EXPORT PFNGLRENDERBUFFERSTORAGEPROC wrRenderbufferStorage;
WR_FUN_EXPORT PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC
              wrRenderbufferStorageMultisample;

#define glCopyTexSubImage3D wrCopyTexSubImage3D
#define glDrawRangeElements wrDrawRangeElements
#define glTexImage3D wrTexImage3D
#define glTexSubImage3D wrTexSubImage3D
#define glActiveTexture wrActiveTexture
#define glCompressedTexImage2D wrCompressedTexImage2D
#define glCompressedTexImage3D wrCompressedTexImage3D
#define glCompressedTexSubImage2D wrCompressedTexSubImage2D
#define glCompressedTexSubImage3D wrCompressedTexSubImage3D
#define glSampleCoverage wrSampleCoverage
#define glBlendColor wrBlendColor
#define glBlendEquation wrBlendEquation
#define glBlendFuncSeparate wrBlendFuncSeparate
#define glBeginQuery wrBeginQuery
#define glBindBuffer wrBindBuffer
#define glBufferData wrBufferData
#define glBufferSubData wrBufferSubData
#define glDeleteBuffers wrDeleteBuffers
#define glDeleteQueries wrDeleteQueries
#define glEndQuery wrEndQuery
#define glGenBuffers wrGenBuffers
#define glGenQueries wrGenQueries
#define glGetBufferParameteriv wrGetBufferParameteriv
#define glGetBufferPointerv wrGetBufferPointerv
#define glGetQueryObjectuiv wrGetQueryObjectuiv
#define glGetQueryiv wrGetQueryiv
#define glIsBuffer wrIsBuffer
#define glIsQuery wrIsQuery
#define glUnmapBuffer wrUnmapBuffer
#define glAttachShader wrAttachShader
#define glBindAttribLocation wrBindAttribLocation
#define glBlendEquationSeparate wrBlendEquationSeparate
#define glCompileShader wrCompileShader
#define glCreateProgram wrCreateProgram
#define glCreateShader wrCreateShader
#define glDeleteProgram wrDeleteProgram
#define glDeleteShader wrDeleteShader
#define glDetachShader wrDetachShader
#define glDisableVertexAttribArray wrDisableVertexAttribArray
#define glDrawBuffers wrDrawBuffers
#define glEnableVertexAttribArray wrEnableVertexAttribArray
#define glGetActiveAttrib wrGetActiveAttrib
#define glGetActiveUniform wrGetActiveUniform
#define glGetAttachedShaders wrGetAttachedShaders
#define glGetAttribLocation wrGetAttribLocation
#define glGetProgramInfoLog wrGetProgramInfoLog
#define glGetProgramiv wrGetProgramiv
#define glGetShaderInfoLog wrGetShaderInfoLog
#define glGetShaderSource wrGetShaderSource
#define glGetShaderiv wrGetShaderiv
#define glGetUniformLocation wrGetUniformLocation
#define glGetUniformfv wrGetUniformfv
#define glGetUniformiv wrGetUniformiv
#define glGetVertexAttribPointerv wrGetVertexAttribPointerv
#define glGetVertexAttribfv wrGetVertexAttribfv
#define glGetVertexAttribiv wrGetVertexAttribiv
#define glIsProgram wrIsProgram
#define glIsShader wrIsShader
#define glLinkProgram wrLinkProgram
#define glShaderSource wrShaderSource
#define glStencilFuncSeparate wrStencilFuncSeparate
#define glStencilMaskSeparate wrStencilMaskSeparate
#define glStencilOpSeparate wrStencilOpSeparate
#define glUniform1f wrUniform1f
#define glUniform1fv wrUniform1fv
#define glUniform1i wrUniform1i
#define glUniform1iv wrUniform1iv
#define glUniform2f wrUniform2f
#define glUniform2fv wrUniform2fv
#define glUniform2i wrUniform2i
#define glUniform2iv wrUniform2iv
#define glUniform3f wrUniform3f
#define glUniform3fv wrUniform3fv
#define glUniform3i wrUniform3i
#define glUniform3iv wrUniform3iv
#define glUniform4f wrUniform4f
#define glUniform4fv wrUniform4fv
#define glUniform4i wrUniform4i
#define glUniform4iv wrUniform4iv
#define glUniformMatrix2fv wrUniformMatrix2fv
#define glUniformMatrix3fv wrUniformMatrix3fv
#define glUniformMatrix4fv wrUniformMatrix4fv
#define glUseProgram wrUseProgram
#define glValidateProgram wrValidateProgram
#define glVertexAttrib1f wrVertexAttrib1f
#define glVertexAttrib1fv wrVertexAttrib1fv
#define glVertexAttrib3f wrVertexAttrib3f
#define glVertexAttrib3fv wrVertexAttrib3fv
#define glVertexAttrib4f wrVertexAttrib4f
#define glVertexAttrib4fv wrVertexAttrib4fv
#define glVertexAttribPointer wrVertexAttribPointer
#define glUniformMatrix2x3fv wrUniformMatrix2x3fv
#define glUniformMatrix2x4fv wrUniformMatrix2x4fv
#define glUniformMatrix3x2fv wrUniformMatrix3x2fv
#define glUniformMatrix3x4fv wrUniformMatrix3x4fv
#define glUniformMatrix4x2fv wrUniformMatrix4x2fv
#define glUniformMatrix4x3fv wrUniformMatrix4x3fv
#define glBeginTransformFeedback wrBeginTransformFeedback
#define glClearBufferfi wrClearBufferfi
#define glClearBufferfv wrClearBufferfv
#define glClearBufferiv wrClearBufferiv
#define glClearBufferuiv wrClearBufferuiv
#define glEndTransformFeedback wrEndTransformFeedback
#define glGetFragDataLocation wrGetFragDataLocation
#define glGetStringi wrGetStringi
#define glGetTransformFeedbackVarying wrGetTransformFeedbackVarying
#define glGetUniformuiv wrGetUniformuiv
#define glGetVertexAttribIiv wrGetVertexAttribIiv
#define glGetVertexAttribIuiv wrGetVertexAttribIuiv
#define glTransformFeedbackVaryings wrTransformFeedbackVaryings
#define glUniform1ui wrUniform1ui
#define glUniform1uiv wrUniform1uiv
#define glUniform2ui wrUniform2ui
#define glUniform2uiv wrUniform2uiv
#define glUniform3ui wrUniform3ui
#define glUniform3uiv wrUniform3uiv
#define glUniform4ui wrUniform4ui
#define glUniform4uiv wrUniform4uiv
#define glVertexAttribI4i wrVertexAttribI4i
#define glVertexAttribI4iv wrVertexAttribI4iv
#define glVertexAttribI4ui wrVertexAttribI4ui
#define glVertexAttribI4uiv wrVertexAttribI4uiv
#define glVertexAttribIPointer wrVertexAttribIPointer
#define glDrawArraysInstanced wrDrawArraysInstanced
#define glDrawElementsInstanced wrDrawElementsInstanced
#define glGetBufferParameteri64v wrGetBufferParameteri64v
#define glGetInteger64i_v wrGetInteger64i_v
#define glVertexAttribDivisor wrVertexAttribDivisor
#define glClearDepthf wrClearDepthf
#define glDepthRangef wrDepthRangef
#define glGetShaderPrecisionFormat wrGetShaderPrecisionFormat
#define glReleaseShaderCompiler wrReleaseShaderCompiler
#define glShaderBinary wrShaderBinary
#define glCopyBufferSubData wrCopyBufferSubData
#define glViewport wrViewport
#define glTexStorage3D wrTexStorage3D
#define glTexParameteri wrTexParameteri
#define glGetIntegerv wrGetIntegerv
#define glGenVertexArrays wrGenVertexArrays
#define glIsTexture wrIsTexture
#define glGenTextures wrGenTextures
#define glDeleteTextures wrDeleteTextures
#define glBindTexture wrBindTexture
#define glDrawArrays wrDrawArrays
#define glBindVertexArray wrBindVertexArray
#define glGetError wrGetError
#define glBindFramebuffer wrBindFramebuffer
#define glBindRenderbuffer wrBindRenderbuffer
#define glBlitFramebuffer wrBlitFramebuffer
#define glCheckFramebufferStatus wrCheckFramebufferStatus
#define glDeleteFramebuffers wrDeleteFramebuffers
#define glDeleteRenderbuffers wrDeleteRenderbuffers
#define glFramebufferRenderbuffer wrFramebufferRenderbuffer
#define glFramebufferTexture2D wrFramebufferTexture2D
#define glFramebufferTextureLayer wrFramebufferTextureLayer
#define glGenFramebuffers wrGenFramebuffers
#define glGenRenderbuffers wrGenRenderbuffers
#define glGenerateMipmap wrGenerateMipmap
#define glGetFramebufferAttachmentParameteriv                                  \
  wrGetFramebufferAttachmentParameteriv
#define glGetRenderbufferParameteriv wrGetRenderbufferParameteriv
#define glIsFramebuffer wrIsFramebuffer
#define glIsRenderbuffer wrIsRenderbuffer
#define glRenderbufferStorage wrRenderbufferStorage
#define glRenderbufferStorageMultisample wrRenderbufferStorageMultisample
#endif /* WRAYS_EMSCRIPTEN */

#endif /* _WRAYS_GL_H_ */
