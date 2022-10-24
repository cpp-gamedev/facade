if("${target_prefix}" STREQUAL "")
  message(FATAL_ERROR "target_prefix unset")
endif()

project(${target_prefix}-compile-options)

add_library(${PROJECT_NAME} INTERFACE)
add_library(${target_prefix}::compile-options ALIAS ${PROJECT_NAME})

if(CMAKE_CXX_COMPILER_ID STREQUAL Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  target_compile_options(${PROJECT_NAME} INTERFACE
    -Wall -Wextra -Wpedantic -Werror=return-type
  )

  if(CMAKE_CXX_COMPILER_ID STREQUAL GNU AND NOT CMAKE_SYSTEM_PROCESSOR STREQUAL aarch64)
    target_compile_options(${PROJECT_NAME} INTERFACE -Wconversion)
  endif()
endif()

if(CMAKE_GENERATOR MATCHES "^(Visual Studio)")
  target_compile_options(${PROJECT_NAME} INTERFACE /MP)
endif()
