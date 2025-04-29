/*
 * Copyright (c) 2024 Mobica Limited, Marcin Hajder
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

#ifdef _WIN32
#include <windows.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <CL/Utils/Utils.hpp>
#include <glm/glm.hpp>

#include <vulkan/vulkan.h>

#define CL_HPP_TARGET_OPENCL_VERSION 300

const float DRAG_SPEED_FAC = 0.2f;
const float ROLL_SPEED_FAC = 8.f;
const int MAX_FRAMES_IN_FLIGHT = 2;

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

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation",
    //"VK_LAYER_LUNARG_api_dump", // useful for debugging but adds a LOT of
    // output!
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

static VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static void
DestroyDebugUtilsMessengerEXT(VkInstance instance,
                              VkDebugUtilsMessengerEXT debugMessenger,
                              const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

struct QueueFamilyIndices
{
    uint32_t graphicsFamily;
    uint32_t presentFamily;

    QueueFamilyIndices(): graphicsFamily(~0), presentFamily(~0) {}

    bool isComplete() { return graphicsFamily != ~0 && presentFamily != ~0; }
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct UniformBufferObject
{
    alignas(4) glm::mat4 view_mat;
    alignas(4) glm::mat4 proj_mat;
    alignas(4) glm::vec3 sun_dir = glm::normalize(glm::vec3(0.f, 1.f, 1.f));
    alignas(4) std::float_t choppiness = 1.f;
    alignas(4) std::float_t alt_scale = 1.f;
};

struct Vertex
{

    glm::vec3 pos;
    glm::vec2 tc;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};

        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2>
    getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 2>
            attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, tc);

        return attributeDescriptions;
    }
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
    std::int32_t vulkan_device = -1;

    bool immediate = false;

    bool linearImages = false;
    bool device_local_images = true;
    bool use_external_memory = true;
    bool validationLayersOn = false;
};

#endif // OCEAN_UTIL_HPP
