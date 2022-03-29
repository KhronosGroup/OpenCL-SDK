// OpenCL SDK includes
#include <CL/Utils/Error.hpp>

#if defined(CL_HPP_ENABLE_EXCEPTIONS)
UTILSCPP_EXPORT cl_int cl::util::detail::errHandler(cl_int err, cl_int*,
                                                    const char* errStr)
{
    if (err != CL_SUCCESS) throw cl::util::Error{ err, errStr };
    return err;
}
#else
UTILSCPP_EXPORT cl_int cl::util::detail::errHandler(cl_int err, cl_int* errPtr,
                                                    const char*)
{
    if (err != CL_SUCCESS)
    {
        if (errPtr != nullptr) *errPtr = err;
    }
    return err;
}
#endif