// OpenCL SDK includes
#include <CL/SDK/Context.h>

// OpenCL Utils includes
#include <CL/Utils/Context.h>

// STL includes
#include <stdlib.h> // malloc, free
#include <stdio.h> // printf

cl_context cl_sdk_get_context_by_triplet(
    const struct cl_sdk_options_DeviceTriplet* const triplet,
    cl_int* const error)
{
    return cl_util_get_context(triplet->plat_index, triplet->dev_index,
                               triplet->dev_type, error);
}

cl_device_id cl_sdk_get_device_by_triplet(
    const struct cl_sdk_options_DeviceTriplet* const triplet,
    cl_int* const error)
{
    return cl_util_get_device(triplet->plat_index, triplet->dev_index,
                              triplet->dev_type, error);
}
