/*
 * Copyright (c) 2023 The Khronos Group Inc.
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

// OpenCL SDK includes.
#include <CL/SDK/CLI.h>
#include <CL/SDK/Context.h>
#include <CL/SDK/Options.h>
#include <CL/SDK/Random.h>

// OpenCL Utils includes.
#include <CL/Utils/Error.h>
#include <CL/Utils/Event.h>
#include <CL/Utils/Utils.h>

// Standard header includes.
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Sample-specific options.
struct convolution_options
{
    size_t x_dim;
    size_t y_dim;
};

// Add option to CLI-parsing SDK utility for input dimensions.
cag_option ConvolutionOptions[] = { { .identifier = 'x',
                                      .access_letters = "x",
                                      .access_name = "x_dim",
                                      .value_name = "(positive integral)",
                                      .description = "x dimension of input" },

                                    { .identifier = 'y',
                                      .access_letters = "y",
                                      .access_name = "y_dim",
                                      .value_name = "(positive integral)",
                                      .description = "y dimension of input" } };

ParseState parse_ConvolutionOptions(const char identifier,
                                    cag_option_context* cag_context,
                                    struct convolution_options* opts)
{
    const char* value;

    switch (identifier)
    {
        case 'x':
            if ((value = cag_option_get_value(cag_context)))
            {
                opts->x_dim = strtoull(value, NULL, 0);
                return ParsedOK;
            }
            else
                return ParseError;
        case 'y':
            if ((value = cag_option_get_value(cag_context)))
            {
                opts->y_dim = strtoull(value, NULL, 0);
                return ParsedOK;
            }
            else
                return ParseError;
    }
    return NotParsed;
}

cl_int parse_options(int argc, char* argv[],
                     struct cl_sdk_options_Diagnostic* diag_opts,
                     struct cl_sdk_options_SingleDevice* dev_opts,
                     struct convolution_options* convolution_opts)
{
    cl_int error = CL_SUCCESS;
    struct cag_option *opts = NULL, *tmp = NULL;
    size_t n = 0;

    // Prepare options array.
    MEM_CHECK(opts = add_CLI_options(opts, &n, DiagnosticOptions,
                                     CAG_ARRAY_SIZE(DiagnosticOptions)),
              error, end);
    MEM_CHECK(tmp = add_CLI_options(opts, &n, SingleDeviceOptions,
                                    CAG_ARRAY_SIZE(SingleDeviceOptions)),
              error, end);
    opts = tmp;
    MEM_CHECK(tmp = add_CLI_options(opts, &n, ConvolutionOptions,
                                    CAG_ARRAY_SIZE(ConvolutionOptions)),
              error, end);
    opts = tmp;

    char identifier;
    cag_option_context cag_context;

    // Prepare the context and iterate over all options.
    cag_option_prepare(&cag_context, opts, n, argc, argv);
    while (cag_option_fetch(&cag_context))
    {
        ParseState state = NotParsed;
        identifier = cag_option_get(&cag_context);

        PARS_OPTIONS(parse_DiagnosticOptions(identifier, diag_opts), state);
        PARS_OPTIONS(
            parse_SingleDeviceOptions(identifier, &cag_context, dev_opts),
            state);
        PARS_OPTIONS(parse_ConvolutionOptions(identifier, &cag_context,
                                              convolution_opts),
                     state);

        if (identifier == 'h')
        {
            printf("Usage: dev_optsdevice [OPTION]...\n");
            printf("Option name and value should be separated by '=' or a "
                   "space\n");
            printf("Demonstrates convolution calculation with two "
                   "(sub)devices.\n\n");
            cag_option_print(opts, n, stdout);
            exit((state == ParseError) ? CL_INVALID_ARG_VALUE : CL_SUCCESS);
        }
    }

end:
    free(opts);
    return error;
}

// Host-side implementation of the convolution for verification. Padded input
// assumed.
void host_convolution(const cl_float* in, cl_float* out, const cl_float* mask,
                      const cl_uint x_dim, const cl_uint y_dim)
{
    const cl_uint mask_dim = 3;
    const cl_uint pad_width = mask_dim / 2;
    const cl_uint in_dim_x = x_dim + pad_width * 2;

    for (cl_uint gid_x = 0; gid_x < x_dim; ++gid_x)
    {
        for (cl_uint gid_y = 0; gid_y < y_dim; ++gid_y)
        {
            float result = 0.f;
            for (cl_uint y = 0; y < mask_dim; ++y)
            {
                for (cl_uint x = 0; x < mask_dim; ++x)
                {
                    result += mask[y * mask_dim + x]
                        * in[(gid_y + y) * in_dim_x + (gid_x + x)];
                }
            }
            out[gid_y * x_dim + gid_x] = result;
        }
    }
}

cl_int opencl_version_contains(const char* dev_version,
                               const char* version_fragment)
{
    char* found_version = strstr(dev_version, version_fragment);
    return (found_version != NULL);
}

int main(int argc, char* argv[])
{
    cl_int error = CL_SUCCESS;
    cl_int end_error = CL_SUCCESS;
    cl_device_id dev;
    cl_context context;
    cl_program program;
    cl_mem dev_input_grid, dev_output_grid, dev_mask;

    cl_kernel convolutions[2] = { 0 };
    cl_command_queue sub_queues[2] = { 0 };
    cl_mem sub_input_grids[2] = { 0 };
    cl_mem sub_output_grids[2] = { 0 };
    cl_event events[2] = { 0 };

    // Parse command-line options.
    struct cl_sdk_options_Diagnostic diag_opts = { .quiet = false,
                                                   .verbose = false };

    // By default assume that there is only one device available.
    // dev_opts->number is set to 1 so that when calling to cl_util_get_device
    // for the second device there is no index out of range.
    struct cl_sdk_options_SingleDevice dev_opts = {
        .triplet = { 0, 0, CL_DEVICE_TYPE_ALL }
    };
    struct convolution_options convolution_opts = { .x_dim = 4096,
                                                    .y_dim = 4096 };

    OCLERROR_RET(
        parse_options(argc, argv, &diag_opts, &dev_opts, &convolution_opts),
        error, end);

    // Create runtime objects based on user preference or default.
    OCLERROR_PAR(dev = cl_util_get_device(dev_opts.triplet.plat_index,
                                          dev_opts.triplet.dev_index,
                                          dev_opts.triplet.dev_type, &error),
                 error, end);

    if (!diag_opts.quiet)
    {
        cl_util_print_device_info(dev);
    }

    if (diag_opts.verbose)
    {
        printf("Creating sub-devices...");
        fflush(stdout);
    }

    // Query OpenCL version supported by device.
    size_t dev_version_size;

    OCLERROR_RET(
        clGetDeviceInfo(dev, CL_DEVICE_VERSION, 0, NULL, &dev_version_size),
        error, end);

    char compiler_options[1023] = "";
    char* dev_version = NULL;
    MEM_CHECK(dev_version = (char*)malloc(dev_version_size), error, end);

    OCLERROR_RET(clGetDeviceInfo(dev, CL_DEVICE_VERSION, dev_version_size,
                                 dev_version, NULL),
                 error, ver);

    if (opencl_version_contains(dev_version, "1.0")
        || opencl_version_contains(dev_version, "1.1"))
    {
        fprintf(stdout,
                "This sample requires device partitioning, which is an OpenCL "
                "1.2 feature, but the device chosen only supports OpenCL %s. "
                "Please try with a different OpenCL device instead.\n",
                dev_version);
        error = CL_SUCCESS;
        goto ver;
    }
    else
    {
        // If no -cl-std option is specified then the highest 1.x version
        // supported by each device is used to compile the program. Therefore,
        // it's only necessary to add the -cl-std option for 2.0 and 3.0 OpenCL
        // versions.

        int written = 0;
        if (opencl_version_contains(dev_version, "3."))
        {
            written = snprintf(compiler_options, sizeof(compiler_options),
                               "-cl-std=CL3.0 ");
        }
        else if (opencl_version_contains(dev_version, "2."))
        {
            written = snprintf(compiler_options, sizeof(compiler_options),
                               "-cl-std=CL2.0 ");
        }

        if (written < 0 || written >= (int)sizeof(compiler_options))
        {
            fprintf(
                stderr,
                "Error: compiler_options buffer overflow or encoding error.\n");
            free(dev_version);
            exit(EXIT_FAILURE);
        }
    }

    // Check if device supports fission.
    cl_device_partition_property* dev_props = NULL;
    size_t props_size = 0;
    OCLERROR_RET(clGetDeviceInfo(dev, CL_DEVICE_PARTITION_PROPERTIES, 0, NULL,
                                 &props_size),
                 error, ver);
    if (props_size == 0)
    {
        fprintf(stdout,
                "This sample requires device fission, which is a "
                "feature available from OpenCL 1.2 on, but the "
                "device chosen does not seem to support it. Please "
                "try with a different OpenCL device instead.\n");
        exit(EXIT_SUCCESS);
    }

    // Check if the "partition equally" type is supported.
    MEM_CHECK(dev_props = (cl_device_partition_property*)malloc(props_size),
              error, ver);
    OCLERROR_RET(clGetDeviceInfo(dev, CL_DEVICE_PARTITION_PROPERTIES,
                                 props_size, dev_props, NULL),
                 error, props);
    size_t prop = 0,
           props_length = props_size / sizeof(cl_device_partition_property);
    for (; prop < props_length; ++prop)
    {
        if (dev_props[prop] == CL_DEVICE_PARTITION_EQUALLY)
        {
            break;
        }
    }
    if (prop == props_length)
    {
        fprintf(stdout,
                "This sample requires partition equally, which is a "
                "partition scheme available from OpenCL 1.2 on, but "
                "the device chosen does not seem to support it. "
                "Please try with a different OpenCL device instead.\n");
        exit(EXIT_SUCCESS);
    }

    // Create sub-devices, each with half of the compute units available.
    cl_uint max_compute_units = 0;
    cl_uint subdev_created = 0;
    const cl_uint subdev_count = 2;
    OCLERROR_RET(clGetDeviceInfo(dev, CL_DEVICE_MAX_COMPUTE_UNITS,
                                 sizeof(cl_uint), &max_compute_units, NULL),
                 error, props);
    cl_device_partition_property subdevices_properties[] = {
        (cl_device_partition_property)CL_DEVICE_PARTITION_EQUALLY,
        (cl_device_partition_property)(max_compute_units / subdev_count), 0
    };

    cl_device_id* subdevices =
        (cl_device_id*)malloc(subdev_count * sizeof(cl_device_id));

    OCLERROR_RET(clCreateSubDevices(dev, subdevices_properties, subdev_count,
                                    subdevices, &subdev_created),
                 error, props);

    if (subdev_created < subdev_count)
    {
        fprintf(stderr,
                "Error: OpenCL cannot create the number of sub-devices "
                "requested\n");
        exit(EXIT_FAILURE);
    }

    OCLERROR_PAR(context = clCreateContext(NULL, subdev_count, subdevices, NULL,
                                           NULL, &error),
                 error, subdev1);

    // Read kernel file.
    const char* kernel_location = "./convolution.cl";
    char *kernel = NULL, *tmp = NULL;
    size_t program_size = 0;
    OCLERROR_PAR(
        kernel = cl_util_read_text_file(kernel_location, &program_size, &error),
        error, contx);
    MEM_CHECK(tmp = (char*)realloc(kernel, program_size), error, ker);
    kernel = tmp;

    // Compile kernel.
    if (diag_opts.verbose)
    {
        printf("done.\nCompiling kernel...");
        fflush(stdout);
    }
    OCLERROR_PAR(program = clCreateProgramWithSource(
                     context, 1, (const char**)&kernel, &program_size, &error),
                 error, ker);

    OCLERROR_RET(
        clBuildProgram(program, 2, subdevices, compiler_options, NULL, NULL),
        error, prg);

    // Initialize host-side storage.
    const cl_uint mask_dim = 3;
    const cl_uint pad_width = mask_dim / 2;
    const size_t x_dim = convolution_opts.x_dim;
    const size_t y_dim = convolution_opts.y_dim;
    const size_t pad_x_dim = x_dim + 2 * pad_width;
    const size_t pad_y_dim = y_dim + 2 * pad_width;

    const size_t input_bytes = sizeof(cl_float) * pad_x_dim * pad_y_dim;
    const size_t output_bytes = sizeof(cl_float) * x_dim * y_dim;
    const size_t mask_bytes = sizeof(cl_float) * mask_dim * mask_dim;

    if (diag_opts.verbose)
    {
        printf("done.\nInitializing host-side storage...\n");
        fflush(stdout);
    }

    // Random number generator.
    pcg32_random_t rng;
    pcg32_srandom_r(&rng, 11111, -2222);

    cl_float* h_input_grid;
    cl_float* h_output_grid;
    cl_float* h_mask;

    // Initialize input matrix. The input will be padded to remove
    // conditional branches from the convolution kernel for determining
    // out-of-bounds.
    MEM_CHECK(h_input_grid = (cl_float*)malloc(input_bytes), error, prg);
    if (diag_opts.verbose)
    {
        printf("  Generating %zu random numbers for convolution input grid...",
               x_dim * y_dim);
        fflush(stdout);
    }
    cl_sdk_fill_with_random_ints_range(&rng, (cl_int*)h_input_grid,
                                       pad_x_dim * pad_y_dim, -1000, 1000);
    // Fill with 0s the extra rows and columns added for padding.
    for (size_t y = 0; y < pad_y_dim; ++y)
    {
        for (size_t x = 0; x < pad_x_dim; ++x)
        {
            if (x == 0 || y == 0 || x == (pad_x_dim - 1)
                || y == (pad_y_dim - 1))
            {
                h_input_grid[y * pad_x_dim + x] = 0;
            }
        }
    }

    // Declare output matrix. Output will not be padded.
    MEM_CHECK(h_output_grid = (cl_float*)malloc(output_bytes), error, hinput);

    // Initialize convolution mask.
    MEM_CHECK(h_mask = (cl_float*)malloc(mask_bytes), error, houtput);
    if (diag_opts.verbose)
    {
        printf("done.\n  Generating %u random numbers for convolution mask...",
               mask_dim * mask_dim);
        fflush(stdout);
    }
    cl_sdk_fill_with_random_ints_range(&rng, (cl_int*)h_mask,
                                       mask_dim * mask_dim, -1000, 1000);

    // Create device buffers, from which we will create the subbuffers for the
    // sub-devices.
    const size_t grid_midpoint = y_dim / 2;
    const size_t pad_grid_midpoint = pad_y_dim / 2;

    if (diag_opts.verbose)
    {
        printf("done.\nInitializing device-side storage...");
        fflush(stdout);
    }

    OCLERROR_PAR(dev_input_grid =
                     clCreateBuffer(context,
                                    CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR
                                        | CL_MEM_HOST_NO_ACCESS,
                                    input_bytes, h_input_grid, &error),
                 error, hmask);
    OCLERROR_PAR(dev_output_grid =
                     clCreateBuffer(context,
                                    CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR
                                        | CL_MEM_HOST_READ_ONLY,
                                    output_bytes, NULL, &error),
                 error, bufin);
    OCLERROR_PAR(dev_mask =
                     clCreateBuffer(context,
                                    CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR
                                        | CL_MEM_HOST_NO_ACCESS,
                                    mask_bytes, h_mask, &error),
                 error, bufout);

    if (diag_opts.verbose)
    {
        printf("done.\nSetting up sub-devices...");
        fflush(stdout);
    }

    // Set up sub-devices for kernel execution.
    const size_t half_input_bytes =
        sizeof(cl_float) * pad_x_dim * (pad_grid_midpoint + 1);
    const size_t input_offset =
        sizeof(cl_float) * pad_x_dim * (pad_grid_midpoint - 1);
    const size_t half_output_bytes = sizeof(cl_float) * x_dim * grid_midpoint;

    cl_uint subdevice = 0;
    for (; subdevice < subdev_count; ++subdevice)
    {
        // Create kernel.
        if (diag_opts.verbose)
        {
            printf("\n  Creating kernel and command queue of sub-device %d...",
                   subdevice);
            fflush(stdout);
        }

        OCLERROR_PAR(convolutions[subdevice] =
                         clCreateKernel(program, "convolution_3x3", &error),
                     error, bufmask);

        // Initialize queues for command execution on each device.
#if defined(CL_VERSION_2_0) || defined(CL_VERSION_3_0)
        cl_command_queue_properties props[] = { CL_QUEUE_PROPERTIES,
                                                CL_QUEUE_PROFILING_ENABLE, 0 };
        OCLERROR_PAR(sub_queues[subdevice] = clCreateCommandQueueWithProperties(
                         context, subdevices[subdevice], props, &error),
                     error, conv);
#else
        OCLERROR_PAR(sub_queues[subdevice] = clCreateCommandQueue(
                         context, subdevices[subdevice],
                         CL_QUEUE_PROFILING_ENABLE, &error),
                     error, conv);
#endif

        // Initialize device-side storage.
        // First device performs the convolution in the upper half and second
        // device in the lower half (middle borders included).
        if (diag_opts.verbose)
        {
            printf("done.\n  Initializing device-side storage of sub-device "
                   "%d...",
                   subdevice);
            fflush(stdout);
        }

        cl_buffer_region input_region = { subdevice * input_offset,
                                          half_input_bytes },
                         output_region = { subdevice * half_output_bytes,
                                           half_output_bytes };
        OCLERROR_PAR(sub_input_grids[subdevice] = clCreateSubBuffer(
                         dev_input_grid, CL_MEM_READ_ONLY,
                         CL_BUFFER_CREATE_TYPE_REGION, &input_region, &error),
                     error, subqueue);
        OCLERROR_PAR(sub_output_grids[subdevice] = clCreateSubBuffer(
                         dev_output_grid, CL_MEM_WRITE_ONLY,
                         CL_BUFFER_CREATE_TYPE_REGION, &output_region, &error),
                     error, subbufin);

        if (diag_opts.verbose)
        {
            printf("done.");
            fflush(stdout);
        }

        // Set kernels arguments.
        OCLERROR_RET(clSetKernelArg(convolutions[subdevice], 0, sizeof(cl_mem),
                                    &sub_input_grids[subdevice]),
                     error, subbufout);
        OCLERROR_RET(clSetKernelArg(convolutions[subdevice], 1, sizeof(cl_mem),
                                    &sub_output_grids[subdevice]),
                     error, subbufout);
        OCLERROR_RET(clSetKernelArg(convolutions[subdevice], 2, sizeof(cl_mem),
                                    &dev_mask),
                     error, subbufout);
        cl_uint2 output_dimensions;
        output_dimensions.x = (cl_uint)x_dim;
        output_dimensions.y = (cl_uint)grid_midpoint;
        OCLERROR_RET(clSetKernelArg(convolutions[subdevice], 3,
                                    sizeof(cl_uint2), &output_dimensions),
                     error, subbufout);
    }

    // Launch kernels.
    if (diag_opts.verbose)
    {
        printf("\nExecuting on device... ");
        fflush(stdout);
    }

    // Enqueue kernel calls and wait for them to finish.
    const size_t* global = (size_t[]){ x_dim, y_dim };

    GET_CURRENT_TIMER(dev_start)
    OCLERROR_RET(clEnqueueNDRangeKernel(sub_queues[0], convolutions[0], 2, NULL,
                                        global, NULL, 0, NULL, &events[0]),
                 error, subbufout);
    OCLERROR_RET(clEnqueueNDRangeKernel(sub_queues[1], convolutions[1], 2, NULL,
                                        global, NULL, 0, NULL, &events[1]),
                 error, event1);

    OCLERROR_RET(clWaitForEvents(1, &events[0]), error, event1);
    OCLERROR_RET(clWaitForEvents(1, &events[1]), error, event2);
    GET_CURRENT_TIMER(dev_end)
    size_t dev_time;
    TIMER_DIFFERENCE(dev_time, dev_start, dev_end)

    // Compute reference host-side convolution.
    if (diag_opts.verbose)
    {
        printf("done.\nExecuting on host... ");
        fflush(stdout);
    }

    GET_CURRENT_TIMER(host_start)
    host_convolution(h_input_grid, h_output_grid, h_mask, (cl_uint)x_dim,
                     (cl_uint)y_dim);
    GET_CURRENT_TIMER(host_end)
    size_t host_time;
    TIMER_DIFFERENCE(host_time, host_start, host_end)

    if (diag_opts.verbose)
    {
        printf("done.\n");
        fflush(stdout);
    }

    // Fetch and combine results from devices.
    cl_float* concatenated_results;
    const size_t mid_output_count = x_dim * grid_midpoint;
    const size_t mid_output_bytes = sizeof(cl_float) * mid_output_count;
    MEM_CHECK(concatenated_results = (cl_float*)malloc(output_bytes), error,
              event2);
    for (cl_uint i = 0; i < subdev_count; ++i)
    {
        OCLERROR_RET(
            clEnqueueReadBuffer(sub_queues[i], sub_output_grids[i], CL_BLOCKING,
                                0, mid_output_bytes,
                                &concatenated_results[i * mid_output_count], 0,
                                NULL, NULL),
            error, result);
    }

    // Validate device-side solution.
    cl_float deviation = 0.f;
    const cl_float tolerance = 1e-6;

    for (size_t i = 0; i < x_dim * y_dim; ++i)
    {
        deviation += fabs(concatenated_results[i] - h_output_grid[i]);
    }
    deviation /= (x_dim * y_dim);

    if (deviation > tolerance)
    {
        printf("Failed convolution! Normalized deviation %.6f between host and "
               "device exceeds tolerance %.6f\n",
               deviation, tolerance);
        fflush(stdout);
    }
    else
    {
        printf("Successful convolution!\n");
        fflush(stdout);
    }

    if (!diag_opts.quiet)
    {
        printf("Kernels execution time as seen by host: %llu us.\n",
               (unsigned long long)(dev_time + 500) / 1000);

        printf("Kernels execution time as measured by devices :\n");
        printf("\t%llu us.\n",
               (unsigned long long)(cl_util_get_event_duration(
                                        events[0], CL_PROFILING_COMMAND_START,
                                        CL_PROFILING_COMMAND_END, &error)
                                    + 500)
                   / 1000);
        printf("\t%llu us.\n",
               (unsigned long long)(cl_util_get_event_duration(
                                        events[1], CL_PROFILING_COMMAND_START,
                                        CL_PROFILING_COMMAND_END, &error)
                                    + 500)
                   / 1000);

        printf("Reference execution as seen by host: %llu us.\n",
               (unsigned long long)(host_time + 500) / 1000);
        fflush(stdout);
    }

result:
    free(concatenated_results);
event2:
    OCLERROR_RET(clReleaseEvent(events[1]), end_error, event1);
event1:
    OCLERROR_RET(clReleaseEvent(events[0]), end_error, subbufout);
subbufout:
    if (subdevice >= 1)
    {
        OCLERROR_RET(clReleaseMemObject(sub_output_grids[1]), end_error,
                     subbufout0);
    }
subbufout0:
    OCLERROR_PAR(clReleaseMemObject(sub_output_grids[0]), end_error, subbufin);
subbufin:
    if (subdevice >= 1)
    {
        OCLERROR_RET(clReleaseMemObject(sub_input_grids[1]), end_error,
                     subbufin0);
    }
subbufin0:
    OCLERROR_RET(clReleaseMemObject(sub_input_grids[0]), end_error, subqueue);
subqueue:
    if (subdevice >= 1)
    {
        OCLERROR_RET(clReleaseCommandQueue(sub_queues[1]), end_error,
                     subqueue0);
    }
subqueue0:
    OCLERROR_RET(clReleaseCommandQueue(sub_queues[0]), end_error, conv);
conv:
    if (subdevice >= 1)
    {
        OCLERROR_RET(clReleaseKernel(convolutions[1]), end_error, conv0);
    }
conv0:
    OCLERROR_RET(clReleaseKernel(convolutions[0]), end_error, bufmask);
bufmask:
    OCLERROR_RET(clReleaseMemObject(dev_mask), end_error, bufout);
bufout:
    OCLERROR_RET(clReleaseMemObject(dev_output_grid), end_error, bufin);
bufin:
    OCLERROR_RET(clReleaseMemObject(dev_input_grid), end_error, hmask);
hmask:
    free(h_mask);
houtput:
    free(h_output_grid);
hinput:
    free(h_input_grid);
prg:
    OCLERROR_RET(clReleaseProgram(program), end_error, ker);
ker:
    free(kernel);
contx:
    OCLERROR_RET(clReleaseContext(context), end_error, subdev1);
subdev1:
    OCLERROR_RET(clReleaseDevice(subdevices[1]), end_error, subdev0);
subdev0:
    OCLERROR_RET(clReleaseDevice(subdevices[0]), end_error, subdevs);
subdevs:
    free(subdevices);
props:
    free(dev_props);
ver:
    if (dev_version) free(dev_version);
end:
    if (error) cl_util_print_error(error);
    return error;
}
