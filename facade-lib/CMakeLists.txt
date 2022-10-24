set(target_prefix facade)

project(${target_prefix}-lib)

add_subdirectory(interface)
add_subdirectory(util)
add_subdirectory(vk)
add_subdirectory(glfw)
add_subdirectory(render)
add_subdirectory(scene)

find_package(Threads)

add_library(${PROJECT_NAME})
add_library(${target_prefix}::lib ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  ${target_prefix}::scene

  PRIVATE
  ${target_prefix}::compile-options
  ${target_prefix}::pch
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
  include

  PRIVATE
  src
)

target_sources(${PROJECT_NAME} PRIVATE
  include/facade/defines.hpp

  include/facade/engine/engine.hpp

  include/facade/editor/common.hpp
  include/facade/editor/inspector.hpp
  include/facade/editor/log.hpp
  include/facade/editor/scene_tree.hpp

  src/engine/engine.cpp

  src/editor/common.cpp
  src/editor/inspector.cpp
  src/editor/log.cpp
  src/editor/scene_tree.cpp
)
