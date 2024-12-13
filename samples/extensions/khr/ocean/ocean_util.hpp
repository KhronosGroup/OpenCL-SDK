/*
 * Copyright (c) 2024 Mobica Limited, Marcin Hajder, Piotr Plebański
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OCEAN_UTIL_HPP
#define OCEAN_UTIL_HPP

#include <GL/glew.h>

#include <CL/cl.h>
#include <CL/Utils/Utils.hpp>
#include <CL/SDK/SDK.hpp>

#include <glm/glm.hpp>

#include <iostream>

#define CL_HPP_TARGET_OPENCL_VERSION 300

const float DRAG_SPEED_FAC = 0.2f;
const float ROLL_SPEED_FAC = 8.f;

inline bool checkError(const char* Title)
{
    int Error;
    if ((Error = glGetError()) != GL_NO_ERROR)
    {
        std::string ErrorString;
        switch (Error)
        {
            case GL_INVALID_ENUM: ErrorString = "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE: ErrorString = "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:
                ErrorString = "GL_INVALID_OPERATION";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                ErrorString = "GL_INVALID_FRAMEBUFFER_OPERATION";
                break;
            case GL_OUT_OF_MEMORY: ErrorString = "GL_OUT_OF_MEMORY"; break;
            default: ErrorString = "UNKNOWN"; break;
        }
        std::cerr << "OpenGL Error(" << ErrorString << "): " << Title
                  << std::endl;
    }
    return Error == GL_NO_ERROR;
}


static const char* IGetErrorString(int clErrorCode)
{
    switch (clErrorCode)
    {
        case CL_SUCCESS: return "CL_SUCCESS";
        case CL_DEVICE_NOT_FOUND: return "CL_DEVICE_NOT_FOUND";
        case CL_DEVICE_NOT_AVAILABLE: return "CL_DEVICE_NOT_AVAILABLE";
        case CL_COMPILER_NOT_AVAILABLE: return "CL_COMPILER_NOT_AVAILABLE";
        case CL_MEM_OBJECT_ALLOCATION_FAILURE:
            return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
        case CL_OUT_OF_RESOURCES: return "CL_OUT_OF_RESOURCES";
        case CL_OUT_OF_HOST_MEMORY: return "CL_OUT_OF_HOST_MEMORY";
        case CL_PROFILING_INFO_NOT_AVAILABLE:
            return "CL_PROFILING_INFO_NOT_AVAILABLE";
        case CL_MEM_COPY_OVERLAP: return "CL_MEM_COPY_OVERLAP";
        case CL_IMAGE_FORMAT_MISMATCH: return "CL_IMAGE_FORMAT_MISMATCH";
        case CL_IMAGE_FORMAT_NOT_SUPPORTED:
            return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
        case CL_BUILD_PROGRAM_FAILURE: return "CL_BUILD_PROGRAM_FAILURE";
        case CL_MAP_FAILURE: return "CL_MAP_FAILURE";
        case CL_MISALIGNED_SUB_BUFFER_OFFSET:
            return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
        case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST:
            return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
        case CL_COMPILE_PROGRAM_FAILURE: return "CL_COMPILE_PROGRAM_FAILURE";
        case CL_LINKER_NOT_AVAILABLE: return "CL_LINKER_NOT_AVAILABLE";
        case CL_LINK_PROGRAM_FAILURE: return "CL_LINK_PROGRAM_FAILURE";
        case CL_DEVICE_PARTITION_FAILED: return "CL_DEVICE_PARTITION_FAILED";
        case CL_KERNEL_ARG_INFO_NOT_AVAILABLE:
            return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";
        case CL_INVALID_VALUE: return "CL_INVALID_VALUE";
        case CL_INVALID_DEVICE_TYPE: return "CL_INVALID_DEVICE_TYPE";
        case CL_INVALID_DEVICE: return "CL_INVALID_DEVICE";
        case CL_INVALID_CONTEXT: return "CL_INVALID_CONTEXT";
        case CL_INVALID_QUEUE_PROPERTIES: return "CL_INVALID_QUEUE_PROPERTIES";
        case CL_INVALID_COMMAND_QUEUE: return "CL_INVALID_COMMAND_QUEUE";
        case CL_INVALID_HOST_PTR: return "CL_INVALID_HOST_PTR";
        case CL_INVALID_MEM_OBJECT: return "CL_INVALID_MEM_OBJECT";
        case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
            return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
        case CL_INVALID_IMAGE_SIZE: return "CL_INVALID_IMAGE_SIZE";
        case CL_INVALID_SAMPLER: return "CL_INVALID_SAMPLER";
        case CL_INVALID_BINARY: return "CL_INVALID_BINARY";
        case CL_INVALID_BUILD_OPTIONS: return "CL_INVALID_BUILD_OPTIONS";
        case CL_INVALID_PLATFORM: return "CL_INVALID_PLATFORM";
        case CL_INVALID_PROGRAM: return "CL_INVALID_PROGRAM";
        case CL_INVALID_PROGRAM_EXECUTABLE:
            return "CL_INVALID_PROGRAM_EXECUTABLE";
        case CL_INVALID_KERNEL_NAME: return "CL_INVALID_KERNEL_NAME";
        case CL_INVALID_KERNEL_DEFINITION:
            return "CL_INVALID_KERNEL_DEFINITION";
        case CL_INVALID_KERNEL: return "CL_INVALID_KERNEL";
        case CL_INVALID_ARG_INDEX: return "CL_INVALID_ARG_INDEX";
        case CL_INVALID_ARG_VALUE: return "CL_INVALID_ARG_VALUE";
        case CL_INVALID_ARG_SIZE: return "CL_INVALID_ARG_SIZE";
        case CL_INVALID_KERNEL_ARGS: return "CL_INVALID_KERNEL_ARGS";
        case CL_INVALID_WORK_DIMENSION: return "CL_INVALID_WORK_DIMENSION";
        case CL_INVALID_WORK_GROUP_SIZE: return "CL_INVALID_WORK_GROUP_SIZE";
        case CL_INVALID_WORK_ITEM_SIZE: return "CL_INVALID_WORK_ITEM_SIZE";
        case CL_INVALID_GLOBAL_OFFSET: return "CL_INVALID_GLOBAL_OFFSET";
        case CL_INVALID_EVENT_WAIT_LIST: return "CL_INVALID_EVENT_WAIT_LIST";
        case CL_INVALID_EVENT: return "CL_INVALID_EVENT";
        case CL_INVALID_OPERATION: return "CL_INVALID_OPERATION";
        case CL_INVALID_GL_OBJECT: return "CL_INVALID_GL_OBJECT";
        case CL_INVALID_BUFFER_SIZE: return "CL_INVALID_BUFFER_SIZE";
        case CL_INVALID_MIP_LEVEL: return "CL_INVALID_MIP_LEVEL";
        case CL_INVALID_GLOBAL_WORK_SIZE: return "CL_INVALID_GLOBAL_WORK_SIZE";
        case CL_INVALID_PROPERTY: return "CL_INVALID_PROPERTY";
        case CL_INVALID_IMAGE_DESCRIPTOR: return "CL_INVALID_IMAGE_DESCRIPTOR";
        case CL_INVALID_COMPILER_OPTIONS: return "CL_INVALID_COMPILER_OPTIONS";
        case CL_INVALID_LINKER_OPTIONS: return "CL_INVALID_LINKER_OPTIONS";
        case CL_INVALID_DEVICE_PARTITION_COUNT:
            return "CL_INVALID_DEVICE_PARTITION_COUNT";
        case CL_INVALID_PIPE_SIZE: return "CL_INVALID_PIPE_SIZE";
        case CL_INVALID_DEVICE_QUEUE: return "CL_INVALID_DEVICE_QUEUE";
        case CL_INVALID_SPEC_ID: return "CL_INVALID_SPEC_ID";
        case CL_MAX_SIZE_RESTRICTION_EXCEEDED:
            return "CL_MAX_SIZE_RESTRICTION_EXCEEDED";
        default: return "(unknown)";
    }
}

#define print_error(errCode, msg)                                              \
    printf("ERROR: %s! (%s from %s:%d)\n", msg, IGetErrorString(errCode),      \
           __FILE__, __LINE__);

#define test_error(errCode, msg)                                               \
    {                                                                          \
        auto errCodeResult = errCode;                                          \
        if (errCodeResult != CL_SUCCESS)                                       \
        {                                                                      \
            print_error(errCodeResult, msg);                                   \
            return errCode;                                                    \
        }                                                                      \
    }

static uint32_t reverse_bits(uint32_t n, uint32_t log_2_N)
{
    uint32_t r = 0;
    for (uint32_t j = 0; j < log_2_N; j++)
    {
        r = (r << 1) + (n & 1);
        n >>= 1;
    }
    return r;
}

struct UniformBufferObject
{
    alignas(4) glm::mat4 view_mat;
    alignas(4) glm::mat4 proj_mat;
    alignas(4) glm::vec3 sun_dir = glm::normalize(glm::vec3(0.f, 1.f, 1.f));
    alignas(4) std::float_t choppiness = 1.f;
    alignas(4) std::float_t alt_scale = 1.f;
};

struct Camera
{
    glm::vec3 eye = glm::vec3(0.0f, 0.0f, 20.0f);
    glm::vec3 dir = glm::vec3(-0.57359f, 0.73945f, -0.35241f);
    glm::vec3 up = glm::vec3(-0.2159f, 0.27846f, 0.93584f);
    glm::vec3 rvec = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec2 begin = glm::vec2(-1.0f, -1.0f);
    float yaw = 37.8f;
    float pitch = 69.3649f;
    bool drag = false;
};

struct CliOptions
{
    bool use_gl_sharing = true;
};

#endif // OCEAN_UTIL_HPP
