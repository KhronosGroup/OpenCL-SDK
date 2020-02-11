
cmake_minimum_required(VERSION 3.0)

project(OpenCL-SDK)

set(OPENCL_ICD_LOADER_HEADERS_DIR
    "${PROJECT_SOURCE_DIR}/external/OpenCL-Headers" CACHE PATH "Path to OpenCL
    Headers")

add_subdirectory(${PROJECT_SOURCE_DIR}/external/OpenCL-ICD-Loader)

set_target_properties(OpenCL PROPERTIES LIBRARY_OUTPUT_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR})

