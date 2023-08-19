cmake_minimum_required(VERSION 3.20)

include(cmake/prelude.cmake)

project(
    async_nats
    VERSION 0.1.0
    DESCRIPTION "Async client for NATS"
    HOMEPAGE_URL "https://github.com/YaZasnyal/async_nats.cpp/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

option(ASYNC_NATS_SHARED "Link shared library by default" ON)

include(FetchContent)
set(CORROSION_NO_WARN_PARSE_TARGET_TRIPLE_FAILED ON)
FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG v0.4.1
)
FetchContent_MakeAvailable(Corrosion)
corrosion_import_crate(
  MANIFEST_PATH ${CMAKE_CURRENT_LIST_DIR}/Cargo.toml
  LOCKED
)

add_library(async_nats_async_nats INTERFACE)

file(GLOB_RECURSE SOURCES include/*.h include/*.hpp)
target_sources(async_nats_async_nats PRIVATE ${SOURCES})
add_library(async_nats::async_nats ALIAS async_nats_async_nats)

include(GenerateExportHeader)
generate_export_header(
    async_nats_async_nats
    BASE_NAME async_nats
    EXPORT_FILE_NAME export/async_nats/async_nats_export.hpp
    CUSTOM_CONTENT_FROM_VARIABLE pragma_suppress_c4251
)

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
    INTERFACE
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_include_directories(
    async_nats_async_nats SYSTEM
    INTERFACE
    "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/export>"
)

target_compile_features(async_nats_async_nats INTERFACE cxx_std_17)

# ---- Dependencies ----

find_package(Boost REQUIRED)

set(SECURITY_FRAMEWORK "")
set(CORE_FOUNDATION_FRAMEWORK "")
if(APPLE)
    find_library(SECURITY_FRAMEWORK Security REQUIRED)
    find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
endif()

target_link_libraries(async_nats_async_nats
  INTERFACE
  Boost::headers
  $<IF:$<BOOL:${ASYNC_NATS_SHARED}>,nats_fabric-shared,nats_fabric>

  # windows
  $<$<BOOL:${WIN32}>:ntdll>
  $<$<BOOL:${WIN32}>:Crypt32>
  $<$<BOOL:${WIN32}>:Ncrypt>
  $<$<BOOL:${WIN32}>:Secur32>
  $<$<BOOL:${WIN32}>:ws2_32>

  # macos
  $<$<BOOL:${APPLE}>:${SECURITY_FRAMEWORK}>
  $<$<BOOL:${APPLE}>:${CORE_FOUNDATION_FRAMEWORK}>

  # TODO: linux
)

if(PROJECT_IS_TOP_LEVEL)
  target_compile_options(async_nats_async_nats
    INTERFACE
    $<$<BOOL:${WIN32}>:/W4>
    $<$<BOOL:${UNIX}>:-Wall -Wextra -Werror>
  )
endif()

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
