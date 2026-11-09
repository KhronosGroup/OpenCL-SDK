if(NOT DEPENDENCIES_FORCE_DOWNLOAD)
  find_package(TCLAP)
  # TCLAP 1.4 (Homebrew's, for one) uses C++20 concepts: when the installed
  # headers do not build with this build's C++ standard, fetch TCLAP instead
  if(TCLAP_FOUND)
    include(CheckCXXSourceCompiles)
    set(CMAKE_REQUIRED_INCLUDES "${TCLAP_INCLUDE_DIR}")
    check_cxx_source_compiles("#include <tclap/CmdLine.h>
int main() { return 0; }" TCLAP_BUILDS_WITH_CXX_STANDARD)
    unset(CMAKE_REQUIRED_INCLUDES)
    if(NOT TCLAP_BUILDS_WITH_CXX_STANDARD)
      message(STATUS "TCLAP in ${TCLAP_INCLUDE_DIR} does not build as C++${CMAKE_CXX_STANDARD}.")
      set(TCLAP_FOUND FALSE)
    endif()
  endif()
endif()

if(NOT TCLAP_FOUND)
  if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/_deps/tclap-external-src")
    if(DEPENDENCIES_FORCE_DOWNLOAD)
      message(STATUS "DEPENDENCIES_FORCE_DOWNLOAD is ON. Fetching TCLAP.")
    else()
      message(STATUS "Fetching TCLAP.")
    endif()
  endif()
  cmake_minimum_required(VERSION 3.11)
  include(FetchContent)
  set(TCLAP_BUILD_EXAMPLES OFF CACHE BOOL "Build TCLAP examples")
  FetchContent_Declare(
    tclap-external
    GIT_REPOSITORY      https://github.com/mirror/tclap.git
    GIT_TAG             v1.2.5 # 58c5c8ef24111072fc21fb723f8ab45d23395809
    PATCH_COMMAND       ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" "${CMAKE_CURRENT_BINARY_DIR}/_deps/tclap-external-src/CMakeLists.txt"
  )
  FetchContent_MakeAvailable(tclap-external)
  list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_BINARY_DIR}/_deps/tclap-external-src/include")
  # the fetched headers, not installed ones the module may have found before
  set(TCLAP_INCLUDE_PATH "${CMAKE_CURRENT_BINARY_DIR}/_deps/tclap-external-src/include"
    CACHE PATH "Path to the TCLAP headers" FORCE)
  find_package(TCLAP REQUIRED)
endif()