#pragma once

// STL includes
//#include <iostream>

// OpenCL includes
#include <CL/opencl.hpp>

#define CL_UTIL_INDEX_OUT_OF_RANGE -2000

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
    void errHandler(cl_int err, cl_int* errPtr, const char* errStr = nullptr);
}

}
}

void cl::util::detail::errHandler(cl_int err, cl_int* errPtr, const char* errStr)
{
    if (err != CL_SUCCESS)
    {
#if defined(CL_HPP_ENABLE_EXCEPTIONS)
        throw cl::util::Error{err, errStr};
#else
        (void)errStr; // suppress unused variable warning
        //std::cerr << errStr << std::endl;
        if (errPtr != nullptr)
            *errPtr = err;
#endif
    }
}
