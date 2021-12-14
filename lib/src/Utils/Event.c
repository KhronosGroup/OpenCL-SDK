// OpenCL Utils includes
#include <CL/Utils/Event.h>

cl_ulong cl_util_get_event_duration(const cl_event event,
                                    const cl_profiling_info start,
                                    const cl_profiling_info end,
                                    cl_int* const error)
{
    cl_int err = CL_SUCCESS;
    cl_ulong start_time = 0, end_time = 0;
    OCLERROR_RET(clGetEventProfilingInfo(event, start, sizeof(cl_ulong),
                                         &start_time, NULL),
                 err, end);
    OCLERROR_RET(
        clGetEventProfilingInfo(event, end, sizeof(cl_ulong), &end_time, NULL),
        err, end);
    if (error != NULL) *error = err;
    return end_time - start_time;

end:
    if (error != NULL) *error = err;
    return 0;
}
