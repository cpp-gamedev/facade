if("${target_prefix}" STREQUAL "")
  message(FATAL_ERROR "target_prefix unset")
endif()

project(${target_prefix}-engine)

add_library(${PROJECT_NAME})
add_library(${target_prefix}::engine ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

target_include_directories(${PROJECT_NAME}
  PUBLIC
  include

  PRIVATE
  src
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  ${target_prefix}::render
  ${target_prefix}::dear_imgui

  PRIVATE
  ${target_prefix}::compile-options
)

string(TOUPPER ${target_prefix} target_prefix_upper)

if(${${target_prefix_upper}_PCH})
  target_precompile_headers(${PROJECT_NAME} REUSE_FROM ${target_prefix}-vk)
endif()

target_sources(${PROJECT_NAME} PRIVATE
  include/${target_prefix}/engine/engine.hpp

  src/engine.cpp
)
