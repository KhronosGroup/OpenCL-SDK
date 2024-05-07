# Copyright (c) 2023 The Khronos Group Inc.
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

find_package(Threads)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/threads.c"
"#include <threads.h>
#include <time.h>
int main(void) { thrd_sleep(&(struct timespec){.tv_nsec=1}, NULL); }
")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/atomic.c"
"#include <stdatomic.h>
int main() { return 0; }
")

# Signature can be modernized at CMake version 3.25
try_compile(
    HAS_C11_THREADS
    "${CMAKE_CURRENT_BINARY_DIR}"
    SOURCES "${CMAKE_CURRENT_BINARY_DIR}/threads.c"
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

try_compile(
    HAS_C11_ATOMICS
    "${CMAKE_CURRENT_BINARY_DIR}"
    SOURCES "${CMAKE_CURRENT_BINARY_DIR}/atomic.c"
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

if (NOT HAS_C11_THREADS)
    message(WARNING
    "Skipping callback sample, C11 standard threads are not supported with the current toolset")
elseif (NOT HAS_C11_ATOMICS)
    message(WARNING
    "Skipping callback sample, C11 standard atomics are not supported with the current toolset")
else()
    add_sample(
        TEST
        TARGET callback
        VERSION 300
        SOURCES main.c
        KERNELS reaction_diffusion.cl)
    target_link_libraries(callback PRIVATE
        $<TARGET_NAME_IF_EXISTS:Threads::Threads>)
endif()

add_sample(
    TEST
    TARGET callbackcpp
    VERSION 300
    SOURCES main.cpp
    KERNELS reaction_diffusion.cl)
target_link_libraries(callbackcpp PRIVATE
    $<TARGET_NAME_IF_EXISTS:Threads::Threads>)
