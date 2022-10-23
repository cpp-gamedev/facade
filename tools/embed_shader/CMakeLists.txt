cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(embed-shader)

add_executable(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt)

target_sources(${PROJECT_NAME} PRIVATE embed_shader.cpp)

if(NOT "${glslc}" STREQUAL "" AND NOT "${glslc}" STREQUAL "glslc-NOTFOUND")
  target_compile_definitions(${PROJECT_NAME} PRIVATE GLSLC_PATH="${glslc}")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Werror=return-type
  )
endif()
