#pragma once

// OpenCL SDK includes
#include <CL/Utils/Utils.hpp>

// STL includes
#include <fstream>
#include <string>

namespace cl
{
namespace util
{
    UTILSCPP_EXPORT
    std::string read_text_file(const char * filename, cl_int * error);
}
}

// Scott Meyers, Effective STL, Addison-Wesley Professional, 2001, Item 29
// with error handling
UTILSCPP_EXPORT
std::string cl::util::read_text_file(const char * filename, cl_int * error)
{
    cl_int err;
    if (error == nullptr)
        error = &err;

    std::ifstream in(filename);
    if (in.good())
    {
        try
        {
            std::string red((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            if (in.good() && in.eof())
            {
                *error = CL_SUCCESS;
                return red;
            }
            else
            {
                *error = CL_INVALID_VALUE;
                return std::string();
            }
        }
        catch (std::bad_alloc& ex)
        {
            *error = CL_OUT_OF_RESOURCES;
            return std::string();
        }
    }
    else
    {
        *error = CL_INVALID_ARG_VALUE;
        return std::string();
    }
}
