cmake_minimum_required(VERSION 3.0)

project(OpenCL-SDK)

option(SDK_EXTERNAL_UPDATE "Disable update step for external dependencies" ON)

add_subdirectory(external)

#set_target_properties(OpenCL PROPERTIES LIBRARY_OUTPUT_DIRECTORY
#    ${CMAKE_CURRENT_BINARY_DIR})

