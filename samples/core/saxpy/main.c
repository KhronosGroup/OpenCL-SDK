/*
 * Copyright (c) 2021 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// OpenCL SDK includes
#include <CL/Utils/Utils.h>
#include <CL/SDK/Context.h>
//#include <CL/SDK/Options.h>
//#include <CL/SDK/CLI.h>
#include <CL/SDK/Random.h>

pcg32_random_t rng = { 111111, 222 };

int main(int argc, char* argv[])
{
    cl_int error = CL_SUCCESS;
    cl_device_type type = CL_DEVICE_TYPE_DEFAULT;
    int plat_id = 0, dev_id = 0;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;

    const char * kernel_location = "./saxpy.cl";
    char * kernel = NULL;
    size_t program_size = 0;
    cl_program program = NULL;

    // Parse command-line options
    bool quiet = false;

    // Create runtime objects based on user preference or default
    //OCLERROR_PAR(context = cl_util_get_context(plat_id, dev_id, type, &error), error, end);
    OCLERROR_PAR(device = cl_util_get_device(plat_id, dev_id, type, &error), error, end);
    OCLERROR_PAR(context = clCreateContext(NULL, 1, &device, NULL, NULL, &error), error, end);
    OCLERROR_PAR(queue = clCreateCommandQueueWithProperties(context, device, NULL, &error), error, cont);
    OCLERROR_RET(clGetDeviceInfo(device, CL_DEVICE_PLATFORM, sizeof(cl_platform_id), &platform, NULL), error, que);

    if (!quiet)//(!diag_opts.quiet)
    {
        char * vendor = NULL,
            * name = NULL;
        size_t v = 0, n = 0;

        OCLERROR_RET(clGetPlatformInfo(platform, CL_PLATFORM_VENDOR, sizeof(size_t), NULL, &v), error, que);
        MEM_CHECK(vendor = (char *)malloc(sizeof(char) * (v+1)), error, ven);
        OCLERROR_RET(clGetPlatformInfo(platform, CL_PLATFORM_VENDOR, sizeof(char) * v, vendor, NULL), error, ven);
        vendor[v] = '\0';

        OCLERROR_RET(clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(size_t), NULL, &n), error, ven);
        MEM_CHECK(name = (char *)malloc(sizeof(char) * (n+1)), error, nam);
        OCLERROR_RET(clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(char) * n, name, NULL), error, nam);
        name[n] = '\0';

        printf("Selected platform: %s\n" "Selected device: %s\n\n", vendor, name);

nam:    free(name);
ven:    free(vendor);
    }

    // Compile kernel
    OCLERROR_PAR(kernel = cl_utils_read_text_file(kernel_location, &program_size, &error), error, que);

    OCLERROR_PAR(program = clCreateProgramWithSource(context, 1, &kernel, &program_size, &error), error, ker);
    OCLERROR_RET(cl_utils_build_program(program, device, NULL), error, prg);

    cl_kernel saxpy;
    OCLERROR_PAR(saxpy = clCreateKernel(program, "saxpy", &error), error, prg);

    // Initialize host-side storage
    const size_t length = 1234;//saxpy_opts.length;

    cl_float * arr_x, * arr_y,
        a = pcg32_random_float(&rng);
    MEM_CHECK(arr_x = (cl_float *)malloc(sizeof(cl_float) * length), error, sxp);
    MEM_CHECK(arr_y = (cl_float *)malloc(sizeof(cl_float) * length), error, arrx);

    cl_sdk_fill_with_random_range(&rng, arr_x, length, -100, 100);
    cl_sdk_fill_with_random_range(&rng, arr_y, length, -100, 100);

    // Initialize device-side storage
    cl_mem buf_x, buf_y;
    OCLERROR_PAR(buf_x = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_float) * length, arr_x, &error), error, arry);
    OCLERROR_PAR(buf_y = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_float) * length, arr_y, &error), error, bufx);

    // Execute kernel
    OCLERROR_RET(clSetKernelArg(saxpy, 0, sizeof(cl_float), &a), error, bufy);
    OCLERROR_RET(clSetKernelArg(saxpy, 1, sizeof(cl_mem), &buf_x), error, bufy);
    OCLERROR_RET(clSetKernelArg(saxpy, 2, sizeof(cl_mem), &buf_y), error, bufy);

    OCLERROR_RET(clEnqueueNDRangeKernel(queue, saxpy, 1, NULL, &length, NULL, 0, NULL, NULL), error, bufy);

    // Concurrently calculate reference dataset
    for (size_t i = 0; i < length; ++i)
        arr_y[i] += a * arr_x[i];

    // Fetch results
    OCLERROR_RET(clEnqueueReadBuffer(queue, buf_y, CL_BLOCKING, 0, sizeof(cl_float) * length, (void *)arr_x, 0, NULL, NULL), error, bufy);

    // Validate
    for (size_t i = 0; i < length; ++i)
        if (arr_y[i] != arr_x[i]) {
            printf("Verification failed! %f != %f at index %zu\n", arr_y[i], arr_x[i], i);
            error = CL_INVALID_VALUE;
            goto bufy;
        }
    printf("Verification passed.\n");

    // Release resources
bufy:   clReleaseMemObject(buf_y);
bufx:   clReleaseMemObject(buf_x);
arry:   free(arr_y);
arrx:   free(arr_x);
sxp:    clReleaseKernel(saxpy);
prg:    clReleaseProgram(program);
ker:    free(kernel);
que:    clReleaseCommandQueue(queue);
cont:   clReleaseContext(context);

end:    return error;
}
