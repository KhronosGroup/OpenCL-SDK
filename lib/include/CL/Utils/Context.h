#pragma once

#include <CL/SDK/Options.h>
#include <CL/Utils/Error.h>

UTILS_EXPORT
cl_context cl_util_get_context(int plat_id, int dev_id, cl_device_type type, cl_int* error);
UTILS_EXPORT
cl_device_id cl_util_get_device(int plat_id, int dev_id, cl_device_type type, cl_int * error);
UTILS_EXPORT
cl_device_id cl_util_get_device_by_triplet(struct cl_sdk_options_Triplet * triplet, cl_int * error);

// builds program and shows log if build is not successful
UTILS_EXPORT
cl_int cl_utils_build_program(cl_program pr, const cl_device_id dev, const char * opt);
