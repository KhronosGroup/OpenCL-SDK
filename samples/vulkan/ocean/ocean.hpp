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

#ifndef OCEAN_HPP
#define OCEAN_HPP

#include "ocean_util.hpp"
#include <CL/SDK/CLI.hpp>

class OceanApplication {

public:
    void run();
    void keyboard(int key, int scancode, int action, int mods);
    void mouse_event(int button, int action, int mods);
    void mouse_pos(double pX, double pY);
    void mouse_roll(double offset_x, double offset_y);

public:
    cl::sdk::options::SingleDevice dev_opts;
    CliOptions app_opts;

private:
    GLFWwindow* window = nullptr;
    Camera camera;
    std::string app_name = "Ocean Surface Simulation";

    // ocean texture size - assume uniform x/y
    size_t ocean_tex_size = 512;

    // used to specify local work size
    size_t group_size = 16;

    // mesh patch size - assume uniform x/y
    size_t ocean_grid_size = 256;

    // mesh patch spacing
    float mesh_spacing = 2.f;

    bool animate = true;
    bool show_fps = true;

    // ocean parameters changed - rebuild initial spectrum resources
    bool changed = true;
    bool twiddle_factors_init = true;

    // ocean in-factors
    float wind_magnitude = 30.f;
    float wind_angle = 45.f;
    float choppiness = 10.f;
    float alt_scale = 20.f;

    float amplitude = 80.f;
    float supress_factor = 0.1f;

    // env factors
    int sun_elevation = 0;
    int sun_azimuth = 90;
    bool wireframe_mode = false;

    std::chrono::system_clock::time_point start =
        std::chrono::system_clock::now();

    std::chrono::system_clock::time_point fps_last_time =
        std::chrono::system_clock::now();
    int delta_frames = 0;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkSurfaceKHR surface;

    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device;

    VkQueue graphics_queue;
    VkQueue present_queue;

    VkSwapchainKHR swap_chain;
    std::vector<VkImage> swap_chain_images;
    VkFormat swap_chain_image_format;
    VkExtent2D swap_chain_extent;
    std::vector<VkImageView> swap_chain_image_views;
    std::vector<VkFramebuffer> swap_chain_framebuffers;

    VkImage depth_image;
    VkDeviceMemory depth_image_memory;
    VkImageView depth_image_view;

    VkRenderPass render_pass;
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;
    VkPipeline wireframe_pipeline;

    VkCommandPool command_pool;

    VkBuffer staging_tex_buffer;
    VkDeviceMemory staging_tex_buffer_memory;

    // Only displacement and normal map images must be shared between OCL and
    // Vulkan
    enum InteropTexType
    {
        IOPT_DISPLACEMENT = 0,
        IOPT_NORMAL_MAP,
        IOPT_COUNT
    };

    struct TextureOCL
    {
        std::vector<VkImage> images;
        std::vector<VkDeviceMemory> image_memories;
        std::vector<VkImageView> image_views;
    };

    // vulkan-opencl interop resources
    std::array<TextureOCL, IOPT_COUNT> texture_images;

    // Ocean grid vertices and related buffers
    std::vector<Vertex> ocean_grid_vertices;
    std::vector<VkBuffer> vertex_buffers;
    std::vector<VkDeviceMemory> vertex_buffer_memories;

    std::vector<std::uint32_t> ocean_grid_indices;
    // separate index buffer for [ocean_grid_size] triangle strips
    struct IndexBuffer
    {
        std::vector<VkBuffer> buffers;
        std::vector<VkDeviceMemory> buffer_memories;
    };
    IndexBuffer index_buffer;

    std::array<VkSampler, IOPT_COUNT> texture_sampler;

    VkDescriptorPool descriptor_pool;
    std::vector<VkDescriptorSet> descriptor_sets;

    std::vector<VkCommandBuffer> command_buffers;

    std::vector<VkSemaphore> image_available_semaphores;
    std::vector<VkSemaphore> render_finished_semaphores;
    std::vector<VkFence> in_flight_fences;
    std::vector<VkFence> images_in_flight;
    size_t current_frame = 0;

#ifdef _WIN32
    using PFN_vkGetMemoryWin32HandleKHR = VkResult(VKAPI_PTR*)(
        VkDevice, const VkMemoryGetWin32HandleInfoKHR*, HANDLE*);
    PFN_vkGetMemoryWin32HandleKHR vkGetMemoryWin32HandleKHR = NULL;
#elif defined(__linux__)
    PFN_vkGetMemoryFdKHR vkGetMemoryFdKHR = NULL;
#endif

    std::vector<VkBuffer> uniform_buffers;
    std::vector<VkDeviceMemory> uniform_buffers_memory;

    // more OpenCL resources
    cl_external_memory_handle_type_khr external_mem_type = 0;

    cl::Context context;
    cl::CommandQueue command_queue;

    // generates twiddle factors kernel
    cl::Kernel twiddle_kernel;

    // initial spectrum kernel
    cl::Kernel init_spectrum_kernel;

    // Fourier components image kernel
    cl::Kernel time_spectrum_kernel;

    // FFT kernel
    cl::Kernel fft_kernel;

    // inversion kernel
    cl::Kernel inversion_kernel;

    // building normals kernel
    cl::Kernel normals_kernel;

    // FFT intermediate computation storages without vulkan iteroperability
    std::unique_ptr<cl::Image2D> dxyz_coef_mem[3];
    std::unique_ptr<cl::Image2D> hkt_pong_mem;
    std::unique_ptr<cl::Image2D> twiddle_factors_mem;
    std::unique_ptr<cl::Image2D> h0k_mem;
    std::unique_ptr<cl::Image2D> noise_mem;

    size_t ocl_max_img2d_width;
    cl_ulong ocl_max_alloc_size, ocl_mem_size;

    // main opencl-vulkan iteroperability resources
    // final computation result with displacements and normal map,
    // needs to follow swap-chain scheme
    std::array<std::vector<std::unique_ptr<cl::Image2D>>, IOPT_COUNT>
        ocl_image_mems;

    void init_window();
    void init_openCL();
    void init_openCL_mems();
    void init_vulkan();
    void main_loop();
    void cleanup();
    void create_instance();

    void populate_dbg_msger_create_info(
        VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void setup_dbg_msger();

    void create_surface();
    void pick_physical_device();
    void create_logical_device();
    void create_swap_chain();
    void create_swap_chain_image_views();
    void create_render_pass();
    void create_uniform_buffer();
    void create_descriptor_det_layout();
    void create_graphics_pipeline();
    void create_framebuffers();
    void create_command_pool();
    void create_vertex_buffers();
    void create_index_buffers();
    void create_texture_images();
    void create_texture_image_views();
    void create_texture_sampler();
    VkImageView create_image_view(VkImage image, VkFormat format,
                                  VkImageAspectFlags aspectFlags);

    void create_shareable_image(uint32_t width, uint32_t height,
                                VkFormat format, VkImageTiling tiling,
                                VkImageUsageFlags usage,
                                VkMemoryPropertyFlags properties,
                                VkImage& image, VkDeviceMemory& imageMemory,
                                VkImageType type = VK_IMAGE_TYPE_2D);

    void create_image(uint32_t width, uint32_t height, VkFormat format,
                      VkImageTiling tiling, VkImageUsageFlags usage,
                      VkMemoryPropertyFlags properties, VkImage& image,
                      VkDeviceMemory& imageMemory);
    VkFormat find_supported_format(const std::vector<VkFormat>& candidates,
                                   VkImageTiling tiling,
                                   VkFormatFeatureFlags features);

    VkFormat find_depth_format();

    bool has_stencil_component(VkFormat format);
    void create_depth_resources();

    void transition_image_layout(VkImage image, VkFormat format,
                                 VkImageLayout oldLayout,
                                 VkImageLayout newLayout, uint32_t layers = 1);

    void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width,
                              uint32_t height);

    void transition_uniform_layout(VkBuffer buffer, VkAccessFlagBits src,
                                   VkAccessFlagBits dst);

    void create_descriptor_pool();

    void create_descriptor_sets();
    void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                       VkMemoryPropertyFlags properties, VkBuffer& buffer,
                       VkDeviceMemory& bufferMemory);

    void copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    uint32_t find_memory_type(uint32_t typeFilter,
                              VkMemoryPropertyFlags properties);

    VkCommandBuffer begin_single_time_commands();

    void end_single_time_commands(VkCommandBuffer commandBuffer);
    void create_command_buffers();

    void create_sync_objects();
    void update_uniforms(uint32_t currentImage);

    void show_fps_window_title();
    void update_spectrum(uint32_t currentImage, float elapsed);
    void update_ocean(uint32_t currentImage);

    void draw_frame();

    void check_openCL_ext_mem_support(cl::Device& device);

    VkShaderModule create_shader_module(const std::vector<unsigned char>& code);

    VkSurfaceFormatKHR choose_swap_surf_format(
        const std::vector<VkSurfaceFormatKHR>& availableFormats);

    VkPresentModeKHR choose_swap_present_mode(
        const std::vector<VkPresentModeKHR>& availablePresentModes);

    VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);

    SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device);

    bool is_device_suitable(VkPhysicalDevice device);

    bool check_device_extension_support(VkPhysicalDevice device);

    QueueFamilyIndices find_queue_families(VkPhysicalDevice device);

    std::vector<const char*> get_required_exts();

    std::vector<const char*> get_required_dev_exts();

    bool check_validation_layer_support();

    static VKAPI_ATTR VkBool32 VKAPI_CALL
    debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                   VkDebugUtilsMessageTypeFlagsEXT messageType,
                   const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                   void* pUserData);
};

#endif // OCEAN_HPP
