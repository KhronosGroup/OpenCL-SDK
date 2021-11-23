#pragma once

// OpenCL Utils includes
#include "OpenCLUtils_Export.h"

// OpenCL includes
#include <CL/cl.h>

UTILS_EXPORT
cl_ulong cl_util_get_event_duration(cl_event event, cl_profiling_info start, cl_profiling_info end, cl_int * error);