if("${target_prefix}" STREQUAL "")
  message(FATAL_ERROR "target_prefix unset")
endif()

string(TOUPPER ${target_prefix} define_prefix)

project(${target_prefix}-interface)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
try_compile(has_make_unique_for_overwrite "${CMAKE_CURRENT_BINARY_DIR}" SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/make_unique_for_overwrite.cpp")

add_library(${target_prefix}-interface INTERFACE)
add_library(${target_prefix}::interface ALIAS ${target_prefix}-interface)

target_compile_definitions(${target_prefix}-interface
  INTERFACE
  $<$<CONFIG:Debug>:${define_prefix}_DEBUG>
  $<$<BOOL:${has_make_unique_for_overwrite}>:${define_prefix}_MUFO>
)

target_include_directories(${target_prefix}-interface INTERFACE include)

target_sources(${target_prefix}-interface PRIVATE
  include/${target_prefix}/defines.hpp
)

add_library(${target_prefix}-compile-options INTERFACE)
add_library(${target_prefix}::compile-options ALIAS ${target_prefix}-compile-options)

if(CMAKE_CXX_COMPILER_ID STREQUAL Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  target_compile_options(${target_prefix}-compile-options INTERFACE
    -Wall -Wextra -Wpedantic -Werror=return-type
  )

  if(CMAKE_CXX_COMPILER_ID STREQUAL GNU AND NOT CMAKE_SYSTEM_PROCESSOR STREQUAL aarch64)
    target_compile_options(${target_prefix}-compile-options INTERFACE -Wconversion)
  endif()
endif()

if(CMAKE_GENERATOR MATCHES "^(Visual Studio)")
  target_compile_options(${target_prefix}-compile-options INTERFACE /MP)
endif()
