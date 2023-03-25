cmake_minimum_required(VERSION 3.14)

project(async_natsTests LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(async_nats REQUIRED)
  enable_testing()
endif()

find_package(Catch2 REQUIRED)
include(Catch)

# ---- Tests ----

add_executable(async_nats_test source/async_nats_test.cpp)
target_link_libraries(
    async_nats_test PRIVATE
    async_nats::async_nats
    Catch2::Catch2WithMain
)
target_compile_features(async_nats_test PRIVATE cxx_std_17)

catch_discover_tests(async_nats_test)

# ---- End-of-file commands ----

add_folders(Test)
