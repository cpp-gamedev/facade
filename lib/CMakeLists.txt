cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

if("${target_prefix}" STREQUAL "")
  message(FATAL_ERROR "target_prefix unset")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_DEBUG_POSTFIX "-d")

project(${target_prefix}-lib)

add_subdirectory(ext)
add_subdirectory(interface)
add_subdirectory(util)
add_subdirectory(vk)
add_subdirectory(glfw)
add_subdirectory(render)
add_subdirectory(scene)
add_subdirectory(engine)
add_subdirectory(context)
