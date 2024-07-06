cmake_minimum_required(VERSION 3.16)

project(PkgConfigTest-UseUtil)

include(CTest)
include("${CMAKE_CURRENT_SOURCE_DIR}/../../../../cmake/Dependencies/whereami/whereami.cmake")

find_package(OpenCL
  REQUIRED
  CONFIG
  COMPONENTS
    Utils
    UtilsCpp
    HeadersCpp
    Headers
    OpenCL
)

# Test consuming from C++ source files

add_executable(${PROJECT_NAME}_cpp
  ../../useutil.cpp
)

target_link_libraries(${PROJECT_NAME}_cpp
  PRIVATE
    OpenCL::UtilsCpp
    OpenCL::HeadersCpp
    OpenCL::Headers
    OpenCL::OpenCL
    whereami
)

target_compile_definitions(${PROJECT_NAME}_cpp
  PRIVATE
    CL_HPP_ENABLE_EXCEPTIONS
    CL_HPP_TARGET_OPENCL_VERSION=300
)

add_test(
  NAME ${PROJECT_NAME}_cpp
  COMMAND ${PROJECT_NAME}_cpp
)

# Test consuming from C source files

add_executable(${PROJECT_NAME}_c
  ../../useutil.c
)

target_link_libraries(${PROJECT_NAME}_c
  PRIVATE
    OpenCL::Utils
    OpenCL::Headers
    OpenCL::OpenCL
    whereami
)

target_compile_definitions(${PROJECT_NAME}_c
  PRIVATE
    CL_TARGET_OPENCL_VERSION=300
)

add_test(
  NAME ${PROJECT_NAME}_c
  COMMAND ${PROJECT_NAME}_c
)
