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
    set(multi_value_args SOURCES KERNELS SHADERS INCLUDES LIBS)
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
        cargs
        OpenCL::Headers
        OpenCL::HeadersCpp
        OpenCL::OpenCL
        OpenCL::Utils
        OpenCL::UtilsCpp
        OpenCL::SDK
        OpenCL::SDKCpp
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
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}"
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
        FOLDER "Samples/${OPENCL_SAMPLE_CATEGORY}/${OPENCL_SAMPLE_TARGET}"
    )

    # We register utility targets that copy kernel/shader code to build tree so that samples can be run from there.
    # We can't use OPENCL_SAMPLE_TARGET-device-code as a target name, because we often have two samples (C and CXX)
    # refering to the same device source code. Instead we use CURRENT_FOLDER_NAME and check for the utility target
    # already existing. If it does, we don't register redundant custom_commands and targets. (They would only cause
    # conflicting jobs in parallel MSBuild builds. Ninja is smart enough.) Either way, we add a convenience dependence
    # of OPENCL_SAMPLE_TARGET on the utility to make sure building the host code copies kernels too.
    get_filename_component(CURRENT_FOLDER_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
    if(NOT TARGET ${CURRENT_FOLDER_NAME}-device-code)
      set(DEVICE_CODE_OUTPUTS)
      foreach(DEVICE_CODE_SOURCE IN LISTS OPENCL_SAMPLE_KERNELS OPENCL_SAMPLE_SHADERS)
        # NOTE: if() and foreach() could be omitted if CMake ver > 3.20 (COMMAND and OUTPUT needs genexpr)
        if(CMAKE_CONFIGURATION_TYPES)
          foreach(CONFIG_TYPE IN LISTS CMAKE_CONFIGURATION_TYPES)
            add_custom_command(
              OUTPUT "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/${CONFIG_TYPE}/${DEVICE_CODE_SOURCE}"
              COMMAND ${CMAKE_COMMAND}
              ARGS -E copy_if_different
                   "${CMAKE_CURRENT_LIST_DIR}/${DEVICE_CODE_SOURCE}"
                   "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/${CONFIG_TYPE}/${DEVICE_CODE_SOURCE}"
              DEPENDS ${DEVICE_CODE_SOURCE}
              COMMENT "Copying ${DEVICE_CODE_SOURCE}"
            )
            list(APPEND DEVICE_CODE_OUTPUTS "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/${CONFIG_TYPE}/${DEVICE_CODE_SOURCE}")
          endforeach()
        else()
          add_custom_command(
            OUTPUT "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/${DEVICE_CODE_SOURCE}"
            COMMAND ${CMAKE_COMMAND}
            ARGS -E copy_if_different
                 "${CMAKE_CURRENT_LIST_DIR}/${DEVICE_CODE_SOURCE}"
                 "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/${DEVICE_CODE_SOURCE}"
            DEPENDS ${DEVICE_CODE_SOURCE}
            COMMENT "Copying ${DEVICE_CODE_SOURCE}"
          )
          list(APPEND DEVICE_CODE_OUTPUTS "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/${DEVICE_CODE_SOURCE}")
        endif()
      endforeach()
      add_custom_target(${CURRENT_FOLDER_NAME}-device-code
        DEPENDS ${DEVICE_CODE_OUTPUTS}
      )
    endif()
    add_dependencies(${OPENCL_SAMPLE_TARGET}
      ${CURRENT_FOLDER_NAME}-device-code
    )

    foreach(CONFIG ${OPENCL_SAMPLE_CONFIGS})
        install(TARGETS ${OPENCL_SAMPLE_TARGET} CONFIGURATIONS ${CONFIG} DESTINATION ${CMAKE_INSTALL_BINDIR})
        install(FILES ${OPENCL_SAMPLE_KERNELS} CONFIGURATIONS ${CONFIG} DESTINATION ${CMAKE_INSTALL_BINDIR})
        install(FILES ${OPENCL_SAMPLE_SHADERS} CONFIGURATIONS ${CONFIG} DESTINATION ${CMAKE_INSTALL_BINDIR})
    endforeach()
    if(OPENCL_SDK_TEST_SAMPLES AND OPENCL_SAMPLE_TEST)
        add_test(
          NAME ${OPENCL_SAMPLE_TARGET}
          COMMAND ${OPENCL_SAMPLE_TARGET}
          WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        )
    endif()
endmacro()


add_subdirectory(core)
add_subdirectory(extensions)
