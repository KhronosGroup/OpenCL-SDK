if(NOT DEPENDENCIES_FORCE_DOWNLOAD AND NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/_deps/cargs-external-src")
  find_package(cargs)
endif()

if(NOT (cargs_FOUND OR TARGET cargs))
  if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/_deps/cargs-external-src")
    if(DEPENDENCIES_FORCE_DOWNLOAD)
      message(STATUS "DEPENDENCIES_FORCE_DOWNLOAD is ON. Fetching TCLAP.")
    else()
      message(STATUS "Fetching cargs.")
    endif()
    message(STATUS "Adding cargs subproject: ${CMAKE_CURRENT_BINARY_DIR}/_deps/cargs-external-src")
  endif()
  cmake_minimum_required(VERSION 3.11)
  include(FetchContent)
  FetchContent_Declare(
    cargs-external
    GIT_REPOSITORY      https://github.com/likle/cargs.git
    GIT_TAG             v1.0.1 # 70173e67a1b7da9f0f37f6bb3ac50e0cefc29558
    PATCH_COMMAND       ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" "${CMAKE_CURRENT_BINARY_DIR}/_deps/cargs-external-src/CMakeLists.txt"
  )
  FetchContent_MakeAvailable(cargs-external)
  set_target_properties(cargs
    PROPERTIES
      POSITION_INDEPENDENT_CODE ON
      RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}"
      INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
      FOLDER "Dependencies"
  )
endif()