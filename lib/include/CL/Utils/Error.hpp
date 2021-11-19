#pragma once

// STL includes
//#include <iostream>

// OpenCL includes
#include <CL/opencl.hpp>

#define CL_UTIL_INDEX_OUT_OF_RANGE -2000
#define CL_UTIL_DEVICE_NOT_INTEROPERABLE -2001

namespace cl
{
namespace util
{
#if defined(CL_HPP_ENABLE_EXCEPTIONS)
    /*! \brief Exception class
     *
     *  This may be thrown by SDK utility functions when CL_HPP_ENABLE_EXCEPTIONS is defined.
     */
    class Error : public std::exception
    {
    private:
        int err_;
        const char * errStr_;
    public:
        /*! \brief Create a new SDK error exception for a given error code
         *  and corresponding message.
         *
         *  \param err error code value.
         *
         *  \param errStr a descriptive string that must remain in scope until
         *                handling of the exception has concluded.  If set, it
         *                will be returned by what().
         */
        Error(cl_int err, const char * errStr = NULL) : err_(err), errStr_(errStr)
        {}

        ~Error() throw() {}

        /*! \brief Get error string associated with exception
         *
         * \return A memory pointer to the error message string.
         */
        virtual const char * what() const throw ()
        {
            if (errStr_ == NULL) {
                return "empty";
            }
            else {
                return errStr_;
            }
        }

        /*! \brief Get error code associated with exception
         *
         *  \return The error code.
         */
        cl_int err(void) const { return err_; }
    };
#endif

namespace detail
{
    cl_int errHandler(cl_int err, cl_int* errPtr, const char* errStr = nullptr);
}

}
}

#if defined(CL_HPP_ENABLE_EXCEPTIONS)
cl_int cl::util::detail::errHandler(cl_int err, cl_int*, const char* errStr)
{
    if (err != CL_SUCCESS)
        throw cl::util::Error{err, errStr};
    return err;
}
#else
cl_int cl::util::detail::errHandler(cl_int err, cl_int* errPtr, const char*)
{
    if (err != CL_SUCCESS)
    {
        if (errPtr != nullptr)
            *errPtr = err;
    }
    return err;
}
#endif
