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

#include <GL/glew.h>

#include <iostream>
#include <valarray>
#include <random>
#include <algorithm>
#include <fstream>
#include <tuple>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <tclap/CmdLine.h>

#include "ocean.hpp"

void OceanApplication::event(const sf::Event& event)
{
    switch (event.type)
    {
        case sf::Event::Closed: close(); break;
        case sf::Event::Resized:
            glViewport(0, 0, getSize().x, getSize().y);
            checkError("glViewport(0, 0, getSize().x, getSize().y)");
            break;
        case sf::Event::KeyPressed: keyboard(event.key.code); break;
        case sf::Event::MouseButtonPressed:
            if (event.mouseButton.button == sf::Mouse::Button::Left)
            {
                camera.drag = true;
                camera.begin =
                    glm::vec2(event.mouseButton.x, event.mouseButton.y);
            }
            break;
        case sf::Event::MouseButtonReleased:
            if (event.mouseButton.button == sf::Mouse::Button::Left)
                camera.drag = false;
            break;
        case sf::Event::MouseMoved:
            if (camera.drag) mouseDrag(event.mouseMove.x, event.mouseMove.y);
            break;
        case sf::Event::MouseWheelMoved:
            camera.eye +=
                camera.dir * (float)event.mouseWheel.delta * ROLL_SPEED_FAC;
            break;
        default: break;
    }
}

void OceanApplication::mouseDrag(const int x, const int y)
{
    if (!camera.drag) return;

    glm::vec2 off = camera.begin - glm::vec2(x, y);
    camera.begin = glm::vec2(x, y);

    camera.yaw -= off.x * DRAG_SPEED_FAC;
    camera.pitch += off.y * DRAG_SPEED_FAC;

    glm::quat yaw(glm::cos(glm::radians(camera.yaw / 2)),
                  glm::vec3(0, 0, 1) * glm::sin(glm::radians(camera.yaw / 2)));
    glm::quat pitch(glm::cos(glm::radians(camera.pitch / 2)),
                    glm::vec3(1, 0, 0)
                        * glm::sin(glm::radians(camera.pitch / 2)));
    glm::mat3 rot_mat(yaw * pitch);
    glm::vec3 dir = rot_mat * glm::vec3(0, 0, -1);

    camera.dir = glm::normalize(dir);
    camera.rvec = glm::normalize(glm::cross(camera.dir, glm::vec3(0, 0, 1)));
    camera.up = glm::normalize(glm::cross(camera.rvec, camera.dir));
}

void OceanApplication::keyboard(int key)
{
    switch (key)
    {
        case sf::Keyboard::Key::Escape: close(); break;
        case sf::Keyboard::Key::Space:
            animate = !animate;
            printf("animation is %s\n", animate ? "ON" : "OFF");
            break;

        case sf::Keyboard::Key::A:
            wind_magnitude += 1.f;
            changed = true;
            break;
        case sf::Keyboard::Key::Z:
            wind_magnitude -= 1.f;
            changed = true;
            break;

        case sf::Keyboard::Key::S:
            wind_angle += 1.f;
            changed = true;
            break;
        case sf::Keyboard::Key::X:
            wind_angle -= 1.f;
            changed = true;
            break;

        case sf::Keyboard::Key::D:
            amplitude += 0.5f;
            changed = true;
            break;
        case sf::Keyboard::Key::C:
            amplitude -= 0.5f;
            changed = true;
            break;

        case sf::Keyboard::Key::F: choppiness += 0.5f; break;
        case sf::Keyboard::Key::V: choppiness -= 0.5f; break;

        case sf::Keyboard::Key::G: alt_scale += 0.5f; break;
        case sf::Keyboard::Key::B: alt_scale -= 0.5f; break;

        case sf::Keyboard::Key::W: wireframe_mode = !wireframe_mode; break;

        case sf::Keyboard::Key::E: show_fps = !show_fps; break;
    }
}

template <> auto cl::sdk::parse<CliOptions>()
{
    return std::make_tuple(std::make_shared<TCLAP::ValueArg<bool>>(
        "", "useGLSharing", "Use cl_khr_gl_sharing", false, true, "boolean"));
}

template <>
CliOptions cl::sdk::comprehend<CliOptions>(
    std::shared_ptr<TCLAP::ValueArg<bool>> useGLSharing)
{
    return CliOptions{ useGLSharing->getValue() };
}

int main(int argc, char* argv[])
{
    try
    {
        // Parse command-line options
        auto opts =
            cl::sdk::parse_cli<cl::sdk::options::Diagnostic,
                               cl::sdk::options::SingleDevice,
                               cl::sdk::options::Window, CliOptions>(
                argc, argv);

        const auto& win_opts = std::get<2>(opts);
        const auto& dev_opts = std::get<1>(opts);

        OceanApplication app(win_opts.width, win_opts.height);
        app.dev_opts = dev_opts;
        app.use_cl_khr_gl_sharing = std::get<3>(opts).use_gl_sharing;
        app.run();
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
        std::cerr << "OpenCL runtime error: " << e.what() << std::endl;
        std::exit(e.err());
    } catch (std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    return 0;
}
