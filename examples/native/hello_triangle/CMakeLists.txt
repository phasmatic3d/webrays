cmake_minimum_required(VERSION 3.0)
cmake_policy(SET CMP0048 NEW)

project (hello_triangle)

add_executable(hello_triangle
    hello_triangle.c 
)

if(WIN32)
  string(REPLACE "SDL2.lib" "SDL2.dll" SDL2_LIBPATH "${SDL2_LIBRARY}")
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E copy_if_different
                     "${SDL2_LIBPATH}"
                     "${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>")

  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E copy_if_different
                     "${GLES_LIBRARY}"
                     "${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>")
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E copy_if_different
                     "${EGL_LIBRARY}"
                     "${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>")
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E copy_directory 
                     "${PROJECT_SOURCE_DIR}/kernels"
                     "${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>/kernels")
  set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>")
endif()

target_compile_definitions(${PROJECT_NAME} PUBLIC _CRT_SECURE_NO_WARNINGS)

set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>")
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>")

target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIR} "${CMAKE_SOURCE_DIR}/include" ${ANGLE_INCLUDE_DIR})
if(WIN32)
  target_link_libraries(${PROJECT_NAME} webrays ${SDL2MAIN_LIBRARY} ${SDL2_LIBRARY} "${GLES_ARCHIVE}" "${EGL_ARCHIVE}")
elseif(APPLE)
  set(MACOSX_RPATH ON)
  set(CMAKE_MACOSX_RPATH ON)
  set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH @executable_path/../lib)
  set(CMAKE_INSTALL_RPATH "@executable_path/../lib")
  target_link_libraries(${PROJECT_NAME} webrays "${SDL2_LIBRARY}" "${GLES_LIBRARY}" "${EGL_LIBRARY}")
  target_link_libraries(${PROJECT_NAME} "-framework Foundation")
else()
  set(CMAKE_INSTALL_RPATH "\$ORIGIN")
target_link_libraries(${PROJECT_NAME} webrays "${SDL2_LIBRARY}" "${GLES_LIBRARY}" "${EGL_LIBRARY}")
endif()

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION webrays/bin)

if(WIN32)
  install(FILES ${SDL2_LIBPATH} DESTINATION webrays/bin)
elseif(APPLE)
  install(FILES "${GLES_LIBRARY}" "${EGL_LIBRARY}" DESTINATION webrays/lib)
  install(DIRECTORY "${SDL2_LIBRARY}" DESTINATION webrays/lib)
else()
  set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH $ORIGIN/../lib)
  install(FILES "${GLES_LIBRARY}" "${EGL_LIBRARY}" DESTINATION webrays/lib)
endif()

install(DIRECTORY kernels DESTINATION webrays/bin)

# There has to be a better solution
if(APPLE)
  install(CODE "execute_process(COMMAND ${CMAKE_INSTALL_NAME_TOOL} -change ./libEGL.dylib @rpath/libEGL.dylib ${CMAKE_INSTALL_PREFIX}/webrays/bin/hello_triangle)")
  install(CODE "execute_process(COMMAND ${CMAKE_INSTALL_NAME_TOOL} -change ./libGLESv2.dylib @rpath/libGLESv2.dylib ${CMAKE_INSTALL_PREFIX}/webrays/bin/hello_triangle)")
endif()