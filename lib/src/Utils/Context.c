#include <CL/Utils/Context.h>

// RET = function returns error code
// PAR = functions sets error code in the paremeter

#define OCLERROR_RET(func, err, label) \
do { err = func; if (err != CL_SUCCESS) goto label; } while (0)

#define OCLERROR_PAR(func, err, label) \
do { func; if (err != CL_SUCCESS) goto label; } while (0)

#define MAXOCLPLAT 65535
#define MAXOCLDEV  65535

cl_context cl_util_get_context(int plat_id, int dev_id, cl_device_type type, cl_int* error)
{
    cl_context result = NULL;
    cl_platform_id * platforms;
    cl_uint num_platforms = 0;
    cl_device_id * devices;
    cl_uint num_devices = 0;
    cl_int err;

    OCLERROR_RET(clGetPlatformIDs(MAXOCLPLAT, NULL, &num_platforms), *error, end);
    platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id) * num_platforms);
    OCLERROR_RET(clGetPlatformIDs(num_platforms, platforms, NULL), *error, plat);

    if (plat_id < 0 || plat_id >= num_platforms) {
        *error = CL_UTIL_INDEX_OUT_OF_RANGE; // "Invalid platform index provided for cl_util_get_context()"
        goto plat;
    }

    OCLERROR_RET(clGetDeviceIDs(platforms[plat_id], type, MAXOCLDEV, NULL, &num_devices), *error, plat);
    devices = (cl_device_id *)malloc(sizeof(cl_device_id) * num_devices);
    OCLERROR_RET(clGetDeviceIDs(platforms[plat_id], type, num_devices, devices, NULL), *error, dev);

    if (dev_id < 0 || dev_id >= num_devices) {
        *error = CL_UTIL_INDEX_OUT_OF_RANGE; // "Invalid device index provided for cl_util_get_context()"
        goto dev;
    }

    OCLERROR_PAR(result = clCreateContext(NULL, 1, devices + dev_id, NULL, NULL, error), *error, dev);

dev:    free(devices);
plat:   free(platforms);
end:    return result;
}

#undef OCLERROR_RET
#undef OCLERROR_PAR
#undef MAXOCLPLAT
#undef MAXOCLDEV
