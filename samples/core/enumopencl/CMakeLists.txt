# Copyright (c) 2019-2020 Ben Ashbaugh
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

set(SAMPLE_NAME enumopencl)

project(${SAMPLE_NAME})
add_executable(${SAMPLE_NAME} main.cpp)

target_include_directories(${SAMPLE_NAME} PRIVATE ${OPENCL_ICD_LOADER_HEADERS_DIR})
target_link_libraries(${SAMPLE_NAME} OpenCL)

target_compile_definitions(${SAMPLE_NAME} PRIVATE CL_TARGET_OPENCL_VERSION=120)

set_target_properties( ${SAMPLE_NAME} PROPERTIES FOLDER "Samples/1.2_${SAMPLE_NAME}" )

if(CMAKE_CONFIGURATION_TYPES)
    set(SAMPLE_CONFIGS ${CMAKE_CONFIGURATION_TYPES})
else()
    set(SAMPLE_CONFIGS ${CMAKE_BUILD_TYPE})
endif()
foreach(CONFIG ${SAMPLE_CONFIGS})
    install(TARGETS ${SAMPLE_NAME} CONFIGURATIONS ${CONFIG} DESTINATION ${CONFIG})
endforeach()
