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

// The code in this sample was derived from several samples in the Vulkan
// Tutorial: https://vulkan-tutorial.com
//
// The code samples in the Vulkan Tutorial are licensed as CC0 1.0 Universal.

#include <CL/opencl.hpp>
#if !defined(cl_khr_external_memory)
#error cl_khr_external_memory not found, please update your OpenCL headers!
#endif

#include <fstream>
#include <random>
#include <set>
#include <stdexcept>

#include <math.h>

// GLM includes
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "ocean.hpp"

OceanApplication::OceanApplication(cl::sdk::options::Window& opts)
    : win_opts(opts), app_name("Ocean Surface Simulation"),
      window(sf::VideoMode({ (std::uint32_t)win_opts.width,
                             (std::uint32_t)win_opts.height }),
             app_name.c_str(), sf::Style::Titlebar | sf::Style::Close)
{}

void OceanApplication::run()
{
    init_openCL();
    init_vulkan();
    init_openCL_mems();
    main_loop();
    cleanup();
}

void OceanApplication::init_openCL()
{
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);

    printf("Running on platform: %s\n",
           platforms[dev_opts.triplet.plat_index]
               .getInfo<CL_PLATFORM_NAME>()
               .c_str());
    std::vector<cl::Device> devices;
    platforms[dev_opts.triplet.plat_index].getDevices(CL_DEVICE_TYPE_ALL,
                                                      &devices);

    printf(
        "Running on device: %s\n",
        devices[dev_opts.triplet.dev_index].getInfo<CL_DEVICE_NAME>().c_str());

    check_openCL_ext_mem_support(devices[dev_opts.triplet.dev_index]);

    int error = CL_SUCCESS;
    error |= clGetDeviceInfo(
        devices[dev_opts.triplet.dev_index](), CL_DEVICE_IMAGE2D_MAX_WIDTH,
        sizeof(ocl_max_img2d_width), &ocl_max_img2d_width, NULL);
    error |= clGetDeviceInfo(
        devices[dev_opts.triplet.dev_index](), CL_DEVICE_MAX_MEM_ALLOC_SIZE,
        sizeof(ocl_max_alloc_size), &ocl_max_alloc_size, NULL);
    error |= clGetDeviceInfo(devices[dev_opts.triplet.dev_index](),
                             CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(ocl_mem_size),
                             &ocl_mem_size, NULL);

    if (error != CL_SUCCESS) printf("clGetDeviceInfo error: %d\n", error);

    context = cl::Context{ devices[dev_opts.triplet.dev_index] };
    command_queue =
        cl::CommandQueue{ context, devices[dev_opts.triplet.dev_index] };

    auto build_opencl_kernel = [&](const char* src_file, cl::Kernel& kernel,
                                   const char* name) {
        try
        {
            std::string kernel_code =
                cl::util::read_exe_relative_text_file(src_file);
            cl::Program program{ context, kernel_code };
            program.build();
            kernel = cl::Kernel{ program, name };
        } catch (const cl::BuildError& e)
        {
            auto bl = e.getBuildLog();
            std::cout << "Build OpenCL " << name
                      << " kernel error: " << std::endl;
            for (auto& elem : bl) std::cout << elem.second << std::endl;
            exit(1);
        }
    };

    build_opencl_kernel("twiddle.cl", twiddle_kernel, "generate");
    build_opencl_kernel("init_spectrum.cl", init_spectrum_kernel,
                        "init_spectrum");
    build_opencl_kernel("time_spectrum.cl", time_spectrum_kernel, "spectrum");
    build_opencl_kernel("fft_kernel.cl", fft_kernel, "fft_1D");
    build_opencl_kernel("inversion.cl", inversion_kernel, "inversion");
    build_opencl_kernel("normals.cl", normals_kernel, "normals");
}

void OceanApplication::init_openCL_mems()
{
    // init opencl resources
    try
    {
        {
            std::vector<cl_float4> phase_array(ocean_tex_size * ocean_tex_size);
            std::random_device dev;
            std::mt19937 rng(dev());
            std::uniform_real_distribution<float> dist(0.f, 1.f);

            for (size_t i = 0; i < phase_array.size(); ++i)
                phase_array[i] = { dist(rng), dist(rng), dist(rng), dist(rng) };

            noise_mem = std::make_unique<cl::Image2D>(
                context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                cl::ImageFormat(CL_RGBA, CL_FLOAT), ocean_tex_size,
                ocean_tex_size, 0, phase_array.data());
        }


        hkt_pong_mem = std::make_unique<cl::Image2D>(
            context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RG, CL_FLOAT),
            ocean_tex_size, ocean_tex_size);

        dxyz_coef_mem[0] = std::make_unique<cl::Image2D>(
            context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RG, CL_FLOAT),
            ocean_tex_size, ocean_tex_size);

        dxyz_coef_mem[1] = std::make_unique<cl::Image2D>(
            context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RG, CL_FLOAT),
            ocean_tex_size, ocean_tex_size);

        dxyz_coef_mem[2] = std::make_unique<cl::Image2D>(
            context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RG, CL_FLOAT),
            ocean_tex_size, ocean_tex_size);

        h0k_mem = std::make_unique<cl::Image2D>(
            context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RGBA, CL_FLOAT),
            ocean_tex_size, ocean_tex_size);

        size_t log_2_N = (size_t)((log((float)ocean_tex_size) / log(2.f)) - 1);

        twiddle_factors_mem = std::make_unique<cl::Image2D>(
            context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RGBA, CL_FLOAT),
            log_2_N, ocean_tex_size);

        for (size_t target = 0; target < IOPT_COUNT; target++)
        {
            ocl_image_mems[target].resize(swap_chain_images.size());

            for (size_t i = 0; i < swap_chain_images.size(); i++)
            {
                if (app_opts.use_external_memory)
                {
#ifdef _WIN32
                    HANDLE handle = NULL;
                    VkMemoryGetWin32HandleInfoKHR getWin32HandleInfo{};
                    getWin32HandleInfo.sType =
                        VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
                    getWin32HandleInfo.memory =
                        texture_images[target].image_memories[i];
                    getWin32HandleInfo.handleType =
                        VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
                    vkGetMemoryWin32HandleKHR(device, &getWin32HandleInfo,
                                              &handle);

                    const cl_mem_properties props[] = {
                        external_mem_type,
                        (cl_mem_properties)handle,
                        0,
                    };
#elif defined(__linux__)
                    int fd = 0;
                    VkMemoryGetFdInfoKHR getFdInfo{};
                    getFdInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
                    getFdInfo.memory =
                        texture_images[target]
                            .image_memories[i]; // textureImageMemories[i];
                    getFdInfo.handleType = external_mem_type
                            == CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR
                        ? VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT
                        : VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
                    vkGetMemoryFdKHR(device, &getFdInfo, &fd);

                    const cl_mem_properties props[] = {
                        external_mem_type,
                        (cl_mem_properties)fd,
                        0,
                    };
#else
                    const cl_mem_properties* props = NULL;
#endif

                    cl_image_format format{};
                    format.image_channel_order = CL_RGBA;
                    format.image_channel_data_type = CL_FLOAT;

                    cl_image_desc desc{};
                    desc.image_type = CL_MEM_OBJECT_IMAGE2D;
                    desc.image_width = ocean_tex_size;
                    desc.image_height = ocean_tex_size;

                    ocl_image_mems[target][i].reset(new cl::Image2D{
                        clCreateImageWithProperties(context(), props,
                                                    CL_MEM_READ_WRITE, &format,
                                                    &desc, NULL, NULL) });
                }
                else
                {
                    ocl_image_mems[target][i].reset(
                        new cl::Image2D{ context, CL_MEM_READ_WRITE,
                                         cl::ImageFormat{ CL_RGBA, CL_FLOAT },
                                         ocean_tex_size, ocean_tex_size });
                }
            }
        }
    } catch (const cl::Error& e)
    {
        std::cerr << "OpenCL runtime error: " << e.what() << std::endl;
        exit(e.err());
    }
}

void OceanApplication::init_vulkan()
{
    create_instance();
    setup_dbg_msger();
    create_surface();
    pick_physical_device();
    create_logical_device();
    create_swap_chain();
    create_swap_chain_image_views();
    create_render_pass();
    create_uniform_buffer();
    create_descriptor_set_layout();
    create_graphics_pipeline();
    create_command_pool();

    create_depth_resources();
    create_vertex_buffers();
    create_index_buffers();

    create_framebuffers();
    create_texture_images();
    create_texture_image_views();
    create_texture_sampler();
    create_descriptor_pool();
    create_descriptor_sets();
    create_command_buffers();
    create_sync_objects();
}

void OceanApplication::cleanup()
{
    for (auto framebuffer : swap_chain_framebuffers)
    {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    for (auto imageView : swap_chain_image_views)
    {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swap_chain, nullptr);

    vkDestroyPipeline(device, graphics_pipeline, nullptr);
    vkDestroyPipeline(device, wireframe_pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    vkDestroyRenderPass(device, render_pass, nullptr);

    vkDestroyImageView(device, depth_image_view, nullptr);
    vkDestroyImage(device, depth_image, nullptr);
    vkFreeMemory(device, depth_image_memory, nullptr);

    vkDestroyDescriptorPool(device, descriptor_pool, nullptr);

    vkDestroyBuffer(device, staging_tex_buffer, nullptr);
    vkFreeMemory(device, staging_tex_buffer_memory, nullptr);

    for (size_t img_num = 0; img_num < texture_images.size(); img_num++)
    {
        for (auto textureImageView : texture_images[img_num].image_views)
        {
            vkDestroyImageView(device, textureImageView, nullptr);
        }
        for (auto textureImage : texture_images[img_num].images)
        {
            vkDestroyImage(device, textureImage, nullptr);
        }
        for (auto textureImageMemory : texture_images[img_num].image_memories)
        {
            vkFreeMemory(device, textureImageMemory, nullptr);
        }
    }

    for (size_t sampler_num = 0; sampler_num < texture_sampler.size();
         sampler_num++)
    {
        vkDestroySampler(device, texture_sampler[sampler_num], nullptr);
    }

    // cleanup vertices buffers
    for (auto& buffer : vertex_buffers)
    {
        vkDestroyBuffer(device, buffer, nullptr);
    }

    for (auto& bufferMemory : vertex_buffer_memories)
    {
        vkFreeMemory(device, bufferMemory, nullptr);
    }

    // cleanup indices buffers
    for (auto buffer : index_buffer.buffers)
    {
        vkDestroyBuffer(device, buffer, nullptr);
    }

    for (auto& bufferMemory : index_buffer.buffer_memories)
    {
        vkFreeMemory(device, bufferMemory, nullptr);
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(device, render_finished_semaphores[i], nullptr);
        vkDestroySemaphore(device, image_available_semaphores[i], nullptr);
        vkDestroyFence(device, in_flight_fences[i], nullptr);
    }

    for (auto& unif_buffer : uniform_buffers)
    {
        vkDestroyBuffer(device, unif_buffer, nullptr);
    }

    for (auto& unif_buf_mem : uniform_buffers_memory)
    {
        vkFreeMemory(device, unif_buf_mem, nullptr);
    }

    vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);

    vkDestroyCommandPool(device, command_pool, nullptr);

    vkDestroyDevice(device, nullptr);

    if (app_opts.validationLayersOn)
    {
        DestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}

void OceanApplication::create_instance()
{
    if (app_opts.validationLayersOn && !check_validation_layer_support())
    {
        throw std::runtime_error(
            "validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName =
        "Ocean Surface Simulation with OpenCL+Vulkan Sample";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    if (app_opts.use_external_memory)
    {
        appInfo.apiVersion = VK_API_VERSION_1_1;
    }
    else
    {
        appInfo.apiVersion = VK_API_VERSION_1_0;
    }

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = get_required_exts();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (app_opts.validationLayersOn)
    {
        createInfo.enabledLayerCount =
            static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populate_dbg_msger_create_info(debugCreateInfo);
        createInfo.pNext =
            (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create instance!");
    }

#ifdef _WIN32
    if (app_opts.use_external_memory)
    {
        vkGetMemoryWin32HandleKHR =
            (PFN_vkGetMemoryWin32HandleKHR)vkGetInstanceProcAddr(
                instance, "vkGetMemoryWin32HandleKHR");
        if (vkGetMemoryWin32HandleKHR == NULL)
        {
            throw std::runtime_error("couldn't get function pointer for "
                                     "vkGetMemoryWin32HandleKHR");
        }
    }
#elif defined(__linux__)
    if (app_opts.use_external_memory)
    {
        vkGetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)vkGetInstanceProcAddr(
            instance, "vkGetMemoryFdKHR");
        if (vkGetMemoryFdKHR == NULL)
        {
            throw std::runtime_error(
                "couldn't get function pointer for vkGetMemoryFdKHR");
        }
    }
#endif
}

void OceanApplication::populate_dbg_msger_create_info(
    VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debug_callback;
}

void OceanApplication::setup_dbg_msger()
{
    if (!app_opts.validationLayersOn) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populate_dbg_msger_create_info(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr,
                                     &debug_messenger)
        != VK_SUCCESS)
    {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void OceanApplication::create_surface()
{
    if (!window.createVulkanSurface(instance, surface))
        throw std::runtime_error("failed to create window surface!");
}

void OceanApplication::pick_physical_device()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (auto i = 0; i < devices.size(); i++)
    {
        if (app_opts.vulkan_device >= 0 && app_opts.vulkan_device != i)
            continue;

        if (is_device_suitable(devices[i]))
        {
            physical_device = devices[i];
            break;
        }
    }

    if (physical_device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physical_device, &properties);

    printf("Running on Vulkan physical device: %s\n", properties.deviceName);
}

void OceanApplication::create_logical_device()
{
    QueueFamilyIndices indices = find_queue_families(physical_device);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily,
                                               indices.presentFamily };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.fillModeNonSolid = true;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount =
        static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    auto extensions = get_required_dev_exts();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = 0;

    if (vkCreateDevice(physical_device, &createInfo, nullptr, &device)
        != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphics_queue);
    vkGetDeviceQueue(device, indices.presentFamily, 0, &present_queue);
}

void OceanApplication::create_swap_chain()
{
    SwapChainSupportDetails swapChainSupport =
        query_swap_chain_support(physical_device);

    VkSurfaceFormatKHR surfaceFormat =
        choose_swap_surf_format(swapChainSupport.formats);
    VkPresentModeKHR presentMode =
        choose_swap_present_mode(swapChainSupport.presentModes);
    VkExtent2D extent = choose_swap_extent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0
        && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = find_queue_families(physical_device);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily,
                                      indices.presentFamily };

    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swap_chain)
        != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, swap_chain, &imageCount, nullptr);
    swap_chain_images.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swap_chain, &imageCount,
                            swap_chain_images.data());

    swap_chain_image_format = surfaceFormat.format;
    swap_chain_extent = extent;
}

void OceanApplication::create_swap_chain_image_views()
{
    swap_chain_image_views.resize(swap_chain_images.size());

    for (uint32_t i = 0; i < swap_chain_images.size(); i++)
    {
        swap_chain_image_views[i] =
            create_image_view(swap_chain_images[i], swap_chain_image_format,
                              VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void OceanApplication::create_render_pass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swap_chain_image_format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = find_depth_format();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
        | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment,
                                                           depthAttachment };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &render_pass)
        != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass!");
    }
}

void OceanApplication::create_uniform_buffer()
{
    VkDeviceSize buffer_size = sizeof(UniformBufferObject);

    uniform_buffers.resize(swap_chain_images.size());
    uniform_buffers_memory.resize(swap_chain_images.size());

    _mapped_unif_data.resize(swap_chain_images.size());

    for (size_t i = 0; i < uniform_buffers.size(); i++)
    {
        create_buffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                          | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      uniform_buffers[i], uniform_buffers_memory[i]);

        vkMapMemory(device, uniform_buffers_memory[i], 0, buffer_size, 0,
                    &_mapped_unif_data[i].buffer_memory);
    }
}

void OceanApplication::create_descriptor_set_layout()
{
    VkDescriptorSetLayoutBinding sampler0LayoutBinding{};
    sampler0LayoutBinding.binding = 0;
    sampler0LayoutBinding.descriptorCount = 1;
    sampler0LayoutBinding.descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler0LayoutBinding.pImmutableSamplers = nullptr;
    sampler0LayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding sampler1LayoutBinding{};
    sampler1LayoutBinding.binding = 1;
    sampler1LayoutBinding.descriptorCount = 1;
    sampler1LayoutBinding.descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler1LayoutBinding.pImmutableSamplers = nullptr;
    sampler1LayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding uniformLayoutBinding{};
    uniformLayoutBinding.binding = 2;
    uniformLayoutBinding.descriptorCount = 1;
    uniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformLayoutBinding.pImmutableSamplers = nullptr;
    uniformLayoutBinding.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 3> bindings = {
        sampler0LayoutBinding, sampler1LayoutBinding, uniformLayoutBinding
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                                    &descriptor_set_layout)
        != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void OceanApplication::create_graphics_pipeline()
{
    auto vertShaderCode =
        cl::util::read_exe_relative_binary_file("ocean.vert.spv");
    auto fragShaderCode =
        cl::util::read_exe_relative_binary_file("ocean.frag.spv");

    VkShaderModule vertShaderModule = create_shader_module(vertShaderCode);
    VkShaderModule fragShaderModule = create_shader_module(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo,
                                                       fragShaderStageInfo };

    // vertex info
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    inputAssembly.primitiveRestartEnable = VK_TRUE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swap_chain_extent.width;
    viewport.height = (float)swap_chain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swap_chain_extent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
        | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
        | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptor_set_layout;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                               &pipeline_layout)
        != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipeline_layout;
    pipelineInfo.renderPass = render_pass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                  nullptr, &graphics_pipeline)
        != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                  nullptr, &wireframe_pipeline)
        != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void OceanApplication::create_framebuffers()
{
    swap_chain_framebuffers.resize(swap_chain_image_views.size());

    for (size_t i = 0; i < swap_chain_image_views.size(); i++)
    {
        std::array<VkImageView, 2> attachments = { swap_chain_image_views[i],
                                                   depth_image_view };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = render_pass;
        framebufferInfo.attachmentCount =
            static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swap_chain_extent.width;
        framebufferInfo.height = swap_chain_extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr,
                                &swap_chain_framebuffers[i])
            != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void OceanApplication::create_command_pool()
{
    QueueFamilyIndices queueFamilyIndices =
        find_queue_families(physical_device);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &command_pool)
        != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create command pool!");
    }
}

void OceanApplication::create_vertex_buffers()
{
    size_t iCXY = (ocean_grid_size + 1) * (ocean_grid_size + 1);
    ocean_grid_vertices.resize(iCXY);

    cl_float dfY = -0.5f * (ocean_grid_size * mesh_spacing),
             dfBaseX = -0.5f * (ocean_grid_size * mesh_spacing);
    cl_float tx = 0.f, ty = 0.f, dtx = 1.f / ocean_grid_size,
             dty = 1.f / ocean_grid_size;
    for (size_t iBase = 0, iY = 0; iY <= ocean_grid_size;
         iY++, iBase += ocean_grid_size + 1)
    {
        tx = 0.f;
        double dfX = dfBaseX;
        for (int iX = 0; iX <= ocean_grid_size; iX++)
        {
            ocean_grid_vertices[iBase + iX].pos = glm::vec3(dfX, dfY, 0.0);
            ocean_grid_vertices[iBase + iX].tc = glm::vec2(tx, ty);
            tx += dtx;
            dfX += mesh_spacing;
        }
        dfY += mesh_spacing;
        ty += dty;
    }

    vertex_buffers.resize(swap_chain_images.size());
    vertex_buffer_memories.resize(swap_chain_images.size());

    VkDeviceSize bufferSize =
        sizeof(ocean_grid_vertices[0]) * ocean_grid_vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    create_buffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                      | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, ocean_grid_vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    for (size_t i = 0; i < swap_chain_images.size(); i++)
    {

        // create local memory buffer
        create_buffer(bufferSize,
                      VK_BUFFER_USAGE_TRANSFER_DST_BIT
                          | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_buffers[i],
                      vertex_buffer_memories[i]);

        copy_buffer(stagingBuffer, vertex_buffers[i], bufferSize);
    }

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void OceanApplication::create_index_buffers()
{
    size_t totalIndices = ((ocean_grid_size + 1) * 2 + 1) * ocean_grid_size;
    ocean_grid_indices.resize(totalIndices);

    VkDeviceSize bufferSize =
        sizeof(ocean_grid_indices[0]) * ocean_grid_indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    create_buffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                      | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  stagingBuffer, stagingBufferMemory);

    size_t indexCount = 0;
    for (size_t iY = 0; iY < ocean_grid_size; iY++)
    {
        size_t iBaseFrom = iY * (ocean_grid_size + 1);
        size_t iBaseTo = iBaseFrom + ocean_grid_size + 1;

        for (size_t iX = 0; iX <= ocean_grid_size; iX++)
        {
            ocean_grid_indices[indexCount++] = static_cast<int>(iBaseFrom + iX);
            ocean_grid_indices[indexCount++] = static_cast<int>(iBaseTo + iX);
        }
        ocean_grid_indices[indexCount++] = -1;
    }

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, ocean_grid_indices.data(), (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    index_buffer.buffers.resize(swap_chain_images.size());
    index_buffer.buffer_memories.resize(swap_chain_images.size());

    for (size_t i = 0; i < swap_chain_images.size(); i++)
    {
        create_buffer(bufferSize,
                      VK_BUFFER_USAGE_TRANSFER_DST_BIT
                          | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      index_buffer.buffers[i], index_buffer.buffer_memories[i]);

        copy_buffer(stagingBuffer, index_buffer.buffers[i], bufferSize);
    }

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void OceanApplication::create_texture_images()
{
    VkImageTiling tiling = app_opts.linearImages ? VK_IMAGE_TILING_LINEAR
                                                 : VK_IMAGE_TILING_OPTIMAL;
    VkMemoryPropertyFlags properties =
        app_opts.device_local_images ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0;

    uint32_t texWidth = static_cast<uint32_t>(ocean_tex_size);
    uint32_t texHeight = static_cast<uint32_t>(ocean_tex_size);

    VkDeviceSize imageSize = texWidth * texHeight * 4 * sizeof(float);

    create_buffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                      | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  staging_tex_buffer, staging_tex_buffer_memory);

    for (size_t target = 0; target < texture_images.size(); target++)
    {
        texture_images[target].images.resize(swap_chain_images.size());
        texture_images[target].image_memories.resize(swap_chain_images.size());

        for (size_t i = 0; i < swap_chain_images.size(); i++)
        {
            create_shareable_image(
                texWidth, texHeight, VK_FORMAT_R32G32B32A32_SFLOAT, tiling,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                properties, texture_images[target].images[i],
                texture_images[target].image_memories[i]);
            if (app_opts.use_external_memory)
            {
                transition_image_layout(
                    texture_images[target].images[i],
                    VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }
        }
    }
}

void OceanApplication::create_texture_image_views()
{
    for (size_t img_num = 0; img_num < texture_images.size(); img_num++)
    {
        texture_images[img_num].image_views.resize(swap_chain_images.size());

        for (size_t i = 0; i < swap_chain_images.size(); i++)
        {
            texture_images[img_num].image_views[i] = create_image_view(
                texture_images[img_num].images[i],
                VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
        }
    }
}

void OceanApplication::create_texture_sampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

    for (size_t sampler_num = 0; sampler_num < texture_sampler.size();
         sampler_num++)
    {
        if (vkCreateSampler(device, &samplerInfo, nullptr,
                            &texture_sampler[sampler_num])
            != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }
}

VkImageView OceanApplication::create_image_view(VkImage image, VkFormat format,
                                                VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewInfo.pNext = nullptr;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1; // VK_REMAINING_MIP_LEVELS;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    viewInfo.subresourceRange.aspectMask = aspectFlags;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

void OceanApplication::create_shareable_image(
    uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
    VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
    VkDeviceMemory& imageMemory, VkImageType type)
{
    VkExternalMemoryImageCreateInfo externalMemCreateInfo{};
    externalMemCreateInfo.sType =
        VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;

#ifdef _WIN32
    externalMemCreateInfo.handleTypes =
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#elif defined(__linux__)
    externalMemCreateInfo.handleTypes =
        external_mem_type == CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR
        ? VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT
        : VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
#endif

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    if (app_opts.use_external_memory)
    {
        imageInfo.pNext = &externalMemCreateInfo;
    }

    imageInfo.imageType = type;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkExportMemoryAllocateInfo exportMemoryAllocInfo{};
    exportMemoryAllocInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
    exportMemoryAllocInfo.handleTypes = externalMemCreateInfo.handleTypes;

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    if (app_opts.use_external_memory)
    {
        allocInfo.pNext = &exportMemoryAllocInfo;
    }
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        find_memory_type(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory)
        != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}

void OceanApplication::create_image(uint32_t width, uint32_t height,
                                    VkFormat format, VkImageTiling tiling,
                                    VkImageUsageFlags usage,
                                    VkMemoryPropertyFlags properties,
                                    VkImage& image, VkDeviceMemory& imageMemory)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        find_memory_type(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory)
        != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}

VkFormat
OceanApplication::find_supported_format(const std::vector<VkFormat>& candidates,
                                        VkImageTiling tiling,
                                        VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR
            && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL
                 && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

VkFormat OceanApplication::find_depth_format()
{
    return find_supported_format(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
          VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool OceanApplication::has_stencil_component(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT
        || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void OceanApplication::create_depth_resources()
{
    VkFormat depthFormat = find_depth_format();

    create_image(
        swap_chain_extent.width, swap_chain_extent.height, depthFormat,
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depth_image, depth_image_memory);
    depth_image_view =
        create_image_view(depth_image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void OceanApplication::transition_image_layout(VkImage image, VkFormat format,
                                               VkImageLayout oldLayout,
                                               VkImageLayout newLayout,
                                               uint32_t layers)
{

    VkCommandBuffer commandBuffer = begin_single_time_commands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    //        vulkan spec: If the calling command’s VkImage parameter is of
    //        VkImageType VK_IMAGE_TYPE_3D, the baseArrayLayer and
    //        layerCount members of imageSubresource must be 0 and 1,
    //        respectively
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layers;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED
        && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
             && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED
             && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT /*|
                           VK_PIPELINE_STAGE_VERTEX_SHADER_BIT*/
            ;
    }
    else
    {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
                         nullptr, 0, nullptr, 1, &barrier);

    end_single_time_commands(commandBuffer);
}

void OceanApplication::copy_buffer_to_image(VkBuffer buffer, VkImage image,
                                            uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = begin_single_time_commands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    end_single_time_commands(commandBuffer);
}

void OceanApplication::create_descriptor_pool()
{
    std::array<VkDescriptorPoolSize, 3> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[0].descriptorCount =
        static_cast<uint32_t>(swap_chain_images.size());

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount =
        static_cast<uint32_t>(swap_chain_images.size());

    poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[2].descriptorCount =
        static_cast<uint32_t>(swap_chain_images.size());

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(swap_chain_images.size());

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptor_pool)
        != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void OceanApplication::create_descriptor_sets()
{
    std::vector<VkDescriptorSetLayout> layouts(swap_chain_images.size(),
                                               descriptor_set_layout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptor_pool;
    allocInfo.descriptorSetCount =
        static_cast<uint32_t>(swap_chain_images.size());
    allocInfo.pSetLayouts = layouts.data();

    descriptor_sets.resize(swap_chain_images.size());
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptor_sets.data())
        != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < swap_chain_images.size(); i++)
    {
        VkDescriptorImageInfo imageInfo[(size_t)InteropTexType::IOPT_COUNT] = {
            0
        };

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniform_buffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        std::array<VkWriteDescriptorSet, IOPT_COUNT + 1> descriptorWrites{};

        for (cl_int target = 0; target < IOPT_COUNT; target++)
        {
            imageInfo[target].imageLayout =
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo[target].imageView = texture_images[target].image_views[i];
            imageInfo[target].sampler = texture_sampler[target];

            descriptorWrites[target].sType =
                VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[target].dstSet = descriptor_sets[i];
            descriptorWrites[target].dstBinding = target;
            descriptorWrites[target].dstArrayElement = 0;
            descriptorWrites[target].descriptorType =
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[target].descriptorCount = 1;
            descriptorWrites[target].pImageInfo = &imageInfo[target];
        }

        descriptorWrites[IOPT_COUNT].sType =
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[IOPT_COUNT].dstSet = descriptor_sets[i];
        descriptorWrites[IOPT_COUNT].dstBinding = IOPT_COUNT;
        descriptorWrites[IOPT_COUNT].dstArrayElement = 0;
        descriptorWrites[IOPT_COUNT].descriptorType =
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[IOPT_COUNT].descriptorCount = 1;
        descriptorWrites[IOPT_COUNT].pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(device,
                               static_cast<uint32_t>(descriptorWrites.size()),
                               descriptorWrites.data(), 0, nullptr);
    }
}

void OceanApplication::create_buffer(VkDeviceSize size,
                                     VkBufferUsageFlags usage,
                                     VkMemoryPropertyFlags properties,
                                     VkBuffer& buffer,
                                     VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        find_memory_type(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory)
        != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void OceanApplication::copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
                                   VkDeviceSize size)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = command_pool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue);

    vkFreeCommandBuffers(device, command_pool, 1, &commandBuffer);
}

uint32_t OceanApplication::find_memory_type(uint32_t typeFilter,
                                            VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i))
            && (memProperties.memoryTypes[i].propertyFlags & properties)
                == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

VkCommandBuffer OceanApplication::begin_single_time_commands()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = command_pool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void OceanApplication::end_single_time_commands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue);

    vkFreeCommandBuffers(device, command_pool, 1, &commandBuffer);
}

void OceanApplication::create_command_buffers()
{
    command_buffers.resize(swap_chain_framebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = command_pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)command_buffers.size();

    if (vkAllocateCommandBuffers(device, &allocInfo, command_buffers.data())
        != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    for (size_t i = 0; i < command_buffers.size(); i++)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(command_buffers[i], &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error(
                "failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = render_pass;
        renderPassInfo.framebuffer = swap_chain_framebuffers[i];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = swap_chain_extent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
        clearValues[1].depthStencil = { 1.0f, 0 };

        renderPassInfo.clearValueCount =
            static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(command_buffers[i], &renderPassInfo,
                             VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                          wireframe_mode ? wireframe_pipeline
                                         : graphics_pipeline);

        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(command_buffers[i], 0, 1, &vertex_buffers[i],
                               offsets);

        vkCmdBindDescriptorSets(
            command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline_layout, 0, 1, &descriptor_sets[i], 0, nullptr);

        vkCmdBindIndexBuffer(command_buffers[i], index_buffer.buffers[i], 0,
                             VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(command_buffers[i],
                         static_cast<uint32_t>(ocean_grid_indices.size()), 1, 0,
                         0, 0);

        vkCmdEndRenderPass(command_buffers[i]);

        if (vkEndCommandBuffer(command_buffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
}

void OceanApplication::create_sync_objects()
{
    image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);
    images_in_flight.resize(swap_chain_images.size(), VK_NULL_HANDLE);

    VkExportSemaphoreCreateInfo exportSemaphoreCreateInfo{};
    exportSemaphoreCreateInfo.sType =
        VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                              &image_available_semaphores[i])
                != VK_SUCCESS
            || vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                                 &render_finished_semaphores[i])
                != VK_SUCCESS
            || vkCreateFence(device, &fenceInfo, nullptr, &in_flight_fences[i])
                != VK_SUCCESS)
        {
            throw std::runtime_error(
                "failed to create synchronization objects for a frame!");
        }
    }
}

void OceanApplication::update_uniforms(uint32_t currentImage)
{
    UniformBufferObject ubo = _mapped_unif_data[currentImage].data;
    ubo.choppiness = choppiness;
    ubo.alt_scale = alt_scale;

    // update camera related uniform
    glm::mat4 view_matrix =
        glm::lookAt(camera.eye, camera.eye + camera.dir, camera.up);

    float fov = (float)glm::radians(60.0);
    float aspect = (float)win_opts.width / win_opts.height;
    glm::mat4 proj_matrix = glm::perspective(
        fov, aspect, 1.f, 2.f * ocean_grid_size * mesh_spacing);
    proj_matrix[1][1] *= -1;

    ubo.view_mat = view_matrix;
    ubo.proj_mat = proj_matrix;

    memcpy(_mapped_unif_data[currentImage].buffer_memory, &ubo,
           sizeof(UniformBufferObject));
}

void OceanApplication::update_spectrum(uint32_t currentImage, float elapsed)
{
    cl_int2 patch =
        cl_int2{ (int)(ocean_grid_size * mesh_spacing), (int)ocean_tex_size };

    cl::NDRange lws; // NullRange by default.
    if (group_size > 0)
    {
        lws = cl::NDRange{ group_size, group_size };
    }

    if (twiddle_factors_init)
    {
        try
        {
            size_t log_2_N =
                (size_t)((log((float)ocean_tex_size) / log(2.f)) - 1);

            /// Prepare vector of values to extract results
            std::vector<cl_int> v(ocean_tex_size);
            for (int i = 0; i < ocean_tex_size; i++)
            {
                int x = reverse_bits(i, log_2_N);
                v[i] = x;
            }

            /// Initialize device-side storage
            cl::Buffer bit_reversed_inds_mem{
                context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                sizeof(cl_int) * v.size(), v.data()
            };

            twiddle_kernel.setArg(0, cl_int(ocean_tex_size));
            twiddle_kernel.setArg(1, bit_reversed_inds_mem);
            twiddle_kernel.setArg(2, *twiddle_factors_mem);

            command_queue.enqueueNDRangeKernel(
                twiddle_kernel, cl::NullRange,
                cl::NDRange{ (cl::size_type)log_2_N, ocean_tex_size },
                cl::NDRange{ 1, 16 });
            twiddle_factors_init = false;
        } catch (const cl::Error& e)
        {
            std::cerr << "OpenCL runtime error: " << e.what() << std::endl;
            exit(e.err());
        }
    }

    // change of some ocean's parameters requires to rebuild initial spectrum
    // image
    if (changed)
    {
        try
        {
            float wind_angle_rad = glm::radians(wind_angle);
            cl_float4 params =
                cl_float4{ wind_magnitude * glm::cos(wind_angle_rad),
                           wind_magnitude * glm::sin(wind_angle_rad), amplitude,
                           supress_factor };
            init_spectrum_kernel.setArg(0, patch);
            init_spectrum_kernel.setArg(1, params);
            init_spectrum_kernel.setArg(2, *noise_mem);
            init_spectrum_kernel.setArg(3, *h0k_mem);

            command_queue.enqueueNDRangeKernel(
                init_spectrum_kernel, cl::NullRange,
                cl::NDRange{ ocean_tex_size, ocean_tex_size }, lws);
            changed = false;
        } catch (const cl::Error& e)
        {
            std::cerr << "OpenCL runtime error: " << e.what() << std::endl;
            exit(e.err());
        }
    }

    // ping-pong phase spectrum kernel launch
    try
    {
        time_spectrum_kernel.setArg(0, elapsed);
        time_spectrum_kernel.setArg(1, patch);
        time_spectrum_kernel.setArg(2, *h0k_mem);
        time_spectrum_kernel.setArg(3, *dxyz_coef_mem[0]);
        time_spectrum_kernel.setArg(4, *dxyz_coef_mem[1]);
        time_spectrum_kernel.setArg(5, *dxyz_coef_mem[2]);

        command_queue.enqueueNDRangeKernel(
            time_spectrum_kernel, cl::NullRange,
            cl::NDRange{ ocean_tex_size, ocean_tex_size }, lws);
    } catch (const cl::Error& e)
    {
        std::cerr << "OpenCL runtime error: " << e.what() << std::endl;
        exit(e.err());
    }


    // perform 1D FFT horizontal and vertical iterations
    size_t log_2_N = (size_t)((log((float)ocean_tex_size) / log(2.f)) - 1);
    fft_kernel.setArg(1, patch);
    fft_kernel.setArg(2, *twiddle_factors_mem);
    for (cl_int i = 0; i < 3; i++)
    {
        const cl::Image* displ_swap[] = { dxyz_coef_mem[i].get(),
                                          hkt_pong_mem.get() };
        cl_int2 mode = cl_int2{ { 0, 0 } };

        bool ifft_pingpong = false;
        for (int p = 0; p < log_2_N; p++)
        {
            if (ifft_pingpong)
            {
                fft_kernel.setArg(3, *displ_swap[1]);
                fft_kernel.setArg(4, *displ_swap[0]);
            }
            else
            {
                fft_kernel.setArg(3, *displ_swap[0]);
                fft_kernel.setArg(4, *displ_swap[1]);
            }

            mode.s[1] = p;
            fft_kernel.setArg(0, mode);

            command_queue.enqueueNDRangeKernel(
                fft_kernel, cl::NullRange,
                cl::NDRange{ ocean_tex_size, ocean_tex_size }, lws);


            ifft_pingpong = !ifft_pingpong;
        }

        // Cols
        mode.s[0] = 1;
        for (int p = 0; p < log_2_N; p++)
        {
            if (ifft_pingpong)
            {
                fft_kernel.setArg(3, *displ_swap[1]);
                fft_kernel.setArg(4, *displ_swap[0]);
            }
            else
            {
                fft_kernel.setArg(3, *displ_swap[0]);
                fft_kernel.setArg(4, *displ_swap[1]);
            }

            mode.s[1] = p;
            fft_kernel.setArg(0, mode);

            command_queue.enqueueNDRangeKernel(
                fft_kernel, cl::NullRange,
                cl::NDRange{ ocean_tex_size, ocean_tex_size }, lws);

            ifft_pingpong = !ifft_pingpong;
        }

        if (log_2_N % 2)
        {
            // swap images if pingpong hold on temporary buffer
            std::array<size_t, 3> orig = { 0, 0, 0 },
                                  region = { ocean_tex_size, ocean_tex_size,
                                             1 };
            command_queue.enqueueCopyImage(*displ_swap[0], *displ_swap[1], orig,
                                           orig, region);
        }
    }

    if (app_opts.use_external_memory)
    {
        for (size_t target = 0; target < IOPT_COUNT; target++)
        {
            command_queue.enqueueAcquireExternalMemObjects(
                { *ocl_image_mems[target][currentImage] });
        }
    }

    // inversion
    {
        inversion_kernel.setArg(0, patch);
        inversion_kernel.setArg(1, *dxyz_coef_mem[0]);
        inversion_kernel.setArg(2, *dxyz_coef_mem[1]);
        inversion_kernel.setArg(3, *dxyz_coef_mem[2]);
        inversion_kernel.setArg(
            4, *ocl_image_mems[IOPT_DISPLACEMENT][currentImage]);

        command_queue.enqueueNDRangeKernel(
            inversion_kernel, cl::NullRange,
            cl::NDRange{ ocean_tex_size, ocean_tex_size }, lws);
    }

    // normals computation
    {
        cl_float2 factors = cl_float2{ choppiness, alt_scale };

        normals_kernel.setArg(0, patch);
        normals_kernel.setArg(1, factors);
        normals_kernel.setArg(2, *noise_mem);
        normals_kernel.setArg(3,
                              *ocl_image_mems[IOPT_DISPLACEMENT][currentImage]);
        normals_kernel.setArg(4,
                              *ocl_image_mems[IOPT_NORMAL_MAP][currentImage]);

        command_queue.enqueueNDRangeKernel(
            normals_kernel, cl::NullRange,
            cl::NDRange{ ocean_tex_size, ocean_tex_size }, lws);
    }
}

void OceanApplication::show_fps_window_title()
{
    if (show_fps)
    {
        auto fps_now = std::chrono::system_clock::now();

        std::chrono::duration<float> elapsed = fps_now - fps_last_time;
        float delta = elapsed.count();

        const float elapsed_tres = 1.f;

        delta_frames++;
        if (delta >= 1.f)
        {
            double fps = double(delta_frames) / delta;

            std::stringstream ss;
            ss << app_name << ", [FPS:" << std::fixed << std::setprecision(2)
               << fps << "]";

            window.setTitle(ss.str().c_str());

            delta_frames = 0;
            fps_last_time = fps_now;
        }
    }
    else
    {
        fps_last_time = std::chrono::system_clock::now();
        delta_frames = 0;
    }
}

void OceanApplication::update_ocean(uint32_t currentImage)
{
    show_fps_window_title();

    update_uniforms(currentImage);

    auto end = std::chrono::system_clock::now();

    // time factor of ocean animation
    static float elapsed = 0.f;

    if (animate)
    {
        std::chrono::duration<float> delta = end - start;
        elapsed = delta.count();

        update_spectrum(currentImage, elapsed);

        if (app_opts.use_external_memory)
        {
            for (size_t target = 0; target < IOPT_COUNT; target++)
            {
                command_queue.enqueueReleaseExternalMemObjects(
                    { *ocl_image_mems[target][currentImage] });
            }

            command_queue.finish();
        }
        else
        {
            for (size_t target = 0; target < IOPT_COUNT; target++)
            {
                size_t rowPitch = 0;
                void* pixels = command_queue.enqueueMapImage(
                    *ocl_image_mems[target][currentImage], CL_TRUE, CL_MAP_READ,
                    { 0, 0, 0 }, { ocean_tex_size, ocean_tex_size, 1 },
                    &rowPitch, nullptr);

                VkDeviceSize imageSize =
                    ocean_tex_size * ocean_tex_size * 4 * sizeof(float);

                void* data;
                vkMapMemory(device, staging_tex_buffer_memory, 0, imageSize, 0,
                            &data);
                memcpy(data, pixels, static_cast<size_t>(imageSize));
                vkUnmapMemory(device, staging_tex_buffer_memory);

                command_queue.enqueueUnmapMemObject(
                    *ocl_image_mems[target][currentImage], pixels);
                command_queue.flush();

                transition_image_layout(
                    texture_images[target].images[currentImage],
                    VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                copy_buffer_to_image(
                    staging_tex_buffer,
                    texture_images[target].images[currentImage],
                    static_cast<uint32_t>(ocean_tex_size),
                    static_cast<uint32_t>(ocean_tex_size));
                transition_image_layout(
                    texture_images[target].images[currentImage],
                    VK_FORMAT_R32G32B32A32_SFLOAT,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }
        }
    }
    else
    {
        // hold the animation at the same time point
        std::chrono::duration<float> duration(elapsed);
        start =
            end - std::chrono::duration_cast<std::chrono::seconds>(duration);

        if (app_opts.use_external_memory)
        {
            command_queue.finish();
        }
    }
}

void OceanApplication::draw_frame()
{
    vkWaitForFences(device, 1, &in_flight_fences[current_frame], VK_TRUE,
                    UINT64_MAX);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(device, swap_chain, UINT64_MAX,
                          image_available_semaphores[current_frame],
                          VK_NULL_HANDLE, &imageIndex);

    update_ocean(imageIndex);

    if (images_in_flight[imageIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(device, 1, &images_in_flight[imageIndex], VK_TRUE,
                        UINT64_MAX);
    }
    images_in_flight[imageIndex] = in_flight_fences[current_frame];

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    std::vector<VkSemaphore> waitSemaphores;
    std::vector<VkPipelineStageFlags> waitStages;
    waitSemaphores.push_back(image_available_semaphores[current_frame]);
    waitStages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    submitInfo.waitSemaphoreCount =
        static_cast<uint32_t>(waitSemaphores.size());
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.pWaitDstStageMask = waitStages.data();

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &command_buffers[imageIndex];

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &render_finished_semaphores[current_frame];

    vkResetFences(device, 1, &in_flight_fences[current_frame]);

    if (vkQueueSubmit(graphics_queue, 1, &submitInfo,
                      in_flight_fences[current_frame])
        != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &render_finished_semaphores[current_frame];

    VkSwapchainKHR swapChains[] = { swap_chain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(present_queue, &presentInfo);

    current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void OceanApplication::check_openCL_ext_mem_support(cl::Device& device)
{
    if (cl::util::supports_extension(device, "cl_khr_external_memory"))
    {
        printf("Device supports cl_khr_external_memory.\n");
        printf("Supported external memory handle types:\n");

        std::vector<cl::ExternalMemoryType> types =
            device.getInfo<CL_DEVICE_EXTERNAL_MEMORY_IMPORT_HANDLE_TYPES_KHR>();
        for (auto type : types)
        {
#define CASE_TO_STRING(_e)                                                     \
    case _e: printf("\t%s\n", #_e); break;
            switch (
                static_cast<std::underlying_type<cl::ExternalMemoryType>::type>(
                    type))
            {
                CASE_TO_STRING(CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR);
                CASE_TO_STRING(CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_WIN32_KHR);
                CASE_TO_STRING(CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_WIN32_KMT_KHR);
                CASE_TO_STRING(CL_EXTERNAL_MEMORY_HANDLE_DMA_BUF_KHR);
                default:
                    printf("Unknown cl_external_memory_handle_type_khr %04X\n",
                           (unsigned int)type);
            }
#undef CASE_TO_STRING
        }


#ifdef _WIN32
        if (std::find_if(
                types.begin(), types.end(),
                [](cl::ExternalMemoryType& emt) {
                    return static_cast<
                               std::underlying_type_t<cl::ExternalMemoryType>>(
                               emt)
                        == CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_WIN32_KHR;
                })
            != types.end())
        {
            external_mem_type = CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_WIN32_KHR;
        }
        else
        {
            printf("Couldn't find a compatible external memory type "
                   "(sample supports OPAQUE_WIN32).\n");
            app_opts.use_external_memory = false;
        }
#elif defined(__linux__)
        if (std::find(
                types.begin(), types.end(),
                cl::ExternalMemoryType(CL_EXTERNAL_MEMORY_HANDLE_DMA_BUF_KHR))
            != types.end())
        {
            external_mem_type = CL_EXTERNAL_MEMORY_HANDLE_DMA_BUF_KHR;
        }
        else if (std::find(types.begin(), types.end(),
                           cl::ExternalMemoryType(
                               CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR))
                 != types.end())
        {
            external_mem_type = CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR;
        }
        else
        {
            printf("Couldn't find a compatible external memory type "
                   "(sample supports DMA_BUF or OPAQUE_FD).\n");
            app_opts.use_external_memory = false;
        }
#endif
    }
    else
    {
        printf("Device does not support cl_khr_external_memory.\n");
        app_opts.use_external_memory = false;
    }
}

VkShaderModule
OceanApplication::create_shader_module(const std::vector<unsigned char>& code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule)
        != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

VkSurfaceFormatKHR OceanApplication::choose_swap_surf_format(
    const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR OceanApplication::choose_swap_present_mode(
    const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (!app_opts.immediate)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return availablePresentMode;
            }
        }
        else
        {
            if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
            {
                return availablePresentMode;
            }
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D OceanApplication::choose_swap_extent(
    const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }
    else
    {
        int width = win_opts.width, height = win_opts.width;

        auto wsize = window.getSize();
        width = wsize.x;
        height = wsize.y;

        VkExtent2D actualExtent = { static_cast<uint32_t>(width),
                                    static_cast<uint32_t>(height) };

        actualExtent.width = std::max(
            capabilities.minImageExtent.width,
            std::min(actualExtent.width, capabilities.maxImageExtent.width));
        actualExtent.height = std::max(
            capabilities.minImageExtent.height,
            std::min(actualExtent.height, capabilities.maxImageExtent.height));

        return actualExtent;
    }
}

SwapChainSupportDetails
OceanApplication::query_swap_chain_support(VkPhysicalDevice device)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                              &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                         nullptr);

    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                             details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
                                              &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

bool OceanApplication::is_device_suitable(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = find_queue_families(device);

    bool extensionsSupported = check_device_extension_support(device);

    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
        SwapChainSupportDetails swapChainSupport =
            query_swap_chain_support(device);
        swapChainAdequate = !swapChainSupport.formats.empty()
            && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

bool OceanApplication::check_device_extension_support(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties pProperties;
    vkGetPhysicalDeviceProperties(device, &pProperties);

    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                         nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                         availableExtensions.data());

    auto extensions = get_required_dev_exts();
    std::set<std::string> requiredExtensions(extensions.begin(),
                                             extensions.end());

    for (const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

QueueFamilyIndices
OceanApplication::find_queue_families(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                             nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                             queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface,
                                             &presentSupport);

        if (presentSupport)
        {
            indices.presentFamily = i;
        }

        if (indices.isComplete())
        {
            break;
        }

        i++;
    }

    return indices;
}

std::vector<const char*> OceanApplication::get_required_exts()
{
    std::vector<const char*> extensions =
        sf::Vulkan::getGraphicsRequiredInstanceExtensions();

    if (app_opts.use_external_memory)
    {
        extensions.push_back(
            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    }
    if (app_opts.use_external_memory)
    {
        extensions.push_back(
            VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
    }
    if (app_opts.validationLayersOn)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

std::vector<const char*> OceanApplication::get_required_dev_exts()
{
    std::vector<const char*> extensions(deviceExtensions);

    if (app_opts.use_external_memory)
    {
        extensions.push_back(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
#ifdef _WIN32
        extensions.push_back(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME);
#elif defined(__linux__)
        extensions.push_back(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
#endif
    }

    return extensions;
}

bool OceanApplication::check_validation_layer_support()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers)
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            return false;
        }
    }

    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL OceanApplication::debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    fprintf(stderr, "validation layer: %s\n", pCallbackData->pMessage);

    return VK_FALSE;
}
