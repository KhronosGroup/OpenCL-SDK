#pragma once

// STL includes
#include <stdbool.h>

#include <CL/cl.h>

struct cl_sdk_options_DeviceTriplet
{
    int plat_index;
    int dev_index;
    cl_device_type dev_type;
};

struct cl_sdk_options_Diagnostic
{
    bool verbose;
    bool quiet;
};

struct cl_sdk_options_SingleDevice
{
    struct cl_sdk_options_DeviceTriplet triplet;
};

struct cl_sdk_options_MultiDevice
{
    struct cl_sdk_options_DeviceTriplet * triplets;
    size_t number;
};
