# Copyright (c) 2020 The Khronos Group Inc.
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

cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 14)

project(OpenCL-SDK
  VERSION 1.0
  LANGUAGES
    C CXX
)

include(CMakeDependentOption)
option(OPENCL_SDK_BUILD_SAMPLES "Build sample code" ON)
cmake_dependent_option(OPENCL_SDK_TEST_SAMPLES "Add CTest to samples (where applicable)" ON OPENCL_SDK_BUILD_SAMPLES OFF)

include(CTest)

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE "Release" CACHE PATH "Build Type" FORCE)
endif()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/install" CACHE PATH "Install Path" FORCE)
endif()

add_subdirectory(external/OpenCL-Headers)
add_subdirectory(external/OpenCL-ICD-Loader)
add_subdirectory(external/OpenCL-CLHPP)

add_subdirectory(lib)
add_subdirectory(samples)
add_subdirectory(third_party/cargs)

include (GNUInstallDirs)
set_target_properties(OpenCL OpenCLUtils OpenCLUtilsCpp
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}
)
