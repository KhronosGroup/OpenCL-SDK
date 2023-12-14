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

#ifndef _SAMPLES_CORE_EXTERNALMEMORY_UTILS_H
#define _SAMPLES_CORE_EXTERNALMEMORY_UTILS_H

// OpenCL C headers includes.
#include <CL/opencl.h>

// OpenCL Utils includes.
#include <CL/Utils/Error.h>

// Vulkan includes.
#include <vulkan/vulkan.h>

// Standard header includes.
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Check if the provided Vulkan error code is \p VK_SUCCESS. If not, prints an
// error message to the standard error output and terminates the program with an
// error code.
#define VK_CHECK(condition)                                                    \
    {                                                                          \
        const VkResult _error = condition;                                     \
        if (_error != VK_SUCCESS)                                              \
        {                                                                      \
            fprintf(stderr, "A vulkan error encountered: %d at %s: %d\n",      \
                    _error, __FILE__, __LINE__);                               \
            exit(EXIT_FAILURE);                                                \
        }                                                                      \
    }

// OpenCL device that is suitable for this example.
struct cl_device_candidate
{
    /// The OpenCL device id representing the device.
    cl_device_id device;

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
bool extensions_supported(
    const VkExtensionProperties* supported_extensions_properties,
    const size_t supported_extensions_count,
    const char* const* required_device_extensions,
    const size_t required_device_extensions_count)
{
    for (size_t i = 0; i < required_device_extensions_count; ++i)
    {
        size_t j = 0;
        while (strcmp(required_device_extensions[i],
                      supported_extensions_properties[j].extensionName)
               && j < supported_extensions_count)
        {
            ++j;
        }
        if (j == supported_extensions_count)
        {
            return false;
        }
    }
    return true;
}

// Check if a given Vulkan device supports all the required Vulkan extensions.
bool check_device_extensions(const VkPhysicalDevice vk_device,
                             const char* const* required_device_extensions,
                             const size_t required_device_extensions_count)
{
    uint32_t supported_extensions_count;
    VK_CHECK(vkEnumerateDeviceExtensionProperties(
        vk_device, NULL, &supported_extensions_count, NULL));
    VkExtensionProperties* vk_supported_extensions_properties =
        (VkExtensionProperties*)malloc(supported_extensions_count
                                       * sizeof(VkExtensionProperties));
    VK_CHECK(vkEnumerateDeviceExtensionProperties(
        vk_device, NULL, &supported_extensions_count,
        vk_supported_extensions_properties));

    bool result = extensions_supported(
        vk_supported_extensions_properties, supported_extensions_count,
        required_device_extensions, required_device_extensions_count);

    free(vk_supported_extensions_properties);

    return result;
}

// Check if a given Vulkan physical device is compatible with any of the OpenCL
// devices available.
bool is_vk_device_suitable(const struct cl_device_candidate* cl_candidates,
                           const size_t cl_candidates_count,
                           VkPhysicalDevice vk_device,
                           struct device_candidate* candidate,
                           const char* const* required_device_extensions,
                           const size_t required_device_extensions_count)
{
    // Check if the device supports OpenCL by checking if there is any device
    // with the same UUID.
    {
        // Query the Vulkan device UUID using vkGetPhysicalDeviceProperties2.
        VkPhysicalDeviceIDPropertiesKHR id_props = { 0 };
        id_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES_KHR;

        VkPhysicalDeviceProperties2KHR props2 = { 0 };
        props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
        props2.pNext = &id_props;

        vkGetPhysicalDeviceProperties2(vk_device, &props2);

        // Look for an OpenCL device which UUID matches the UUID reported by
        // Vulkan.
        size_t compatible_dev_index = cl_candidates_count;
        for (size_t i = 0; i < cl_candidates_count
             && compatible_dev_index == cl_candidates_count;
             ++i)
        {
            compatible_dev_index = i;
            for (uint32_t j = 0; j < CL_UUID_SIZE_KHR; ++j)
            {
                if (cl_candidates[i].uuid[j] != id_props.deviceUUID[j])
                {
                    compatible_dev_index = cl_candidates_count;
                    break;
                }
            }
        }

        if (compatible_dev_index == cl_candidates_count)
        {
            return false;
        }

        candidate->vk_props = props2.properties;
        candidate->cl_candidate = cl_candidates[compatible_dev_index];
    }

    // Check if the device supports the required extensions.
    if (!check_device_extensions(vk_device, required_device_extensions,
                                 required_device_extensions_count))
    {
        return false;
    }

    candidate->vk_candidate = vk_device;
    return true;
}

// Check if a given OpenCL device supports a particular set of Khronos
// extensions.
bool check_khronos_extensions(
    const cl_device_id cl_device,
    const char* const* const required_khronos_extensions,
    const size_t required_khronos_extensions_count)
{
    cl_int error = CL_SUCCESS;
    size_t supported_extensions_count;
    OCLERROR_RET(clGetDeviceInfo(cl_device, CL_DEVICE_EXTENSIONS, 0, NULL,
                                 &supported_extensions_count),
                 error, ret);
    char* supported_extensions =
        (char*)malloc(supported_extensions_count * sizeof(char));
    OCLERROR_RET(clGetDeviceInfo(cl_device, CL_DEVICE_EXTENSIONS,
                                 supported_extensions_count,
                                 supported_extensions, NULL),
                 error, err);

    for (size_t i = 0; i < required_khronos_extensions_count; ++i)
    {
        if (!strstr(supported_extensions, required_khronos_extensions[i]))
        {
            free(supported_extensions);
            return false;
        }
    }
    free(supported_extensions);
    return true;
err:
    free(supported_extensions);
ret:
    return false;
}

// Find a suitable device for the example, that is, an OpenCL
// device that is also Vulkan-compatible and that supports the required
// Vulkan device extensions.
struct device_candidate
find_suitable_device(VkInstance instance,
                     const char* const* required_device_extensions,
                     const size_t required_device_extensions_count)
{
    // Query OpenCL devices available.
    cl_int error = CL_SUCCESS;
    bool candidate_found = false;
    cl_uint platform_count = 0;
    struct device_candidate found_candidate = {0};
    OCLERROR_RET(clGetPlatformIDs(0, NULL, &platform_count), error, ret);

    cl_platform_id* platforms =
        (cl_platform_id*)malloc(platform_count * sizeof(cl_platform_id));
    OCLERROR_RET(clGetPlatformIDs(platform_count, platforms, NULL), error,
                 platforms);

    size_t cl_device_count = 0;
    const char* uuid_khronos_extension[] = {
        CL_KHR_DEVICE_UUID_EXTENSION_NAME
    };
    for (cl_uint platform_id = 0; platform_id < platform_count;
         ++platform_id)
    {
        cl_uint cl_platform_devices_count = 0;
        OCLERROR_RET(clGetDeviceIDs(platforms[platform_id],
                                    CL_DEVICE_TYPE_ALL, 0, NULL,
                                    &cl_platform_devices_count),
                     error, platforms);
        for (cl_uint device_id = 0; device_id < cl_platform_devices_count;
             ++device_id)
        {
            cl_device_id device;
            OCLERROR_PAR(device = cl_util_get_device(
                platform_id, device_id, CL_DEVICE_TYPE_ALL, &error), error, platforms);
            cl_device_count +=
                check_khronos_extensions(device, uuid_khronos_extension, 1);
        }
    }

    // For each OpenCL device, query its Vulkan-compatible device UUID and
    // add it to the list of candidates. The device must support the
    // cl_khr_device_uuid extension for us to be able to query the device's
    // UUID.
    struct cl_device_candidate* cl_candidates =
        (struct cl_device_candidate*)malloc(
            cl_device_count * sizeof(struct cl_device_candidate));
    cl_device_count = 0;
    for (cl_uint platform_id = 0; platform_id < platform_count;
         ++platform_id)
    {
        cl_uint cl_platform_devices_count = 0;
        OCLERROR_RET(clGetDeviceIDs(platforms[platform_id],
                                    CL_DEVICE_TYPE_ALL, 0, NULL,
                                    &cl_platform_devices_count),
                     error, candidates);

        for (cl_uint cl_candidate_id = 0;
             cl_candidate_id < cl_platform_devices_count;
             ++cl_candidate_id, ++cl_device_count)
        {
            cl_device_id device = cl_util_get_device(
                platform_id, cl_candidate_id, CL_DEVICE_TYPE_ALL, &error);
            if (check_khronos_extensions(device, uuid_khronos_extension, 1))
            {
                cl_uchar vk_candidate_uuid[CL_UUID_SIZE_KHR];
                OCLERROR_RET(clGetDeviceInfo(device, CL_DEVICE_UUID_KHR,
                                             CL_UUID_SIZE_KHR,
                                             &vk_candidate_uuid, NULL),
                             error, candidates);

                struct cl_device_candidate candidate;
                candidate.device = device;
                memcpy(candidate.uuid, &vk_candidate_uuid,
                       sizeof(cl_uchar) * CL_UUID_SIZE_KHR);
                cl_candidates[cl_device_count] = candidate;
            }
        }
    }

    // Query the Vulkan physical devices available.
    uint32_t vk_device_count;
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &vk_device_count, NULL));

    VkPhysicalDevice* vk_devices =
        (VkPhysicalDevice*)malloc(vk_device_count * sizeof(VkPhysicalDevice));
    VK_CHECK(
        vkEnumeratePhysicalDevices(instance, &vk_device_count, vk_devices));

    // Find a suitable Vulkan physical device compatible with one of the OpenCL
    // devices available.
    for (cl_uint vk_device_id = 0; vk_device_id < vk_device_count;
         ++vk_device_id)
    {
        VkPhysicalDevice vk_device = vk_devices[vk_device_id];
        if (is_vk_device_suitable(cl_candidates, cl_device_count, vk_device,
                                  &found_candidate, required_device_extensions,
                                  required_device_extensions_count))
        {
            candidate_found = true;
            break;
        }
    }
    if (!candidate_found)
    {
        printf("No suitable OpenCL Vulkan-compatible devices available\n");
    }

    free(vk_devices);
candidates:
    free(cl_candidates);
platforms:
    free(platforms);
ret:
    if (candidate_found)
    {
        return found_candidate;
    }
    exit(error);
}


// Check if a given Vulkan device supports a particular external memory handle
// type.
bool vk_check_external_memory_handle_type(
    VkPhysicalDevice vk_physical_device,
    VkExternalMemoryHandleTypeFlagBits vk_external_memory_handle_type)
{
    VkPhysicalDeviceExternalBufferInfo physical_device_external_buffer_info = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_BUFFER_INFO
    };
    physical_device_external_buffer_info.handleType =
        vk_external_memory_handle_type;

    VkExternalBufferProperties external_buffer_properties;

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

#endif // _SAMPLES_CORE_EXTERNALMEMORY_UTILS_H
