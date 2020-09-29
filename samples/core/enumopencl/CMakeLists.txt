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

set(SAMPLE_NAME enumopencl)

project(${SAMPLE_NAME})
add_executable(${SAMPLE_NAME} main.cpp)

target_include_directories(${SAMPLE_NAME} PRIVATE ${OPENCL_SDK_INCLUDE_DIRS})
target_link_libraries(${SAMPLE_NAME} OpenCL)

target_compile_definitions(${SAMPLE_NAME} PRIVATE CL_TARGET_OPENCL_VERSION=120)

set_target_properties(${SAMPLE_NAME} PROPERTIES FOLDER "Samples/core/${SAMPLE_NAME}")

if(CMAKE_CONFIGURATION_TYPES)
    set(SAMPLE_CONFIGS ${CMAKE_CONFIGURATION_TYPES})
else()
    set(SAMPLE_CONFIGS ${CMAKE_BUILD_TYPE})
endif()
foreach(CONFIG ${SAMPLE_CONFIGS})
    install(TARGETS ${SAMPLE_NAME} CONFIGURATIONS ${CONFIG} DESTINATION ${CONFIG})
endforeach()
