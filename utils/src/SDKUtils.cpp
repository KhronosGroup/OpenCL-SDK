#include <CL/SDK/Utils.hpp>

void cl::sdk::detail::errHandler(cl_int err, cl_int* errPtr, const char* errStr)
{
    if (err != CL_SUCCESS)
    {
#if defined(CL_HPP_ENABLE_EXCEPTIONS)
        throw Error(err, errStr);
#else
        (void) errStr; // suppress unused variable warning
        if (errPtr != nullptr)
            *errPtr = err;
#endif
    }
}

cl::Context cl::sdk::get_context(Triplet triplet, cl_int* error)
{
    return cl::sdk::get_context(
        triplet.plat_index,
        triplet.dev_index,
        triplet.dev_type,
        error
    );
}

cl::Context cl::sdk::get_context(int plat_id, int dev_id, cl_device_type type, cl_int* error)
{
    cl::vector<cl::Platform> platforms;
    cl_int plat_err = cl::Platform::get(&platforms);

    if (plat_err == CL_SUCCESS)
    {
        if (plat_id > 0 && plat_id < platforms.size())
        {
            cl::vector<cl::Device> devices;
            cl_int dev_err = platforms[plat_id].getDevices(type, &devices);

            if (dev_err == CL_SUCCESS)
            {
                if (dev_id > 0 && dev_id < devices.size())
                {
                    return cl::Context(devices[dev_id]);
                }
                else
                    detail::errHandler(
                        CL_SDK_INDEX_OUT_OF_RANGE,
                        error,
                        "Invalid device index provided for cl::Context cl::sdk::get_context()"
                    );
            }
            else
                detail::errHandler(plat_err, error);
        }
        else
            detail::errHandler(
                CL_SDK_INDEX_OUT_OF_RANGE,
                error,
                "Invalid platform index provided for cl::Context cl::sdk::get_context()"
            );
    }
    else
        detail::errHandler(plat_err, error);

    return cl::Context{};
}
