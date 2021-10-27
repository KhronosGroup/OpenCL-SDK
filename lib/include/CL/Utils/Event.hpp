#pragma once

#include "OpenCLUtilsCpp_Export.h"

#include <CL/opencl.hpp>

#include <chrono>

namespace cl
{
namespace util
{
    template <cl_int From, cl_int To, typename Dur = std::chrono::nanoseconds>
    auto get_duration(cl::Event& ev)
    {
        return std::chrono::duration_cast<Dur>(std::chrono::nanoseconds{ ev.getProfilingInfo<To>() - ev.getProfilingInfo<From>() });
    }
}
}
