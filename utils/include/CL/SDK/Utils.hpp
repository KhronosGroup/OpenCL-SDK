#pragma once

#include <CL/SDK/Error.hpp>
#include <CL/SDK/Options.hpp>

#include <CL/opencl.hpp>

#include <initializer_list>

namespace cl
{
namespace sdk
{
    Context get_context(options::Triplet triplet, cl_int* error = nullptr);

    Context get_context(int plat_id, int dev_id, cl_device_type type, cl_int* error = nullptr);

    template <typename PRNG, typename... Containers>
    void fill_with_random(PRNG&& prng, Containers&&... containers)
    {
        detail::for_each_arg([&](auto&& container)
        {
            std::generate_n(std::begin(container), container.size(), prng);
        }, containers...);
    }
}
}

cl::Context cl::sdk::get_context(options::Triplet triplet, cl_int* error)
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
        if (plat_id >= 0 && plat_id < platforms.size())
        {
            cl::vector<cl::Device> devices;
            cl_int dev_err = platforms[plat_id].getDevices(type, &devices);

            if (dev_err == CL_SUCCESS)
            {
                if (dev_id >= 0 && dev_id < devices.size())
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
