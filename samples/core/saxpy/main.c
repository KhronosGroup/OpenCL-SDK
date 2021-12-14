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
#include <CL/SDK/CLI.h>
#include <CL/SDK/Random.h>

// STL includes
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

// Sample-specific option
struct options_Saxpy
{
    size_t length;
};

cag_option SaxpyOptions[] = { { .identifier = 'l',
                                .access_letters = "l",
                                .access_name = "length",
                                .value_name = "(positive integer)",
                                .description = "Length of input" } };

ParseState parse_SaxpyOptions(const char identifier,
                              cag_option_context *cag_context,
                              struct options_Saxpy *opts)
{
    const char *value;
    switch (identifier)
    {
        case 'l':
            if ((value = cag_option_get_value(cag_context)))
            {
                opts->length = strtoul(value, NULL, 0);
                return ParsedOK;
            }
            else
                return ParseError;
    }
    return NotParsed;
}

cl_int parse_options(int argc, char *argv[],
                     struct cl_sdk_options_Diagnostic *diag_opts,
                     struct cl_sdk_options_SingleDevice *dev_opts,
                     struct options_Saxpy *saxpy_opts)
{
    cl_int error = CL_SUCCESS;
    struct cag_option *opts = NULL, *tmp = NULL;
    size_t n = 0;

    /* Prepare all options array. */
    MEM_CHECK(opts = add_CLI_options(opts, &n, DiagnosticOptions,
                                     CAG_ARRAY_SIZE(DiagnosticOptions)),
              error, end);
    MEM_CHECK(tmp = add_CLI_options(opts, &n, SingleDeviceOptions,
                                    CAG_ARRAY_SIZE(SingleDeviceOptions)),
              error, end);
    opts = tmp;
    MEM_CHECK(tmp = add_CLI_options(opts, &n, SaxpyOptions,
                                    CAG_ARRAY_SIZE(SaxpyOptions)),
              error, end);
    opts = tmp;

    char identifier;
    cag_option_context cag_context;

    /* Prepare the context and iterate over all options. */
    cag_option_prepare(&cag_context, opts, n, argc, argv);
    while (cag_option_fetch(&cag_context))
    {
        ParseState state = NotParsed;
        identifier = cag_option_get(&cag_context);

        PARS_OPTIONS(parse_DiagnosticOptions(identifier, diag_opts), state);
        PARS_OPTIONS(
            parse_SingleDeviceOptions(identifier, &cag_context, dev_opts),
            state);
        PARS_OPTIONS(parse_SaxpyOptions(identifier, &cag_context, saxpy_opts),
                     state);

        if (identifier == 'h')
        {
            printf("Usage: saxpy [OPTION]...\n");
            printf("Demonstrates typical OpenCL application layout.\n\n");
            cag_option_print(opts, n, stdout);
            exit((state == ParseError) ? CL_INVALID_ARG_VALUE : CL_SUCCESS);
        }
    }

end:
    free(opts);
    return error;
}

// Random number generator state
pcg32_random_t rng;

int main(int argc, char *argv[])
{
    cl_int error = CL_SUCCESS, end_error = CL_SUCCESS;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;

    const char *kernel_location = "./saxpy.cl";
    char *kernel = NULL;
    size_t program_size = 0;
    cl_program program = NULL;

    // Parse command-line options
    struct cl_sdk_options_Diagnostic diag_opts = { .quiet = false,
                                                   .verbose = false };
    struct cl_sdk_options_SingleDevice dev_opts = {
        .triplet = { 0, 0, CL_DEVICE_TYPE_ALL }
    };
    struct options_Saxpy saxpy_opts = { .length = 1048576 };

    OCLERROR_RET(parse_options(argc, argv, &diag_opts, &dev_opts, &saxpy_opts),
                 error, end);

    // Create runtime objects based on user preference or default
    OCLERROR_PAR(device = cl_util_get_device(dev_opts.triplet.plat_index,
                                             dev_opts.triplet.dev_index,
                                             dev_opts.triplet.dev_type, &error),
                 error, end);
    OCLERROR_PAR(context =
                     clCreateContext(NULL, 1, &device, NULL, NULL, &error),
                 error, end);
    OCLERROR_PAR(queue = clCreateCommandQueue(context, device, 0, &error),
                 error, cont);
    OCLERROR_RET(clGetDeviceInfo(device, CL_DEVICE_PLATFORM,
                                 sizeof(cl_platform_id), &platform, NULL),
                 error, cont);
#if CL_HPP_TARGET_OPENCL_VERSION >= 200
    OCLERROR_PAR(queue = clCreateCommandQueueWithProperties(context, device,
                                                            NULL, &error),
                 error, cont);
#else
    OCLERROR_PAR(queue = clCreateCommandQueue(
                     context, device, CL_QUEUE_PROFILING_ENABLE, &error),
                 error, cont);
#endif

    if (!diag_opts.quiet)
    {
        cl_util_print_device_info(device);
    }

    // Compile kernel
    OCLERROR_PAR(
        kernel = cl_util_read_text_file(kernel_location, &program_size, &error),
        error, que);

    OCLERROR_PAR(program = clCreateProgramWithSource(
                     context, 1, (const char **)&kernel, &program_size, &error),
                 error, ker);
    OCLERROR_RET(cl_util_build_program(program, device, NULL), error, prg);

    cl_kernel saxpy;
    OCLERROR_PAR(saxpy = clCreateKernel(program, "saxpy", &error), error, prg);

    // Initialize host-side storage
    const size_t length = saxpy_opts.length;

    pcg32_srandom_r(&rng, 111111, -222);
    cl_float *arr_x, *arr_y, a;
    MEM_CHECK(arr_x = (cl_float *)malloc(sizeof(cl_float) * length), error,
              sxp);
    MEM_CHECK(arr_y = (cl_float *)malloc(sizeof(cl_float) * length), error,
              arrx);

    cl_sdk_fill_with_random_floats_range(&rng, &a, 1, -100, 100);
    cl_sdk_fill_with_random_floats_range(&rng, arr_x, length, -100, 100);
    cl_sdk_fill_with_random_floats_range(&rng, arr_y, length, -100, 100);

    // Initialize device-side storage
    cl_mem buf_x, buf_y;
    OCLERROR_PAR(
        buf_x = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                               sizeof(cl_float) * length, arr_x, &error),
        error, arry);
    OCLERROR_PAR(buf_y = clCreateBuffer(
                     context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                     sizeof(cl_float) * length, arr_y, &error),
                 error, bufx);

    // Execute kernel
    OCLERROR_RET(clSetKernelArg(saxpy, 0, sizeof(cl_float), &a), error, bufy);
    OCLERROR_RET(clSetKernelArg(saxpy, 1, sizeof(cl_mem), &buf_x), error, bufy);
    OCLERROR_RET(clSetKernelArg(saxpy, 2, sizeof(cl_mem), &buf_y), error, bufy);

    OCLERROR_RET(clEnqueueNDRangeKernel(queue, saxpy, 1, NULL, &length, NULL, 0,
                                        NULL, NULL),
                 error, bufy);

    // Concurrently calculate reference dataset
    for (size_t i = 0; i < length; ++i)
        arr_y[i] =
            fmaf(a, arr_x[i], arr_y[i]); // arr_y[i] = a * arr_x[i] + arr_y[i];

    // Fetch results
    OCLERROR_RET(clEnqueueReadBuffer(queue, buf_y, CL_BLOCKING, 0,
                                     sizeof(cl_float) * length, (void *)arr_x,
                                     0, NULL, NULL),
                 error, bufy);

    // Validate
    for (size_t i = 0; i < length; ++i)
        if (arr_y[i] != arr_x[i])
        {
            printf("Verification failed! %f != %f at index %zu\n", arr_y[i],
                   arr_x[i], i);
            error = CL_INVALID_VALUE;
            if (!diag_opts.verbose) goto bufy;
        }
    if (error == CL_SUCCESS) printf("Verification passed.\n");

    // Release resources
bufy:
    OCLERROR_RET(clReleaseMemObject(buf_y), end_error, bufx);
bufx:
    OCLERROR_RET(clReleaseMemObject(buf_x), end_error, arry);
arry:
    free(arr_y);
arrx:
    free(arr_x);
sxp:
    OCLERROR_RET(clReleaseKernel(saxpy), end_error, prg);
prg:
    OCLERROR_RET(clReleaseProgram(program), end_error, ker);
ker:
    free(kernel);
que:
    OCLERROR_RET(clReleaseCommandQueue(queue), end_error, cont);
cont:
    OCLERROR_RET(clReleaseContext(context), end_error, end);
end:
    if (error) cl_util_print_error(error);
    return error;
}
