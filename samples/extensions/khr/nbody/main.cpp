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
#include <CL/Utils/Utils.hpp>
#include <CL/SDK/SDK.hpp>

// STL includes
#include <iostream>
#include <valarray>
#include <random>
#include <algorithm>
#include <fstream>
#include <tuple> // std::make_tuple

// TCLAP includes
#include <tclap/CmdLine.h>

// OpenGL includes
#include <SFML/OpenGL.hpp>

// GLM includes
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

template <typename T> struct DoubleBuffer
{
    T front, back;

    void swap() { std::swap(front, back); }
};

class NBody : public cl::sdk::InteropWindow {
public:
    explicit NBody(unsigned int platform_id = 0, unsigned int device_id = 0,
                   cl_bitfield device_type = CL_DEVICE_TYPE_DEFAULT)
        : InteropWindow{ sf::VideoMode(800, 800),
                         "Gravitational NBody",
                         sf::Style::Default,
                         sf::ContextSettings{
                             32, 0, 0, // Depth, Stencil, AA
                             3, 3, // OpenGL version
                             sf::ContextSettings::Attribute::Core },
                         platform_id,
                         device_id,
                         device_type },
          particle_count(8192), x_abs_range(192.f), y_abs_range(128.f),
          z_abs_range(32.f), mass_min(100.f), mass_max(500.f),
          RMB_pressed(false),
          dist(std::max({ x_abs_range, y_abs_range, z_abs_range }) * 3), phi(0),
          theta(0), needMatrixReset(true), animating(true)
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
    // Simulation related variables
    std::size_t particle_count;
    float x_abs_range, y_abs_range, z_abs_range, mass_min, mass_max;

    // Host-side containers
    std::vector<cl_float4> pos_mass;
    std::vector<cl_float3> velocity;
    std::vector<cl_float3> forces;

    // OpenCL objects
    cl::Device device;
    cl::CommandQueue queue;
    cl::Program cl_program;
    cl::Kernel kernel;
    cl::Sampler sampler;

    cl::Buffer velocity_buffer;
    DoubleBuffer<cl::BufferGL> cl_pos_mass;

    cl::vector<cl::Memory> interop_resources;
    cl::vector<cl::Event> acquire_wait_list, release_wait_list;
    cl::NDRange gws, lws; // Global/local work-sizes
    cl::Kernel step_kernel; // Kernel

    // OpenGL objects
    cl_GLuint vertex_shader, fragment_shader, gl_program;
    DoubleBuffer<cl_GLuint> vertex_array;
    DoubleBuffer<cl_GLuint> gl_pos_mass;

    bool RMB_pressed; // Variables to enable dragging
    sf::Vector2<int> mousePos; // Variables to enable dragging
    float dist, phi, theta; // Mouse polar coordinates
    bool needMatrixReset; // Whether matrices need to be reset in shaders
    bool animating;

    void
    mouseDrag(const sf::Event::MouseMoveEvent& event); // Handle mouse dragging
    void mouseWheel(
        const sf::Event::MouseWheelEvent& event); // Handle mouse wheel movement

    void setMatrices(); // Update shader matrices
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
        std::cerr << "OpenGL Error(" << ErrorString << "): " << Title
                  << std::endl;
    }
    return Error == GL_NO_ERROR;
}

void NBody::initializeGL()
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

    vertex_shader = create_shader("./nbody.vert.glsl", GL_VERTEX_SHADER);
    fragment_shader = create_shader("./nbody.frag.glsl", GL_FRAGMENT_SHADER);
    gl_program = create_program({ vertex_shader, fragment_shader });

    using uni = std::uniform_real_distribution<float>;
    std::generate_n(std::back_inserter(pos_mass), particle_count,
                    [prng = std::default_random_engine(),
                     x_dist = uni(-x_abs_range, x_abs_range),
                     y_dist = uni(-y_abs_range, y_abs_range),
                     z_dist = uni(-z_abs_range, z_abs_range),
                     m_dist = uni(mass_min, mass_max)]() mutable {
                        return cl_float4{ x_dist(prng), y_dist(prng),
                                          z_dist(prng), m_dist(prng) };
                    });

    glUseProgram(gl_program);
    checkError("glUseProgram(gl_program)");
    for (auto vbo_vao :
         { std::make_pair(&gl_pos_mass.front, &vertex_array.front),
           std::make_pair(&gl_pos_mass.back, &vertex_array.back) })
    {
        glGenBuffers(1, vbo_vao.first);
        checkError("glGenBuffers(1, &vertex_buffer)");
        glBindBuffer(GL_ARRAY_BUFFER, *vbo_vao.first);
        checkError("glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer)");
        glBufferData(GL_ARRAY_BUFFER, pos_mass.size() * sizeof(cl_float4),
                     pos_mass.data(), GL_STATIC_DRAW);
        checkError("glBufferData(GL_ARRAY_BUFFER, quad.size() * sizeof(float), "
                   "quad.data(), GL_STATIC_DRAW)");
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        checkError("glBindBuffer(GL_ARRAY_BUFFER, 0)");

        glGenVertexArrays(1, vbo_vao.second);
        checkError("glGenVertexArrays(1, &vertex_array)");
        glBindVertexArray(*vbo_vao.second);
        checkError("glBindVertexArray(vertex_array)");
        glBindBuffer(GL_ARRAY_BUFFER, *vbo_vao.first);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(cl_float4),
                              (GLvoid*)(NULL));
        checkError("glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, "
                   "sizeof(cl_float4), (GLvoid *)(NULL))");
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(cl_float4),
                              (GLvoid*)(0 + 3 * sizeof(float)));
        checkError("glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, "
                   "sizeof(cl_float4), (GLvoid *)(0 + 3 * sizeof(float)))");
        glEnableVertexAttribArray(0);
        checkError("glEnableVertexAttribArray(0)");
        glEnableVertexAttribArray(1);
        checkError("glEnableVertexAttribArray(1)");
        glBindVertexArray(0);
        checkError("glBindVertexArray(0)");
    }

    glViewport(0, 0, getSize().x, getSize().y);
    checkError("glViewport(0, 0, getSize().x, getSize().y)");
    glClearColor(0.f, 0.f, 0.f, 1.f);
    checkError("glClearColor(0.f, 0.f, 0.f, 0.f)");
    glEnable(GL_DEPTH_TEST);
    checkError("glDisable(GL_DEPTH_TEST)");
    glDepthFunc(GL_LESS);
    checkError("glDepthFunc(GL_LESS)");
    glDisable(GL_CULL_FACE);
    checkError("glDisable(GL_CULL_FACE)");
    glPointSize(1.5f);
}

void NBody::initializeCL()
{
    device = opencl_context.getInfo<CL_CONTEXT_DEVICES>().at(0);
    queue = cl::CommandQueue{ opencl_context, device };

    // Compile kernel
    const char* kernel_location = "./nbody.cl";
    std::ifstream kernel_stream{ kernel_location };
    if (!kernel_stream.is_open())
        throw std::runtime_error{ std::string{ "Cannot open kernel source: " }
                                  + kernel_location };

    cl_program = cl::Program{ opencl_context,
                              std::string{ std::istreambuf_iterator<char>{
                                               kernel_stream },
                                           std::istreambuf_iterator<char>{} } };
    cl_program.build(device);
    kernel = cl::Kernel{ cl_program, "nbody" };

    gws = cl::NDRange{ particle_count };
    lws = cl::NullRange;

    // velocity = std::vector<cl_float4>(particle_count, cl_float4{ 0, 0, 0, 0
    // });
    velocity_buffer = cl::Buffer{ opencl_context, CL_MEM_READ_WRITE,
                                  particle_count * sizeof(cl_float3), nullptr };
    queue.enqueueFillBuffer(velocity_buffer, cl_float4{ 0, 0, 0, 0 }, 0,
                            particle_count * sizeof(cl_float4));
    queue.finish();

    // Translate OpenGL object handles into OpenCL handles
    cl_pos_mass.front =
        cl::BufferGL{ opencl_context, CL_MEM_READ_WRITE, gl_pos_mass.front };
    cl_pos_mass.back =
        cl::BufferGL{ opencl_context, CL_MEM_READ_WRITE, gl_pos_mass.back };

    // Translate
    interop_resources =
        cl::vector<cl::Memory>{ cl_pos_mass.front, cl_pos_mass.back };
}

void NBody::updateScene()
{
    if (animating)
    {
        auto nbody =
            cl::KernelFunctor<cl::BufferGL, cl::BufferGL, cl::Buffer, cl_uint,
                              cl_float>{ cl_program, "nbody" };
        cl::Event acquire, release;

        queue.enqueueAcquireGLObjects(&interop_resources, nullptr, &acquire);

        nbody(cl::EnqueueArgs{ queue, cl::NDRange{ particle_count } },
              cl_pos_mass.front, cl_pos_mass.back, velocity_buffer,
              static_cast<cl_uint>(particle_count), 0.0001f);

        queue.enqueueReleaseGLObjects(&interop_resources, nullptr, &release);

        // Wait for all OpenCL commands to finish
        if (!cl_khr_gl_event_supported)
            cl::finish();
        else
            release.wait();

        // Swap front and back buffer handles
        cl_pos_mass.swap();
        gl_pos_mass.swap();
    }
}

void NBody::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    checkError("glClear(GL_COLOR_BUFFER_BIT)");

    glUseProgram(gl_program);
    checkError("glUseProgram(gl_program)");
    glBindVertexArray(vertex_array.front);
    checkError("glBindVertexArray(vertex_array)");
    glBindBuffer(GL_ARRAY_BUFFER, gl_pos_mass.front);
    checkError("glBindBuffer(GL_ARRAY_BUFFER, gl_pos_mass.front)");

    if (needMatrixReset) setMatrices();

    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(particle_count));
    checkError(
        "glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(particle_count))");

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    checkError("glBindBuffer(GL_ARRAY_BUFFER, 0)");
    glBindVertexArray(0);
    checkError("glBindVertexArray(0)");

    // Wait for all drawing commands to finish
    if (!cl_khr_gl_event_supported)
        glFinish();
    else
        glFlush();
    checkError("glFlush()/glFinish()");
}

void NBody::event(const sf::Event& event)
{
    switch (event.type)
    {
        case sf::Event::Closed: close(); break;
        case sf::Event::Resized:
            glViewport(0, 0, getSize().x, getSize().y);
            checkError("glViewport(0, 0, getSize().x, getSize().y)");
            needMatrixReset = true; // projection matrix need to be recalculated
            break;
        case sf::Event::KeyPressed:
            if (event.key.code == sf::Keyboard::Key::Space)
            {
                animating = !animating;
            }
            break;
        case sf::Event::MouseButtonPressed:
            if (event.mouseButton.button == sf::Mouse::Button::Right)
            {
                RMB_pressed = true;
                mousePos =
                    sf::Vector2i{ event.mouseButton.x, event.mouseButton.y };
            }
            break;
        case sf::Event::MouseButtonReleased:
            if (event.mouseButton.button == sf::Mouse::Button::Right)
                RMB_pressed = false;
            break;
        case sf::Event::MouseMoved:
            if (RMB_pressed) mouseDrag(event.mouseMove);
            break;
        case sf::Event::MouseWheelMoved:
            dist += (float)sqrt(pow((double)x_abs_range, 2)
                                + pow((double)y_abs_range, 2))
                * 1.1f * event.mouseWheel.delta * (-0.2f);
            dist = abs(dist);
            needMatrixReset = true; // view matrix need to be recalculated
            break;
    }
}

void NBody::mouseDrag(const sf::Event::MouseMoveEvent& event)
{
    if (sf::Vector2i{ event.x, event.y } != mousePos)
    {
        phi += 0.01f * (event.x - mousePos.x);
        theta += 0.01f * (event.y - mousePos.y);

        needMatrixReset = true;
    }
    mousePos = sf::Vector2i{ event.x, event.y };
}

void NBody::setMatrices()
{
    // Set shader variables
    const float fov = 45.f;

    // Set camera to view the origo from the z-axis with up along the y-axis
    // and distance so the entire sim space is visible with given field-of-view
    glm::vec3 vecTarget{ 0, 0, 0 };
    glm::vec3 vecUp{ 0, 1, 0 };
    glm::vec3 vecEye = vecTarget + glm::vec3{ 0, 0, dist };

    glm::mat4 matWorld = glm::rotate(
        // glm::identity<glm::mat4>() is GLM 0.9.9.1 feature and Ubuntu 18.04
        // ships 0.9.9.0
        glm::rotate(glm::mat4(1), theta,
                    glm::vec3{ 1, 0, 0 }), // theta rotates around z-axis
        phi, glm::vec3{ 0, 0, 1 } // theta rotates around z-axis
    );

    glm::mat4 matView = glm::lookAt(vecEye, vecTarget, vecUp);

    glm::mat4 matProj = glm::perspective<float>(
        fov, static_cast<float>(getSize().x) / getSize().y, 0.001f, 1000000.0f);

    auto loc_MVP = glGetUniformLocation(gl_program, "mat_MVP");
    checkError("glGetUniformLocation(gl_program, \"mat_MVP\");");
    glUniformMatrix4fv(loc_MVP, 1, GL_FALSE,
                       glm::value_ptr(matProj * matView * matWorld));
    checkError("glUniformMatrix4fv(loc_MVP, 1, GL_FALSE, "
               "glm::value_ptr(matProj * matView * matWorld));");

    needMatrixReset = false;
}

int main(int argc, char* argv[])
{
    try
    {
        // Parse command-line options
        auto opts =
            cl::sdk::parse_cli<cl::sdk::options::Diagnostic,
                               cl::sdk::options::SingleDevice>(argc, argv);
        const auto& dev_opts = std::get<1>(opts).triplet;

        NBody window{ dev_opts.plat_index, dev_opts.dev_index,
                      dev_opts.dev_type };

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
