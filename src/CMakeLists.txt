cmake_minimum_required(VERSION 3.0)
cmake_policy(SET CMP0048 NEW)

set(WEBRAYS_LIBRARY_NAME "webrays")
set(WEBRAYS_LIBRARY_SOURCES webrays.cpp
    webrays_ads.cpp
    webrays_gl.cpp
    webrays_cpu.cpp
    webrays_queue.cpp
    webrays_shader_engine.cpp)

project(webrays)

if (EMSCRIPTEN)
add_library (${PROJECT_NAME} ${WEBRAYS_LIBRARY_SOURCES})
else()
add_library (${PROJECT_NAME} SHARED ${WEBRAYS_LIBRARY_SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "../include/webrays/webrays.h")
endif()

if (EMSCRIPTEN)

    set(CMAKE_STATIC_LIBRARY_PREFIX "")
    set(CMAKE_AR "${EMSCRIPTEN_ROOT_PATH}/emcc${EMCC_SUFFIX}")
    set(EMSCRIPTEN_GENERATE_BITCODE_STATIC_LIBRARIES True)
    set(CMAKE_STATIC_LIBRARY_SUFFIX ".js")
    set(CMAKE_C_CREATE_STATIC_LIBRARY "<CMAKE_AR> <OBJECTS> <LINK_FLAGS> -o <TARGET>")
    set(CMAKE_CXX_CREATE_STATIC_LIBRARY "<CMAKE_AR> <OBJECTS> <LINK_FLAGS> -o <TARGET>")

    set(CMAKE_C_FLAGS_ALL_CONFIGS "-fno-exceptions -s WASM=1 -Wall -Wextra -Wno-double-promotion -Wno-unused-parameter -Wno-unused-variable -Wno-sign-compare -Wno-missing-braces")
    set(CMAKE_LINKER_FLAGS_ALL_CONFIGS "-lEGL -lGLESv2 -s MAX_WEBGL_VERSION=2 -s MIN_WEBGL_VERSION=2 -s MODULARIZE=1 -s EXPORT_ES6=1 -s WASM=1 -s EXPORT_NAME=\"'createWebRaysModule'\" -s ABORTING_MALLOC=0 -s ALLOW_MEMORY_GROWTH=1 -s FULL_ES3=1 -s WASM=1 -s NO_EXIT_RUNTIME=1 -s GL_PREINITIALIZED_CONTEXT=1 -s EXPORTED_RUNTIME_METHODS=\"['cwrap','GL','UTF8ToString','lengthBytesUTF8','stringToUTF8']\" -s EXPORTED_FUNCTIONS=\"['_free', '_malloc']\" --extern-pre-js \"${CMAKE_SOURCE_DIR}/src/webrays_loader.js\"")

    set(CMAKE_C_FLAGS_RELEASE "-O3 ${CMAKE_C_FLAGS_ALL_CONFIGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "-O3 ${CMAKE_LINKER_FLAGS_ALL_CONFIGS}")

    set(CMAKE_C_FLAGS_MINSIZEREL "-Os ${CMAKE_C_FLAGS_ALL_CONFIGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_MINSIZEREL "-Os ${CMAKE_LINKER_FLAGS_ALL_CONFIGS}")

    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_ALL_CONFIGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_ALL_CONFIGS}")

elseif(WIN32)

  string(REPLACE "/cl.exe" "" CMAKE_C_COMPILER_DIRECTORY "${CMAKE_C_COMPILER}")
  find_program(DUMPBIN_EXE dumpbin HINTS "${CMAKE_C_COMPILER_DIRECTORY}" PATHS "${CMAKE_C_COMPILER_DIRECTORY}")
  find_program(LIB_EXE lib HINTS "${CMAKE_C_COMPILER_DIRECTORY}" PATHS "${CMAKE_C_COMPILER_DIRECTORY}")

  #if("${CMAKE_GENERATOR}" MATCHES "(Win64|IA64|x64)")
  if (${CMAKE_SIZEOF_VOID_P} MATCHES 8)
    set(ARCH "X64")
  else()
    set(ARCH "X86")
  endif()

  exec_program(${DUMPBIN_EXE} ARGS "/EXPORTS \"${EGL_LIBRARY}\"" OUTPUT_VARIABLE EGL_EXPORTS)
  string(REGEX MATCHALL "[ ]egl[a-zA-Z_0-9@?]+" EGL_EXPORT_LIST ${EGL_EXPORTS})
  string(APPEND EGL_DEFINITIONS_FILE "EXPORTS\n")
  foreach(export_name ${EGL_EXPORT_LIST})
    string(APPEND EGL_DEFINITIONS_FILE "${export_name}\n")
  endforeach()
  file(WRITE "${CMAKE_BINARY_DIR}/libEGL.def" "${EGL_DEFINITIONS_FILE}")
  exec_program(${LIB_EXE} ARGS "/def:\"${CMAKE_BINARY_DIR}/libEGL.def\" /machine:${ARCH} /out:\"${CMAKE_BINARY_DIR}/libEGL.lib\" /NOLOGO")

  exec_program(${DUMPBIN_EXE} ARGS "/EXPORTS \"${GLES_LIBRARY}\"" OUTPUT_VARIABLE GLES_EXPORTS)
  string(REGEX MATCHALL "[ ][?][a-zA-Z_0-9@?]+|[ ]gl[a-zA-Z_0-9@?]+|[ ]GL[a-zA-Z_0-9@?]+|[ ]EGL[a-zA-Z_0-9@?]+|[ ]ANGLE[a-zA-Z_0-9@?]+" GLES_EXPORT_LIST ${GLES_EXPORTS})
  string(APPEND GLES_DEFINITIONS_FILE "EXPORTS\n")
  foreach(export_name ${GLES_EXPORT_LIST})
    string(APPEND GLES_DEFINITIONS_FILE "${export_name}\n")
  endforeach()
  file(WRITE "${CMAKE_BINARY_DIR}/libGLESv2.def" "${GLES_DEFINITIONS_FILE}")
  exec_program(${LIB_EXE} ARGS "/def:\"${CMAKE_BINARY_DIR}/libGLESv2.def\" /machine:${ARCH} /out:\"${CMAKE_BINARY_DIR}/libGLESv2.lib\" /NOLOGO")

  set(ANGLE_ARCHIVE "${CMAKE_BINARY_DIR}/libGLESv2.lib" CACHE INTERNAL "")
  set(GLES_ARCHIVE "${CMAKE_BINARY_DIR}/libGLESv2.lib" CACHE INTERNAL "")
  set(EGL_ARCHIVE "${CMAKE_BINARY_DIR}/libEGL.lib" CACHE INTERNAL "")

endif()

if(NOT MSVC)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
endif()

target_include_directories(${PROJECT_NAME} PRIVATE "../include")
if (EMSCRIPTEN)

  install(FILES "${CMAKE_BINARY_DIR}/src/${WEBRAYS_LIBRARY_NAME}.wasm" DESTINATION webrays/js/common)
  install(FILES "${CMAKE_BINARY_DIR}/src/${WEBRAYS_LIBRARY_NAME}.js" DESTINATION webrays/js/common)
  install(DIRECTORY ../examples/web/ DESTINATION webrays)

  if(PREPARE_FOR_PUBLISH)
    install(FILES "${CMAKE_BINARY_DIR}/src/${WEBRAYS_LIBRARY_NAME}.wasm" DESTINATION ${CMAKE_SOURCE_DIR}/dist)
    install(FILES "${CMAKE_BINARY_DIR}/src/${WEBRAYS_LIBRARY_NAME}.js" DESTINATION ${CMAKE_SOURCE_DIR}/dist)
  endif()
else()
  
  target_include_directories(${PROJECT_NAME} PRIVATE "../deps/include")

  set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${CMAKE_PROJECT_VERSION} SOVERSION ${CMAKE_PROJECT_VERSION})
  set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
  
  target_compile_definitions(${PROJECT_NAME} PUBLIC WRAYS_BUILD PUBLIC _CRT_SECURE_NO_WARNINGS)

  install(TARGETS ${PROJECT_NAME}
          ARCHIVE DESTINATION webrays/lib
          LIBRARY DESTINATION webrays/lib
          RUNTIME DESTINATION webrays/bin
          PUBLIC_HEADER DESTINATION webrays/include/webrays)
  install(DIRECTORY "../deps/include/" DESTINATION webrays/include)
  if(WIN32)
    install(FILES "${GLES_LIBRARY}" DESTINATION webrays/bin)
    install(FILES "${EGL_LIBRARY}" DESTINATION webrays/bin)
  else()
    install(FILES "${GLES_LIBRARY}" DESTINATION webrays/lib)
    install(FILES "${EGL_LIBRARY}" DESTINATION webrays/lib)
  endif()
  if(NOT "${GLES_ARCHIVE}" MATCHES "")
    install(FILES "${GLES_ARCHIVE}" DESTINATION webrays/lib)
  endif()
  if(NOT "${EGL_ARCHIVE}" MATCHES "")
    install(FILES "${EGL_ARCHIVE}" DESTINATION webrays/lib)
  endif()
endif()