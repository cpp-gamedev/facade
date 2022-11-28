if("${target_prefix}" STREQUAL "")
  message(FATAL_ERROR "target_prefix unset")
endif()

project(${target_prefix}-vk)

add_library(${PROJECT_NAME})
add_library(${target_prefix}::vk ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

target_include_directories(${PROJECT_NAME}
  PUBLIC
  include

  PRIVATE
  src
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  ${target_prefix}::vma
  ${target_prefix}::util

  PRIVATE
  spirv-cross-glsl
  ${target_prefix}::compile-options
)

string(TOUPPER ${target_prefix} target_prefix_upper)

if(${${target_prefix_upper}_PCH})
  target_precompile_headers(${PROJECT_NAME} PRIVATE
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
endif()

target_sources(${PROJECT_NAME} PRIVATE
  include/${target_prefix}/vk/buffer.hpp
  include/${target_prefix}/vk/cmd.hpp
  include/${target_prefix}/vk/defer_queue.hpp
  include/${target_prefix}/vk/defer.hpp
  include/${target_prefix}/vk/descriptor_set.hpp
  include/${target_prefix}/vk/geometry.hpp
  include/${target_prefix}/vk/gfx.hpp
  include/${target_prefix}/vk/pipeline.hpp
  include/${target_prefix}/vk/pipes.hpp
  include/${target_prefix}/vk/render_frame.hpp
  include/${target_prefix}/vk/render_pass.hpp
  include/${target_prefix}/vk/render_target.hpp
  include/${target_prefix}/vk/rotator.hpp
  include/${target_prefix}/vk/set_allocator.hpp
  include/${target_prefix}/vk/shader.hpp
  include/${target_prefix}/vk/skybox.hpp
  include/${target_prefix}/vk/spir_v.hpp
  include/${target_prefix}/vk/static_mesh.hpp
  include/${target_prefix}/vk/swapchain.hpp
  include/${target_prefix}/vk/texture.hpp
  include/${target_prefix}/vk/vk.hpp
  include/${target_prefix}/vk/vma.hpp
  include/${target_prefix}/vk/wsi.hpp

  src/buffer.cpp
  src/cmd.cpp
  src/descriptor_set.cpp
  src/geometry.cpp
  src/gfx.cpp
  src/pipeline.cpp
  src/pipes.cpp
  src/render_frame.cpp
  src/render_pass.cpp
  src/set_allocator.cpp
  src/shader.cpp
  src/skybox.cpp
  src/spir_v.cpp
  src/static_mesh.cpp
  src/swapchain.cpp
  src/texture.cpp
  src/vk.cpp
  src/vma.cpp
)
