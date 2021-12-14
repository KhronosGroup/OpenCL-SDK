// OpenCL SDK includes
#include <CL/Utils/Context.hpp>

cl::Context cl::util::get_context(cl_uint plat_id, cl_uint dev_id,
                                  cl_device_type type, cl_int* error)
{
    cl::vector<cl::Platform> platforms;
    cl_int plat_err = cl::Platform::get(&platforms);

    if (plat_err == CL_SUCCESS)
    {
        if (plat_id < platforms.size())
        {
            cl::vector<cl::Device> devices;
            cl_int dev_err = platforms[plat_id].getDevices(type, &devices);

            if (dev_err == CL_SUCCESS)
            {
                if (dev_id < devices.size())
                {
                    return cl::Context(devices[dev_id]);
                }
                else
                    detail::errHandler(CL_UTIL_INDEX_OUT_OF_RANGE, error,
                                       "Invalid device index provided for "
                                       "cl::Context cl::sdk::get_context()");
            }
            else
                detail::errHandler(dev_err, error);
        }
        else
            detail::errHandler(CL_UTIL_INDEX_OUT_OF_RANGE, error,
                               "Invalid platform index provided for "
                               "cl::Context cl::sdk::get_context()");
    }
    else
        detail::errHandler(plat_err, error,
                           "Failed to get platforms inside cl::Context "
                           "cl::sdk::get_context()");

    return cl::Context{};
}
