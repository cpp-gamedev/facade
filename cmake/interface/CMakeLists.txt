project(facade-compile-options)

add_library(${PROJECT_NAME} INTERFACE)
add_library(facade::compile-options ALIAS ${PROJECT_NAME})

if(CMAKE_CXX_COMPILER_ID STREQUAL Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  target_compile_options(${PROJECT_NAME} INTERFACE
    -Wall -Wextra -Wpedantic -Werror=return-type
  )

  if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    target_compile_options(${PROJECT_NAME} INTERFACE -Wconversion)
  endif()
endif()

if(CMAKE_GENERATOR MATCHES "^(Visual Studio)")
  target_compile_options(${PROJECT_NAME} INTERFACE /MP)
endif()
