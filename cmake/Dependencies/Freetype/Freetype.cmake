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
  set(FT_DISABLE_ZLIB ON CACHE BOOL "Disable use of system zlib and use internal zlib library instead.")
  set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "Disable HarfBuzz (used for improving auto-hinting of OpenType fonts).")
  set(FT_DISABLE_PNG ON CACHE BOOL "Disable support of PNG compressed OpenType embedded bitmaps.")
  set(FT_DISABLE_ZLIB ON CACHE BOOL "Disable FreeType dependence on HarfBuzz")
  set(FT_DISABLE_BZIP2 ON CACHE BOOL "Disable support of bzip2 compressed fonts.")
  set(FT_DISABLE_BROTLI ON CACHE BOOL "Disable support of compressed WOFF2 fonts.")
  FetchContent_Declare(
    freetype-external
    GIT_REPOSITORY      https://github.com/freetype/freetype.git
    GIT_TAG             VER-2-14-3 # 0a0221a1347e2f1e07c395263540026e9a0aa7c7
  )
  FetchContent_MakeAvailable(freetype-external)
  set_target_properties(freetype
    PROPERTIES
      POSITION_INDEPENDENT_CODE ON
      FOLDER "Dependencies"
  )
endif()