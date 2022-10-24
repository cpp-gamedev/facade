if("${target_prefix}" STREQUAL "")
  message(FATAL_ERROR "target_prefix unset")
endif()

project(${target_prefix}-editor)

add_library(${PROJECT_NAME})
add_library(${target_prefix}::editor ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

target_include_directories(${PROJECT_NAME}
  PUBLIC
  include

  PRIVATE
  src
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  ${target_prefix}::scene
  ${target_prefix}::dear_imgui

  PRIVATE
  ${target_prefix}::compile-options
  ${target_prefix}::pch
)

target_sources(${PROJECT_NAME} PRIVATE
  include/${target_prefix}/editor/common.hpp
  include/${target_prefix}/editor/inspector.hpp
  include/${target_prefix}/editor/log.hpp
  include/${target_prefix}/editor/scene_tree.hpp

  src/common.cpp
  src/inspector.cpp
  src/log.cpp
  src/scene_tree.cpp
)
