cmake_minimum_required(VERSION 3.11) # specify include directories for imported target
cmake_policy(VERSION 3.11...3.18)

project(OpenCL-SDK)

option(SDK_SUBMODULE_DOWNLOAD "Download SDK Submodules as part of the build" ON)
option(SDK_EXTERNAL_UPDATE "Update step for external dependencies" ON)

add_subdirectory(external)

add_subdirectory(samples)

include(GNUInstallDirs)

foreach(CONFIGSCRIPT IN ITEMS
  ${PROJECT_SOURCE_DIR}/cmake/OpenCLConfig.cmake
  ${PROJECT_SOURCE_DIR}/cmake/OpenCLConfigVersion.cmake
)
  file(
    COPY ${CONFIGSCRIPT}
    DESTINATION ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/cmake/OpenCL
    PATTERN "cmake"
  )
endforeach()

install(
  DIRECTORY
    ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_INCLUDEDIR}
    ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}
    ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}
    ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}
  DESTINATION
    ${CMAKE_INSTALL_PREFIX}
)