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

#include <fstream>
#include <random>
#include <set>
#include <stdexcept>

#include <math.h>

// GLM includes
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "ocean.hpp"

void OceanApplication::initializeGL()
{
    if (glewInit() != GLEW_OK) std::exit(EXIT_FAILURE);

    window_width = getSize().x;
    window_height = getSize().y;

    auto create_shader = [](std::string file_path, cl_GLenum shader_stage) {
        std::string shader_string =
            cl::util::read_exe_relative_text_file(file_path.c_str());
        auto pshader_string = shader_string.c_str();
        GLuint shader = glCreateShader(shader_stage);
        glShaderSource(shader, 1, &pshader_string, NULL);
        checkError("glShaderSource(shader, 1, &pshader_string, NULL)");
        glCompileShader(shader);
        checkError("glCompileShader(shader)");

        GLint status = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        checkError("glGetShaderiv(shader, GL_COMPILE_STATUS, &status)");
        if (status != GL_TRUE)
        {
            int log_length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
            checkError(
                "glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length)");
            std::vector<GLchar> shader_log(log_length);
            glGetShaderInfoLog(shader, log_length, NULL, shader_log.data());
            checkError("glGetShaderInfoLog(shader, log_length, NULL, "
                       "shader_log.data())");
            std::cerr << std::string(shader_log.cbegin(), shader_log.cend())
                      << std::endl;
        }

        return shader;
    };
    auto create_program = [](std::initializer_list<GLuint> shader_stages) {
        GLuint program = glCreateProgram();
        checkError("glCreateProgram()");
        for (auto shader_stage : shader_stages)
        {
            glAttachShader(program, shader_stage);
            checkError("glAttachShader(program, shader_stage)");
        }

        glLinkProgram(program);
        checkError("glLinkProgram(program)");
        GLint status = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        checkError("glGetProgramiv(program, GL_LINK_STATUS, &status)");
        if (status != GL_TRUE)
        {
            int log_length = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
            checkError(
                "glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length)");
            std::vector<GLchar> program_log(log_length);
            glGetProgramInfoLog(program, log_length, NULL, program_log.data());
            checkError("glGetProgramInfoLog(program, log_length, NULL, "
                       "program_log.data())");
            std::cerr << std::string(program_log.cbegin(), program_log.cend())
                      << std::endl;
        }

        return program;
    };

    cl_GLuint vertex_shader =
        create_shader("ocean.vert.glsl", GL_VERTEX_SHADER);
    cl_GLuint fragment_shader =
        create_shader("ocean.frag.glsl", GL_FRAGMENT_SHADER);
    gl_program = create_program({ vertex_shader, fragment_shader });

    glUseProgram(gl_program);
    checkError("glUseProgram(gl_program)");

    create_texture_images();
    create_vertex_buffer();
    create_index_buffer();
    create_uniform_buffer();

    glViewport(0, 0, getSize().x, getSize().y);
    checkError("glViewport(0, 0, getSize().x, getSize().y)");
    glClearColor(0.f, 0.f, 0.f, 1.f);
    checkError("glClearColor(0.f, 0.f, 0.f, 0.f)");
    glEnable(GL_DEPTH_TEST);
    checkError("glEnable(GL_DEPTH_TEST)");
    glDepthFunc(GL_LESS);
    checkError("glDepthFunc(GL_LESS)");
    glDisable(GL_CULL_FACE);
    checkError("glDisable(GL_CULL_FACE)");
}

void OceanApplication::initializeCL()
{
    auto device = opencl_context.getInfo<CL_CONTEXT_DEVICES>().at(0);
    command_queue = cl::CommandQueue{ opencl_context, device };

    if (use_cl_khr_gl_sharing
        && cl::util::supports_extension(device, "cl_khr_gl_sharing"))
    {
        std::cout << "cl_khr_gl_sharing supported" << std::endl;
    }
    else
    {
        std::cout << "cl_khr_gl_sharing not supported" << std::endl;
        use_cl_khr_gl_sharing = false;
    }

    int error = CL_SUCCESS;
    error |= clGetDeviceInfo(device(), CL_DEVICE_IMAGE2D_MAX_WIDTH,
                             sizeof(ocl_max_img2d_width), &ocl_max_img2d_width,
                             NULL);
    error |=
        clGetDeviceInfo(device(), CL_DEVICE_MAX_MEM_ALLOC_SIZE,
                        sizeof(ocl_max_alloc_size), &ocl_max_alloc_size, NULL);
    error |= clGetDeviceInfo(device(), CL_DEVICE_GLOBAL_MEM_SIZE,
                             sizeof(ocl_mem_size), &ocl_mem_size, NULL);

    if (error != CL_SUCCESS) printf("clGetDeviceInfo error: %d\n", error);

    auto build_opencl_kernel = [&](const char* src_file, cl::Kernel& kernel,
                                   const char* name) {
        try
        {
            std::string kernel_code =
                cl::util::read_exe_relative_text_file(src_file);
            cl::Program program{ opencl_context, kernel_code };
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
                opencl_context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                cl::ImageFormat(CL_RGBA, CL_FLOAT), ocean_tex_size,
                ocean_tex_size, 0, phase_array.data());
        }

        hkt_pong_mem = std::make_unique<cl::Image2D>(
            opencl_context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RG, CL_FLOAT),
            ocean_tex_size, ocean_tex_size);

        dxyz_coef_mem[0] = std::make_unique<cl::Image2D>(
            opencl_context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RG, CL_FLOAT),
            ocean_tex_size, ocean_tex_size);

        dxyz_coef_mem[1] = std::make_unique<cl::Image2D>(
            opencl_context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RG, CL_FLOAT),
            ocean_tex_size, ocean_tex_size);

        dxyz_coef_mem[2] = std::make_unique<cl::Image2D>(
            opencl_context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RG, CL_FLOAT),
            ocean_tex_size, ocean_tex_size);

        h0k_mem = std::make_unique<cl::Image2D>(
            opencl_context, CL_MEM_READ_WRITE,
            cl::ImageFormat(CL_RGBA, CL_FLOAT), ocean_tex_size, ocean_tex_size);

        size_t log_2_N = (size_t)((log((float)ocean_tex_size) / log(2.f)) - 1);

        twiddle_factors_mem = std::make_unique<cl::Image2D>(
            opencl_context, CL_MEM_READ_WRITE,
            cl::ImageFormat(CL_RGBA, CL_FLOAT), log_2_N, ocean_tex_size);

        for (size_t target = 0; target < IOPT_COUNT; target++)
        {
            if (use_cl_khr_gl_sharing)
            {
                ocl_image_mems[target].reset(new cl::Image2D{
                    clCreateFromGLTexture(opencl_context(), CL_MEM_READ_WRITE,
                                          GL_TEXTURE_2D, 0,
                                          texture_images[target], NULL) });
            }
            else
            {
                ocl_image_mems[target].reset(
                    new cl::Image2D{ opencl_context, CL_MEM_READ_WRITE,
                                     cl::ImageFormat{ CL_RGBA, CL_FLOAT },
                                     ocean_tex_size, ocean_tex_size });
            }
        }
    } catch (const cl::Error& e)
    {
        printf("initOpenCLMems: OpenCL %s image error: %s\n", e.what(),
               IGetErrorString(e.err()));
        exit(1);
    }
}

void OceanApplication::updateScene()
{
    show_fps_window_title();

    update_uniforms();

    auto end = std::chrono::system_clock::now();

    // time factor of ocean animation
    static float elapsed = 0.f;

    if (animate)
    {
        std::chrono::duration<float> delta = end - start;
        elapsed = delta.count();
        update_spectrum(elapsed);
    }
    else
    {
        // hold the animation at the same time point
        std::chrono::duration<float> duration(elapsed);
        start =
            end - std::chrono::duration_cast<std::chrono::seconds>(duration);
        command_queue.finish();
    }
}

void OceanApplication::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    checkError("glClear(GL_COLOR_BUFFER_BIT)");

    glUseProgram(gl_program);
    checkError("glUseProgram(gl_program)");
    glBindVertexArray(vertex_array);
    checkError("glBindVertexArray(vertex_array)");
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    checkError("glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer)");
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
    checkError("glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,..)");

    for (size_t target = 0; target < IOPT_COUNT; target++)
    {
        glActiveTexture(GL_TEXTURE0 + target);
        checkError("glActiveTexture");
        glBindTexture(GL_TEXTURE_2D, texture_images[target]);
        checkError("glBindTexture");
    }

    if (wireframe_mode)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        checkError("glPolygonMode");
    }
    else
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        checkError("glPolygonMode");
    }

    glDrawElements(GL_TRIANGLE_STRIP, ocean_grid_indices.size(),
                   GL_UNSIGNED_INT, 0);
    checkError("glDrawElements");

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    checkError("glBindBuffer(GL_ARRAY_BUFFER, 0)");
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    checkError("glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0)");
    glBindVertexArray(0);
    checkError("glBindVertexArray(0)");

    // Wait for all drawing commands to finish
    if (!cl_khr_gl_event_supported)
        glFinish();
    else
        glFlush();
    checkError("glFlush()/glFinish()");
}

void OceanApplication::cleanup()
{
    if (vertex_buffer != 0)
    {
        glDeleteBuffers(1, &vertex_buffer);
        checkError("glDeleteBuffers");
        vertex_buffer = 0;
    }

    if (index_buffer != 0)
    {
        glDeleteBuffers(1, &index_buffer);
        checkError("glDeleteBuffers");
        index_buffer = 0;
    }

    if (vertex_array != 0)
    {
        glDeleteVertexArrays(1, &vertex_array);
        checkError("glBindVertexArray(0)");
        vertex_array = 0;
    }

    if (gl_program != 0)
    {
        glDeleteProgram(gl_program);
        checkError("glUseProgram(gl_program)");
        gl_program = 0;
    }

    for (size_t target = 0; target < texture_images.size(); target++)
    {
        glDeleteTextures(1, &texture_images[target]);
        checkError("glDeleteTextures");
    }
}

void OceanApplication::create_uniform_buffer()
{
    glGenBuffers(1, &view_data_ubo);
    checkError("glGenBuffers");
    glBindBuffer(GL_UNIFORM_BUFFER, view_data_ubo);
    checkError("glBindBuffer");
    glBufferData(GL_UNIFORM_BUFFER, sizeof(UniformBufferObject), NULL,
                 GL_DYNAMIC_DRAW);
    checkError("glBufferData");
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    checkError("glBindBuffer");
}

void OceanApplication::create_vertex_buffer()
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
        cl_float dfX = dfBaseX;
        for (int iX = 0; iX <= ocean_grid_size; iX++)
        {
            *((cl_float4*)&ocean_grid_vertices[iBase + iX].pos) = { dfX, dfY,
                                                                    0.f, 0.f };
            *((cl_float2*)&ocean_grid_vertices[iBase + iX].tc) = { tx, ty };
            tx += dtx;
            dfX += mesh_spacing;
        }
        dfY += mesh_spacing;
        ty += dty;
    }

    glGenBuffers(1, &vertex_buffer);
    checkError("glGenBuffers(1, &vertex_buffer)");
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    checkError("glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer)");
    glBufferData(GL_ARRAY_BUFFER, ocean_grid_vertices.size() * sizeof(Vertex),
                 ocean_grid_vertices.data(), GL_STATIC_DRAW);
    checkError("glBufferData(GL_ARRAY_BUFFER,..., GL_STATIC_DRAW)");

    glGenVertexArrays(1, &vertex_array);
    checkError("glGenVertexArrays(1, &vertex_array)");
    glBindVertexArray(vertex_array);
    checkError("glBindVertexArray(vertex_array)");

    const GLsizei vertSize = sizeof(Vertex);
    const GLsizei uvOffset = sizeof(Vertex::pos);

    // Setup shader attributes
    GLint attribPos = glGetAttribLocation(gl_program, "in_position");
    checkError("getAttribLocation");
    if (attribPos != -1)
    {
        glVertexAttribPointer(attribPos, 4, GL_FLOAT, GL_FALSE, vertSize,
                              nullptr);
        checkError("glVertexAttribPointer");
        glEnableVertexAttribArray(attribPos);
        checkError("glEnableVertexAttribArray");
    }
    else
        std::cerr << "Shader attribute not valid" << std::endl;

    GLint attribUV = glGetAttribLocation(gl_program, "in_tex_coords");
    checkError("getAttribLocation");
    if (attribUV != -1)
    {
        glVertexAttribPointer(attribUV, 2, GL_FLOAT, GL_FALSE, vertSize,
                              (const GLvoid*)uvOffset);
        checkError("glVertexAttribPointer");
        glEnableVertexAttribArray(attribUV);
        checkError("glEnableVertexAttribArray");
    }
    else
        std::cerr << "Shader attribute not valid" << std::endl;

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    checkError("glBindBuffer(GL_ARRAY_BUFFER, 0)");
    glBindVertexArray(0);
    checkError("glBindVertexArray(0)");
}

void OceanApplication::create_index_buffer()
{
    size_t totalIndices = ((ocean_grid_size + 1) * 2 + 1) * ocean_grid_size;
    ocean_grid_indices.resize(totalIndices);

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

    glEnable(GL_PRIMITIVE_RESTART);
    checkError("glEnable");

    glPrimitiveRestartIndex(-1);
    checkError("glPrimitiveRestartIndex(-1)");

    glGenBuffers(1, &index_buffer);
    checkError("glGenBuffers(1, &index_buffer)");

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
    checkError("glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,..)");

    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(ocean_grid_indices.front()) * ocean_grid_indices.size(),
                 ocean_grid_indices.data(), GL_STATIC_DRAW);
    checkError("glBufferData(GL_ELEMENT_ARRAY_BUFFER,..)");
}

void OceanApplication::create_texture_images()
{
    std::string uni_names[] = { "u_displacement_map", "u_normal_map" };
    for (size_t target = 0; target < texture_images.size(); target++)
    {
        texture_images[target] = 0;
        glGenTextures(1, &texture_images[target]);
        checkError("glGenTextures");
        glBindTexture(GL_TEXTURE_2D, texture_images[target]);
        checkError("glBindTexture");

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, (GLsizei)ocean_tex_size,
                     (GLsizei)ocean_tex_size, 0, GL_RGBA, GL_FLOAT, NULL);
        checkError("glTexImage2D");

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        checkError("glTexParameteri");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        checkError("glTexParameteri");

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        checkError("glTexParameteri");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        checkError("glTexParameteri");

        GLint uniformLocation =
            glGetUniformLocation(gl_program, uni_names[target].c_str());
        checkError("glGetUniformLocation");

        if (uniformLocation != -1)
        {
            glUniform1i(uniformLocation, target);
            checkError("glUniform1i");
        }
    }
}

void OceanApplication::update_uniforms()
{
    UniformBufferObject ubo = {};
    ubo.choppiness = choppiness;
    ubo.alt_scale = alt_scale;

    // update camera related uniform
    glm::mat4 view_matrix =
        glm::lookAt(camera.eye, camera.eye + camera.dir, camera.up);

    float fov = (float)glm::radians(60.0);
    float aspect = (float)window_width / window_height;
    glm::mat4 proj_matrix = glm::perspective(
        fov, aspect, 1.f, 2.f * ocean_grid_size * mesh_spacing);

    ubo.view_mat = view_matrix;
    ubo.proj_mat = proj_matrix;

    glBindBuffer(GL_UNIFORM_BUFFER, view_data_ubo);
    checkError("glBindBuffer");
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(UniformBufferObject), &ubo);
    checkError("glBufferSubData");
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    checkError("glBindBuffer");
    GLuint blockIndex = glGetUniformBlockIndex(gl_program, "ViewData");
    checkError("glGetUniformBlockIndex");
    glUniformBlockBinding(gl_program, blockIndex, 2);
    checkError("glUniformBlockBinding");
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, view_data_ubo);
    checkError("glBindBufferBase");
}

void OceanApplication::update_spectrum(float elapsed)
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
                opencl_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
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
            printf("twiddle indices: OpenCL %s kernel error: %s\n", e.what(),
                   IGetErrorString(e.err()));
            exit(1);
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
            printf("initial spectrum: OpenCL %s kernel error: %s\n", e.what(),
                   IGetErrorString(e.err()));
            exit(1);
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
        printf("updateSpectrum: OpenCL %s kernel error: %s\n", e.what(),
               IGetErrorString(e.err()));
        exit(1);
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

    if (use_cl_khr_gl_sharing)
    {
        for (size_t target = 0; target < texture_images.size(); target++)
            clEnqueueAcquireGLObjects(command_queue(), 1,
                                      &(*ocl_image_mems[target])(), 0, nullptr,
                                      nullptr);
    }

    // inversion
    {
        inversion_kernel.setArg(0, patch);
        inversion_kernel.setArg(1, *dxyz_coef_mem[0]);
        inversion_kernel.setArg(2, *dxyz_coef_mem[1]);
        inversion_kernel.setArg(3, *dxyz_coef_mem[2]);
        inversion_kernel.setArg(4, *ocl_image_mems[IOPT_DISPLACEMENT]);

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
        normals_kernel.setArg(3, *ocl_image_mems[IOPT_DISPLACEMENT]);
        normals_kernel.setArg(4, *ocl_image_mems[IOPT_NORMAL_MAP]);

        command_queue.enqueueNDRangeKernel(
            normals_kernel, cl::NullRange,
            cl::NDRange{ ocean_tex_size, ocean_tex_size }, lws);
    }

    if (use_cl_khr_gl_sharing)
    {
        if (cl_khr_gl_event_supported == false) command_queue.finish();
        for (size_t target = 0; target < texture_images.size(); target++)
            clEnqueueReleaseGLObjects(command_queue(), 1,
                                      &(*ocl_image_mems[target])(), 0, nullptr,
                                      nullptr);
    }
    else
    {
        for (size_t target = 0; target < texture_images.size(); target++)
        {
            size_t rowPitch = 0;
            void* pixels = command_queue.enqueueMapImage(
                *ocl_image_mems[target], CL_TRUE, CL_MAP_READ, { 0, 0, 0 },
                { ocean_tex_size, ocean_tex_size, 1 }, &rowPitch, nullptr);

            glBindTexture(GL_TEXTURE_2D, texture_images[target]);
            checkError("glBindTexture");

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, (GLsizei)ocean_tex_size,
                         (GLsizei)ocean_tex_size, 0, GL_RGBA, GL_FLOAT, pixels);

            command_queue.enqueueUnmapMemObject(*ocl_image_mems[target],
                                                pixels);
        }
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

            setTitle(ss.str().c_str());

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
