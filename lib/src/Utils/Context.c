// OpenCL SDK includes
#include <CL/Utils/Utils.h>
#include <CL/Utils/Error.h>
#include <CL/Utils/Context.h>

// STL includes
#include <stdlib.h> // malloc, free
#include <stdio.h>  // printf

static cl_uint MAXOCLPLAT = 65535;
static cl_uint MAXOCLDEV = 65535;

UTILS_EXPORT
cl_context cl_util_get_context(int plat_id, int dev_id, cl_device_type type, cl_int * error)
{
    cl_context result = NULL;
    cl_platform_id * platforms;
    cl_uint num_platforms = 0;
    cl_device_id * devices;
    cl_uint num_devices = 0;

    OCLERROR_RET(clGetPlatformIDs(MAXOCLPLAT, NULL, &num_platforms), *error, end);
    MEM_CHECK(platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id) * num_platforms), *error, end);
    OCLERROR_RET(clGetPlatformIDs(num_platforms, platforms, NULL), *error, plat);

    if (plat_id < 0 || plat_id >= num_platforms) {
        *error = CL_UTIL_INDEX_OUT_OF_RANGE; // "Invalid platform index provided for cl_util_get_context()"
        goto plat;
    }

    OCLERROR_RET(clGetDeviceIDs(platforms[plat_id], type, MAXOCLDEV, NULL, &num_devices), *error, plat);
    MEM_CHECK(devices = (cl_device_id *)malloc(sizeof(cl_device_id) * num_devices), *error, plat);
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

UTILS_EXPORT
cl_device_id cl_util_get_device(int plat_id, int dev_id, cl_device_type type, cl_int * error)
{
    cl_device_id result = NULL;
    cl_platform_id * platforms;
    cl_uint num_platforms = 0;
    cl_device_id * devices;
    cl_uint num_devices = 0;

    OCLERROR_RET(clGetPlatformIDs(MAXOCLPLAT, NULL, &num_platforms), *error, end);
    MEM_CHECK(platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id) * num_platforms), *error, end);
    OCLERROR_RET(clGetPlatformIDs(num_platforms, platforms, NULL), *error, plat);

    if (plat_id < 0 || plat_id >= num_platforms) {
        *error = CL_UTIL_INDEX_OUT_OF_RANGE; // "Invalid platform index provided for cl_util_get_context()"
        goto plat;
    }

    OCLERROR_RET(clGetDeviceIDs(platforms[plat_id], type, MAXOCLDEV, NULL, &num_devices), *error, plat);
    MEM_CHECK(devices = (cl_device_id *)malloc(sizeof(cl_device_id) * num_devices), *error, plat);
    OCLERROR_RET(clGetDeviceIDs(platforms[plat_id], type, num_devices, devices, NULL), *error, dev);

    if (dev_id < 0 || dev_id >= num_devices) {
        *error = CL_UTIL_INDEX_OUT_OF_RANGE; // "Invalid device index provided for cl_util_get_context()"
        goto dev;
    }

    result = devices[dev_id];

dev:    free(devices);
plat:   free(platforms);
end:    return result;
}

UTILS_EXPORT
void cl_util_print_device_info(cl_device_id device)
{
    cl_int error = CL_SUCCESS;
    char * name = NULL;

    cl_platform_id platform;
    OCLERROR_PAR(clGetDeviceInfo(device, CL_DEVICE_PLATFORM, sizeof(cl_platform_id), &platform, NULL), error, nam);

    OCLERROR_PAR(name = cl_util_get_platform_info(platform, CL_PLATFORM_VENDOR, &error), error, ven);
    printf("Selected platform by %s\n", name);

ven:    free(name);
    OCLERROR_PAR(name = cl_util_get_device_info(device, CL_DEVICE_NAME, &error), error, ocl);
    printf("Selected device: %s\n", name);

ocl:    free(name);
    OCLERROR_PAR(name = cl_util_get_device_info(device, CL_DEVICE_OPENCL_C_VERSION, &error), error, nam);
    printf("%s\n\n", name);

nam:    free(name);
}

UTILS_EXPORT
char * cl_util_get_platform_info(cl_platform_id platform, cl_platform_info info, cl_int * error)
{
    char * name = NULL;
    size_t n = 0;

    switch (info) {
        case CL_PLATFORM_PROFILE:
        case CL_PLATFORM_VERSION:
        case CL_PLATFORM_NAME:
        case CL_PLATFORM_VENDOR:
        case CL_PLATFORM_EXTENSIONS:
            OCLERROR_RET(clGetPlatformInfo(platform, info, 0, NULL, &n), *error, err);
            MEM_CHECK(name = (char *)malloc(sizeof(char) * (n+1)), *error, err);
            OCLERROR_RET(clGetPlatformInfo(platform, info, sizeof(char) * n, name, NULL), *error, err);
            name[n] = '\0';
            return name;
    }

err:    free(name);
    return NULL;
}

UTILS_EXPORT
char * cl_util_get_device_info(cl_device_id device, cl_device_info info, cl_int * error)
{
    char * name = NULL;
    size_t n = 0;

    switch (info) {
        case CL_DEVICE_EXTENSIONS:
        case CL_DEVICE_NAME:
        case CL_DEVICE_VENDOR:
        case CL_DEVICE_PROFILE:
        case CL_DEVICE_VERSION:
#ifdef CL_VERSION_1_1
        case CL_DEVICE_OPENCL_C_VERSION:
#endif
#ifdef CL_VERSION_1_2
        case CL_DEVICE_BUILT_IN_KERNELS:
#endif
#ifdef CL_VERSION_2_1
        case CL_DEVICE_IL_VERSION:
#endif
#ifdef CL_VERSION_3_0
        case CL_DEVICE_LATEST_CONFORMANCE_VERSION_PASSED:
#endif
        case CL_DRIVER_VERSION:
            OCLERROR_RET(clGetDeviceInfo(device, info, 0, NULL, &n), *error, err);
            MEM_CHECK(name = (char *)malloc(sizeof(char) * (n+1)), *error, err);
            OCLERROR_RET(clGetDeviceInfo(device, info, sizeof(char) * n, name, NULL), *error, err);
            name[n] = '\0';
            return name;
    }

err:    free(name);
    return NULL;
}

// build program and show log if build is not successful
UTILS_EXPORT
cl_int cl_util_build_program(cl_program pr, const cl_device_id dev, const char * opt) {
    // if error
    cl_int err = clBuildProgram(pr, 1, &dev, opt, NULL, NULL);
    if (err != CL_SUCCESS) { // no error handling here as error from build program is more valuable
        char * program_log;
        size_t log_size = 0;
        clGetProgramBuildInfo(pr, dev, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        if ((program_log = (char *)malloc((log_size + 1) * sizeof(char)))) {
            clGetProgramBuildInfo(pr, dev, CL_PROGRAM_BUILD_LOG, log_size, program_log, NULL);
            program_log[log_size] = '\0';
            printf("Build log is:\n\n%s\nOptions:\n%s\n\n", program_log, opt);
            free(program_log);
        }
    }
    return err;
}

UTILS_EXPORT
cl_ulong cl_util_get_event_duration(cl_event event, cl_profiling_info start, cl_profiling_info end, cl_int * error)
{
    cl_ulong start_time = 0, end_time = 0;
    OCLERROR_RET(clGetEventProfilingInfo(event, start, sizeof(cl_ulong), &start_time, NULL), *error, end);
    OCLERROR_RET(clGetEventProfilingInfo(event, end, sizeof(cl_ulong), &end_time, NULL), *error, end);
    return end_time - start_time;

end:    return 0;
}