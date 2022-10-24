if("${target_prefix}" STREQUAL "")
  message(FATAL_ERROR "target_prefix unset")
endif()

project(${target_prefix}-context)

add_library(${PROJECT_NAME})
add_library(${target_prefix}::context ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

target_include_directories(${PROJECT_NAME}
  PUBLIC
  include

  PRIVATE
  src
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  ${target_prefix}::engine
  ${target_prefix}::scene

  PRIVATE
  ${target_prefix}::compile-options
  ${target_prefix}::pch
)

target_sources(${PROJECT_NAME} PRIVATE
  include/${target_prefix}/context/context.hpp

  src/context.cpp
)
