if(NOT DEPENDENCIES_FORCE_DOWNLOAD AND NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/_deps/freetype-external-src")
  find_package(Freetype)
endif()

if(NOT (Freetype_FOUND OR TARGET freetype))
  if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/_deps/freetype-external-src")
    if(DEPENDENCIES_FORCE_DOWNLOAD)
      message(STATUS "DEPENDENCIES_FORCE_DOWNLOAD is ON. Fetching FreeType.")
    else()
      message(STATUS "Fetching FreeType.")
    endif()
    message(STATUS "Adding FreeType subproject: ${CMAKE_CURRENT_BINARY_DIR}/_deps/freetype-external-src")
  endif()
  cmake_minimum_required(VERSION 3.11)
  include(FetchContent)
  set(SKIP_INSTALL_HEADERS ON CACHE BOOL "Skip installing FreeType headers")
  FetchContent_Declare(
    freetype-external
    GIT_REPOSITORY      https://github.com/freetype/freetype.git
    GIT_TAG             VER-2-11-1 # 3f83daeecb1a78d851b660eed025eeba362c0e4a
  )
  FetchContent_MakeAvailable(freetype-external)
  set_target_properties(freetype
    PROPERTIES
      POSITION_INDEPENDENT_CODE ON
      RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}"
      ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}"
      LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}"
      INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
      FOLDER "Dependencies"
  )
endif()