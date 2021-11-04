#pragma once

UTILS_EXPORT
cl_context cl_util_get_context(int plat_id, int dev_id, cl_device_type type, cl_int* error);
UTILS_EXPORT
cl_device_id cl_util_get_device(int plat_id, int dev_id, cl_device_type type, cl_int * error);

// build program and show log if build is not successful
UTILS_EXPORT
cl_int cl_util_build_program(cl_program pr, const cl_device_id dev, const char * opt);
