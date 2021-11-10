#pragma once

// OpenCL SDK includes
#include "OpenCLUtilsCpp_Export.h"

// OpenCL includes
#include <CL/opencl.hpp>

namespace cl
{
namespace sdk
{
    struct Image
    {
        int width, height, pixel_size;
        std::vector<unsigned char> pixels;
    };

    UTILSCPP_EXPORT
    Image read_image(const char* file_name, cl_int* err);

    UTILSCPP_EXPORT
    void write_image(const char* file_name, const Image& image, cl_int* err);
}
}
