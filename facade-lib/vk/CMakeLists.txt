if("${target_prefix}" STREQUAL "")
  message(FATAL_ERROR "target_prefix unset")
endif()

project(${target_prefix}-vk)

find_package(Threads)

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
  ${target_prefix}::pch
)

target_sources(${PROJECT_NAME} PRIVATE
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

  src/buffer.cpp
  src/cmd.cpp
  src/descriptor_set.cpp
  src/drawer.cpp
  src/gfx.cpp
  src/pipeline.cpp
  src/pipes.cpp
  src/render_frame.cpp
  src/render_pass.cpp
  src/set_allocator.cpp
  src/shader.cpp
  src/spir_v.cpp
  src/static_mesh.cpp
  src/swapchain.cpp
  src/texture.cpp
  src/vk.cpp
  src/vma.cpp
)
