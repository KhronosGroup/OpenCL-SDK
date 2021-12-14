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
#include <CL/Utils/Event.h>

// STL includes
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <limits.h>

// Sample-specific option
struct options_Binaries
{
    size_t start;
    size_t length;
};

cag_option BinariesOptions[] = { { .identifier = 'l',
                                   .access_letters = "l",
                                   .access_name = "length",
                                   .value_name = "(positive integer)",
                                   .description = "Length of range to test" },

                                 { .identifier = 's',
                                   .access_letters = "s",
                                   .access_name = "start",
                                   .value_name = "(positive integer)",
                                   .description = "Starting number" } };

ParseState parse_BinariesOptions(const char identifier,
                                 cag_option_context *cag_context,
                                 struct options_Binaries *opts)
{
    const char *value;

    switch (identifier)
    {
        case 's':
            if ((value = cag_option_get_value(cag_context)))
            {
                opts->start = strtoull(value, NULL, 0);
                return ParsedOK;
            }
            else
                return ParseError;
        case 'l':
            if ((value = cag_option_get_value(cag_context)))
            {
                opts->length = strtoull(value, NULL, 0);
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
                     struct options_Binaries *binaries_opts)
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
    MEM_CHECK(tmp = add_CLI_options(opts, &n, BinariesOptions,
                                    CAG_ARRAY_SIZE(BinariesOptions)),
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
        PARS_OPTIONS(
            parse_BinariesOptions(identifier, &cag_context, binaries_opts),
            state);

        if (identifier == 'h')
        {
            printf("Usage: binaries [OPTION]...\n");
            printf("Option name and value should be separated by '=' or a "
                   "space\n");
            printf("Demonstrates saving and loading of compiled binary OpenCL "
                   "programs for specific device.\n\n");
            cag_option_print(opts, n, stdout);
            exit((state == ParseError) ? CL_INVALID_ARG_VALUE : CL_SUCCESS);
        }
    }

end:
    free(opts);
    return error;
}


cl_int print_timings(struct timespec start, struct timespec end,
                     const cl_event *const event_list, cl_uint event_number)
{
    cl_int error = CL_SUCCESS;

    cl_ulong time_host, time_device = 0;
    TIMER_DIFFERENCE(time_host, start, end)
    for (cl_uint i = 0; i < event_number; ++i)
    {
        time_device += cl_util_get_event_duration(
            event_list[i], CL_PROFILING_COMMAND_START, CL_PROFILING_COMMAND_END,
            &error);
        if (error != CL_SUCCESS) return error;
    }
    printf("Execution time as seen by host: %llu us, by device: %llu us\n",
           (unsigned long long)(time_host + 500) / 1000,
           (unsigned long long)(time_device + 500) / 1000);

    return error;
}


int main(int argc, char *argv[])
{
    cl_int error = CL_SUCCESS;
    cl_int end_error = CL_SUCCESS;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;

    cl_program program;

    /// Parse command-line options
    struct cl_sdk_options_Diagnostic diag_opts = { .quiet = false,
                                                   .verbose = false };
    struct cl_sdk_options_SingleDevice dev_opts = {
        .triplet = { 0, 0, CL_DEVICE_TYPE_ALL }
    };
    struct options_Binaries binaries_opts = { .start = 1, .length = 100000 };

    OCLERROR_RET(
        parse_options(argc, argv, &diag_opts, &dev_opts, &binaries_opts), error,
        end);

    /// Create runtime objects based on user preference or default
    OCLERROR_PAR(device = cl_util_get_device(dev_opts.triplet.plat_index,
                                             dev_opts.triplet.dev_index,
                                             dev_opts.triplet.dev_type, &error),
                 error, end);
    OCLERROR_PAR(context =
                     clCreateContext(NULL, 1, &device, NULL, NULL, &error),
                 error, end);

    if (!diag_opts.quiet) cl_util_print_device_info(device);

    /// Try to read binary
    program = cl_util_read_binaries(context, &device, 1, "Collatz", &error);

    if (error != CL_SUCCESS)
    { // if binary not present, compile and save
        const char *kernel_location = "./Collatz.cl";
        char *kernel = NULL;
        size_t program_size = 0;
        char *options = NULL;

        OCLERROR_PAR(kernel = cl_util_read_text_file(kernel_location,
                                                     &program_size, &error),
                     error, cont);
        printf("OpenCL file red... ");

        OCLERROR_PAR(program = clCreateProgramWithSource(context, 1,
                                                         (const char **)&kernel,
                                                         &program_size, &error),
                     error, ker);
        OCLERROR_RET(cl_util_build_program(program, device, options), error,
                     prgs);

        OCLERROR_RET(cl_util_write_binaries(program, "Collatz"), error, prgs);
        printf("Binary file written.\n\n");

    prgs:
        OCLERROR_RET(clReleaseProgram(program), end_error, que);
    ker:
        free(kernel);

        OCLERROR_PAR(program = cl_util_read_binaries(context, &device, 1,
                                                     "Collatz", &error),
                     error, cont);
    }

    // if the binary is already present - calculate
    printf("Saved program found\n");
    OCLERROR_RET(cl_util_build_program(program, device, NULL), error, prg);

    /// Create all remaining runtime objects
    OCLERROR_RET(clGetDeviceInfo(device, CL_DEVICE_PLATFORM,
                                 sizeof(cl_platform_id), &platform, NULL),
                 error, prg);
#if CL_HPP_TARGET_OPENCL_VERSION >= 200
    cl_command_queue_properties props[] = { CL_QUEUE_PROPERTIES,
                                            CL_QUEUE_PROFILING_ENABLE, 0 };
    OCLERROR_PAR(queue = clCreateCommandQueueWithProperties(context, device,
                                                            props, &error),
                 error, prg);
#else
    OCLERROR_PAR(queue = clCreateCommandQueue(
                     context, device, CL_QUEUE_PROFILING_ENABLE, &error),
                 error, prg);
#endif

    cl_kernel Collatz;
    OCLERROR_PAR(Collatz = clCreateKernel(program, "Collatz", &error), error,
                 que);

    const size_t length = binaries_opts.length;
    const size_t start = binaries_opts.start - 1;
    /// Prepare vector of values to extract results
    cl_int *v = NULL;
    MEM_CHECK(v = (cl_int *)malloc(sizeof(cl_int) * length), error, col);

    /// Initialize device-side storage
    cl_mem buf;
    OCLERROR_PAR(buf = clCreateBuffer(context,
                                      CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY,
                                      sizeof(cl_int) * length, NULL, &error),
                 error, vec);

    /// Run kernel
    cl_event pass;
    OCLERROR_RET(clSetKernelArg(Collatz, 0, sizeof(cl_mem), &buf), error, buff);

    GET_CURRENT_TIMER(start_time)
    OCLERROR_RET(clEnqueueNDRangeKernel(queue, Collatz, 1, &start, &length,
                                        NULL, 0, NULL, &pass),
                 error, buff);
    OCLERROR_RET(clWaitForEvents(1, &pass), error, buff);
    GET_CURRENT_TIMER(end_time)

    if (diag_opts.verbose) print_timings(start_time, end_time, &pass, 1);

    OCLERROR_RET(clEnqueueReadBuffer(queue, buf, CL_BLOCKING, 0,
                                     sizeof(cl_int) * length, v, 0, NULL, NULL),
                 error, buff);

    /// Show results
    int max_steps = 0;
    size_t max_ind = -1;
    for (size_t i = 0; i < length; ++i)
        if (v[i] < 0)
        {
            fprintf(stderr, "Number %zu gets out of 64 bits at step %i\n",
                    start + 1 + i, -v[i]);
        }
        else if ((v[i] == 0) && (start + i != 0))
        {
            fprintf(stderr, "Number %zu did not converge to 1 at step %i\n",
                    start + 1 + i, INT_MAX - 2);
        }
        else if (v[i] > max_steps)
        {
            max_steps = v[i];
            max_ind = start + 1 + i;
        }
    printf("From %zu numbers checked starting from %zu, maximum %i steps was "
           "needed to get to 1 for number %zu\n",
           length, start + 1, max_steps, max_ind);

    /// Cleanup
buff:
    OCLERROR_RET(clReleaseMemObject(buf), end_error, vec);
vec:
    free(v);
col:
    OCLERROR_RET(clReleaseKernel(Collatz), end_error, que);
que:
    OCLERROR_RET(clReleaseCommandQueue(queue), end_error, prg);
prg:
    OCLERROR_RET(clReleaseProgram(program), end_error, cont);
cont:
    OCLERROR_RET(clReleaseContext(context), end_error, end);
end:
    if (error != CL_SUCCESS) cl_util_print_error(error);
    return error;
}
