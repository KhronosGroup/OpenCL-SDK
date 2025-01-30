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

#include "ocean.hpp"
#include <glm/gtc/type_ptr.hpp>

void OceanApplication::event(const sf::Event& event)
{
    switch (event.type)
    {
        case sf::Event::Closed: window.close(); break;
        case sf::Event::Resized:
            // not supported
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
        case sf::Keyboard::Key::Escape: window.close(); break;
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

void OceanApplication::main_loop()
{
    while (window.isOpen())
    {
        // Process events
        sf::Event e;
        while (window.pollEvent(e))
        {
            event(e);
        }

        // Render the frame
        draw_frame();
    }

    vkDeviceWaitIdle(device);
}

template <> auto cl::sdk::parse<CliOptions>()
{
    return std::make_tuple(
        std::make_shared<TCLAP::ValueArg<std::int32_t>>(
            "", "vulkan_device", "Vulkan physical device", false, -1,
            "integral number"),
        std::make_shared<TCLAP::ValueArg<bool>>(
            "", "immediate", "Prefer VK_PRESENT_MODE_IMMEDIATE_KHR (no vsync)",
            false, false, "boolean"),
        std::make_shared<TCLAP::ValueArg<bool>>(
            "", "linear", "Use linearly tiled images", false, false, "boolean"),
        std::make_shared<TCLAP::ValueArg<bool>>("", "deviceLocalImages",
                                                "Use device local images",
                                                false, true, "boolean"),
        std::make_shared<TCLAP::ValueArg<bool>>("", "useExternalMemory",
                                                "Use cl_khr_external_memory",
                                                false, true, "boolean"),
        std::make_shared<TCLAP::ValueArg<bool>>("", "validationLayersOn",
                                                "Use vulkan validation layers",
                                                false, false, "boolean"));
}

template <>
CliOptions cl::sdk::comprehend<CliOptions>(
    std::shared_ptr<TCLAP::ValueArg<std::int32_t>> vulkan_device,
    std::shared_ptr<TCLAP::ValueArg<bool>> immediate,
    std::shared_ptr<TCLAP::ValueArg<bool>> linearImages,
    std::shared_ptr<TCLAP::ValueArg<bool>> deviceLocalImages,
    std::shared_ptr<TCLAP::ValueArg<bool>> useExternalMemory,
    std::shared_ptr<TCLAP::ValueArg<bool>> validationLayersOn)
{
    return CliOptions{
        vulkan_device->getValue(),     immediate->getValue(),
        linearImages->getValue(),      deviceLocalImages->getValue(),
        useExternalMemory->getValue(), validationLayersOn->getValue()
    };
}

int main(int argc, char** argv)
{
    auto opts = cl::sdk::parse_cli<cl::sdk::options::Window,
                                   cl::sdk::options::SingleDevice, CliOptions>(
        argc, argv);

    OceanApplication app(std::get<0>(opts));

    app.dev_opts = std::get<1>(opts);
    app.app_opts = std::get<2>(opts);

    try
    {
        app.run();
    } catch (const std::exception& e)
    {
        fprintf(stderr, "%s\n", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
