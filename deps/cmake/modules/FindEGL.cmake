find_library(EGL_LIBRARY NAMES
        EGL

        # ANGLE (CMake doesn't search for lib prefix on Windows)
        libEGL

        # On iOS a part of OpenGLES
        OpenGLES)

# Include dir
find_path(EGL_INCLUDE_DIR NAMES
    EGL/egl.h

    # iOS
    EAGL.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EGL DEFAULT_MSG
    EGL_LIBRARY
    EGL_INCLUDE_DIR)

#mark_as_advanced(EGL_LIBRARY EGL_INCLUDE_DIR)