cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_DEBUG_POSTFIX "-d")

project(facade)

option(FACADE_BUILD_SHADERS ON)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
try_compile(has_make_unique_for_overwrite "${CMAKE_CURRENT_BINARY_DIR}" SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/src/make_unique_for_overwrite.cpp")

add_subdirectory(ext)
add_subdirectory(cmake/interface)
add_subdirectory(tools/embed_shader)

add_subdirectory(facade-lib)

if(FACADE_BUILD_SHADERS)
  add_custom_command(
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/default.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/unlit.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/lit.frag

    OUTPUT
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bin/default_vert.spv.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bin/unlit_frag.spv.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bin/lit_frag.spv.hpp

    COMMAND embed-shader ${CMAKE_CURRENT_SOURCE_DIR}/shaders/default.vert > ${CMAKE_CURRENT_SOURCE_DIR}/src/bin/default_vert.spv.hpp
    COMMAND embed-shader ${CMAKE_CURRENT_SOURCE_DIR}/shaders/unlit.frag > ${CMAKE_CURRENT_SOURCE_DIR}/src/bin/unlit_frag.spv.hpp
    COMMAND embed-shader ${CMAKE_CURRENT_SOURCE_DIR}/shaders/lit.frag > ${CMAKE_CURRENT_SOURCE_DIR}/src/bin/lit_frag.spv.hpp
  )
endif()

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
  src/main.cpp

  src/bin/shaders.cpp
  src/bin/shaders.hpp

  src/bin/default_vert.spv.hpp
  src/bin/unlit_frag.spv.hpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE facade::lib facade::compile-options)
target_include_directories(${PROJECT_NAME} PRIVATE src)
