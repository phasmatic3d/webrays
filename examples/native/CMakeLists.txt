cmake_minimum_required(VERSION 3.0)
cmake_policy(SET CMP0048 NEW)

find_package(SDL2)

add_subdirectory (hello_triangle)

set_target_properties(hello_triangle PROPERTIES FOLDER "examples")