#pragma once

// OpenCL SDK includes
#include <CL/Utils/Context.hpp>
#include <CL/SDK/Options.hpp>

namespace cl {
namespace sdk {
    Context get_context(options::DeviceTriplet triplet,
                        cl_int* error = nullptr);
}
}

cl::Context cl::sdk::get_context(options::DeviceTriplet triplet, cl_int* error)
{
    return cl::util::get_context(triplet.plat_index, triplet.dev_index,
                                 triplet.dev_type, error);
}