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

set(CMAKE_CXX_STANDARD 11)

project(OpenCL-SDK)

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE "Release" CACHE PATH "Build Type" FORCE)
endif()

set(OPENCL_ICD_LOADER_HEADERS_DIR
    "${PROJECT_SOURCE_DIR}/external/OpenCL-Headers" CACHE PATH "Path to OpenCL
    Headers")

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/install" CACHE PATH "Install Path" FORCE)
endif()

set(OPENCL_SDK_INCLUDE_DIRS
    "${PROJECT_SOURCE_DIR}/external/OpenCL-Headers"
    "${PROJECT_SOURCE_DIR}/external/OpenCL-CLHPP/include")

add_subdirectory(${PROJECT_SOURCE_DIR}/external/OpenCL-ICD-Loader)
add_subdirectory(${PROJECT_SOURCE_DIR}/samples)

set_target_properties(OpenCL PROPERTIES LIBRARY_OUTPUT_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR})
