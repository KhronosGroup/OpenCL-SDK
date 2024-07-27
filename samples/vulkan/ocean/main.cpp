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

namespace {

void glfw_keyboard(GLFWwindow* window, int key, int scancode, int action,
                   int mods)
{
    auto app = (OceanApplication*)glfwGetWindowUserPointer(window);
    app->keyboard(key, scancode, action, mods);
}

void glfw_mouse_event(GLFWwindow* window, int button, int action, int mods)
{
    auto app = (OceanApplication*)glfwGetWindowUserPointer(window);
    app->mouse_event(button, action, mods);
}

void glfw_mouse_pos(GLFWwindow* window, double pX, double pY)
{
    auto app = (OceanApplication*)glfwGetWindowUserPointer(window);
    app->mouse_pos(pX, pY);
}

void glfw_mouse_roll(GLFWwindow* window, double oX, double oY)
{
    auto app = (OceanApplication*)glfwGetWindowUserPointer(window);
    app->mouse_roll(oX, oY);
}

} // anonymous namespace


void OceanApplication::main_loop()
{
    glfwSetKeyCallback(window, glfw_keyboard);
    glfwSetMouseButtonCallback(window, glfw_mouse_event);
    glfwSetCursorPosCallback(window, glfw_mouse_pos);
    glfwSetScrollCallback(window, glfw_mouse_roll);

    while (!glfwWindowShouldClose(window))
    {
        draw_frame();
        glfwPollEvents();
    }

    vkDeviceWaitIdle(device);
}

template <> auto cl::sdk::parse<CliOptions>()
{
    return std::make_tuple(
        std::make_shared<TCLAP::ValueArg<size_t>>("", "window_width",
                                                  "Window width", false, 1024,
                                                  "positive integral"),
        std::make_shared<TCLAP::ValueArg<size_t>>("", "window_height",
                                                  "Window height", false, 1024,
                                                  "positive integral"),
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
                                                false, true, "boolean"));
}

template <>
CliOptions cl::sdk::comprehend<CliOptions>(
    std::shared_ptr<TCLAP::ValueArg<size_t>> window_width,
    std::shared_ptr<TCLAP::ValueArg<size_t>> window_height,
    std::shared_ptr<TCLAP::ValueArg<bool>> immediate,
    std::shared_ptr<TCLAP::ValueArg<bool>> linearImages,
    std::shared_ptr<TCLAP::ValueArg<bool>> deviceLocalImages,
    std::shared_ptr<TCLAP::ValueArg<bool>> useExternalMemory)
{
    return CliOptions{
        window_width->getValue(),      window_height->getValue(),
        immediate->getValue(),         linearImages->getValue(),
        deviceLocalImages->getValue(), useExternalMemory->getValue()
    };
}

int main(int argc, char** argv)
{
    OceanApplication app;

    auto opts = cl::sdk::parse_cli<cl::sdk::options::Diagnostic,
                                   cl::sdk::options::SingleDevice, CliOptions>(
        argc, argv);

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
