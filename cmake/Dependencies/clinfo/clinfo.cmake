if(NOT DEPENDENCIES_FORCE_DOWNLOAD)
  find_package(clinfo QUIET)
endif()

if(NOT clinfo_FOUND)
  if(DEPENDENCIES_FORCE_DOWNLOAD)
    message(STATUS "DEPENDENCIES_FORCE_DOWNLOAD is ON. Fetching clinfo.")
  else()
    message(STATUS "Fetching clinfo.")
  endif()

  include(FetchContent)
  FetchContent_Declare(
    clinfo
    GIT_REPOSITORY      https://github.com/Oblomov/clinfo.git
    GIT_TAG             3.0.23.01.25
    PATCH_COMMAND       ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_LIST_DIR}/patch.cmake"
  )
  FetchContent_MakeAvailable(clinfo)
endif()
