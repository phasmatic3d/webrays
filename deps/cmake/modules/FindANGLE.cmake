#.rst:
# Find ANGLE
# ----------------
#
# Finds the OpenGL ES 3 library. This module defines:
#
#  OpenGLES3_FOUND          - True if OpenGL ES 3 library is found
#  OpenGLES3::OpenGLES3     - OpenGL ES 3 imported target
#
# Additionally these variables are defined for internal usage:
#
#  ANGLE_LIBRARY        - OpenGL ES 3 library
#
# Please note this find module is tailored especially for the needs of Magnum.
# In particular, it depends on its platform definitions and doesn't look for
# OpenGL ES includes as Magnum has its own, generated using flextGL.
#

#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
#               2020, 2021 Vladimír Vondruš <mosra@centrum.cz>
#
#   Permission is hereby granted, free of charge, to any person obtaining a
#   copy of this software and associated documentation files (the "Software"),
#   to deal in the Software without restriction, including without limitation
#   the rights to use, copy, modify, merge, publish, distribute, sublicense,
#   and/or sell copies of the Software, and to permit persons to whom the
#   Software is furnished to do so, subject to the following conditions:
#
#   The above copyright notice and this permission notice shall be included
#   in all copies or substantial portions of the Software.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#   DEALINGS IN THE SOFTWARE.
#

# Under Emscripten, GL is linked implicitly. With MINIMAL_RUNTIME you need to
# specify -lGL. Simply set the library name to that.

macro(SUBDIRLIST result curdir)
  file(GLOB children RELATIVE ${curdir} ${curdir}/*)
  set(dirlist "")
  foreach(child ${children})
    if(IS_DIRECTORY ${curdir}/${child})
      list(APPEND dirlist ${child})
    endif()
  endforeach()
  set(${result} ${dirlist})
endmacro()

set(PROGRAM_FILES_X86 "C:/Program Files (x86)")
set(PROGRAM_FILES "C:/Program Files")
set(FIREFOX_DIRECTORY "${PROGRAM_FILES}/Mozilla Firefox")
set(CHROME_DIRECTORY "${PROGRAM_FILES_X86}/Google/Chrome/Application")
set(EDGE_DIRECTORY "${PROGRAM_FILES_X86}/Microsoft/Edge/Application")

# First try Chrome, then Edge, then Firefox
SUBDIRLIST(CHROME_SUBDIRECTORIES ${CHROME_DIRECTORY})
foreach(subdir ${CHROME_SUBDIRECTORIES})
    if ( ${subdir} MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+)$" )
        set(CHROME_DIRECTORY "${CHROME_DIRECTORY}/${subdir}")

        find_file(EGL_LIBRARY libEGL.dll HINTS ${CHROME_DIRECTORY} PATHS ${CHROME_DIRECTORY} NO_CMAKE_FIND_ROOT_PATH)
        find_file(GLES_LIBRARY libGLESv2.dll HINTS ${CHROME_DIRECTORY} PATHS ${CHROME_DIRECTORY} NO_CMAKE_FIND_ROOT_PATH)
    endif()
endforeach()

SUBDIRLIST(EDGE_SUBDIRECTORIES ${EDGE_DIRECTORY})
foreach(subdir ${EDGE_SUBDIRECTORIES})
    if ( ${subdir} MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+)$" )
        set(EDGE_DIRECTORY "${EDGE_DIRECTORY}/${subdir}")
        
        find_file(EGL_LIBRARY libEGL.dll HINTS ${EDGE_DIRECTORY} PATHS ${EDGE_DIRECTORY} NO_CMAKE_FIND_ROOT_PATH)
        find_file(GLES_LIBRARY libGLESv2.dll HINTS ${EDGE_DIRECTORY} PATHS ${EDGE_DIRECTORY} NO_CMAKE_FIND_ROOT_PATH)
    endif()
endforeach()

find_file(EGL_LIBRARY libEGL.dll HINTS ${FIREFOX_DIRECTORY} PATHS ${FIREFOX_DIRECTORY} NO_CMAKE_FIND_ROOT_PATH)
find_file(GLES_LIBRARY libGLESv2.dll HINTS ${FIREFOX_DIRECTORY} PATHS ${FIREFOX_DIRECTORY} NO_CMAKE_FIND_ROOT_PATH)

# Include dir
find_path(EGL_INCLUDE_DIR NAMES
    EGL/egl.h HINTS "${CMAKE_SOURCE_DIR}/deps/include" PATHS "${CMAKE_SOURCE_DIR}/deps/include")
# Include dir
find_path(ANGLE_INCLUDE_DIR NAMES
    GLES2/gl2ext_angle.h HINTS "${CMAKE_SOURCE_DIR}/deps/include" PATHS "${CMAKE_SOURCE_DIR}/deps/include")

if(EXISTS "${GLES_LIBRARY}")
  set(ANGLE_LIBRARY "${GLES_LIBRARY}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args("ANGLE" DEFAULT_MSG
    ANGLE_LIBRARY ANGLE_INCLUDE_DIR EGL_INCLUDE_DIR) 

#mark_as_advanced(ANGLE_LIBRARY)
