#pragma once

#include "OpenCLUtilsCpp_Export.h"
#include <CL/Utils/Error.hpp>

#include <CL/opencl.hpp>

#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>

namespace cl
{
namespace util
{
    class UTILSCPP_EXPORT InteropWindow : public sf::Window
    {
    public:
        explicit InteropWindow(
            sf::VideoMode mode,
            const sf::String& title,
            sf::Uint32 style = sf::Style::Default,
            const sf::ContextSettings& settings = sf::ContextSettings{},
            int platform_id = 0,
            int device_id = 0,
            cl_bitfield device_type = CL_DEVICE_TYPE_DEFAULT
        );

        void run();

    protected:
        // Core functionality to be overriden
        virtual void initializeGL() = 0;            // Function that initializes all OpenGL assets needed to draw a scene
        virtual void initializeCL() = 0;            // Function that initializes all OpenCL assets needed to draw a scene
        virtual void updateScene() = 0;             // Function that holds scene update guaranteed not to conflict with drawing
        virtual void render() = 0;                  // Function that does the native rendering
        virtual void event(const sf::Event& e) = 0; // Function that handles render area resize

        cl::Context opencl_context;

    private:
        int plat_id;
        int dev_id;
        cl_bitfield dev_type;
    };
}
}
