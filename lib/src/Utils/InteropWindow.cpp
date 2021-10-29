#include <CL/Utils/InteropWindow.hpp>

#include <CL/Utils/InteropContext.hpp>

cl::util::InteropWindow::InteropWindow(
    sf::VideoMode mode,
    const sf::String& title,
    sf::Uint32 style,
    const sf::ContextSettings& settings,
    int platform_id,
    int device_id,
    cl_bitfield device_type
)
    : sf::Window{mode, title, style, settings}
    , plat_id{platform_id}
    , dev_id{device_id}
    , dev_type{device_type}
{
}

void cl::util::InteropWindow::run()
{
    setActive(true);

    initializeGL();

    opencl_context = get_interop_context(plat_id, dev_id, dev_type);

    initializeCL();

    while(isOpen())
    {
        sf::Event ev;
        while (pollEvent(ev))
            event(ev);

        updateScene();

        render();

        display();
    }

    setActive(false);
}
