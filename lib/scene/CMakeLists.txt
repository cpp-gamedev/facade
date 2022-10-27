if("${target_prefix}" STREQUAL "")
  message(FATAL_ERROR "target_prefix unset")
endif()

project(${target_prefix}-scene)

add_library(${PROJECT_NAME})
add_library(${target_prefix}::scene ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

target_include_directories(${PROJECT_NAME}
  PUBLIC
  include

  PRIVATE
  src
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  ${target_prefix}::vk
  djson::djson

  PRIVATE
  ${target_prefix}::compile-options
)

string(TOUPPER ${target_prefix} target_prefix_upper)

if(${${target_prefix_upper}_PCH})
  target_precompile_headers(${PROJECT_NAME} REUSE_FROM ${target_prefix}-vk)
endif()

target_sources(${PROJECT_NAME} PRIVATE
  include/${target_prefix}/scene/camera.hpp
  include/${target_prefix}/scene/fly_cam.hpp
  include/${target_prefix}/scene/id.hpp
  include/${target_prefix}/scene/lights.hpp
  include/${target_prefix}/scene/material.hpp
  include/${target_prefix}/scene/node_data.hpp
  include/${target_prefix}/scene/node.hpp
  include/${target_prefix}/scene/rect.hpp
  include/${target_prefix}/scene/scene.hpp
  include/${target_prefix}/scene/transform.hpp

  src/detail/gltf.cpp
  src/detail/gltf.hpp

  src/camera.cpp
  src/material.cpp
  src/scene.cpp
  src/transform.cpp
)
