if(NOT DEPENDENCIES_FORCE_DOWNLOAD)
  find_package(Stb)
endif()

if(NOT Stb_FOUND)
  if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/_deps/stb-external-src")
    if(DEPENDENCIES_FORCE_DOWNLOAD)
      message(STATUS "DEPENDENCIES_FORCE_DOWNLOAD is ON. Fetching Stb.")
    else()
      message(STATUS "Fetching Stb.")
    endif()
  endif()
  cmake_minimum_required(VERSION 3.11)
  include(FetchContent)
  FetchContent_Declare(
    stb-external
    GIT_REPOSITORY      https://github.com/nothings/stb.git
    GIT_TAG             af1a5bc352164740c1cc1354942b1c6b72eacb8a
    UPDATE_COMMAND      ""
    PATCH_COMMAND       ""
  )
  FetchContent_MakeAvailable(stb-external)
  list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_BINARY_DIR}/_deps/stb-external-src")
  find_package(Stb REQUIRED)
endif()