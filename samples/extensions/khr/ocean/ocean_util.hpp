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
