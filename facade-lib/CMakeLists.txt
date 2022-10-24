set(target_prefix facade)

project(${target_prefix}-lib)

add_subdirectory(interface)
add_subdirectory(util)

find_package(Threads)

add_library(${PROJECT_NAME})
add_library(${target_prefix}::lib ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  glfw::glfw
  glm::glm
  ${target_prefix}::vk
  ${target_prefix}::vma
  ${target_prefix}::dear_imgui
  ${target_prefix}::util
  djson::djson

  PRIVATE
  spirv-cross-glsl
  ${target_prefix}::compile-options
  ${target_prefix}::pch
)

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
endif()

target_precompile_headers(${PROJECT_NAME} PUBLIC
  <vulkan/vulkan.hpp>
  <vk_mem_alloc.h>
  <fmt/format.h>
  <algorithm>
  <atomic>
  <functional>
  <memory>
  <mutex>
  <string>
  <vector>
  <unordered_map>
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
  include

  PRIVATE
  src
)

target_sources(${PROJECT_NAME} PRIVATE
  include/facade/defines.hpp

  include/facade/vk/buffer.hpp
  include/facade/vk/cmd.hpp
  include/facade/vk/defer.hpp
  include/facade/vk/descriptor_set.hpp
  include/facade/vk/drawer.hpp
  include/facade/vk/gfx.hpp
  include/facade/vk/pipeline.hpp
  include/facade/vk/pipes.hpp
  include/facade/vk/render_frame.hpp
  include/facade/vk/render_pass.hpp
  include/facade/vk/render_target.hpp
  include/facade/vk/set_allocator.hpp
  include/facade/vk/shader.hpp
  include/facade/vk/spir_v.hpp
  include/facade/vk/static_mesh.hpp
  include/facade/vk/swapchain.hpp
  include/facade/vk/texture.hpp
  include/facade/vk/vk.hpp
  include/facade/vk/vma.hpp
  include/facade/vk/wsi.hpp

  include/facade/glfw/glfw.hpp
  include/facade/glfw/input.hpp

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

  src/vk/buffer.cpp
  src/vk/cmd.cpp
  src/vk/descriptor_set.cpp
  src/vk/drawer.cpp
  src/vk/gfx.cpp
  src/vk/pipeline.cpp
  src/vk/pipes.cpp
  src/vk/render_frame.cpp
  src/vk/render_pass.cpp
  src/vk/set_allocator.cpp
  src/vk/shader.cpp
  src/vk/spir_v.cpp
  src/vk/static_mesh.cpp
  src/vk/swapchain.cpp
  src/vk/texture.cpp
  src/vk/vk.cpp
  src/vk/vma.cpp

  src/glfw/glfw.cpp

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
