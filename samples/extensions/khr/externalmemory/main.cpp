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

// OpenCL C++ headers includes.
#include <CL/opencl.hpp>

// OpenCL SDK includes.
#include <CL/SDK/CLI.hpp>
#include <CL/SDK/Context.hpp>
#include <CL/SDK/Options.hpp>
#include <CL/SDK/Random.hpp>

// OpenCL Utils includes.
#include <CL/Utils/Error.hpp>
#include <CL/Utils/Event.hpp>
#include <CL/Utils/Utils.hpp>

// Vulkan includes.
#include <vulkan/vulkan.h>

// Vulkan utils includes.
#include "vulkan_utils.hpp"

// Standard header includes.
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <string>

// Sample-specific option.
struct SaxpyOptions
{
    size_t length;
};

// Add option to CLI-parsing SDK utility for input dimensions.
template <> auto cl::sdk::parse<SaxpyOptions>()
{
    return std::make_tuple(std::make_shared<TCLAP::ValueArg<size_t>>(
        "l", "length", "Length of input", false, 1'048'576,
        "positive integral"));
}
template <>
SaxpyOptions cl::sdk::comprehend<SaxpyOptions>(
    std::shared_ptr<TCLAP::ValueArg<size_t>> length_arg)
{
    return SaxpyOptions{ length_arg->getValue() };
}

// Host-side saxpy implementation.
void host_saxpy(std::vector<float> x, std::vector<float>& y, const float a,
                size_t length)
{
    for (size_t i = 0; i < length; ++i)
    {
        y[i] = std::fmaf(a, x[i], y[i]);
    }
}

// Vulkan instance extensions required for sharing OpenCL and Vulkan types:
// - VK_KHR_EXTERNAL_MEMORY_CAPABILITIES required for sharing buffers.
// - VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2 required for the previous one
//   and for querying the device's UUID.
const std::vector<std::string> required_instance_extensions_str = {
    VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, /*VK_KHR_external_memory_capabilities*/
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME /*VK_KHR_get_physical_device_properties2*/
};

// General Vulkan extensions that a device needs to support to run this
// example:
// - VK_KHR_EXTERNAL_MEMORY required for sharing memory.
const std::vector<std::string> required_device_extensions_str = {
    std::string{
        VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME }, /*VK_KHR_external_memory*/
#ifdef _WIN64
    std::string{
        VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME } /*VK_KHR_external_memory_win32*/
#else
    std::string{
        VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME } /*VK_KHR_external_memory_fd*/
#endif
};

// Required Vulkan external memory handle.
const VkExternalMemoryHandleTypeFlagBits vk_external_memory_handle_type =
#ifdef _WIN32
    VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR;
#else
    VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
#endif

// Khronos extensions that a device needs to support memory sharing with Vulkan.
const std::vector<std::string> required_khronos_extensions = {
#ifdef _WIN32
    std::string{ "cl_khr_external_memory_win32" }
#else
    std::string{ "cl_khr_external_memory_opaque_fd" }
#endif
};

// Required OpenCL external memory handle.
const cl_external_memory_handle_type_khr cl_external_memory_handle_type =
#ifdef _WIN32
    CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_WIN32_KHR;
#else
    CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR;
#endif

// Check if a given OpenCL device supports a particular external memory handle
// type.
bool cl_check_external_memory_handle_type(
    const cl::Device cl_device,
    cl_external_memory_handle_type_khr external_memory_handle_type)
{
    std::vector<cl_external_memory_handle_type_khr> supported_handle_types;
    cl_device.getInfo(CL_DEVICE_EXTERNAL_MEMORY_IMPORT_HANDLE_TYPES_KHR,
                      &supported_handle_types);

    const auto it = std::find_if(
        supported_handle_types.begin(), supported_handle_types.end(),
        [&](const cl_external_memory_handle_type_khr& supported_handle_type) {
            return external_memory_handle_type == supported_handle_type;
        });
    return it != supported_handle_types.end();
}

bool opencl_version_is_major(const cl_name_version& dev_name_version,
                             const cl_uint& major)
{
    return CL_VERSION_MAJOR(dev_name_version.version) == major;
}

int main(int argc, char* argv[])
{
    try
    {
        // Parse command-line options.
        auto opts =
            cl::sdk::parse_cli<cl::sdk::options::Diagnostic, SaxpyOptions>(
                argc, argv);
        const auto& diag_opts = std::get<0>(opts);
        const auto& saxpy_opts = std::get<1>(opts);

        // Fill in Vulkan application info.
        VkApplicationInfo app_info{};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "OpenCL-Vulkan interop example";
        app_info.applicationVersion = VK_MAKE_VERSION(3, 0, 0);
        app_info.pEngineName = "OpenCL-SDK samples";
        app_info.engineVersion = VK_MAKE_VERSION(3, 0, 0);
        app_info.apiVersion = VK_MAKE_VERSION(3, 0, 0);

        // Initialize Vulkan instance info and create Vulkan instance.
        std::vector<const char*> required_instance_extensions(
            required_instance_extensions_str.size(), nullptr);
        std::transform(required_instance_extensions_str.begin(),
                       required_instance_extensions_str.end(),
                       required_instance_extensions.begin(),
                       [&](const std::string& str) { return str.c_str(); });
        VkInstanceCreateInfo instance_create_info{};
        instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instance_create_info.pApplicationInfo = &app_info;
        instance_create_info.enabledExtensionCount =
            static_cast<uint32_t>(required_instance_extensions.size());
        instance_create_info.ppEnabledExtensionNames =
            required_instance_extensions.data();

        VkInstance instance;
        VK_CHECK(vkCreateInstance(&instance_create_info, nullptr, &instance));

        // Find a suitable (Vulkan-compatible) OpenCL device for the sample.
        std::vector<const char*> required_device_extensions(
            required_device_extensions_str.size(), nullptr);
        std::transform(required_device_extensions_str.begin(),
                       required_device_extensions_str.end(),
                       required_device_extensions.begin(),
                       [&](const std::string& str) { return str.c_str(); });
        device_candidate candidate =
            find_suitable_device(instance, required_device_extensions);

        // OpenCL device and platform objects for the selected device.
        cl::Device cl_device = candidate.cl_candidate.device;
        const cl::Platform cl_platform{
            cl_device.getInfo<CL_DEVICE_PLATFORM>()
        };

        // Vulkan physical device object for the selected device.
        const VkPhysicalDevice vk_physical_device = candidate.vk_candidate;

        // Set up necessary info and create Vulkan device from physical device.
        constexpr float default_queue_priority = 1.0f;
        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = 0;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &default_queue_priority;

        VkDeviceCreateInfo device_create_info{};
        device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_create_info.queueCreateInfoCount = 1;
        device_create_info.pQueueCreateInfos = &queue_create_info;
        device_create_info.enabledExtensionCount =
            static_cast<uint32_t>(required_device_extensions.size());
        device_create_info.ppEnabledExtensionNames =
            required_device_extensions.data();

        VkDevice vk_device;
        VK_CHECK(vkCreateDevice(vk_physical_device, &device_create_info,
                                nullptr, &vk_device));

        if (!diag_opts.quiet)
        {
            std::cout << "Selected platform: "
                      << cl_platform.getInfo<CL_PLATFORM_VENDOR>() << "\n"
                      << "Selected device: "
                      << cl_device.getInfo<CL_DEVICE_NAME>() << "\n"
                      << std::endl;
        }

        // Create OpenCL runtime objects.
        cl::Context cl_context{ cl_device };

        // Check if the device supports the Khronos extensions needed before
        // attempting to compile the kernel.
        if (diag_opts.verbose)
        {
            std::cout << "Checking Khronos extensions support... ";
            std::cout.flush();
        }

        for (const auto& extension : required_khronos_extensions)
        {
            if (!cl::util::supports_extension(cl_device, extension))
            {
                std::cout << "OpenCL device does not support the required "
                             "Khronos extension "
                          << extension << std::endl;
                exit(EXIT_SUCCESS);
            }
        }

        // Compile kernel.
        if (diag_opts.verbose)
        {
            std::cout << " done.\nCompiling OpenCL kernel... ";
            std::cout.flush();
        }
        const char* kernel_location = "./external_saxpy.cl";
        std::ifstream kernel_stream{ kernel_location };
        if (!kernel_stream.is_open())
            throw std::runtime_error{
                std::string{ "Cannot open kernel source: " } + kernel_location
            };
        cl::Program cl_program{
            cl_context,
            std::string{ std::istreambuf_iterator<char>{ kernel_stream },
                         std::istreambuf_iterator<char>{} }
        };

        // Build OpenCL executable.
        cl_program.build(cl_device);

        // Query maximum workgroup size (WGS) supported based on private mem
        // (registers) constraints.
        auto saxpy = cl::KernelFunctor<cl_float, cl::Buffer, cl::Buffer>(
            cl_program, "saxpy");
        auto wgs =
            saxpy.getKernel().getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(
                cl_device);

        // Initialize host-side storage.
        const auto length = saxpy_opts.length;

        // Random number generator.
        auto prng = [engine = std::default_random_engine{},
                     dist = std::uniform_real_distribution<cl_float>{
                         -1.0, 1.0 }]() mutable { return dist(engine); };

        // Initialize input and output vectors and constant.
        std::vector<cl_float> arr_x(length), arr_y(length);
        if (diag_opts.verbose)
        {
            std::cout << "Generating random scalar and " << length
                      << " random numbers for saxpy input vector." << std::endl;
        }
        cl_float a = prng();
        cl::sdk::fill_with_random(prng, arr_x, arr_y);

        // Check if the device supports the required OpenCL handle type.
        if (diag_opts.verbose)
        {
            std::cout << "done.\nChecking OpenCL external memory handle type "
                         "support... ";
            std::cout.flush();
        }

        if (!cl_check_external_memory_handle_type(
                cl_device, cl_external_memory_handle_type))
        {
            std::cerr
                << "\nError: Unsupported OpenCL external memory handle type"
                << std::endl;
            exit(EXIT_FAILURE);
        }

        VkBufferUsageFlags vk_external_memory_usage =
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        if (!vk_check_external_memory_handle_type(
                vk_physical_device, vk_external_memory_usage,
                vk_external_memory_handle_type))
        {
            std::cerr
                << "\nError: Unsupported Vulkan external memory handle type"
                << std::endl;
            exit(EXIT_FAILURE);
        }

        // Initialize Vulkan device-side storage.
        if (diag_opts.verbose)
        {
            std::cout << "done.\nInitializing Vulkan device storage... ";
            std::cout.flush();
        }

        // Create Vulkan (external) buffers and assign memory to them.
        VkExternalMemoryBufferCreateInfo external_memory_buffer_info{};
        external_memory_buffer_info.sType =
            VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO;
        external_memory_buffer_info.handleTypes =
            vk_external_memory_handle_type;

        VkBufferCreateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.pNext = &external_memory_buffer_info;
        buffer_info.size = sizeof(cl_float) * length;
        buffer_info.usage = vk_external_memory_usage;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkBuffer vk_buf_x, vk_buf_y;
        VK_CHECK(vkCreateBuffer(vk_device, &buffer_info, nullptr, &vk_buf_x));
        VK_CHECK(vkCreateBuffer(vk_device, &buffer_info, nullptr, &vk_buf_y));

        // Get requirements and necessary information for (exportable) memory.
        VkMemoryRequirements mem_requirements_x{}, mem_requirements_y{};
        vkGetBufferMemoryRequirements(vk_device, vk_buf_x, &mem_requirements_x);
        vkGetBufferMemoryRequirements(vk_device, vk_buf_y, &mem_requirements_y);

        VkExportMemoryAllocateInfo export_memory_alloc_info{};
        export_memory_alloc_info.sType =
            VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
        export_memory_alloc_info.handleTypes = vk_external_memory_handle_type;

        VkMemoryAllocateInfo memory_alloc_info_x{};
        memory_alloc_info_x.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memory_alloc_info_x.pNext = &export_memory_alloc_info;
        memory_alloc_info_x.allocationSize = mem_requirements_x.size;
        memory_alloc_info_x.memoryTypeIndex = find_vk_memory_type(
            vk_physical_device, mem_requirements_x.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        VkMemoryAllocateInfo memory_alloc_info_y{};
        memory_alloc_info_y.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memory_alloc_info_y.pNext = &export_memory_alloc_info;
        memory_alloc_info_y.allocationSize = mem_requirements_y.size;
        memory_alloc_info_y.memoryTypeIndex = find_vk_memory_type(
            vk_physical_device, mem_requirements_y.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        // Allocate and bind memory.
        VkDeviceMemory vk_buf_x_memory, vk_buf_y_memory;
        VK_CHECK(vkAllocateMemory(vk_device, &memory_alloc_info_x, nullptr,
                                  &vk_buf_x_memory));
        VK_CHECK(vkAllocateMemory(vk_device, &memory_alloc_info_y, nullptr,
                                  &vk_buf_y_memory));

        VK_CHECK(vkBindBufferMemory(vk_device, vk_buf_x, vk_buf_x_memory, 0));
        VK_CHECK(vkBindBufferMemory(vk_device, vk_buf_y, vk_buf_y_memory, 0));

        // Map memory.
        void *vk_arr_x, *vk_arr_y;
        VK_CHECK(vkMapMemory(vk_device, vk_buf_x_memory, 0, VK_WHOLE_SIZE, 0,
                             &vk_arr_x));
        VK_CHECK(vkMapMemory(vk_device, vk_buf_y_memory, 0, VK_WHOLE_SIZE, 0,
                             &vk_arr_y));

        memcpy(vk_arr_x, arr_x.data(), sizeof(cl_float) * length);
        memcpy(vk_arr_y, arr_y.data(), sizeof(cl_float) * length);

#ifdef _WIN32
        // Get Vulkan external memory file descriptors for accessing external
        // memory with OpenCL.
        VkMemoryGetWin32HandleInfoKHR handle_info_x{};
        handle_info_x.sType =
            VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
        handle_info_x.pNext = nullptr;
        handle_info_x.memory = vk_buf_x_memory;
        handle_info_x.handleType = vk_external_memory_handle_type;
        HANDLE handle_x;

        VkMemoryGetWin32HandleInfoKHR handle_info_y{};
        handle_info_y.sType =
            VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
        handle_info_y.pNext = nullptr;
        handle_info_y.memory = vk_buf_y_memory;
        handle_info_y.handleType = vk_external_memory_handle_type;
        HANDLE handle_y;

        // We need to get the pointer to the
        // vkGetMemoryFdKHR/vkGetMemoryWin32HandleKHR function because it's from
        // the extension VK_KHR_external_memory_fd. This Vulkan function exports
        // a POSIX file descriptor/Windows handle referencing the payload of a
        // Vulkan device memory object.
        PFN_vkGetMemoryWin32HandleKHR vkGetMemoryWin32Handle;
        *(PFN_vkGetMemoryWin32HandleKHR*)&vkGetMemoryWin32Handle =
            (PFN_vkGetMemoryWin32HandleKHR)vkGetDeviceProcAddr(
                vk_device, "vkGetMemoryWin32HandleKHR");
        VK_CHECK(vkGetMemoryWin32Handle(vk_device, &handle_info_x, &handle_x));
        VK_CHECK(vkGetMemoryWin32Handle(vk_device, &handle_info_y, &handle_y));
#else
        // Get Vulkan external memory file descriptors for accessing external
        // memory with OpenCL.
        VkMemoryGetFdInfoKHR fd_info_x{};
        fd_info_x.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
        fd_info_x.pNext = nullptr;
        fd_info_x.memory = vk_buf_x_memory;
        fd_info_x.handleType = vk_external_memory_handle_type;
        int fd_x;

        VkMemoryGetFdInfoKHR fd_info_y{};
        fd_info_y.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
        fd_info_y.pNext = nullptr;
        fd_info_y.memory = vk_buf_y_memory;
        fd_info_y.handleType = vk_external_memory_handle_type;
        int fd_y;

        // We need to get the pointer to the
        // vkGetMemoryFdKHR/vkGetMemoryWin32HandleKHR function because it's from
        // extension VK_KHR_external_memory_fd. This Vulkan function exports a
        // POSIX file descriptor/Windows handle referencing the payload of a
        // Vulkan device memory object.
        PFN_vkGetMemoryFdKHR vkGetMemoryFd;
        *(PFN_vkGetMemoryFdKHR*)&vkGetMemoryFd =
            (PFN_vkGetMemoryFdKHR)vkGetDeviceProcAddr(vk_device,
                                                      "vkGetMemoryFdKHR");
        VK_CHECK(vkGetMemoryFd(vk_device, &fd_info_x, &fd_x));
        VK_CHECK(vkGetMemoryFd(vk_device, &fd_info_y, &fd_y));
#endif

        // Create OpenCL buffers from Vulkan external memory file descriptors.
        std::vector<cl_mem_properties> ext_mem_props_x = {
#ifdef _WIN32
            (cl_mem_properties)CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_WIN32_KHR,
            (cl_mem_properties)handle_x,
#else
            (cl_mem_properties)CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR,
            (cl_mem_properties)fd_x,
#endif
            (cl_mem_properties)CL_MEM_DEVICE_HANDLE_LIST_KHR,
            (cl_mem_properties)cl_device(),
            CL_MEM_DEVICE_HANDLE_LIST_END_KHR,
            0
        };
        std::vector<cl_mem_properties> ext_mem_props_y = {
#ifdef _WIN32
            (cl_mem_properties)CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_WIN32_KHR,
            (cl_mem_properties)handle_y,
#else
            (cl_mem_properties)CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR,
            (cl_mem_properties)fd_y,
#endif
            (cl_mem_properties)CL_MEM_DEVICE_HANDLE_LIST_KHR,
            (cl_mem_properties)cl_device(),
            CL_MEM_DEVICE_HANDLE_LIST_END_KHR,
            0
        };

        cl::Buffer cl_buf_x{ cl_context, ext_mem_props_x, CL_MEM_READ_ONLY,
                             sizeof(cl_float) * length };
        cl::Buffer cl_buf_y{ cl_context, ext_mem_props_y, CL_MEM_READ_WRITE,
                             sizeof(cl_float) * length };

        // Initialize queue for command execution.
        cl_command_queue_properties queue_props[] = { CL_QUEUE_PROFILING_ENABLE,
                                                      0 };
        cl::CommandQueue queue{ cl_context, cl_device, *queue_props };

        // Acquire OpenCL memory objects created from Vulkan external memory
        // handles.
        std::vector<cl_mem> cl_mem_objects = { cl_buf_x(), cl_buf_y() };
        clEnqueueAcquireExternalMemObjectsKHR_fn
            clEnqueueAcquireExternalMemObjects =
                (clEnqueueAcquireExternalMemObjectsKHR_fn)
                    clGetExtensionFunctionAddressForPlatform(
                        cl_platform(), "clEnqueueAcquireExternalMemObjectsKHR");
        clEnqueueAcquireExternalMemObjects(
            queue(), static_cast<cl_uint>(cl_mem_objects.size()),
            cl_mem_objects.data(), 0, nullptr, nullptr);

        // Launch kernel.
        if (diag_opts.verbose)
        {
            std::cout << "done.\nExecuting on device... ";
            std::cout.flush();
        }

        std::vector<cl::Event> kernel_run;
        auto dev_start = std::chrono::high_resolution_clock::now();
        kernel_run.push_back(
            saxpy(cl::EnqueueArgs{ queue, cl::NDRange{ length }, wgs }, a,
                  cl_buf_x, cl_buf_y));
        cl::WaitForEvents(kernel_run);
        auto dev_end = std::chrono::high_resolution_clock::now();

        // Release OpenCL memory objects created from Vulkan external memory
        // handles.
        clEnqueueReleaseExternalMemObjectsKHR_fn
            clEnqueueReleaseExternalMemObjects =
                (clEnqueueReleaseExternalMemObjectsKHR_fn)
                    clGetExtensionFunctionAddressForPlatform(
                        cl_platform(), "clEnqueueReleaseExternalMemObjectsKHR");
        clEnqueueReleaseExternalMemObjects(
            queue(), static_cast<cl_uint>(cl_mem_objects.size()),
            cl_mem_objects.data(), 0, nullptr, nullptr);

        // Concurrently calculate reference saxpy.
        if (diag_opts.verbose)
        {
            std::cout << "done.\nExecuting on host... ";
            std::cout.flush();
        }

        auto host_start = std::chrono::high_resolution_clock::now();
        host_saxpy(arr_x, arr_y, a, length);
        auto host_end = std::chrono::high_resolution_clock::now();

        if (diag_opts.verbose)
        {
            std::cout << "done.\n";
            std::cout.flush();
        }

        // Fetch results.
        cl::copy(queue, cl_buf_y, arr_x.begin(), arr_x.end());

        // Validate solution.
        if (std::equal(std::begin(arr_x), std::end(arr_x), std::begin(arr_y),
                       std::end(arr_y)))
            std::cout << "Verification passed." << std::endl;
        else
            throw std::runtime_error{ "Verification failed!" };

        if (!diag_opts.quiet)
        {
            std::cout << "Kernel execution time as seen by host: "
                      << std::chrono::duration_cast<std::chrono::microseconds>(
                             dev_end - dev_start)
                             .count()
                      << " us." << std::endl;

            std::cout << "Kernel execution time as measured by device: ";
            std::cout << cl::util::get_duration<CL_PROFILING_COMMAND_START,
                                                CL_PROFILING_COMMAND_END,
                                                std::chrono::microseconds>(
                             kernel_run[0])
                             .count()
                      << " us." << std::endl;

            std::cout << "Reference execution as seen by host: "
                      << std::chrono::duration_cast<std::chrono::microseconds>(
                             host_end - host_start)
                             .count()
                      << " us." << std::endl;
        }

        // Release resources.
        vkDestroyBuffer(vk_device, vk_buf_y, nullptr);
        vkDestroyBuffer(vk_device, vk_buf_x, nullptr);
        vkUnmapMemory(vk_device, vk_buf_y_memory);
        vkUnmapMemory(vk_device, vk_buf_x_memory);
        vkFreeMemory(vk_device, vk_buf_y_memory, nullptr);
        vkFreeMemory(vk_device, vk_buf_x_memory, nullptr);

    } catch (cl::BuildError& e)
    {
        std::cerr << "OpenCL build error: " << e.what() << std::endl;
        for (auto& build_log : e.getBuildLog())
        {
            std::cerr << "\tBuild log for device: "
                      << build_log.first.getInfo<CL_DEVICE_NAME>() << "\n"
                      << std::endl;
            std::cerr << build_log.second << "\n" << std::endl;
        }
        std::exit(e.err());
    } catch (cl::util::Error& e)
    {
        std::cerr << "OpenCL utils error: " << e.what() << std::endl;
        std::exit(e.err());
    } catch (cl::Error& e)
    {
        std::cerr << "OpenCL runtime error: " << e.what() << std::endl;
        std::exit(e.err());
    } catch (std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    return 0;
}
