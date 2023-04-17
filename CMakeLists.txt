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
  VERSION 2023.04.17
  LANGUAGES
    C CXX
)

include(CMakeDependentOption)
option(OPENCL_SDK_BUILD_SAMPLES "Build sample code" ON)
cmake_dependent_option(OPENCL_SDK_BUILD_OPENGL_SAMPLES "Build OpenCL-OpenGL interop sample code" ON OPENCL_SDK_BUILD_SAMPLES OFF)
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

list(APPEND CMAKE_MODULE_PATH
  ${PROJECT_SOURCE_DIR}/cmake
  ${PROJECT_SOURCE_DIR}/cmake/Modules
)
include(Dependencies)

add_subdirectory(lib)
if(OPENCL_SDK_BUILD_SAMPLES)
  add_subdirectory(samples)
endif()

include(GNUInstallDirs)
file(
  WRITE ${PROJECT_BINARY_DIR}/OpenCL/OpenCLConfig.cmake
  [[
get_filename_component(PARENT_DIR ${CMAKE_CURRENT_LIST_DIR} PATH)
include("${PARENT_DIR}/OpenCLHeaders/OpenCLHeadersConfig.cmake")
include("${PARENT_DIR}/OpenCLICDLoader/OpenCLICDLoaderConfig.cmake")
include("${PARENT_DIR}/OpenCLHeadersCpp/OpenCLHeadersCppConfig.cmake")
include("${PARENT_DIR}/OpenCLUtils/OpenCLUtilsConfig.cmake")
include("${PARENT_DIR}/OpenCLUtilsCpp/OpenCLUtilsCppConfig.cmake")
  ]]
)
set(config_package_location ${CMAKE_INSTALL_DATADIR}/cmake/OpenCL)
install(
  FILES ${PROJECT_BINARY_DIR}/OpenCL/OpenCLConfig.cmake
  DESTINATION ${config_package_location}
)

unset(CMAKE_SIZEOF_VOID_P)
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/OpenCL/OpenCLConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/OpenCL/OpenCLConfigVersion.cmake
  DESTINATION ${config_package_location}
)

include(CPack)
