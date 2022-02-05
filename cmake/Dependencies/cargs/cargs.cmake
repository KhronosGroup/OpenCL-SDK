if(NOT DEPENDENCIES_FORCE_DOWNLOAD)
  find_path(
    cargs_INCLUDE_PATH cargs.h
    PATHS ${cargs_DIR}
    PATH_SUFFIXES
      inc
      include
  )
  find_library(
    cargs_LIBRARY cargs
    PATHS ${cargs_DIR}
    PATH_SUFFIXES
      lib
  )
endif()

if(cargs_INCLUDE_PATH AND cargs_LIBRARY AND NOT DEPENDENCIES_FORCE_DOWNLOAD)
  message(STATUS "Found cargs: ${cargs_LIBRARY}")
  add_library(cargs INTERFACE)
  target_include_directories(cargs INTERFACE "${cargs_INCLUDE_PATH}")
  target_link_libraries(cargs INTERFACE "${cargs_LIBRARY}")
else()
  message(STATUS
    "cargs not found or DEPENDENCIES_FORCE_DOWNLOAD is ON. Fetching cargs."
  )
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
      RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}"
      INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
      FOLDER "Dependencies"
  )
endif()