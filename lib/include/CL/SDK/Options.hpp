#pragma once

// OpenCL includes
#include <CL/opencl.hpp>

namespace cl {
namespace sdk {
    namespace options {
        struct DeviceTriplet
        {
            cl_uint plat_index;
            cl_uint dev_index;
            cl_device_type dev_type;
        };
        struct Diagnostic
        {
            bool verbose, quiet;
        };
        struct SingleDevice
        {
            DeviceTriplet triplet;
        };
        struct MultiDevice
        {
            cl::vector<DeviceTriplet> triplets;
        };
        struct Window
        {
            int width;
            int height;
            bool fullscreen;
        };
    }
}
}
