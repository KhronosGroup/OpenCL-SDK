
cmake_minimum_required(VERSION 3.0)

project(OpenCL-SDK)

set(OPENCL_ICD_LOADER_HEADERS_DIR
    "${PROJECT_SOURCE_DIR}/external/OpenCL-Headers" CACHE PATH "Path to OpenCL
    Headers")

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/install" CACHE PATH "Install Path" FORCE)
endif()

add_subdirectory(${PROJECT_SOURCE_DIR}/external/OpenCL-ICD-Loader)
add_subdirectory(${PROJECT_SOURCE_DIR}/samples/core)

set_target_properties(OpenCL PROPERTIES LIBRARY_OUTPUT_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR})
