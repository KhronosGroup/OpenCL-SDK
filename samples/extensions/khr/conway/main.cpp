/*
 * Copyright (c) 2020 The Khronos Group Inc.
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

#include <GL/glew.h>

// OpenCL SDK includes
#include <CL/Utils/Context.hpp>
#include <CL/Utils/InteropWindow.hpp>
#include <CL/SDK/Context.hpp>
#include <CL/SDK/Options.hpp>
#include <CL/SDK/CLI.hpp>
#include <CL/SDK/Random.hpp>

// STL includes
#include <iostream>
#include <valarray>
#include <random>
#include <algorithm>
#include <fstream>
#include <tuple>        // std::make_tuple

// TCLAP includes
#include <tclap/CmdLine.h>

// OpenGL includes
#include <SFML/OpenGL.hpp>

template <typename T>
struct DoubleBuffer
{
    T front, back;

    void swap()
    {
        std::swap(front, back);
    }
};

class Conway : public cl::util::InteropWindow
{
public:
    explicit Conway(
        int platform_id = 0,
        int device_id = 0,
        cl_bitfield device_type = CL_DEVICE_TYPE_DEFAULT
    ) : InteropWindow{
            sf::VideoMode::VideoMode(800,800),
            "Conway's Game of Life",
            sf::Style::Default,
            sf::ContextSettings{
                0, 0, 0, // Depth, Stencil, AA
                3, 3,    // OpenGL version
                sf::ContextSettings::Attribute::Core
            },
            platform_id,
            device_id,
            device_type
        }
    {}

protected:
    virtual void initializeGL() override;            // Function that initializes all OpenGL assets needed to draw a scene
    virtual void initializeCL() override;            // Function that initializes all OpenCL assets needed to draw a scene
    virtual void updateScene() override;             // Function that holds scene update guaranteed not to conflict with drawing
    virtual void render() override;                  // Function that does the native rendering
    virtual void event(const sf::Event& e) override; // Function that handles render area resize

private:
    // OpenGL objects
    DoubleBuffer<cl_GLuint> gl_images;
    cl_GLuint vertex_shader, fragment_shader, shader_program;
    cl_GLuint vertex_buffer, vertex_array;

    // OpenCL objects
    cl::Device device;
    cl::CommandQueue queue;
    cl::Program program;
    cl::Kernel kernel;

    DoubleBuffer<cl::ImageGL> cl_images;
    cl::vector<cl::Memory> interop_resources;
};

void Conway::initializeGL()
{
    if (glewInit() != GLEW_OK) std::exit(EXIT_FAILURE);

    auto create_shader = [](std::string file_path, cl_GLenum shader_stage)
    {
        std::ifstream shader_stream(file_path);
        std::string shader_string{ std::istreambuf_iterator<GLchar>{ shader_stream },
                                   std::istreambuf_iterator<GLchar>{} };
        auto pshader_string = shader_string.c_str();
        GLuint shader = glCreateShader(shader_stage);
        glShaderSource(shader, 1, &pshader_string, NULL);
        glCompileShader(shader);

        GLint status = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if(status != GL_TRUE)
        {
            int log_length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
            std::vector<GLchar> shader_log(log_length);
            glGetShaderInfoLog(shader, log_length, NULL, shader_log.data());
            std::cerr << std::string(shader_log.cbegin(), shader_log.cend()) << std::endl;
        }

        return shader;
    };
    auto create_program = [](std::initializer_list<GLuint> shader_stages)
    {
        GLuint program = glCreateProgram();
        for (auto shader_stage : shader_stages)
            glAttachShader(program, shader_stage);

        glLinkProgram(program);
        GLint status = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if(status != GL_TRUE)
        {
            int log_length = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
            std::vector<GLchar> program_log(log_length);
            glGetProgramInfoLog(program, log_length, NULL, program_log.data());
            std::cerr << std::string(program_log.cbegin(), program_log.cend()) << std::endl;
        }

        return program;
    };

    vertex_shader   = create_shader("./conway.vert.glsl", GL_VERTEX_SHADER);
    fragment_shader = create_shader("./conway.frag.glsl", GL_FRAGMENT_SHADER);
    shader_program = create_program({ vertex_shader, fragment_shader });

    std::vector<float> quad =
        //  vertices  , tex coords
        //  x  ,   y  ,  u  ,   v
        { -1.0f, -1.0f, 0.0f, 0.0f,
          -1.0f,  1.0f, 0.0f, 1.0f,
           1.0f, -1.0f, 1.0f, 0.0f,
           1.0f,  1.0f, 1.0f, 1.0f };

    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, quad.size() * sizeof(float), quad.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (GLvoid *)(NULL));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (GLvoid *)(NULL + 2 * sizeof(float)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    
}

void Conway::initializeCL()
{
    device = opencl_context.getInfo<CL_CONTEXT_DEVICES>().at(0);

    // Compile kernel
    const char* kernel_location = "./conway.cl";
    std::ifstream kernel_stream{ kernel_location };
    if (!kernel_stream.is_open())
        throw std::runtime_error{ std::string{ "Cannot open kernel source: " } + kernel_location };

    program = cl::Program{ opencl_context,
                           std::string{ std::istreambuf_iterator<char>{ kernel_stream },
                                        std::istreambuf_iterator<char>{} } };
    program.build( device );
    kernel = cl::Kernel{program, "conway"};

    // Translate OpenGL object handles into OpenCL handles
    cl_images.front = cl::ImageGL{opencl_context, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, gl_images.front};
    cl_images.back  = cl::ImageGL{opencl_context, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, gl_images.back};

    // Translate
    interop_resources = cl::vector<cl::Memory>{ cl_images.front, cl_images.back };
}

void Conway::updateScene()
{
}

void Conway::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Conway::event(const sf::Event&) {}

int main(int argc, char* argv[])
{
    try
    {
        // Parse command-line options
        auto opts = cl::sdk::parse_cli<
                        cl::sdk::options::Diagnostic,
                        cl::sdk::options::SingleDevice>(argc, argv);
        const auto& dev_opts   = std::get<1>(opts).triplet;

        Conway window{
            dev_opts.plat_index,
            dev_opts.dev_index,
            dev_opts.dev_type
        };

        window.run();
    }
    catch(cl::util::Error& e)
    {
        std::cerr << "OpenCL Utils error: " << e.what() << std::endl;
        std::exit(e.err());
    }
    catch(cl::BuildError& e)
    {
        std::cerr << "OpenCL runtime error: " << e.what() << std::endl;
        for (auto& build_log : e.getBuildLog())
        {
            std::cerr << "\tBuild log for device: " << build_log.first.getInfo<CL_DEVICE_NAME>() << "\n" << std::endl;
            std::cerr << build_log.second << "\n" << std::endl;
        }
        std::exit(e.err());
    }
    catch(cl::Error& e)
    {
        std::cerr << "OpenCL rutnime error: " << e.what() << std::endl;
        std::exit(e.err());
    }
    catch(std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    return 0;
}