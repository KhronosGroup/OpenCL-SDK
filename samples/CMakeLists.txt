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

if(CMAKE_CONFIGURATION_TYPES)
    set(SAMPLE_CONFIGS ${CMAKE_CONFIGURATION_TYPES})
else()
    set(SAMPLE_CONFIGS ${CMAKE_BUILD_TYPE})
endif()

# Usage: add_sample(name source0 [source1] ...)
function(add_sample name)
  add_executable(${name} ${ARGN})
  target_include_directories(${name} PRIVATE ${OPENCL_SDK_INCLUDE_DIRS})
  target_link_libraries(${name} OpenCL)
  foreach(config ${SAMPLE_CONFIGS})
    install(TARGETS ${name} CONFIGURATIONS ${config} DESTINATION ${config})
  endforeach()
endfunction()

add_subdirectory(core)
