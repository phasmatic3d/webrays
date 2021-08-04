
find_library(OPENGLES3_LIBRARY NAMES
    GLESv3

    # On some platforms (e.g. desktop emulation with Mesa or NVidia) ES3
    # support is provided in ES2 lib
    GLESv2

    # ANGLE (CMake doesn't search for lib prefix on Windows)
    libGLESv2

    # iOS
    OpenGLES)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args("OpenGLES3" DEFAULT_MSG
    OPENGLES3_LIBRARY)
set(GLES_LIBRARY "${OPENGLES3_LIBRARY}")

mark_as_advanced(OPENGLES3_LIBRARY)