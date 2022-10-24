set(target_prefix facade)

project(${target_prefix}-lib)

add_subdirectory(interface)
add_subdirectory(util)
add_subdirectory(vk)
add_subdirectory(glfw)

find_package(Threads)

add_library(${PROJECT_NAME})
add_library(${target_prefix}::lib ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  glm::glm
  ${target_prefix}::glfw
  ${target_prefix}::dear_imgui
  djson::djson

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

  include/facade/render/renderer.hpp

  include/facade/scene/camera.hpp
  include/facade/scene/fly_cam.hpp
  include/facade/scene/id.hpp
  include/facade/scene/lights.hpp
  include/facade/scene/material.hpp
  include/facade/scene/node_data.hpp
  include/facade/scene/node.hpp
  include/facade/scene/rect.hpp
  include/facade/scene/scene.hpp
  include/facade/scene/transform.hpp

  include/facade/engine/engine.hpp

  include/facade/editor/common.hpp
  include/facade/editor/inspector.hpp
  include/facade/editor/log.hpp
  include/facade/editor/scene_tree.hpp

  src/detail/dear_imgui.cpp
  src/detail/dear_imgui.hpp
  src/render/renderer.cpp

  src/detail/gltf.cpp
  src/detail/gltf.hpp

  src/scene/camera.cpp
  src/scene/material.cpp
  src/scene/scene.cpp
  src/scene/transform.cpp

  src/engine/engine.cpp

  src/editor/common.cpp
  src/editor/inspector.cpp
  src/editor/log.cpp
  src/editor/scene_tree.cpp
)
