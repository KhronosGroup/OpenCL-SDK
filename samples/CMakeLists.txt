# Copyright (c) 2021 The Khronos Group Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

if(CMAKE_CONFIGURATION_TYPES)
    set(OPENCL_SAMPLE_CONFIGS ${CMAKE_CONFIGURATION_TYPES})
else()
    set(OPENCL_SAMPLE_CONFIGS ${CMAKE_BUILD_TYPE})
endif()

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules)
find_package(TCLAP REQUIRED)
# add math library to link if needed
# method by Michael Ambrus, https://stackoverflow.com/questions/34625627
include(CheckLibraryExists)
CHECK_LIBRARY_EXISTS(m sin "" HAVE_LIB_M)

# Usage:
# add_sample(
#     TEST              # optional, adds a ctest for the sample
#     TARGET <name>     # specifies the name of the sample
#     VERSION <number>  # specifies the OpenCL version for the sample
#     CATEGORY <name>   # optional, specifies a category for the sample
#     SOURCES <file0> <file1> ... # specifies source files for the sample
#     KERNELS <file0> <file1> ... # optional, specifies kernel files for the sample
#     INCLUDES <dir0> <dir1> ...  # optional, specifies additional include directories for the sample
#     LIBS <lib0> <lib1> ...      # optional, specifies additional libraries for the sample
# )
macro(add_sample)
    set(options TEST)
    set(one_value_args TARGET VERSION CATEGORY)
    set(multi_value_args SOURCES KERNELS INCLUDES LIBS)
    cmake_parse_arguments(OPENCL_SAMPLE
        "${options}" "${one_value_args}" "${multi_value_args}"
        ${ARGN}
    )

    if(NOT OPENCL_SAMPLE_VERSION)
        message(STATUS "No OpenCL version specified for sample ${OPENCL_SAMPLE_TARGET}, using OpenCL 3.0.")
        set(OPENCL_SAMPLE_VERSION 300)
    endif()

    add_executable(${OPENCL_SAMPLE_TARGET} ${OPENCL_SAMPLE_SOURCES})

    target_include_directories(${OPENCL_SAMPLE_TARGET}
      PRIVATE
        ${OPENCL_SDK_INCLUDE_DIRS}
        ${OPENCL_SAMPLE_INCLUDES}
    )
    target_link_libraries(${OPENCL_SAMPLE_TARGET}
      PRIVATE
        stb
        cargs
        OpenCL::Headers
        OpenCL::HeadersCpp
        OpenCL::OpenCL
        OpenCL::Utils
        OpenCL::UtilsCpp
        $<$<BOOL:${HAVE_LIB_M}>:m>
        ${OPENCL_SAMPLE_LIBS}
    )
    target_compile_definitions(${OPENCL_SAMPLE_TARGET}
      PRIVATE
        CL_TARGET_OPENCL_VERSION=${OPENCL_SAMPLE_VERSION}
        CL_HPP_TARGET_OPENCL_VERSION=${OPENCL_SAMPLE_VERSION}
        CL_HPP_MINIMUM_OPENCL_VERSION=${OPENCL_SAMPLE_VERSION}
        CL_HPP_ENABLE_EXCEPTIONS
        $<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS> # TODO: remove
    )

    set_target_properties(${OPENCL_SAMPLE_TARGET}
      PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}
        FOLDER "Samples/${OPENCL_SAMPLE_CATEGORY}/${OPENCL_SAMPLE_TARGET}"
    )

    foreach(CONFIG ${OPENCL_SAMPLE_CONFIGS})
        install(TARGETS ${OPENCL_SAMPLE_TARGET} CONFIGURATIONS ${CONFIG} DESTINATION ${CONFIG})
        install(FILES ${OPENCL_SAMPLE_KERNELS} CONFIGURATIONS ${CONFIG} DESTINATION ${CONFIG})
    endforeach()
    if(OPENCL_SDK_TEST_SAMPLES)
        add_test(
          NAME ${OPENCL_SAMPLE_TARGET}
          COMMAND ${OPENCL_SAMPLE_TARGET}
          WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        )
    endif()
endmacro()


add_subdirectory(core)
add_subdirectory(extensions)
