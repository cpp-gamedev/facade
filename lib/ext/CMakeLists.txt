if("${target_prefix}" STREQUAL "")
  message(FATAL_ERROR "target_prefix unset")
endif()

file(ARCHIVE_EXTRACT INPUT "${CMAKE_CURRENT_SOURCE_DIR}/src.zip" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(src/vk_dynamic)
add_library(${target_prefix}::vk-dynamic ALIAS vk-dynamic)

add_subdirectory(src/vma)
add_library(${target_prefix}::vma ALIAS vma)

set(GLFW_VULKAN_STATIC OFF)
set(GLFW_INSTALL OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
add_subdirectory(src/glfw)
add_library(glfw::glfw ALIAS glfw)

add_subdirectory(src/glm)

if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  target_compile_options(glm INTERFACE -Wno-volatile)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  target_compile_options(glm INTERFACE -Wno-deprecated-volatile)
endif()

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(SPIRV_CROSS_CLI OFF)
set(SPIRV_CROSS_ENABLE_CPP OFF)
set(SPIRV_CROSS_ENABLE_C_API OFF)
set(SPIRV_CROSS_ENABLE_GLSL ON)
set(SPIRV_CROSS_ENABLE_HLSL OFF)
set(SPIRV_CROSS_ENABLE_MSL OFF)
set(SPIRV_CROSS_ENABLE_REFLECT OFF)
set(SPIRV_CROSS_ENABLE_TESTS OFF)
set(SPIRV_CROSS_ENABLE_UTIL OFF)
set(SPIRV_CROSS_SHARED OFF)
set(SPIRV_CROSS_STATIC ON)
set(SPIRV_CROSS_SKIP_INSTALL ON)
add_subdirectory(src/spirv_cross)

add_subdirectory(src/dear_imgui)
add_library(${target_prefix}::dear_imgui ALIAS dear_imgui)

add_subdirectory(src/stb)
add_library(${target_prefix}::stb-image ALIAS stb-image)

add_subdirectory(src/djson)

add_subdirectory(src/fmt)
