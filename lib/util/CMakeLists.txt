if("${target_prefix}" STREQUAL "")
  message(FATAL_ERROR "target_prefix unset")
endif()

project(${target_prefix}-util)

find_package(Threads)

add_library(${PROJECT_NAME})
add_library(${target_prefix}::util ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

target_include_directories(${PROJECT_NAME}
  PUBLIC
  include

  PRIVATE
  src
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  fmt::fmt
  glm::glm
  ${target_prefix}::interface

  PRIVATE
  ${target_prefix}::stb-image
  ${target_prefix}::compile-options

  Threads::Threads
)

string(TOUPPER ${target_prefix} target_prefix_upper)

if(${${target_prefix_upper}_PCH})
  target_precompile_headers(${PROJECT_NAME} PRIVATE
    <fmt/format.h>
    <algorithm>
    <atomic>
    <functional>
    <memory>
    <mutex>
    <string>
    <vector>
    <unordered_map>
  )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
endif()

target_compile_definitions(${PROJECT_NAME}
  PUBLIC
  GLM_FORCE_XYZW_ONLY
  GLM_FORCE_RADIANS
  GLM_FORCE_DEPTH_ZERO_TO_ONE
  GLM_FORCE_SILENT_WARNINGS
  GLM_ENABLE_EXPERIMENTAL
  GLM_EXT_INCLUDED
)

target_sources(${PROJECT_NAME} PRIVATE
  include/${target_prefix}/util/async_queue.hpp
  include/${target_prefix}/util/bool.hpp
  include/${target_prefix}/util/byte_buffer.hpp
  include/${target_prefix}/util/cli_opts.hpp
  include/${target_prefix}/util/colour_space.hpp
  include/${target_prefix}/util/data_provider.hpp
  include/${target_prefix}/util/enum_array.hpp
  include/${target_prefix}/util/enumerate.hpp
  include/${target_prefix}/util/env.hpp
  include/${target_prefix}/util/error.hpp
  include/${target_prefix}/util/fixed_string.hpp
  include/${target_prefix}/util/flex_array.hpp
  include/${target_prefix}/util/hash_combine.hpp
  include/${target_prefix}/util/image.hpp
  include/${target_prefix}/util/logger.hpp
  include/${target_prefix}/util/mufo.hpp
  include/${target_prefix}/util/nvec3.hpp
  include/${target_prefix}/util/pinned.hpp
  include/${target_prefix}/util/ptr.hpp
  include/${target_prefix}/util/rgb.hpp
  include/${target_prefix}/util/thread_pool.hpp
  include/${target_prefix}/util/time.hpp
  include/${target_prefix}/util/transform.hpp
  include/${target_prefix}/util/type_id.hpp
  include/${target_prefix}/util/unique_task.hpp
  include/${target_prefix}/util/unique.hpp
  include/${target_prefix}/util/version.hpp
  include/${target_prefix}/util/visitor.hpp
  include/${target_prefix}/util/zip_ranges.hpp

  src/cli_opts.cpp
  src/data_provider.cpp
  src/env.cpp
  src/image.cpp
  src/logger.cpp
  src/rgb.cpp
  src/thread_pool.cpp
  src/time.cpp
  src/transform.cpp
)
