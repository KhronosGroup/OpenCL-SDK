cmake_minimum_required(
  VERSION 3.8 # CXX_STANDARD 17
)

project(ConfigureFile
  LANGUAGES CXX
)

find_package(OpenCL REQUIRED)

set(Files_CNFG ${CMAKE_CURRENT_BINARY_DIR}/KernelLocation.hpp)
set(Files_SRCS ${PROJECT_NAME}.cpp)
set(Files_KRNS SAXPY.cl)

get_filename_component(Path_KRNS
  ${Files_KRNS}
  ABSOLUTE # Obtain absolute path to kernel file
)

configure_file(
  KernelLocation.in.hpp
  ${Files_CNFG}
)

add_executable(${PROJECT_NAME}
  ${Files_CNFG}
  ${Files_SRCS}
  ${Files_KRNS}
)

set_target_properties(${PROJECT_NAME}
  PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
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