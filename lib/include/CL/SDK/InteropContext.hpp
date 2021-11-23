#pragma once

// OpenCL SDK includes
#include "OpenCLSDKCpp_Export.h"

// OpenCL includes
#include <CL/opencl.hpp>

namespace cl
{
namespace sdk
{
    vector<cl_context_properties> SDKCPP_EXPORT get_interop_context_properties(const cl::Device& plat);

    Context SDKCPP_EXPORT get_interop_context(int plat_id, int dev_id, cl_device_type type, cl_int* error = nullptr);
}
}
