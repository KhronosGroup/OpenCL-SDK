# Copyright (c) 2024 Mobica Limited, Marcin Hajder
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
    TARGET ocean_vk_ocl_interop
    VERSION 300 # clCreateImageWithProperties
    CATEGORY vulkan
    SOURCES main.cpp ocean.cpp ocean.hpp ocean_util.hpp
    SHADERS ocean.vert.spv ocean.frag.spv
    KERNELS twiddle.cl time_spectrum.cl inversion.cl normals.cl fft_kernel.cl init_spectrum.cl
    INCLUDES ${Vulkan_INCLUDE_DIR}
    LIBS ${Vulkan_LIBRARY} glfw glm::glm)
