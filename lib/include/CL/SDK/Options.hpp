#pragma once

#include <CL/opencl.hpp>

namespace cl
{
namespace sdk
{
namespace options
{
    struct Triplet
    {
        int plat_index;
        int dev_index;
        cl_device_type dev_type;
    };
    struct Diagnostic
    {
        bool verbose,
             quiet;
    };
    struct SingleDevice
    {
        Triplet triplet;
    };
    struct MultiDevice
    {
        cl::vector<Triplet> triplets;
    };
}
}
}
