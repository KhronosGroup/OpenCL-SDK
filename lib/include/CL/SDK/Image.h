#pragma once

// OpenCL SDK includes
#include "OpenCLSDK_Export.h"

// OpenCL includes
#include <CL/cl.h>

typedef struct cl_sdk_image
{
    int width, height, pixel_size;
    unsigned char* pixels;
} cl_sdk_image;

SDK_EXPORT
cl_sdk_image cl_sdk_read_image(const char* file_name, cl_int* err);

SDK_EXPORT
cl_int cl_sdk_write_image(const char* file_name, const cl_sdk_image* im);
