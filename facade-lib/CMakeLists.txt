set(target_prefix facade)

project(${target_prefix}-lib)

add_subdirectory(interface)
add_subdirectory(util)
add_subdirectory(vk)
add_subdirectory(glfw)
add_subdirectory(render)
add_subdirectory(scene)
add_subdirectory(engine)
add_subdirectory(editor)

find_package(Threads)

add_library(${PROJECT_NAME})
add_library(${target_prefix}::lib ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  ${target_prefix}::scene
  ${target_prefix}::engine
  ${target_prefix}::editor

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
  include/facade/facade.hpp

  src/facade.cpp
)
