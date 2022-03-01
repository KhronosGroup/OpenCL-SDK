if(NOT DEPENDENCIES_FORCE_DOWNLOAD AND NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/_deps/sfml-external-src")
  find_package(SFML 2
    QUIET
    CONFIG
    COMPONENTS window graphics
  )
endif()

if(NOT (SFML_FOUND OR TARGET SFML::Graphics))
  if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/_deps/sfml-external-src")
    if(DEPENDENCIES_FORCE_DOWNLOAD)
      message(STATUS "DEPENDENCIES_FORCE_DOWNLOAD is ON. Fetching SFML.")
    else()
      message(STATUS "Fetching SFML.")
    endif()
    message(STATUS "Adding SFML subproject: ${CMAKE_CURRENT_BINARY_DIR}/_deps/sfml-external-src")
  endif()
  cmake_minimum_required(VERSION 3.11)
  include(FetchContent)
  set(SFML_BUILD_WINDOW ON CACHE BOOL "TRUE to build SFML's Window module. This setting is ignored, if the graphics module is built.")
  set(SFML_BUILD_GRAPHICS ON CACHE BOOL "TRUE to build SFML's Graphics module.")
  set(SFML_BUILD_AUDIO OFF CACHE BOOL "TRUE to build SFML's Audio module.")
  set(SFML_BUILD_NETWORK OFF CACHE BOOL "TRUE to build SFML's Network module.")
  FetchContent_Declare(
    sfml-external
    GIT_REPOSITORY      https://github.com/SFML/SFML.git
    GIT_TAG             2.5.1 # 2f11710abc5aa478503a7ff3f9e654bd2078ebab
    PATCH_COMMAND       ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" "${CMAKE_CURRENT_BINARY_DIR}/_deps/sfml-external-src/src/SFML/Graphics/CMakeLists.txt"
  )
  FetchContent_MakeAvailable(sfml-external)
  set_target_properties(sfml-window sfml-graphics sfml-system
    PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}"
      ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}"
      LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}"
      INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
      FOLDER "Dependencies"
  )
endif()