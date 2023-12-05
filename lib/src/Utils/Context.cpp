// OpenCL SDK includes
#include <CL/Utils/Context.hpp>

#include <iostream>
#include <string>

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

void cl::util::print_device_info(const cl::Device& device)
{
    const cl::Platform platform(device.getInfo<CL_DEVICE_PLATFORM>());
    const std::string platform_vendor = platform.getInfo<CL_PLATFORM_VENDOR>();
    const std::string device_name = device.getInfo<CL_DEVICE_NAME>();
    const std::string device_opencl_c_version =
        device.getInfo<CL_DEVICE_OPENCL_C_VERSION>();
    std::cout << "Selected platform by " << platform_vendor
              << "\nSelected device: " << device_name << '\n'
              << device_opencl_c_version << '\n'
              << std::endl;
}
