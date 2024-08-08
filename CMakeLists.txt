cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    pubsubpp
    VERSION 0.1.0
    DESCRIPTION "Short description"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES NONE
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(pubsubpp_pubsubpp INTERFACE)
add_library(pubsubpp::pubsubpp ALIAS pubsubpp_pubsubpp)

set_property(
    TARGET pubsubpp_pubsubpp PROPERTY
    EXPORT_NAME pubsubpp
)

target_include_directories(
    pubsubpp_pubsubpp ${warning_guard}
    INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)

target_compile_features(pubsubpp_pubsubpp INTERFACE cxx_std_20)

find_package(fmt REQUIRED)
target_link_libraries(pubsubpp_pubsubpp INTERFACE fmt::fmt)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Examples ----

if(PROJECT_IS_TOP_LEVEL)
  option(BUILD_EXAMPLES "Build examples tree." "${pubsubpp_DEVELOPER_MODE}")
  if(BUILD_EXAMPLES)
    add_subdirectory(example)
  endif()
endif()

# ---- Developer mode ----

if(NOT pubsubpp_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of pubsubpp"
  )
endif()

include(cmake/dev-mode.cmake)
