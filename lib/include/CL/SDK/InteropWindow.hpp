#pragma once

// OpenCL SDK includes
#include "OpenCLSDKCpp_Export.h"

// OpenCL Utils includes
#include <CL/Utils/Error.hpp>

// OpenCL includes
#include <CL/opencl.hpp>

// SFML includes
#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>

namespace cl {
namespace sdk {
    class SDKCPP_EXPORT InteropWindow : public sf::Window {
    public:
        explicit InteropWindow(
            sf::VideoMode mode, const sf::String& title,
            sf::Uint32 style = sf::Style::Default,
            const sf::ContextSettings& settings = sf::ContextSettings{},
            cl_uint platform_id = 0, cl_uint device_id = 0,
            cl_bitfield device_type = CL_DEVICE_TYPE_DEFAULT);

        void run();

    protected:
        // Core functionality to be overriden
        virtual void initializeGL() = 0; // Function that initializes all OpenGL
                                         // assets needed to draw a scene
        virtual void initializeCL() = 0; // Function that initializes all OpenCL
                                         // assets needed to draw a scene
        virtual void
        updateScene() = 0; // Function that holds scene update guaranteed not to
                           // conflict with drawing
        virtual void render() = 0; // Function that does the native rendering
        virtual void event(
            const sf::Event& e) = 0; // Function that handles render area resize

        cl::Context opencl_context;
        bool cl_khr_gl_event_supported;

    private:
        cl_uint plat_id;
        cl_uint dev_id;
        cl_bitfield dev_type;
    };
}
}
