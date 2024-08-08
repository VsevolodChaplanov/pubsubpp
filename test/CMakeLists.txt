cmake_minimum_required(VERSION 3.14)

project(pubsubppTests LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(pubsubpp REQUIRED)
  enable_testing()
endif()

find_package(Catch2 REQUIRED)
include(Catch)

# ---- Tests ----

add_executable(pubsubpp_test source/pubsubpp_test.cpp)
target_link_libraries(
    pubsubpp_test PRIVATE
    pubsubpp::pubsubpp
    Catch2::Catch2WithMain
)
target_compile_features(pubsubpp_test PRIVATE cxx_std_20)

catch_discover_tests(pubsubpp_test)

# ---- End-of-file commands ----

add_folders(Test)
