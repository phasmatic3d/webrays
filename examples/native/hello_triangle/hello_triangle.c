#include <SDL.h>
#include <SDL_syswm.h>

/* Embeded GL */
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglext_angle.h>
#include <EGL/eglplatform.h>

#include <webrays/webrays.h>

/* Embeded GL Version 3 */
#define GL_GLEXT_PROTOTYPES
#include <GLES3/gl3.h>
#ifdef USE_TIMERS
#include <GLES2/gl2ext.h>
#endif

#include <stdio.h>

#define VIEWER_WIDTH 512
#define VIEWER_HEIGHT 512
#define VIEWER_TITLE "Hello Triangle"
#define VIEWER_GL_VERSION "#version 300 es\n"
#define VIEWER_D3D11 1
#define VIEWER_GL 1

typedef struct
{
  float x, y, z;
} vec3;

typedef struct
{
  float x, y;
} vec2;

typedef struct
{
  char viewer_title[128];
  char viewer_rsrc_path[128];

  char screen_fill_vs[128];
  char hello_fs[128];

  wr_handle         webrays;
  wr_handle         ads;
  const char*       ads_accessor;
  const wr_binding* ads_bindings;
  wr_size           ads_binding_count;

  SDL_GLContext viewer_gl_context;
  SDL_Window*   viewer_window;
  EGLContext    egl_context;
  EGLDisplay    egl_display;
  EGLSurface    egl_surface;

  int viewer_width;
  int viewer_height;
  int viewer_alive;

  int egl_major_version;
  int egl_minor_version;
  int gles_major_version;
  int gles_minor_version;

  GLuint vao;
  GLuint vbo;

  GLuint hello_program;

  vec3  camera_pos;
  vec3  camera_front;
  vec3  camera_up;
  float camera_fov;

} app_context;

int
sdl_gl_update(app_context* app);
int
sdl_gl_init(app_context* app);
int
sdl_gl_swap_buffers(app_context* app);
int
sdl_gl_get_drawable_size(app_context* app, int* width, int* height);

static int
sdl_gl_shader_create(GLuint* shader_handle, GLsizei count,
                     const char** shader_source, GLenum shader_type)
{
  *shader_handle = -1;
  GLint compiled = GL_FALSE;

  *shader_handle = glCreateShader(shader_type);
  glShaderSource(*shader_handle, count, shader_source, 0);
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

  return 0;
}

static int
sdl_gl_program_create(GLuint* program_handle, GLuint vertex_shader,
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

  return 0;
}

static size_t
sdl_file_size(const char* filename)
{
  FILE*  file_handle  = 0;
  size_t content_size = 0;

  file_handle = fopen(filename, "rb");
  if (!file_handle) {
    return content_size;
  }

  fseek(file_handle, 0L, SEEK_END);
  content_size = ftell(file_handle);
  fclose(file_handle);

  return content_size;
}

static size_t
sdl_file_contents(const char* filename, char* buffer, size_t count)
{
  FILE*  file_handle  = 0;
  size_t content_size = 0;

  file_handle = fopen(filename, "rb");
  if (!file_handle) {
    return content_size;
  }

  content_size = fread(buffer, 1, count, file_handle);

  fclose(file_handle);

  return content_size;
}

int
main(int argc, char* argv[])
{
  wr_error     err = WR_SUCCESS;
  app_context  app_placement;
  app_context* app = &app_placement;
  memset(app, 0, sizeof(*app));

  strcpy(app->viewer_title, VIEWER_TITLE);
  app->viewer_width  = VIEWER_WIDTH;
  app->viewer_height = VIEWER_HEIGHT;
  app->viewer_alive  = WR_TRUE;

  sdl_gl_init(app);

  /* After initializing with EGL and having made the ANGLE context current */
  app->webrays = wrays_init(WR_BACKEND_TYPE_GLES, WR_NULL);

  /* Qucickly extract the resource path */
  const char* resources_separator = WR_NULL;
  const char* riterator           = argv[0];
  char*       witerator           = app->viewer_rsrc_path;
  while (*riterator) {
    resources_separator = (*riterator == '/' || *riterator == '\\') ? riterator : resources_separator;
    *witerator++        = *riterator++;
  }
  if (resources_separator) {
    app->viewer_rsrc_path[resources_separator - argv[0]] = 0;
  } else {
    app->viewer_rsrc_path[0] = '.';
  }
  
  /* After initializing with EGL and having made the ANGLE context current */
  app->webrays = wrays_init(WR_BACKEND_TYPE_GLES, WR_NULL);

  /* Set camera */
  app->camera_pos   = (vec3){ 0.0, 0.5, -1.0 };
  app->camera_front = (vec3){ 0.0, 0.0, 1.0 };
  app->camera_up    = (vec3){ 0.0, 1.0, 0.0 };
  app->camera_fov   = 90.0f;

  /* Set Shaders */
  sprintf(app->hello_fs, "%s/%s", app->viewer_rsrc_path,
          "kernels/hello.fs.glsl");
  sprintf(app->screen_fill_vs, "%s/%s", app->viewer_rsrc_path,
          "kernels/screen_fill.vs.glsl");

  wr_ads_descriptor descriptors[] = { { "type", "BLAS" } };
  wr_size           descriptor_count =
    (wr_size)(sizeof(descriptors) / sizeof(descriptors[0]));

  err =
    wrays_create_ads(app->webrays, &app->ads, descriptors, descriptor_count);

  glGenVertexArrays(1, &app->vao);
  glBindVertexArray(app->vao);
  float screen_fill_triangle[][3] = { { -4.0f, -4.0f, 0.0f },
                                      { 4.0f, -4.0f, 0.0f },
                                      { 0.0f, 4.0f, 0.0f } };
  glGenBuffers(1, &app->vbo);
  glBindBuffer(GL_ARRAY_BUFFER, app->vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(screen_fill_triangle),
               screen_fill_triangle, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  int  faces[]    = { 0, 1, 2, 0 };
  vec3 vertices[] = { { -1.0f, 0.0f, 0.0f },
                      { 1.0f, 0.0f, 0.0f },
                      { 0.0f, 1.0f, 0.0f } };
  vec3 normals[]  = { { 0.0f, 0.0f, -1.0f },
                     { 0.0f, 0.0f, -1.0f },
                     { 0.0f, 0.0f, -1.0f } };
  vec2 uvs[]      = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 0.5f } };

  int shape_id = 0;
  err          = wrays_add_shape(app->webrays, app->ads, (float*)vertices, 3,
                        (float*)normals, 3, (float*)uvs, 2, 3, (int*)faces, 1,
                        &shape_id);

  while (app->viewer_alive) {

    wr_update_flags flags;
    err = wrays_update(app->webrays, &flags);
    if (WR_UPDATE_FLAG_NONE != flags)
      sdl_gl_update(app);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        app->viewer_alive = WR_FALSE;
      }

      switch (event.type) {
        case SDL_WINDOWEVENT: {
          // rg_sdl_handle_window_event(app_ctx, &event.window);
          break;
        }
        case SDL_MOUSEMOTION: break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
          switch (event.key.keysym.sym) {
            case SDLK_ESCAPE: app->viewer_alive = WR_FALSE; break;
          }
          break;
        case SDL_MOUSEWHEEL: break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: break;
        default: break;
      }
    }

    /* Handle high-DPI screens */
    int viewport_width, viewport_height;
    sdl_gl_get_drawable_size(app, &viewport_width,
                           &viewport_height);

    glViewport(0, 0, viewport_width, viewport_height);

    glBindVertexArray(app->vao);

    GLuint prog = app->hello_program;
    glUseProgram(prog);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glUniform1i(glGetUniformLocation(prog, "u_ADS"), (GLint)(size_t)app->ads);
    glUniform3fv(glGetUniformLocation(prog, "u_CameraPos"), 1,
                 (const GLfloat*)&app->camera_pos);
    glUniform3fv(glGetUniformLocation(prog, "u_CameraUp"), 1,
                 (const GLfloat*)&app->camera_up);
    glUniform3fv(glGetUniformLocation(prog, "u_CameraFront"), 1,
                 (const GLfloat*)&app->camera_front);
    glUniform1f(glGetUniformLocation(prog, "u_CameraFov"), app->camera_fov);
    glUniform2i(glGetUniformLocation(prog, "u_Dimensions"), viewport_width,
                viewport_height);

    GLint current_texture_unit = 0;
    for (wr_size i = 0; i < app->ads_binding_count; ++i) {
      GLint index = glGetUniformLocation(prog, app->ads_bindings[i].name);
      if (app->ads_bindings[i].type == WR_BINDING_TYPE_GL_TEXTURE_2D) {
        glActiveTexture(GL_TEXTURE0 + current_texture_unit);
        glBindTexture(GL_TEXTURE_2D, app->ads_bindings[i].data.texture);
        glUniform1i(index, current_texture_unit);
        ++current_texture_unit;
      } else if (app->ads_bindings[i].type ==
                 WR_BINDING_TYPE_GL_TEXTURE_2D_ARRAY) {
        glActiveTexture(GL_TEXTURE0 + current_texture_unit);
        glBindTexture(GL_TEXTURE_2D_ARRAY, app->ads_bindings[i].data.texture);
        glUniform1i(index, current_texture_unit);
        ++current_texture_unit;
      }
    }

    glBindVertexArray(0);
    glUseProgram(0);

    sdl_gl_swap_buffers(app);
  }

  /* Cleanup */
  eglDestroySurface(app->egl_display, app->egl_surface);
  eglDestroyContext(app->egl_display, app->egl_context);
  eglTerminate(app->egl_display);

  SDL_Quit();

  return 0;
}

int
sdl_gl_get_drawable_size(app_context* app, int* width, int* height)
{
#ifdef __APPLE__
  SDL_GL_GetDrawableSize(app->viewer_window, width, height);
#else
  *width = app->viewer_width;
  *height = app->viewer_height;
#endif
  return 1;
}

int
sdl_gl_init(app_context* app)
{
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("FAILED <-> Could not initialize SDL\n");
    return -1;
  }

#ifdef __APPLE__
  // Create window
  SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_EGL, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

  // Explicitly set channel depths, otherwise we might get some < 8
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  uint32_t windowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;
  app->viewer_window   = SDL_CreateWindow(
    app->viewer_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    app->viewer_width, app->viewer_height, windowFlags);

  // Init GL
  SDL_GLContext glContext = SDL_GL_CreateContext(app->viewer_window);
  SDL_GL_MakeCurrent(app->viewer_window, glContext);
#else

  app->viewer_window = SDL_CreateWindow(
    app->viewer_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    app->viewer_width, app->viewer_height, SDL_WINDOW_RESIZABLE);

  if (WR_NULL == app->viewer_window) {
    printf("FAILED <-> Could not initialize SDL Window\n");
    return -1;
  }

  /* Highly Win32 specific code */
  EGLBoolean egl_successful = EGL_FALSE;

  /* Finer control over created EGLContext */
#if VIEWER_D3D11 && defined(WIN32)
  EGLint display_attributes[] = { EGL_PLATFORM_ANGLE_TYPE_ANGLE,
                                  EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
                                  EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE,
                                  11,
                                  EGL_PLATFORM_ANGLE_MAX_VERSION_MINOR_ANGLE,
                                  0,
                                  EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE,
                                  EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE,
                                  EGL_NONE };
#else
  EGLint display_attributes[] = { EGL_PLATFORM_ANGLE_TYPE_ANGLE,
                                  EGL_PLATFORM_ANGLE_TYPE_OPENGL_ANGLE,
                                  EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE,
                                  3,
                                  EGL_PLATFORM_ANGLE_MAX_VERSION_MINOR_ANGLE,
                                  0,
                                  EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE,
                                  EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE,
                                  EGL_NONE };
#endif

  /* For some reason, we need to specifically use the EXT variant */
  app->egl_display = eglGetPlatformDisplayEXT(
    EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, display_attributes);
  if (EGL_NO_DISPLAY == app->egl_display) {
    printf("FAILED <-> eglGetPlatformDisplayEXT failed\n");
    return -1;
  }

  /* Initialize EGL for this display, returns EGL version */
  egl_successful = eglInitialize(app->egl_display, &app->egl_major_version,
                                 &app->egl_minor_version);
  if (!egl_successful) {
    printf("FAILED <-> eglInitialize failed\n");
    return -1;
  }

  egl_successful = eglBindAPI(EGL_OPENGL_ES_API);
  if (!egl_successful) {
    printf("FAILED <-> eglBindAPI failed\n");
    return -1;
  }

  EGLint configAttributes[] = { EGL_BUFFER_SIZE,
                                0,
                                EGL_RED_SIZE,
                                8,
                                EGL_GREEN_SIZE,
                                8,
                                EGL_BLUE_SIZE,
                                8,
                                EGL_COLOR_BUFFER_TYPE,
                                EGL_RGB_BUFFER,
                                EGL_DEPTH_SIZE,
                                24,
                                EGL_LEVEL,
                                0,
                                EGL_RENDERABLE_TYPE,
                                EGL_OPENGL_ES3_BIT,
                                EGL_STENCIL_SIZE,
                                8,
                                EGL_SURFACE_TYPE,
                                EGL_WINDOW_BIT,
                                EGL_NONE };

  EGLint    numConfigs;
  EGLConfig windowConfig;
  egl_successful = eglChooseConfig(app->egl_display, configAttributes,
                                   &windowConfig, 1, &numConfigs);
  if (!egl_successful) {
    printf("FAILED <-> eglChooseConfig failed\n");
    return -1;
  }

  struct SDL_SysWMinfo wmInfo;
  SDL_VERSION(&wmInfo.version);

  if (SDL_GetWindowWMInfo(app->viewer_window, &wmInfo) < 0) {
    printf("FAILED <-> Failed to get native handle for SDL Window\n");
    return -1;
  }

#ifdef WIN32
  EGLNativeWindowType window = (EGLNativeWindowType)wmInfo.info.win.window;
#endif

#ifdef __linux__
  EGLNativeWindowType window = (EGLNativeWindowType)wmInfo.info.x11.window;
#endif

  EGLint surfaceAttributes[] = { EGL_NONE };
  app->egl_surface = eglCreateWindowSurface(app->egl_display, windowConfig,
                                            window, surfaceAttributes);
  if (!app->egl_surface) {
    printf("FAILED <-> eglCreateContext failed\n");
    return -1;
  }

  EGLint contextAttributes[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
  app->egl_context           = eglCreateContext(app->egl_display, windowConfig,
                                      EGL_NO_CONTEXT, contextAttributes);
  if (!app->egl_context) {
    printf("FAILED <-> eglCreateContext failed\n");
    return -1;
  }

  egl_successful = eglSwapInterval(app->egl_display, 0);

  egl_successful = eglMakeCurrent(app->egl_display, app->egl_surface,
                                  app->egl_surface, app->egl_context);
  if (!egl_successful) {
    printf("FAILED <-> eglMakeCurrent failed\n");
    return -1;
  }

  egl_successful = eglSwapBuffers(app->egl_display, app->egl_surface);
  if (!egl_successful) {
    printf("FAILED <-> eglSwapBuffers failed\n");
    return -1;
  }
#endif /* __APPLE__ */
  return 1;
}

int
sdl_gl_swap_buffers(app_context* app)
{
#ifdef __APPLE__
  SDL_GL_SwapWindow(app->viewer_window);
#else  /* WIN32 || Linux */
  eglSwapBuffers(app->egl_display, app->egl_surface);
#endif /* __APPLE__ */
  return 1;
}

int
sdl_gl_update(app_context* app)
{
  GLuint      vertex_shader;
  GLuint      fragment_shader;
  size_t      content_size      = 0;
  char*       kernel_source     = WR_NULL;
  const char* shader_sources[3] = { 0 };

  shader_sources[0] = VIEWER_GL_VERSION;

  app->ads_accessor = wrays_get_scene_accessor(app->webrays);
  app->ads_bindings =
    wrays_get_scene_accessor_bindings(app->webrays, &app->ads_binding_count);

  content_size                = sdl_file_size(app->screen_fill_vs);
  kernel_source               = (char*)realloc(kernel_source, content_size + 1);
  kernel_source[content_size] = 0;
  sdl_file_contents(app->screen_fill_vs, kernel_source, content_size);

  shader_sources[1] = kernel_source;

  sdl_gl_shader_create(&vertex_shader, 2, shader_sources, GL_VERTEX_SHADER);

  content_size                = sdl_file_size(app->hello_fs);
  kernel_source               = (char*)realloc(kernel_source, content_size + 1);
  kernel_source[content_size] = 0;
  sdl_file_contents(app->hello_fs, kernel_source, content_size);

  shader_sources[1] = app->ads_accessor;
  shader_sources[2] = kernel_source;

  sdl_gl_shader_create(&fragment_shader, 3, shader_sources, GL_FRAGMENT_SHADER);

  sdl_gl_program_create(&app->hello_program, vertex_shader, fragment_shader);

  return 0;
}