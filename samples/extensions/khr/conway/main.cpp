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
#include <CL/SDK/SDK.hpp>

// OpenCL Utils includes
#include <CL/Utils/Utils.hpp>

// STL includes
#include <iostream>
#include <valarray>
#include <random>
#include <algorithm>
#include <fstream>
#include <tuple> // std::make_tuple

// OpenGL includes
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

template <typename T> struct DoubleBuffer
{
    T front, back;

    void swap() { std::swap(front, back); }
};

class Conway : public cl::sdk::InteropWindow {
public:
    explicit Conway(int width, int height, bool fullscreen,
                    cl_uint platform_id = 0, cl_uint device_id = 0,
                    cl_bitfield device_type = CL_DEVICE_TYPE_DEFAULT)
        : InteropWindow{ sf::VideoMode(width, height),
                         "Conway's Game of Life",
                         fullscreen ? sf::Style::Fullscreen
                                    : sf::Style::Default,
                         sf::ContextSettings{
                             0, 0, 0, // Depth, Stencil, AA
                             3, 3, // OpenGL version
                             sf::ContextSettings::Attribute::Core },
                         platform_id,
                         device_id,
                         device_type },
          animating(true)
    {}

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

private:
    // OpenGL objects
    cl_GLuint vertex_shader, fragment_shader, gl_program;
    cl_GLuint vertex_buffer, vertex_array;
    DoubleBuffer<cl_GLuint> gl_images;

    // OpenCL objects
    cl::Device device;
    cl::CommandQueue queue;
    cl::Program cl_program;
    cl::Kernel kernel;
    cl::Sampler sampler;

    DoubleBuffer<cl::ImageGL> cl_images;
    cl::vector<cl::Memory> interop_resources;
    bool animating;
};

inline bool checkError(const char* Title)
{
    int Error;
    if ((Error = glGetError()) != GL_NO_ERROR)
    {
        std::string ErrorString;
        switch (Error)
        {
            case GL_INVALID_ENUM: ErrorString = "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE: ErrorString = "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:
                ErrorString = "GL_INVALID_OPERATION";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                ErrorString = "GL_INVALID_FRAMEBUFFER_OPERATION";
                break;
            case GL_OUT_OF_MEMORY: ErrorString = "GL_OUT_OF_MEMORY"; break;
            default: ErrorString = "UNKNOWN"; break;
        }
        std::cerr << "OpenGL Error(" << ErrorString.c_str() << "): " << Title
                  << std::endl;
    }
    return Error == GL_NO_ERROR;
}

void Conway::initializeGL()
{
    if (glewInit() != GLEW_OK) std::exit(EXIT_FAILURE);

    auto create_shader = [](std::string file_path, cl_GLenum shader_stage) {
        std::ifstream shader_stream(file_path);
        std::string shader_string{ std::istreambuf_iterator<GLchar>{
                                       shader_stream },
                                   std::istreambuf_iterator<GLchar>{} };
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

    vertex_shader = create_shader("./conway.vert.glsl", GL_VERTEX_SHADER);
    fragment_shader = create_shader("./conway.frag.glsl", GL_FRAGMENT_SHADER);
    gl_program = create_program({ vertex_shader, fragment_shader });

    std::vector<float> quad =
        //  vertices  , tex coords
        //  x  ,   y  ,  u  ,   v
        { -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f,
          1.0f,  -1.0f, 1.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f };

    glGenBuffers(1, &vertex_buffer);
    checkError("glGenBuffers(1, &vertex_buffer)");
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    checkError("glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer)");
    glBufferData(GL_ARRAY_BUFFER, quad.size() * sizeof(float), quad.data(),
                 GL_STATIC_DRAW);
    checkError("glBufferData(GL_ARRAY_BUFFER, quad.size() * sizeof(float), "
               "quad.data(), GL_STATIC_DRAW)");
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    checkError("glBindBuffer(GL_ARRAY_BUFFER, 0)");

    glGenVertexArrays(1, &vertex_array);
    checkError("glGenVertexArrays(1, &vertex_array)");
    glBindVertexArray(vertex_array);
    checkError("glBindVertexArray(vertex_array)");
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    checkError("glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer)");
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (GLvoid*)(NULL));
    checkError("glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, "
               "4*sizeof(float), (GLvoid *)(NULL))");
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (GLvoid*)(0 + 2 * sizeof(float)));
    checkError("glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, "
               "4*sizeof(float), (GLvoid *)(0 + 2 * sizeof(float)))");
    glEnableVertexAttribArray(0);
    checkError("glEnableVertexAttribArray(0)");
    glEnableVertexAttribArray(1);
    checkError("glEnableVertexAttribArray(1)");
    glBindVertexArray(0);
    checkError("glBindVertexArray(0)");

    std::vector<GLchar> texels;
    std::generate_n(
        std::back_inserter(texels), getSize().x * getSize().y,
        [prng = std::ranlux48{ std::random_device{}() },
         dist = std::uniform_int_distribution<std::uint16_t>{
             0, 1 }]() mutable { return static_cast<GLchar>(dist(prng)); });

    glUseProgram(gl_program);
    checkError("");
    for (auto image : { &gl_images.front, &gl_images.back })
    {
        glGenTextures(1, image);
        checkError("glGenTextures(1, image);");
        glActiveTexture(GL_TEXTURE0);
        checkError("glActiveTexture(GL_TEXTURE0);");
        glBindTexture(GL_TEXTURE_2D, *image);
        checkError("glBindTexture(GL_TEXTURE_2D, *image);");
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, getSize().x, getSize().y, 0,
                     GL_RED_INTEGER, GL_UNSIGNED_BYTE, texels.data());
        checkError(
            "glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, getSize().x, getSize().y, "
            "0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, texels.data())");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        checkError(
            "glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        checkError(
            "glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        checkError("glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, "
                   "GL_NEAREST);");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        checkError("glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, "
                   "GL_NEAREST);");
        glBindTexture(GL_TEXTURE_2D, 0);
        checkError("glBindTexture(GL_TEXTURE_2D, 0);");
    }
    glUseProgram(0);
    checkError("glUseProgram(0);");

    glViewport(0, 0, getSize().x, getSize().y);
    checkError("glViewport(0, 0, getSize().x, getSize().y)");
    glDisable(GL_DEPTH_TEST);
    checkError("glDisable(GL_DEPTH_TEST)");
}

void Conway::initializeCL()
{
    device = opencl_context.getInfo<CL_CONTEXT_DEVICES>().at(0);
    queue = cl::CommandQueue{ opencl_context, device };

    // Compile kernel
    const char* kernel_location = "./conway.cl";
    std::ifstream kernel_stream{ kernel_location };
    if (!kernel_stream.is_open())
        throw std::runtime_error{ std::string{ "Cannot open kernel source: " }
                                  + kernel_location };

    cl_program = cl::Program{ opencl_context,
                              std::string{ std::istreambuf_iterator<char>{
                                               kernel_stream },
                                           std::istreambuf_iterator<char>{} } };
    cl_program.build(device);
    kernel = cl::Kernel{ cl_program, "conway" };

    // Translate OpenGL object handles into OpenCL handles
    cl_images.front = cl::ImageGL{ opencl_context, CL_MEM_READ_WRITE,
                                   GL_TEXTURE_2D, 0, gl_images.front };
    cl_images.back = cl::ImageGL{ opencl_context, CL_MEM_READ_WRITE,
                                  GL_TEXTURE_2D, 0, gl_images.back };

    // Translate
    interop_resources =
        cl::vector<cl::Memory>{ cl_images.front, cl_images.back };
}

void Conway::updateScene()
{
    if (animating)
    {
        auto conway =
            cl::KernelFunctor<cl::ImageGL, cl::ImageGL, cl_float2>{ cl_program,
                                                                    "conway" };
        cl::Event acquire, release;

        queue.enqueueAcquireGLObjects(&interop_resources, nullptr, &acquire);

        conway(
            cl::EnqueueArgs{ queue, cl::NDRange{ getSize().x, getSize().y } },
            cl_images.front, cl_images.back,
            cl_float2{ 1.f / getSize().x, 1.f / getSize().y });

        queue.enqueueReleaseGLObjects(&interop_resources, nullptr, &release);

        // Wait for all OpenCL commands to finish
        if (!cl_khr_gl_event_supported)
            cl::finish();
        else
            release.wait();

        // Swap front and back buffer handles
        std::swap(cl_images.front, cl_images.back);
        std::swap(gl_images.front, gl_images.back);
    }
}

void Conway::render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(gl_program);
    glBindVertexArray(vertex_array);
    glBindTexture(GL_TEXTURE_2D, gl_images.front);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(4));

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);

    // Wait for all drawing commands to finish
    if (!cl_khr_gl_event_supported)
        glFinish();
    else
        glFlush();
}

void Conway::event(const sf::Event& event)
{
    switch (event.type)
    {
        case sf::Event::Closed: close(); break;
        case sf::Event::KeyPressed:
            if (event.key.code == sf::Keyboard::Key::Space)
            {
                animating = !animating;
            }
            break;
    }
}

int main(int argc, char* argv[])
{
    try
    {
        // Parse command-line options
        auto opts = cl::sdk::parse_cli<cl::sdk::options::Diagnostic,
                                       cl::sdk::options::SingleDevice,
                                       cl::sdk::options::Window>(argc, argv);
        const auto& dev_opts = std::get<1>(opts).triplet;
        const auto& win_opts = std::get<2>(opts);

        Conway window{ win_opts.width,      win_opts.height,
                       win_opts.fullscreen, dev_opts.plat_index,
                       dev_opts.dev_index,  dev_opts.dev_type };

        window.run();
    } catch (cl::util::Error& e)
    {
        std::cerr << "OpenCL Utils error: " << e.what() << std::endl;
        std::exit(e.err());
    } catch (cl::BuildError& e)
    {
        std::cerr << "OpenCL runtime error: " << e.what() << std::endl;
        for (auto& build_log : e.getBuildLog())
        {
            std::cerr << "\tBuild log for device: "
                      << build_log.first.getInfo<CL_DEVICE_NAME>() << "\n"
                      << std::endl;
            std::cerr << build_log.second << "\n" << std::endl;
        }
        std::exit(e.err());
    } catch (cl::Error& e)
    {
        std::cerr << "OpenCL rutnime error: " << e.what() << std::endl;
        std::exit(e.err());
    } catch (std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    return 0;
}
