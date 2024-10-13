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

add_sample(
    TEST
    TARGET externalmemory
    VERSION 300
    SOURCES main.c
    KERNELS external_saxpy.cl
    LIBS Vulkan::Vulkan
    DEFINITIONS $<$<PLATFORM_ID:Windows>:VK_USE_PLATFORM_WIN32_KHR>)

add_sample(
    TEST
    TARGET externalmemorycpp
    VERSION 300
    SOURCES main.cpp
    KERNELS external_saxpy.cl
    LIBS Vulkan::Vulkan
    DEFINITIONS $<$<PLATFORM_ID:Windows>:VK_USE_PLATFORM_WIN32_KHR>)
