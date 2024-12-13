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

#ifndef OCEAN_HPP
#define OCEAN_HPP

#include "ocean_util.hpp"

#include <CL/SDK/CLI.hpp>
#include <CL/Utils/Utils.hpp>
#include <CL/SDK/SDK.hpp>

// OpenGL includes
#include <SFML/OpenGL.hpp>

class OceanApplication : public cl::sdk::InteropWindow {
public:
    explicit OceanApplication(unsigned int platform_id = 0,
                              unsigned int device_id = 0,
                              cl_bitfield device_type = CL_DEVICE_TYPE_DEFAULT)
        : InteropWindow{ sf::VideoMode(800, 800),
                         "Ocean waves with OpenGL and OpenCL interop",
                         sf::Style::Default,
                         sf::ContextSettings{
                             24, 0, 0, // Depth, Stencil, AA
                             3, 3, // OpenGL version
                             sf::ContextSettings::Attribute::Core },
                         platform_id,
                         device_id,
                         device_type }
    {}

    ~OceanApplication() { cleanup(); }

protected:
    virtual void
    initializeGL() override; // Function that initializes all OpenGL assets
                             // needed to draw a scene
    virtual void
    initializeCL() override; // Function that initializes all OpenCL assets
                             // needed to draw a scene
    virtual void
    updateScene() override; // Function that holds scene update guaranteed not
                            // to conflict with drawing
    virtual void render() override; // Function that does the native rendering
    virtual void event(const sf::Event& e)
        override; // Function that handles render area resize

    void create_texture_images();
    void create_vertex_buffer();
    void create_index_buffer();
    void create_uniform_buffer();

    void update_uniforms();
    void update_spectrum(float elapsed);

    void show_fps_window_title();

    void cleanup();

private:
    // OpenGL objects
    cl_GLuint gl_program;

public:
    void keyboard(int key);
    void mouseDrag(const int x, const int y);

public:
    cl::sdk::options::SingleDevice dev_opts;
    bool use_cl_khr_gl_sharing = true;

private:
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

    size_t window_width = 1024;
    size_t window_height = 1024;

    std::chrono::system_clock::time_point start =
        std::chrono::system_clock::now();

    std::chrono::system_clock::time_point fps_last_time =
        std::chrono::system_clock::now();
    int delta_frames = 0;

    // Only displacement and normal map images must be shared
    enum InteropTexType
    {
        IOPT_DISPLACEMENT = 0,
        IOPT_NORMAL_MAP,
        IOPT_COUNT
    };

    // vulkan-opencl interop resources
    std::array<GLuint, IOPT_COUNT> texture_images;

    struct Vertex
    {
        alignas(4) float pos[4];
        alignas(4) float tc[2];
    };

    // Ocean grid vertices and related buffers
    std::vector<Vertex> ocean_grid_vertices;
    cl_GLuint vertex_buffer;
    cl_GLuint vertex_array;

    std::vector<std::uint32_t> ocean_grid_indices;
    cl_GLuint index_buffer;

    cl_GLuint view_data_ubo;

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

    std::array<std::unique_ptr<cl::Image2D>, IOPT_COUNT> ocl_image_mems;
};

#endif // OCEAN_HPP
