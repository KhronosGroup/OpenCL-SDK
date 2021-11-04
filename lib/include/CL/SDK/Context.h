#pragma once

// OpenCL SDK
#include <CL/SDK/Options.h>
#include <CL/Utils/Context.h>

cl_context cl_sdk_get_context_by_triplet(struct cl_sdk_options_Triplet * triplet, cl_int * error)
{
    return cl_util_get_context(triplet->plat_index, triplet->dev_index, triplet->dev_type, error);
}

cl_device_id cl_sdk_get_device_by_triplet(struct cl_sdk_options_Triplet * triplet, cl_int * error)
{
    return cl_util_get_device(triplet->plat_index, triplet->dev_index, triplet->dev_type, error);
}
