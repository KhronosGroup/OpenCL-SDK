// OpenCL SDK includes
#include <CL/SDK/InteropWindow.hpp>
#include <CL/SDK/InteropContext.hpp>

cl::sdk::InteropWindow::InteropWindow(sf::VideoMode mode,
                                      const sf::String& title, sf::Uint32 style,
                                      const sf::ContextSettings& settings,
                                      cl_uint platform_id, cl_uint device_id,
                                      cl_bitfield device_type)
    : sf::Window{ mode, title, style, settings }, plat_id{ platform_id },
      dev_id{ device_id }, dev_type{ device_type }
{}

void cl::sdk::InteropWindow::run()
{
    setActive(true);

    initializeGL();

    opencl_context = get_interop_context(plat_id, dev_id, dev_type);

    cl_khr_gl_event_supported = opencl_context.getInfo<CL_CONTEXT_DEVICES>()
                                    .at(0)
                                    .getInfo<CL_DEVICE_EXTENSIONS>()
                                    .find("cl_khr_gl_event")
        != cl::string::npos;

    initializeCL();

    while (isOpen())
    {
        render();

        display();

        updateScene();

        sf::Event ev;
        while (pollEvent(ev)) event(ev);
    }

    setActive(false);
}
