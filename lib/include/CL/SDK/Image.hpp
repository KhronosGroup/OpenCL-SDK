#pragma once

// OpenCL SDK includes
#include "OpenCLSDKCpp_Export.h"

// OpenCL includes
#include <CL/opencl.hpp>

namespace cl {
namespace sdk {
    struct Image
    {
        int width = 0, height = 0, pixel_size = 1;
        cl::vector<unsigned char> pixels;
    };

    SDKCPP_EXPORT
    Image read_image(const char* file_name, cl_int* err);

    SDKCPP_EXPORT
    cl_int write_image(const char* file_name, const Image& image);
}
}
