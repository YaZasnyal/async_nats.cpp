cmake_minimum_required(VERSION 3.14)

project(async_natsTests LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(async_nats REQUIRED)
  enable_testing()
endif()

find_package(GTest REQUIRED)

# ---- Tests ----

add_executable(async_nats_test)
target_link_libraries(async_nats_test
    PRIVATE
    async_nats::async_nats
    GTest::gtest_main
)

target_sources(async_nats_test
  PRIVATE
  source/nats_fixture.hpp

  source/nats_fixture.cpp

  source/mailbox.cpp
  source/messaging.cpp
  source/subscribtion.cpp
  source/nonblocking.cpp
  source/reply_to.cpp
  source/req_rep.cpp
)

target_include_directories(async_nats_test
  PRIVATE
  source
)

target_compile_features(async_nats_test PRIVATE cxx_std_20)

add_test(NAME async_nats COMMAND async_nats_test)
set_tests_properties(async_nats PROPERTIES TIMEOUT 10)

# ---- End-of-file commands ----

add_folders(Test)
