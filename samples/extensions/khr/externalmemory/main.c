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

// OpenCL SDK includes.
#include <CL/SDK/CLI.h>
#include <CL/SDK/Context.h>
#include <CL/SDK/Options.h>
#include <CL/SDK/Random.h>

// OpenCL Utils includes.
#include <CL/Utils/Error.h>
#include <CL/Utils/Event.h>
#include <CL/Utils/Utils.h>

// Vulkan includes.
#include <vulkan/vulkan.h>

// Vulkan utils includes.
#include "vulkan_utils.h"

// Standard header includes.
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Sample-specific option.
struct options_Saxpy
{
    size_t length;
};

// Add option to CLI-parsing SDK utility for input length.
cag_option SaxpyOptions[] = { { .identifier = 'l',
                                .access_letters = "l",
                                .access_name = "length",
                                .value_name = "(positive integer)",
                                .description = "Length of input" } };

ParseState parse_SaxpyOptions(const char identifier,
                              cag_option_context* cag_context,
                              struct options_Saxpy* opts)
{
    const char* value;
    switch (identifier)
    {
        case 'l':
            if (0 != (value = cag_option_get_value(cag_context)))
            {
                opts->length = strtoul(value, NULL, 0);
                return ParsedOK;
            }
            else
                return ParseError;
    }
    return NotParsed;
}

cl_int parse_options(int argc, char* argv[],
                     struct cl_sdk_options_Diagnostic* diag_opts,
                     struct options_Saxpy* saxpy_opts)
{
    cl_int error = CL_SUCCESS;
    struct cag_option *opts = NULL, *tmp = NULL;
    size_t n = 0;

    // Prepare options array.
    MEM_CHECK(opts = add_CLI_options(opts, &n, DiagnosticOptions,
                                     CAG_ARRAY_SIZE(DiagnosticOptions)),
              error, end);
    opts = tmp;
    MEM_CHECK(tmp = add_CLI_options(opts, &n, SaxpyOptions,
                                    CAG_ARRAY_SIZE(SaxpyOptions)),
              error, end);
    opts = tmp;

    char identifier;
    cag_option_context cag_context;

    // Prepare the context and iterate over all options.
    cag_option_prepare(&cag_context, opts, n, argc, argv);
    while (cag_option_fetch(&cag_context))
    {
        ParseState state = NotParsed;
        identifier = cag_option_get(&cag_context);

        PARS_OPTIONS(parse_DiagnosticOptions(identifier, diag_opts), state);
        PARS_OPTIONS(parse_SaxpyOptions(identifier, &cag_context, saxpy_opts),
                     state);

        if (identifier == 'h')
        {
            printf("Usage: externalmemory [OPTION]...\n");
            printf("Option name and value should be separated by '=' or a "
                   "space\n");
            printf("Demonstrates OpenCL--Vulkan interop.\n\n");
            cag_option_print(opts, n, stdout);
            exit((state == ParseError) ? CL_INVALID_ARG_VALUE : CL_SUCCESS);
        }
    }
end:
    free(opts);
    return error;
}

// Host-side saxpy implementation.
void host_saxpy(const cl_float* x, cl_float* y, const float a, size_t length)
{
    for (size_t i = 0; i < length; ++i)
    {
        y[i] = fmaf(a, x[i], y[i]);
    }
}

// Vulkan instance extensions required for sharing OpenCL and Vulkan types:
// - VK_KHR_EXTERNAL_MEMORY_CAPABILITIES required for sharing buffers.
// - VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2 required for the previous one
//   and for querying the device's UUID.
const char* const required_instance_extensions[] = {
    VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, /*VK_KHR_external_memory_capabilities*/
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME /*VK_KHR_get_physical_device_properties2*/
};
const size_t required_instance_extensions_count =
    sizeof(required_instance_extensions) / sizeof(const char*);

// General Vulkan extensions that a device needs to support for exporting
// memory.
const char* required_device_extensions[] = {
    VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME, /*VK_KHR_external_memory*/
#ifdef _WIN32
    VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME /*VK_KHR_external_memory_win32*/
#else
    VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME /*VK_KHR_external_memory_fd*/
#endif
};
const size_t required_device_extensions_count =
    sizeof(required_device_extensions) / sizeof(const char*);

// Khronos extensions that a device needs to support memory sharing with Vulkan.
const char* required_khronos_extensions[] = {
#ifdef _WIN32
    CL_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME /*cl_khr_external_memory_win32*/
#else
    CL_KHR_EXTERNAL_MEMORY_OPAQUE_FD_EXTENSION_NAME /*cl_khr_external_memory_opaque_fd*/
#endif
};
const size_t required_khronos_extensions_count =
    sizeof(required_khronos_extensions) / sizeof(const char*);

// Required Vulkan external memory handle.
const VkExternalMemoryHandleTypeFlagBits vk_external_memory_handle_type =
#ifdef _WIN32
    VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR;
#else
    VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
#endif

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
    const cl_device_id cl_device,
    cl_external_memory_handle_type_khr external_memory_handle_type)
{
    cl_external_memory_handle_type_khr* supported_handle_types = NULL;
    size_t supported_handle_types_byte_count = 0;
    const size_t handle_type_size = sizeof(cl_external_memory_handle_type_khr);
    cl_int error = CL_SUCCESS;

    OCLERROR_RET(
        clGetDeviceInfo(cl_device,
                        CL_DEVICE_EXTERNAL_MEMORY_IMPORT_HANDLE_TYPES_KHR, 0,
                        NULL, &supported_handle_types_byte_count),
        error, err);
    supported_handle_types = (cl_external_memory_handle_type_khr*)malloc(
        supported_handle_types_byte_count);

    OCLERROR_RET(
        clGetDeviceInfo(
            cl_device, CL_DEVICE_EXTERNAL_MEMORY_IMPORT_HANDLE_TYPES_KHR,
            supported_handle_types_byte_count, supported_handle_types, NULL),
        error, err);

    const size_t supported_handle_types_count =
        supported_handle_types_byte_count / handle_type_size;
    for (size_t i = 0; i < supported_handle_types_count; ++i)
    {
        if (external_memory_handle_type == supported_handle_types[i])
        {
            free(supported_handle_types);
            return true;
        }
    }
    free(supported_handle_types);
    return false;
err:
    fprintf(stderr,
            "Error: OpenCL could not query supported external memory handle "
            "types\n");
    free(supported_handle_types);
    exit(EXIT_FAILURE);
}

cl_int opencl_version_is_major(cl_name_version* dev_name_version, cl_uint major)
{
    return CL_VERSION_MAJOR(dev_name_version->version) == major;
}

int main(int argc, char* argv[])
{
    cl_int error = CL_SUCCESS;
    cl_int end_error = CL_SUCCESS;
    cl_platform_id cl_platform;
    cl_device_id cl_device;
    VkPhysicalDevice vk_physical_device;
    VkDevice vk_device;
    cl_context context = NULL;
    cl_command_queue queue = NULL;

    cl_program program;

    // Parse command-line options.
    struct cl_sdk_options_Diagnostic diag_opts = { .quiet = false,
                                                   .verbose = false };
    // Define as default length 1048576 = 4 * 262144 = sizeof(cl_float) * 2^18.
    struct options_Saxpy saxpy_opts = { .length = 1048576 };

    OCLERROR_RET(parse_options(argc, argv, &diag_opts, &saxpy_opts), error,
                 end);

    // Fill in Vulkan application info.
    VkApplicationInfo app_info = { 0 };
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "OpenCL-Vulkan interop example";
    app_info.applicationVersion = VK_MAKE_VERSION(3, 0, 0);
    app_info.pEngineName = "OpenCL-SDK samples";
    app_info.engineVersion = VK_MAKE_VERSION(3, 0, 0);
    app_info.apiVersion = VK_MAKE_VERSION(3, 0, 0);

    // Initialize Vulkan instance info and create Vulkan instance.
    VkInstanceCreateInfo instance_create_info = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO
    };
    instance_create_info.pApplicationInfo = &app_info;
    instance_create_info.enabledExtensionCount =
        (uint32_t)required_instance_extensions_count;
    instance_create_info.ppEnabledExtensionNames = required_instance_extensions;

    VkInstance instance;
    VK_CHECK(vkCreateInstance(&instance_create_info, NULL, &instance));

    // Find a suitable (Vulkan-compatible) OpenCL device for the sample.
    struct device_candidate candidate = find_suitable_device(
        instance, required_device_extensions, required_device_extensions_count);

    // OpenCL device object for the selected device.
    cl_device = candidate.cl_candidate.device;

    // Vulkan physical device object for the selected device.
    vk_physical_device = candidate.vk_candidate;

    // Set up necessary info and create Vulkan device from physical device.
    const float default_queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_info = {
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO
    };
    queue_create_info.queueFamilyIndex = 0;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &default_queue_priority;

    VkDeviceCreateInfo device_create_info = {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO
    };
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos = &queue_create_info;
    device_create_info.enabledExtensionCount =
        (uint32_t)required_device_extensions_count;
    device_create_info.ppEnabledExtensionNames = required_device_extensions;

    VK_CHECK(vkCreateDevice(vk_physical_device, &device_create_info, NULL,
                            &vk_device));

    if (!diag_opts.quiet)
    {
        cl_util_print_device_info(cl_device);
    }

    // Create OpenCL runtime objects.
    OCLERROR_RET(clGetDeviceInfo(cl_device, CL_DEVICE_PLATFORM,
                                 sizeof(cl_platform_id), &cl_platform, NULL),
                 error, cont);
    cl_context_properties context_props[] = {
        CL_CONTEXT_PLATFORM, (cl_context_properties)cl_platform, 0
    };
    OCLERROR_PAR(context = clCreateContext(context_props, 1, &cl_device, NULL,
                                           NULL, &error),
                 error, end);

    // Check if the device supports the Khronos extensions needed before
    // attempting to compile the kernel.
    if (diag_opts.verbose)
    {
        printf("\nChecking Khronos extensions support... ");
        fflush(stdout);
    }

    if (!check_khronos_extensions(cl_device, required_khronos_extensions,
                                  required_khronos_extensions_count))
    {
        fprintf(stdout,
                "OpenCL device does not support the required Khronos "
                "extensions\n");
        exit(EXIT_SUCCESS);
    }

    // Compile kernel.
    if (diag_opts.verbose)
    {
        printf("done.\nCompiling OpenCL kernel... ");
        fflush(stdout);
    }
    const char* kernel_location = "./external_saxpy.cl";
    char *kernel = NULL, *tmp = NULL;
    size_t program_size = 0;
    kernel = cl_util_read_text_file(kernel_location, &program_size, &error);
    if (error != CL_SUCCESS)
    {
        fprintf(stderr, "Cannot open kernel source: %s\n", kernel_location);
        goto cont;
    }
    MEM_CHECK(tmp = (char*)realloc(kernel, program_size), error, ker);
    kernel = tmp;
    OCLERROR_PAR(program = clCreateProgramWithSource(
                     context, 1, (const char**)&kernel, &program_size, &error),
                 error, ker);

    // The Khronos extension showcased requires OpenCL 3.0 version.
    // Get number of versions supported.
    size_t versions_size = 0;
    OCLERROR_RET(clGetDeviceInfo(cl_device, CL_DEVICE_OPENCL_C_ALL_VERSIONS, 0,
                                 NULL, &versions_size),
                 error, prg);
    size_t versions_count = versions_size / sizeof(cl_name_version);

    // Get and check versions.
    cl_name_version* dev_versions = (cl_name_version*)malloc(versions_size);
    OCLERROR_RET(clGetDeviceInfo(cl_device, CL_DEVICE_OPENCL_C_ALL_VERSIONS,
                                 versions_size, dev_versions, NULL),
                 error, prg);
    char compiler_options[1024] = "";
    for (cl_uint i = 0; i < versions_count; ++i)
    {
        if (opencl_version_is_major(&dev_versions[i], 3))
        {
            strcat(compiler_options, "-cl-std=CL3.0 ");
        }
    }

    if (compiler_options[0] == '\0')
    {
        fprintf(stderr, "\nError: OpenCL version must be at least 3.0\n");
        exit(EXIT_FAILURE);
    }

    OCLERROR_RET(cl_util_build_program(program, cl_device, compiler_options),
                 error, prg);

    // Query maximum workgroup size (WGS) supported based on private mem
    // (registers) constraints.
    size_t wgs;
    cl_kernel saxpy;
    OCLERROR_PAR(saxpy = clCreateKernel(program, "saxpy", &error), error, prg);
    OCLERROR_RET(clGetKernelWorkGroupInfo(saxpy, cl_device,
                                          CL_KERNEL_WORK_GROUP_SIZE,
                                          sizeof(size_t), &wgs, NULL),
                 error, ker);

    // Initialize host-side storage.
    const size_t length = saxpy_opts.length;

    // Random number generator.
    pcg32_random_t rng;
    pcg32_srandom_r(&rng, 11111, 2222);

    // Initialize input and output vectors and constant.
    cl_float *arr_x, *arr_y, a;
    MEM_CHECK(arr_x = (cl_float*)malloc(sizeof(cl_float) * length), error, sxp);
    MEM_CHECK(arr_y = (cl_float*)malloc(sizeof(cl_float) * length), error,
              arrx);
    if (diag_opts.verbose)
    {
        printf("done.\nGenerating random scalar and %zd random numbers for "
               "saxpy input vector...",
               length);
        fflush(stdout);
    }
    cl_sdk_fill_with_random_floats_range(&rng, &a, 1, -100, 100);
    cl_sdk_fill_with_random_floats_range(&rng, arr_x, length, -100, 100);
    cl_sdk_fill_with_random_floats_range(&rng, arr_y, length, -100, 100);

    // Check if the device supports the required OpenCL handle type.
    if (diag_opts.verbose)
    {
        printf(
            "done.\nChecking OpenCL external memory handle type support... ");
        fflush(stdout);
    }

    if (!cl_check_external_memory_handle_type(cl_device,
                                              cl_external_memory_handle_type))
    {
        fprintf(stderr,
                "\nError: Unsupported OpenCL external memory handle type\n");
        goto arry;
    }

    VkBufferUsageFlags vk_external_memory_usage =
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    if (!vk_check_external_memory_handle_type(vk_physical_device,
                                              vk_external_memory_usage,
                                              vk_external_memory_handle_type))
    {
        fprintf(stderr,
                "\nError: Unsupported Vulkan external memory handle type\n");
        goto arry;
    }

    // Initialize Vulkan device-side storage.
    if (diag_opts.verbose)
    {
        printf("done.\nInitializing Vulkan device storage... ");
        fflush(stdout);
    }

    // Create Vulkan (external) buffers and assign memory to them.
    VkExternalMemoryBufferCreateInfo external_memory_buffer_info = {
        VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO
    };
    external_memory_buffer_info.handleTypes = vk_external_memory_handle_type;

    VkBufferCreateInfo buffer_info = { 0 };
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.pNext = &external_memory_buffer_info;
    buffer_info.size = sizeof(cl_float) * length;
    buffer_info.usage = vk_external_memory_usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer vk_buf_x, vk_buf_y;
    VK_CHECK(vkCreateBuffer(vk_device, &buffer_info, NULL, &vk_buf_x));
    VK_CHECK(vkCreateBuffer(vk_device, &buffer_info, NULL, &vk_buf_y));

    // Get requirements and necessary information for (exportable) memory.
    VkMemoryRequirements mem_requirements_x = { 0 }, mem_requirements_y = { 0 };
    vkGetBufferMemoryRequirements(vk_device, vk_buf_x, &mem_requirements_x);
    vkGetBufferMemoryRequirements(vk_device, vk_buf_y, &mem_requirements_y);

    VkExportMemoryAllocateInfo export_memory_alloc_info = {
        VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO
    };
    export_memory_alloc_info.handleTypes = vk_external_memory_handle_type;

    VkMemoryAllocateInfo memory_alloc_info_x = { 0 };
    memory_alloc_info_x.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_alloc_info_x.pNext = &export_memory_alloc_info;
    memory_alloc_info_x.allocationSize = mem_requirements_x.size;
    memory_alloc_info_x.memoryTypeIndex = find_vk_memory_type(
        vk_physical_device, mem_requirements_x.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkMemoryAllocateInfo memory_alloc_info_y = { 0 };
    memory_alloc_info_y.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_alloc_info_y.pNext = &export_memory_alloc_info;
    memory_alloc_info_y.allocationSize = mem_requirements_y.size;
    memory_alloc_info_y.memoryTypeIndex = find_vk_memory_type(
        vk_physical_device, mem_requirements_y.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // Allocate and bind memory.
    VkDeviceMemory vk_buf_x_memory, vk_buf_y_memory;
    VK_CHECK(vkAllocateMemory(vk_device, &memory_alloc_info_x, NULL,
                              &vk_buf_x_memory));
    VK_CHECK(vkAllocateMemory(vk_device, &memory_alloc_info_y, NULL,
                              &vk_buf_y_memory));

    VK_CHECK(vkBindBufferMemory(vk_device, vk_buf_x, vk_buf_x_memory, 0));
    VK_CHECK(vkBindBufferMemory(vk_device, vk_buf_y, vk_buf_y_memory, 0));

    // Map memory.
    void *vk_arr_x, *vk_arr_y;
    VK_CHECK(vkMapMemory(vk_device, vk_buf_x_memory, 0, VK_WHOLE_SIZE, 0,
                         &vk_arr_x));
    VK_CHECK(vkMapMemory(vk_device, vk_buf_y_memory, 0, VK_WHOLE_SIZE, 0,
                         &vk_arr_y));

    memcpy(vk_arr_x, arr_x, sizeof(cl_float) * length);
    memcpy(vk_arr_y, arr_y, sizeof(cl_float) * length);

#ifdef _WIN32
    // Get Vulkan external memory file descriptors for accessing external memory
    // with OpenCL.
    VkMemoryGetWin32HandleInfoKHR handle_info_x = { 0 };
    handle_info_x.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
    handle_info_x.pNext = NULL;
    handle_info_x.memory = vk_buf_x_memory;
    handle_info_x.handleType = vk_external_memory_handle_type;
    HANDLE handle_x;

    VkMemoryGetWin32HandleInfoKHR handle_info_y = { 0 };
    handle_info_y.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
    handle_info_y.pNext = NULL;
    handle_info_y.memory = vk_buf_y_memory;
    handle_info_y.handleType = vk_external_memory_handle_type;
    HANDLE handle_y;

    // We need to get the pointer to the
    // vkGetMemoryFdKHR/vkGetMemoryWin32HandleKHR function because it's from
    // extension VK_KHR_external_memory_fd. This Vulkan function exports a POSIX
    // file descriptor/Windows handle referencing the payload of a Vulkan device
    // memory object.
    PFN_vkGetMemoryWin32HandleKHR vkGetMemoryWin32Handle;
    *(PFN_vkGetMemoryWin32HandleKHR*)&vkGetMemoryWin32Handle =
        (PFN_vkGetMemoryWin32HandleKHR)vkGetDeviceProcAddr(
            vk_device, "vkGetMemoryWin32HandleKHR");
    VK_CHECK(vkGetMemoryWin32Handle(vk_device, &handle_info_x, &handle_x));
    VK_CHECK(vkGetMemoryWin32Handle(vk_device, &handle_info_y, &handle_y));
#else
    // Get Vulkan external memory file descriptors for accessing external memory
    // with OpenCL.
    VkMemoryGetFdInfoKHR fd_info_x = { 0 };
    fd_info_x.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
    fd_info_x.pNext = NULL;
    fd_info_x.memory = vk_buf_x_memory;
    fd_info_x.handleType = vk_external_memory_handle_type;
    int fd_x;

    VkMemoryGetFdInfoKHR fd_info_y = { 0 };
    fd_info_y.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
    fd_info_y.pNext = NULL;
    fd_info_y.memory = vk_buf_y_memory;
    fd_info_y.handleType = vk_external_memory_handle_type;
    int fd_y;

    // We need to get the pointer to the
    // vkGetMemoryFdKHR/vkGetMemoryWin32HandleKHR function because it's from
    // extension VK_KHR_external_memory_fd. This Vulkan function exports a POSIX
    // file descriptor/Windows handle referencing the payload of a Vulkan device
    // memory object.
    PFN_vkGetMemoryFdKHR vkGetMemoryFd;
    *(PFN_vkGetMemoryFdKHR*)&vkGetMemoryFd =
        (PFN_vkGetMemoryFdKHR)vkGetDeviceProcAddr(vk_device,
                                                  "vkGetMemoryFdKHR");
    VK_CHECK(vkGetMemoryFd(vk_device, &fd_info_x, &fd_x));
    VK_CHECK(vkGetMemoryFd(vk_device, &fd_info_y, &fd_y));
#endif


    // Create OpenCL buffers from Vulkan external memory file descriptors.
    cl_mem_properties ext_mem_props_x[] = {
#ifdef _WIN32
        (cl_mem_properties)CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_WIN32_KHR,
        (cl_mem_properties)handle_x,
#else
        (cl_mem_properties)CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR,
        (cl_mem_properties)fd_x,
#endif
        (cl_mem_properties)CL_MEM_DEVICE_HANDLE_LIST_KHR,
        (cl_mem_properties)(uintptr_t)cl_device,
        CL_MEM_DEVICE_HANDLE_LIST_END_KHR,
        0
    };
    cl_mem_properties ext_mem_props_y[] = {
#ifdef _WIN32
        (cl_mem_properties)CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_WIN32_KHR,
        (cl_mem_properties)handle_y,
#else
        (cl_mem_properties)CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR,
        (cl_mem_properties)fd_y,
#endif
        (cl_mem_properties)CL_MEM_DEVICE_HANDLE_LIST_KHR,
        (cl_mem_properties)(uintptr_t)cl_device,
        CL_MEM_DEVICE_HANDLE_LIST_END_KHR,
        0
    };
    cl_mem cl_buf_x, cl_buf_y;
    OCLERROR_PAR(cl_buf_x = clCreateBufferWithProperties(
                     context, ext_mem_props_x, CL_MEM_READ_ONLY,
                     sizeof(cl_float) * length, NULL, &error),
                 error, vulkan);
    OCLERROR_PAR(cl_buf_y = clCreateBufferWithProperties(
                     context, ext_mem_props_y, CL_MEM_READ_WRITE,
                     sizeof(cl_float) * length, NULL, &error),
                 error, clbufx);

    // Initialize queue for command execution.
    cl_command_queue_properties queue_props[] = { CL_QUEUE_PROPERTIES,
                                                  CL_QUEUE_PROFILING_ENABLE,
                                                  0 };
    OCLERROR_PAR(queue = clCreateCommandQueueWithProperties(
                     context, cl_device, queue_props, &error),
                 error, clbufy);

    // Set kernel arguments.
    OCLERROR_RET(clSetKernelArg(saxpy, 0, sizeof(cl_float), &a), error, que);
    OCLERROR_RET(clSetKernelArg(saxpy, 1, sizeof(cl_mem), &cl_buf_x), error,
                 que);
    OCLERROR_RET(clSetKernelArg(saxpy, 2, sizeof(cl_mem), &cl_buf_y), error,
                 que);

    // Acquire OpenCL memory objects created from Vulkan external memory
    // handles.
    cl_mem cl_mem_objects[] = { cl_buf_x, cl_buf_y };
    clEnqueueAcquireExternalMemObjectsKHR_fn
        clEnqueueAcquireExternalMemObjects =
            (clEnqueueAcquireExternalMemObjectsKHR_fn)
                clGetExtensionFunctionAddressForPlatform(
                    cl_platform, "clEnqueueAcquireExternalMemObjectsKHR");
    clEnqueueAcquireExternalMemObjects(queue, 2, cl_mem_objects, 0, NULL, NULL);

    // Launch kernel.
    if (diag_opts.verbose)
    {
        printf("done.\nExecuting on device... ");
        fflush(stdout);
    }

    cl_event kernel_run;
    GET_CURRENT_TIMER(dev_start)
    OCLERROR_RET(clEnqueueNDRangeKernel(queue, saxpy, 1, NULL, &length, &wgs, 0,
                                        NULL, &kernel_run),
                 error, que);
    OCLERROR_RET(clWaitForEvents(1, &kernel_run), error, que);
    GET_CURRENT_TIMER(dev_end)

    cl_ulong dev_time;
    TIMER_DIFFERENCE(dev_time, dev_start, dev_end)

    // Release OpenCL memory objects created from Vulkan external memory
    // handles.
    clEnqueueReleaseExternalMemObjectsKHR_fn
        clEnqueueReleaseExternalMemObjects =
            (clEnqueueReleaseExternalMemObjectsKHR_fn)
                clGetExtensionFunctionAddressForPlatform(
                    cl_platform, "clEnqueueReleaseExternalMemObjectsKHR");
    clEnqueueReleaseExternalMemObjects(queue, 2, cl_mem_objects, 0, NULL, NULL);

    // Concurrently calculate reference saxpy.
    if (diag_opts.verbose)
    {
        printf("done.\nExecuting on host... ");
    }

    GET_CURRENT_TIMER(host_start)
    host_saxpy(arr_x, arr_y, a, length);
    GET_CURRENT_TIMER(host_end)
    cl_ulong host_time;
    TIMER_DIFFERENCE(host_time, host_start, host_end)

    if (diag_opts.verbose)
    {
        printf("done.\n");
    }

    // Fetch results.
    OCLERROR_RET(clEnqueueReadBuffer(queue, cl_buf_y, CL_BLOCKING, 0,
                                     sizeof(cl_float) * length, (void*)arr_x, 0,
                                     NULL, NULL),
                 error, que);

    // Validate solution.
    for (size_t i = 0; i < length; ++i)
        if (arr_y[i] != arr_x[i])
        {
            printf("Verification failed! %f != %f at index %zu\n", arr_y[i],
                   arr_x[i], i);
            error = CL_INVALID_VALUE;
        }
    if (error == CL_SUCCESS)
    {
        printf("Verification passed.\n");
    }

    if (!diag_opts.quiet)
    {
        printf("Kernel execution time as seen by host: %llu us.\n",
               (unsigned long long)(dev_time + 500) / 1000);

        printf("Kernel execution time as measured by device: %llu us.\n",
               (unsigned long long)(cl_util_get_event_duration(
                                        kernel_run, CL_PROFILING_COMMAND_START,
                                        CL_PROFILING_COMMAND_END, &error)
                                    + 500)
                   / 1000);

        printf("Reference execution as seen by host: %llu us.\n",
               (unsigned long long)(host_time + 500) / 1000);
    }

    // Release resources.
que:
    OCLERROR_RET(clReleaseCommandQueue(queue), end_error, cont);
clbufy:
    OCLERROR_RET(clReleaseMemObject(cl_buf_y), end_error, clbufx);
clbufx:
    OCLERROR_RET(clReleaseMemObject(cl_buf_x), end_error, vulkan);
vulkan:
    vkDestroyBuffer(vk_device, vk_buf_y, NULL);
    vkDestroyBuffer(vk_device, vk_buf_x, NULL);
    vkUnmapMemory(vk_device, vk_buf_y_memory);
    vkUnmapMemory(vk_device, vk_buf_x_memory);
    vkFreeMemory(vk_device, vk_buf_y_memory, NULL);
    vkFreeMemory(vk_device, vk_buf_x_memory, NULL);
arry:
    free(arr_y);
arrx:
    free(arr_x);
sxp:
    OCLERROR_RET(clReleaseKernel(saxpy), end_error, prg);
prg:
    OCLERROR_RET(clReleaseProgram(program), end_error, ker);
ker:
    free(kernel);
cont:
    OCLERROR_RET(clReleaseContext(context), end_error, end);
end:
    if (error) cl_util_print_error(error);
    return error;
}
