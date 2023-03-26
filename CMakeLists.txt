cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    async_nats
    VERSION 0.1.0
    DESCRIPTION "Async client for NATS"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(async_nats_async_nats)

file(GLOB_RECURSE SOURCES source/*.cpp include/*.h include/*.hpp)
message("${SOURCES}")
target_sources(async_nats_async_nats PRIVATE ${SOURCES})
add_library(async_nats::async_nats ALIAS async_nats_async_nats)

include(GenerateExportHeader)
generate_export_header(
    async_nats_async_nats
    BASE_NAME async_nats
    EXPORT_FILE_NAME export/async_nats/async_nats_export.hpp
    CUSTOM_CONTENT_FROM_VARIABLE pragma_suppress_c4251
)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(async_nats_async_nats PUBLIC ASYNC_NATS_STATIC_DEFINE)
endif()

set_target_properties(
    async_nats_async_nats PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}"
    EXPORT_NAME async_nats
    OUTPUT_NAME async_nats
)

target_include_directories(
    async_nats_async_nats ${warning_guard}
    PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_include_directories(
    async_nats_async_nats SYSTEM
    PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/export>"
)

target_compile_features(async_nats_async_nats PUBLIC cxx_std_17)

# ---- Dependencies ----

find_package(fmt REQUIRED)
find_package(Boost REQUIRED)

include(FetchContent)
FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG v0.3.3 # Optionally specify a commit hash, version tag or branch here
)
FetchContent_MakeAvailable(Corrosion)
corrosion_import_crate(MANIFEST_PATH ${CMAKE_CURRENT_LIST_DIR}/rust/Cargo.toml)

target_link_libraries(async_nats_async_nats
  PRIVATE
  fmt::fmt
  nats_fabric

  PUBLIC
  Boost::boost

  # windows
  $<$<BOOL:${WIN32}>:ntdll>
  $<$<BOOL:${WIN32}>:Crypt32>
  $<$<BOOL:${WIN32}>:Ncrypt>
  $<$<BOOL:${WIN32}>:Secur32>
)


# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Examples ----

if(PROJECT_IS_TOP_LEVEL)
  option(BUILD_EXAMPLES "Build examples tree." "${async_nats_DEVELOPER_MODE}")
  if(BUILD_EXAMPLES)
    add_subdirectory(example)
  endif()
endif()

# ---- Developer mode ----

if(NOT async_nats_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of async_nats"
  )
endif()

include(cmake/dev-mode.cmake)
