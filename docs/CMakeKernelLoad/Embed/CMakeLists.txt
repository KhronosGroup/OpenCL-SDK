cmake_minimum_required(
  VERSION 3.8 # CXX_STANDARD 17
)

project(Embed
  LANGUAGES CXX
)

find_package(OpenCL REQUIRED)

set(Files_SRCS ${PROJECT_NAME}.cpp)
set(Files_KRNS SAXPY.cl)

add_executable(${PROJECT_NAME}
  ${Files_SRCS}
  ${Files_KRNS}
)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Kernel.hpp
  COMMAND ${CMAKE_COMMAND}
  ARGS -D INPUT_FILE=SAXPY.cl
       -D VAR_NAME=Kernel_STRING
       -D CONFIG_FILE=Kernel.in.hpp
       -D OUTPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/Kernel.hpp
       -P ${PROJECT_SOURCE_DIR}/cmake/Embed.cmake
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  DEPENDS SAXPY.cl
  COMMENT "Embedding SAXPY.cl into Kernel.hpp"
)
add_custom_target(${PROJECT_NAME}-kernels
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Kernel.hpp
)
add_dependencies(${PROJECT_NAME}
  ${PROJECT_NAME}-kernels
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