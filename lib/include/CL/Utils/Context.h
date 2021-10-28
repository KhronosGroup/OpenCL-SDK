#pragma once

#include "OpenCLUtils_Export.h"
#include <CL/Utils/Error.h>

#include <CL/opencl.h>

cl_context UTILS_EXPORT cl_util_get_context(int plat_id, int dev_id, cl_device_type type, cl_int* error);
cl_device_id UTILS_EXPORT cl_util_get_device(int plat_id, int dev_id, cl_device_type type, cl_int * error);
