#pragma once

// OpenCL SDK includes
#include "OpenCLSDK_Export.h"

// OpenCL SDK includes
#include <CL/SDK/Options.h>

// OpenCL includes
#include <CL/cl.h>

SDK_EXPORT
cl_context cl_sdk_get_context_by_triplet(
    const struct cl_sdk_options_DeviceTriplet* const triplet,
    cl_int* const error);

SDK_EXPORT
cl_device_id cl_sdk_get_device_by_triplet(
    const struct cl_sdk_options_DeviceTriplet* const triplet,
    cl_int* const error);
