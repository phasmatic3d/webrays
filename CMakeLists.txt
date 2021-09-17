cmake_minimum_required(VERSION 3.0)
cmake_policy(SET CMP0048 NEW)
project(webrays VERSION 0.8.0)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(BUILD_EXAMPLES false CACHE BOOL "Should the examples be built")
set(SINGLE_FILE false CACHE BOOL "Embed WASM binary into emscripten's JS glue code")
set(PREPARE_FOR_PUBLISH false CACHE BOOL "Should the library be packaged for publishing")
mark_as_advanced(PREPARE_FOR_PUBLISH)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/deps/cmake/modules/" ${CMAKE_MODULE_PATH})

if (NOT EMSCRIPTEN)
  find_package(ANGLE REQUIRED) 
endif()

add_subdirectory (src)
if (NOT EMSCRIPTEN AND BUILD_EXAMPLES)
  add_subdirectory (examples/native)
  add_dependencies(hello_triangle webrays)
endif()
