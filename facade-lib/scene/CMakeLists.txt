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
  ${target_prefix}::render
  djson::djson

  PRIVATE
  ${target_prefix}::compile-options
  ${target_prefix}::pch
)

target_sources(${PROJECT_NAME} PRIVATE
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

  src/detail/gltf.cpp
  src/detail/gltf.hpp

  src/camera.cpp
  src/material.cpp
  src/scene.cpp
  src/transform.cpp
)
