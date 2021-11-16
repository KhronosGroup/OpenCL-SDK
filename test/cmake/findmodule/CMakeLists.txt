cmake_minimum_required(VERSION 3.0)

project(FindModuleTest)

include(CTest)

find_package(OpenCL
  MODULE
  REQUIRED
)

# Test consuming from C++ source files
#
# NOTE: At the time of writing, the presence of the
#       C++ headers don't manifest as a COMPONENT.
#       They are usually expected to reside next to
#       the C headers, even though they live in
#       different `apt` packages for eg.

add_executable(${PROJECT_NAME}_cpp
  ../platformenum.cpp
)

if(${CMAKE_VERSION} VERSION_LESS 3.7)
  target_include_directories(${PROJECT_NAME}_cpp
    PRIVATE
      ${OpenCL_INCLUDE_DIR}
  )
  target_link_libraries(${PROJECT_NAME}_cpp
    PRIVATE
      ${OpenCL_LIBRARY}
  )
else()
  target_link_libraries(${PROJECT_NAME}_cpp
    PRIVATE
      OpenCL::OpenCL
  )
endif()

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
  ../platformenum.c
)

if(${CMAKE_VERSION} VERSION_LESS 3.7)
  target_include_directories(${PROJECT_NAME}_c
    PRIVATE
      ${OpenCL_INCLUDE_DIR}
  )
  target_link_libraries(${PROJECT_NAME}_c
    PRIVATE
      ${OpenCL_LIBRARY}
  )
else()
  target_link_libraries(${PROJECT_NAME}_c
    PRIVATE
      OpenCL::OpenCL
  )
endif()

target_compile_definitions(${PROJECT_NAME}_c
  PRIVATE
    CL_TARGET_OPENCL_VERSION=300
)

add_test(
  NAME ${PROJECT_NAME}_c
  COMMAND ${PROJECT_NAME}_c
)
