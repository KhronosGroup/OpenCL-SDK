if(NOT DEPENDENCIES_FORCE_DOWNLOAD AND NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/_deps/glm-external-src")
  find_package(glm CONFIG)
  # NOTE 1: GLM 0.9.9.0 in Ubuntu 18.04 repo doesn't install the IMPORTED
  #         INTERFACE target, only the legacy variable is defined in glm-config.cmake
  # NOTE 2: auto-fetched subproject build doesn't define the (legacy) variable
  #         anymore, only the INTERFACE target
  #
  # To avoid every test depening on GLM define their deps using
  #
  # add_sample(
  # LIBS
  #   $<$<TARGET_EXISTS:glm::glm>:glm::glm>
  # INCLUDES
  #   $<$<NOT:$<TARGET_EXISTS:glm::glm>>:"${GLM_INCLUDE_DIRS}">
  # )
  #
  # we create the INTERFACE target in case it didn't exist.
  if(glm_FOUND AND NOT TARGET glm::glm)
    add_library(glm::glm INTERFACE)
    target_include_directories(glm::glm INTERFACE "${GLM_INCLUDE_DIRS}")
  endif()
endif()

if(NOT (glm_FOUND OR TARGET glm::glm))
  if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/_deps/glm-external-src")
    if(DEPENDENCIES_FORCE_DOWNLOAD)
      message(STATUS "DEPENDENCIES_FORCE_DOWNLOAD is ON. Fetching glm.")
    else()
      message(STATUS "Fetching glm.")
    endif()
    message(STATUS "Adding glm subproject: ${CMAKE_CURRENT_BINARY_DIR}/_deps/glm-external-src")
  endif()
  cmake_minimum_required(VERSION 3.11)
  include(FetchContent)
  FetchContent_Declare(
    glm-external
    GIT_REPOSITORY      https://github.com/g-truc/glm
    GIT_TAG             0.9.9.8 # e79109058964d8d18b8a3bb7be7b900be46692ad
  )
  FetchContent_MakeAvailable(glm-external)
endif()