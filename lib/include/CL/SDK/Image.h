#pragma once

// OpenCL SDK includes
#include "OpenCLUtils_Export.h"

typedef struct cl_sdk_image
{
    int width, height, pixel_size;
    unsigned char* pixels;
}
cl_sdk_image;

UTILS_EXPORT
cl_sdk_image cl_sdk_read_image(const char* file_name, cl_int* err);

UTILS_EXPORT
void cl_sdk_write_image(const char * file_name, const cl_sdk_image * im, cl_int * err);
