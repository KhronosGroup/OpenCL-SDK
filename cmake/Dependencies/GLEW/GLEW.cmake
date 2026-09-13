if(NOT DEPENDENCIES_FORCE_DOWNLOAD AND NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/_deps/glew-external-src")
  find_package(GLEW)
endif()

if(NOT (TARGET GLEW::GLEW OR TARGET glew))
  if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/_deps/glew-external-src")
    if(DEPENDENCIES_FORCE_DOWNLOAD)
      message(STATUS "DEPENDENCIES_FORCE_DOWNLOAD is ON. Fetching GLEW.")
    else()
      message(STATUS "Fetching GLEW.")
    endif()
    message(STATUS "Adding GLEW subproject: ${CMAKE_CURRENT_BINARY_DIR}/_deps/glew-external-src")
  endif()
  cmake_minimum_required(VERSION 3.11)
  include(FetchContent)
  set(ONLY_LIBS ON CACHE BOOL "Build only the GLEW libs")
  set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "Build shared GLEW library")
  FetchContent_Declare(
    glew-external
    GIT_REPOSITORY      https://github.com/Perlmint/glew-cmake.git
    GIT_TAG             glew-cmake-2.3.1 # 2f38fcbb3b6c051130a4f624675584a11bfaaf9f
  )
  FetchContent_MakeAvailable(glew-external)
  add_library(GLEW::GLEW ALIAS libglew_static)
  set_target_properties(libglew_static
    PROPERTIES
      POSITION_INDEPENDENT_CODE ON
      FOLDER "Dependencies"
  )
endif()