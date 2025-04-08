/*
 * Copyright (c) 2023 The Khronos Group Inc.
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

#ifndef _SAMPLES_CORE_EXTERNALMEMORY_UTILS_HPP
#define _SAMPLES_CORE_EXTERNALMEMORY_UTILS_HPP

// OpenCL C++ headers includes.
#include <CL/opencl.hpp>

// OpenCL Utils includes.
#include <CL/Utils/Utils.hpp>

// Vulkan includes.
#include <vulkan/vulkan.h>

// Standard header includes.
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// Check if the provided Vulkan error code is \p VK_SUCCESS. If not, prints an
// error message to the standard error output and terminates the program with an
// error code.
#define VK_CHECK(condition)                                                    \
    {                                                                          \
        const VkResult error = condition;                                      \
        if (error != VK_SUCCESS)                                               \
        {                                                                      \
            std::cerr << "A vulkan error encountered: " << error << " at "     \
                      << __FILE__ << ':' << __LINE__ << std::endl;             \
            std::exit(EXIT_FAILURE);                                           \
        }                                                                      \
    }

// OpenCL device that is suitable for this example.
struct cl_device_candidate
{
    /// The OpenCL device id representing the device.
    cl::Device device;

    /// The Vulkan-compatible device UUID.
    cl_uchar uuid[CL_UUID_SIZE_KHR];
};

// OpenCL and Vulkan physical device suitable for the sample.
struct device_candidate
{
    /// The Vulkan physical device handle of the device to be used.
    VkPhysicalDevice vk_candidate;

    /// The candidate device's Vulkan device properties.
    VkPhysicalDeviceProperties vk_props;

    /// The OpenCL device candidate that this Vulkan device corresponds to.
    struct cl_device_candidate cl_candidate;
};

// Check if the extensions supported by a Vulkan device includes a given set of
// required extensions.
template <typename IteratorT>
bool extensions_supported(
    const std::vector<VkExtensionProperties> supported_extensions_properties,
    const IteratorT required_device_extensions_begin,
    const IteratorT required_device_extensions_end)
{
    IteratorT it = required_device_extensions_begin;
    for (; it != required_device_extensions_end; ++it)
    {
        const auto supported_it =
            std::find_if(supported_extensions_properties.begin(),
                         supported_extensions_properties.end(),
                         [&](const VkExtensionProperties& props) {
                             return std::strcmp(*it, props.extensionName) == 0;
                         });
        if (supported_it == supported_extensions_properties.end())
        {
            return false;
        }
    }
    return true;
}

// Check if a given Vulkan device supports all the required Vulkan extensions.
bool check_device_extensions(
    const VkPhysicalDevice vk_device,
    const std::vector<const char*> required_device_extensions)
{
    uint32_t supported_extensions_count;
    VK_CHECK(vkEnumerateDeviceExtensionProperties(
        vk_device, nullptr, &supported_extensions_count, nullptr));
    std::vector<VkExtensionProperties> vk_supported_extensions_properties(
        supported_extensions_count);
    VK_CHECK(vkEnumerateDeviceExtensionProperties(
        vk_device, nullptr, &supported_extensions_count,
        vk_supported_extensions_properties.data()));

    return extensions_supported(vk_supported_extensions_properties,
                                required_device_extensions.begin(),
                                required_device_extensions.end());
}

// Check if a given Vulkan physical device is compatible with any of the OpenCL
// devices available.
bool is_vk_device_suitable(
    const std::vector<cl_device_candidate> cl_candidates,
    VkPhysicalDevice vk_device, device_candidate& candidate,
    const std::vector<const char*> required_device_extensions)
{
    // Check if the device supports OpenCL by checking if there is any device
    // with the same UUID.
    {
        // Query the Vulkan device UUID using vkGetPhysicalDeviceProperties2.
        VkPhysicalDeviceIDPropertiesKHR id_props = {};
        id_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES_KHR;

        VkPhysicalDeviceProperties2KHR props2 = {};
        props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
        props2.pNext = &id_props;

        vkGetPhysicalDeviceProperties2(vk_device, &props2);

        // Look for an OpenCL device which UUID matches the UUID reported by
        // Vulkan.
        const auto cmp_device_uuid =
            [&](const cl_device_candidate& cl_candidate) {
                return std::equal(std::begin(cl_candidate.uuid),
                                  std::end(cl_candidate.uuid),
                                  std::begin(id_props.deviceUUID),
                                  std::end(id_props.deviceUUID));
            };
        const auto it = std::find_if(cl_candidates.begin(), cl_candidates.end(),
                                     cmp_device_uuid);
        if (it == cl_candidates.end())
        {
            // This device does not support HIP.
            return false;
        }

        candidate.vk_props = props2.properties;
        candidate.cl_candidate = *it;
    }

    // Check if the device supports the required extensions.
    if (!check_device_extensions(vk_device, required_device_extensions))
    {
        return false;
    }

    candidate.vk_candidate = vk_device;
    return true;
}

// Find a suitable device for the example, that is, an OpenCL
// device that is also Vulkan-compatible and that supports the required
// Vulkan device extensions.
struct device_candidate
find_suitable_device(VkInstance instance,
                     std::vector<const char*> required_device_extensions)
{
    // Query OpenCL devices available.
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);

    // For each OpenCL device, query its Vulkan-compatible device UUID and
    // add it to the list of candidates.
    std::vector<cl_device_candidate> cl_candidates;
    for (const auto& platform : platforms)
    {
        std::vector<cl::Device> platform_devices;
        platform.getDevices(CL_DEVICE_TYPE_ALL, &platform_devices);

        for (const auto& device : platform_devices)
        {
            if (cl::util::supports_extension(device, "cl_khr_device_uuid"))
            {
                cl_uchar vk_candidate_uuid[CL_UUID_SIZE_KHR];
                device.getInfo(CL_DEVICE_UUID_KHR, &vk_candidate_uuid);

                cl_device_candidate candidate;
                candidate.device = device;
                std::memcpy(candidate.uuid, &vk_candidate_uuid,
                            sizeof(cl_uchar) * CL_UUID_SIZE_KHR);
                cl_candidates.push_back(candidate);
            }
        }
    }

    // Query the Vulkan physical devices available.
    uint32_t vk_device_count;
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &vk_device_count, nullptr));

    std::vector<VkPhysicalDevice> vk_devices(vk_device_count);
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &vk_device_count,
                                        vk_devices.data()));

    // Find a suitable Vulkan physical device compatible with one of the OpenCL
    // devices available.
    device_candidate candidate;
    for (const auto vk_device : vk_devices)
    {
        if (is_vk_device_suitable(cl_candidates, vk_device, candidate,
                                  required_device_extensions))
        {
            return candidate;
        }
    }

    std::cout << "No suitable OpenCL Vulkan-compatible devices available"
              << std::endl;
    exit(EXIT_SUCCESS);
}

// Check if a given OpenCL device supports a particular external memory handle
// type.
bool vk_check_external_memory_handle_type(
    VkPhysicalDevice vk_physical_device,
    VkBufferUsageFlags vk_external_memory_usage,
    VkExternalMemoryHandleTypeFlagBits vk_external_memory_handle_type)
{
    VkPhysicalDeviceExternalBufferInfo
        physical_device_external_buffer_info = {};
    physical_device_external_buffer_info.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_BUFFER_INFO;
    physical_device_external_buffer_info.usage = vk_external_memory_usage;
    physical_device_external_buffer_info.handleType =
        vk_external_memory_handle_type;

    VkExternalBufferProperties external_buffer_properties = {};
    external_buffer_properties.sType =
        VK_STRUCTURE_TYPE_EXTERNAL_BUFFER_PROPERTIES;
    external_buffer_properties.pNext = nullptr;

    vkGetPhysicalDeviceExternalBufferProperties(
        vk_physical_device, &physical_device_external_buffer_info,
        &external_buffer_properties);

    return (vk_external_memory_handle_type
            & external_buffer_properties.externalMemoryProperties
                  .compatibleHandleTypes);
}

// Find Vulkan memory properties from Vulkan physical device property flags.
uint32_t find_vk_memory_type(VkPhysicalDevice vk_device, uint32_t type_filter,
                             VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(vk_device, &mem_properties);
    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
    {
        if ((type_filter & (1 << i))
            && (mem_properties.memoryTypes[i].propertyFlags & properties)
                == properties)
        {
            return i;
        }
    }
    return 0;
}

#endif // _SAMPLES_CORE_EXTERNALMEMORY_UTILS_HPP
