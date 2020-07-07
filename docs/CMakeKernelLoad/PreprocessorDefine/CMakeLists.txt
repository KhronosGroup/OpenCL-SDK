cmake_minimum_required(
  VERSION 3.8 # CXX_STANDARD 17
)

project(PreprocessorDefine
  LANGUAGES CXX
)

find_package(OpenCL REQUIRED)

set(Files_SRCS ${PROJECT_NAME}.cpp)
set(Files_KRNS SAXPY.cl)

add_executable(${PROJECT_NAME}
  ${Files_SRCS}
  ${Files_KRNS}
)

file(
  COPY
    ${Files_KRNS}
  DESTINATION
    ${CMAKE_CURRENT_BINARY_DIR}
)
file(TO_CMAKE_PATH
  "${CMAKE_CURRENT_BINARY_DIR}/${Files_KRNS}"
  Files_KRNS_PATH
)

set_target_properties(${PROJECT_NAME}
  PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    OpenCL::OpenCL
)

target_compile_definitions(${PROJECT_NAME}
  PRIVATE
    CL_HPP_ENABLE_EXCEPTIONS
    CL_HPP_MINIMUM_OPENCL_VERSION=100
    CL_HPP_TARGET_OPENCL_VERSION=120
    KERNEL_PATH="${Files_KRNS_PATH}"
)

target_compile_options(${PROJECT_NAME}
  PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
      -Wall     # Turn on all warnings
      -Wextra   # Turn on even more warnings
      -pedantic # Turn on strict language conformance
    >
    $<$<CXX_COMPILER_ID:MSVC>:
      /W4          # Turn on all (sensible) warnings
      /permissive- # Turn on strict language conformance
      /EHsc        # Specify exception handling model
    >
)