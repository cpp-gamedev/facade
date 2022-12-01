cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_DEBUG_POSTFIX "-d")

set(target_prefix facade)

project(facade)

set(is_root_project OFF)

if(PROJECT_NAME STREQUAL CMAKE_PROJECT_NAME)
  set(is_root_project ON)
endif()

option(FACADE_BUILD_SHADERS "Build facade shaders" ${is_root_project})
option(FACADE_PCH "Use PCH" ON)
option(FACADE_BUILD_EXE "Build facade application (else only library)" ${is_root_project})

if(FACADE_BUILD_EXE AND FACADE_BUILD_SHADERS)
  find_program(glslc glslc)

  if(glslc STREQUAL glslc-NOTFOUND)
    message(WARNING "glslc not found, disabling FACADE_BUILD_SHADERS")
    set(FACADE_BUILD_SHADERS OFF CACHE STRING "" FORCE)
  endif()
endif()

add_subdirectory(tools/embed_shader)
add_subdirectory(lib)

if(FACADE_BUILD_EXE AND FACADE_BUILD_SHADERS)
  message(STATUS "Adding build step to embed shaders")
  add_custom_command(
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/default.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/unlit.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/lit.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/skybox.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/skin.vert

    OUTPUT
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bin/default_vert.spv.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bin/unlit_frag.spv.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bin/lit_frag.spv.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bin/skybox_frag.spv.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bin/skin.vert.hpp

    COMMAND embed-shader ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/default.vert > ${CMAKE_CURRENT_SOURCE_DIR}/src/bin/default_vert.spv.hpp
    COMMAND embed-shader ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/unlit.frag > ${CMAKE_CURRENT_SOURCE_DIR}/src/bin/unlit_frag.spv.hpp
    COMMAND embed-shader ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/lit.frag > ${CMAKE_CURRENT_SOURCE_DIR}/src/bin/lit_frag.spv.hpp
    COMMAND embed-shader ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/skybox.frag > ${CMAKE_CURRENT_SOURCE_DIR}/src/bin/skybox_frag.spv.hpp
    COMMAND embed-shader ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/skin.vert > ${CMAKE_CURRENT_SOURCE_DIR}/src/bin/skin_vert.spv.hpp
  )
endif()

if(FACADE_BUILD_EXE)
  add_executable(${PROJECT_NAME})

  target_sources(${PROJECT_NAME} PRIVATE
    src/config/config.cpp
    src/config/config.hpp
    src/events/events.hpp
    src/gui/events.hpp
    src/gui/gltf_sources.hpp
    src/gui/gltf_sources.cpp
    src/gui/main_menu.cpp
    src/gui/main_menu.hpp
    src/gui/path_source.hpp

    src/main.cpp

    src/bin/shaders.cpp
    src/bin/shaders.hpp

    src/bin/default_vert.spv.hpp
    src/bin/unlit_frag.spv.hpp
  )

  target_link_libraries(${PROJECT_NAME} PRIVATE
    facade::engine
    facade::compile-options
  )

  string(TOUPPER ${target_prefix} target_prefix_upper)

  if(${${target_prefix_upper}_PCH})
    target_precompile_headers(${PROJECT_NAME} REUSE_FROM ${target_prefix}-vk)
  endif()

  target_include_directories(${PROJECT_NAME} PRIVATE src)
endif()
