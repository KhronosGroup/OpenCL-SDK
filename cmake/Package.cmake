if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    include("${CMAKE_CURRENT_LIST_DIR}/PackageSetup.cmake")
    set(CPACK_DEBIAN_PACKAGE_DEBUG ON)
endif()
