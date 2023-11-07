cmake_minimum_required(VERSION 3.11)
include(FetchContent)
FetchContent_Declare(
  whereami-external
  GIT_REPOSITORY https://github.com/gpakosz/whereami.git
  GIT_TAG        ba364cd54fd431c76c045393b6522b4bff547f50 # master @ 2023.04.20.
)
FetchContent_MakeAvailable(whereami-external)
add_library(whereami IMPORTED INTERFACE)
target_include_directories(whereami
  INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/_deps/whereami-external-src/src"
)
target_sources(whereami
  INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/_deps/whereami-external-src/src/whereami.c"
)
