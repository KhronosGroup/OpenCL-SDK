if(NOT DEPENDENCIES_FORCE_DOWNLOAD AND NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/_deps/glfw-external-src")
  find_package(glfw3 CONFIG)
  # To avoid every test depening on GLFW define their deps using
  #
  # add_sample(
  # LIBS
  #   $<$<TARGET_EXISTS:glfw>:glfw>
  # INCLUDES
  #   $<$<NOT:$<TARGET_EXISTS:glfw>>:"${GLFW_INCLUDE_DIRS}">
  # )
  #
  # we create the INTERFACE target in case it didn't exist.
  if(glfw3_FOUND AND NOT TARGET glfw)
    add_library(glfw INTERFACE)
    target_include_directories(glfw INTERFACE "${GLFW_INCLUDE_DIRS}")
  endif()
endif()

if(NOT (glfw3_FOUND OR TARGET glfw))
  if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/_deps/glfw-external-src")
    if(DEPENDENCIES_FORCE_DOWNLOAD)
      message(STATUS "DEPENDENCIES_FORCE_DOWNLOAD is ON. Fetching glfw.")
    else()
      message(STATUS "Fetching glfw.")
    endif()
    message(STATUS "Adding glfw subproject: ${CMAKE_CURRENT_BINARY_DIR}/_deps/glfw-external-src")
  endif()
  cmake_minimum_required(VERSION 3.11)
  include(FetchContent)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs.")
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs.")
  FetchContent_Declare(
    glfw-external
    GIT_REPOSITORY      https://github.com/glfw/glfw
    GIT_TAG             3.3.6 # 7d5a16ce714f0b5f4efa3262de22e4d948851525
  )
  FetchContent_MakeAvailable(glfw-external)
  set_target_properties(glfw
    PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}"
      ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}"
      LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}"
      INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
      FOLDER "Dependencies"
  )  
endif()
