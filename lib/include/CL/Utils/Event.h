#pragma once

// OpenCL includes
#include <CL/CL.h>

cl_ulong cl_util_get_event_duration(cl_event event, cl_profiling_info start, cl_profiling_info end, cl_int * error)
{
    cl_ulong start_time = 0, end_time = 0;
    OCLERROR_RET(clGetEventProfilingInfo(event, start, sizeof(cl_ulong), &start_time, NULL), *error, end);
    OCLERROR_RET(clGetEventProfilingInfo(event, end, sizeof(cl_ulong), &end_time, NULL), *error, end);
    return end_time - start_time;

end:    return 0;
}