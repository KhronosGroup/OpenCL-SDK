// OpenCL SDK includes
#include <CL/SDK/InteropContext.hpp>

// OpenCL Utils includes
#include <CL/Utils/Utils.hpp>

// Platform includes
#ifdef _WIN32
#include <wtypes.h>
#include <wingdi.h> // wglGetCurrent...()
#endif
#ifdef __linux__
#include <GL/glxew.h>
#undef None
#endif

cl::vector<cl_context_properties> cl::sdk::get_interop_context_properties(const cl::Device& device)
{
    return cl::vector<cl_context_properties>{
        CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(cl::Platform{ device.getInfo<CL_DEVICE_PLATFORM>() }()),
#ifdef _WIN32
        CL_WGL_HDC_KHR, reinterpret_cast<cl_context_properties>(wglGetCurrentDC()),
        CL_GL_CONTEXT_KHR, reinterpret_cast<cl_context_properties>(wglGetCurrentContext()),
#endif
#ifdef __linux__
        CL_GLX_DISPLAY_KHR, reinterpret_cast<cl_context_properties>(glXGetCurrentDisplay()),
        CL_GL_CONTEXT_KHR, reinterpret_cast<cl_context_properties>(glXGetCurrentContext()),
#endif
        0
    };
}

cl::Context cl::sdk::get_interop_context(int plat_id, int dev_id, cl_device_type type, cl_int* error)
{
    cl::vector<cl::Platform> platforms;
    cl_int plat_err = cl::Platform::get(&platforms);

    if (plat_err == CL_SUCCESS)
    {
        if (plat_id >= 0 && plat_id < platforms.size())
        {
            cl::vector<cl::Device> devices;
            cl_int dev_err = platforms[plat_id].getDevices(type, &devices);

            if (dev_err == CL_SUCCESS)
            {
                if (dev_id >= 0 && dev_id < devices.size())
                {
                    cl_int ctx_err = CL_SUCCESS;
                    auto props = get_interop_context_properties(devices[dev_id]);
#if defined(CL_HPP_ENABLE_EXCEPTIONS)
                    try
                    {
#endif
                        cl::Context context(
                            devices[dev_id],
                            props.data(),
                            nullptr,
                            nullptr,
                            &ctx_err
                        );
#if defined(CL_HPP_ENABLE_EXCEPTIONS)
                    }
                    catch (cl::Error& e)
                    {
                        ctx_err = e.err();
                    }
#endif
                    if (ctx_err == CL_SUCCESS)
                        return context;
                    else
                    {
                        cl::util::detail::errHandler(
                            CL_UTIL_DEVICE_NOT_INTEROPERABLE,
                            error,
                            "Selected device isn't interoperable with the current OpenGL context."
                        );
                        return cl::Context{};
                    }
                }
                else
                    cl::util::detail::errHandler(
                        CL_UTIL_INDEX_OUT_OF_RANGE,
                        error,
                        "Invalid device index provided for cl::Context cl::sdk::get_context()"
                    );
            }
            else
                cl::util::detail::errHandler(plat_err, error);
        }
        else
            cl::util::detail::errHandler(
                CL_UTIL_INDEX_OUT_OF_RANGE,
                error,
                "Invalid platform index provided for cl::Context cl::sdk::get_context()"
            );
    }
    else
        cl::util::detail::errHandler(plat_err, error, "Failed to get platforms inside cl::Context cl::sdk::get_context()");

    return cl::Context{};
}
