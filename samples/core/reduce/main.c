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
#include <CL/Utils/Event.h>

// STL includes
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

// Sample-specific option
struct options_Reduce
{
    size_t length;
    const char *op;
};

cag_option ReduceOptions[] = { { .identifier = 'l',
                                 .access_letters = "l",
                                 .access_name = "length",
                                 .value_name = "(positive integer)",
                                 .description = "Length of input" },

                               { .identifier = 'o',
                                 .access_letters = "o",
                                 .access_name = "op",
                                 .value_name = "(min|sum)",
                                 .description = "Operation to perform" } };

ParseState parse_ReduceOptions(const char identifier,
                               cag_option_context *cag_context,
                               struct options_Reduce *opts)
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
        case 'o':
            if ((value = cag_option_get_value(cag_context))
                && (!strcmp(value, "min") || !strcmp(value, "sum")))
            {
                opts->op = value;
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
                     struct options_Reduce *reduce_opts)
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
    MEM_CHECK(tmp = add_CLI_options(opts, &n, ReduceOptions,
                                    CAG_ARRAY_SIZE(ReduceOptions)),
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
        PARS_OPTIONS(parse_ReduceOptions(identifier, &cag_context, reduce_opts),
                     state);

        if (identifier == 'h')
        {
            printf("Usage: reduce [OPTION]...\n");
            printf("Demonstrates how to query various OpenCL extensions "
                   "applicable "
                   "in the context of a reduction algorithm and to touch up "
                   "kernel sources "
                   "at runtime to select the best kernel implementation for "
                   "the task.\n\n");
            cag_option_print(opts, n, stdout);
            exit((state == ParseError) ? CL_INVALID_ARG_VALUE : CL_SUCCESS);
        }
    }

end:
    free(opts);
    return error;
}

int check_use_work_group_reduce(cl_platform_id platform, cl_device_id device,
                                cl_int *error)
{
    int res = 0;
    char *name = NULL;

    OCLERROR_PAR(
        name = cl_util_get_platform_info(platform, CL_PLATFORM_VERSION, error),
        *error, nam);

    if (strstr(name, "OpenCL 2."))
    {
        free(name);
        OCLERROR_PAR(name = cl_util_get_device_info(
                         device, CL_DEVICE_OPENCL_C_VERSION, error),
                     *error, nam);
        if (strstr(name, "OpenCL C 2.")) res = 2;
    }
    else if (strstr(name, "OpenCL 3."))
    {
        cl_bool coll_func;
        OCLERROR_RET(
            clGetDeviceInfo(device,
                            CL_DEVICE_WORK_GROUP_COLLECTIVE_FUNCTIONS_SUPPORT,
                            sizeof(cl_bool), &coll_func, NULL),
            *error, nam);

        if (coll_func)
        {
            cl_name_version *c_features = NULL;
            size_t n = 0;

            OCLERROR_RET(clGetDeviceInfo(device, CL_DEVICE_OPENCL_C_FEATURES, 0,
                                         NULL, &n),
                         *error, nam);
            MEM_CHECK(c_features = (cl_name_version *)malloc(sizeof(char) * n),
                      *error, nam);
            OCLERROR_RET(clGetDeviceInfo(device, CL_DEVICE_OPENCL_C_FEATURES,
                                         sizeof(char) * n, c_features, NULL),
                         *error, cf);

            const size_t feat = sizeof(char) * n / sizeof(cl_name_version);
            for (size_t i = 0; i < feat; ++i)
                if (strstr(c_features[i].name,
                           "__opencl_c_work_group_collective_functions"))
                {
                    res = 3;
                    break;
                }
        cf:
            free(c_features);
        }
    }
nam:
    free(name);
    return res;
}

int check_use_sub_group_reduce(cl_platform_id platform, cl_device_id device,
                               cl_int *error)
{
    int res = 0;
    char *name = NULL;

    OCLERROR_PAR(
        name = cl_util_get_platform_info(platform, CL_PLATFORM_VERSION, error),
        *error, nam);

    if (strstr(name, "OpenCL 3."))
    {
        free(name);
        OCLERROR_PAR(
            name = cl_util_get_device_info(device, CL_DEVICE_EXTENSIONS, error),
            *error, nam);
        if (strstr(name, "cl_khr_subgroups")) res = 3;
    }
nam:
    free(name);
    return res;
}

cl_int min_op(cl_int x, cl_int y) { return x < y ? x : y; }

cl_int plus_op(cl_int x, cl_int y) { return x + y; }

cl_int accumulate(cl_int *arr, size_t len, cl_int zero_elem,
                  cl_int (*host_op)(cl_int, cl_int))
{
    cl_int res = zero_elem;
    for (size_t i = len; i > 0; res = (*host_op)(res, arr[--i]))
        ;
    return res;
}

// Every pass reduces input length by 'factor'.
// If actual size is not divisible by factor,
// an extra output element is produced using some
// number of zero_elem inputs.
cl_ulong new_size(const cl_ulong actual, const cl_ulong factor)
{
    return actual / factor + (actual % factor == 0 ? 0 : 1);
};
// NOTE: because one work-group produces one output
//       new_size == number_of_work_groups
size_t global(const size_t actual, const cl_ulong factor, const size_t wgs)
{
    return new_size(actual, factor) * wgs;
};

// Random number generator state
pcg32_random_t rng;

int main(int argc, char *argv[])
{
    cl_int error = CL_SUCCESS, end_error = CL_SUCCESS;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;

    // Parse command-line options
    struct cl_sdk_options_Diagnostic diag_opts = { .quiet = false,
                                                   .verbose = false };
    struct cl_sdk_options_SingleDevice dev_opts = {
        .triplet = { 0, 0, CL_DEVICE_TYPE_ALL }
    };
    struct options_Reduce reduce_opts = { .length = 1048576, .op = "min" };

    OCLERROR_RET(parse_options(argc, argv, &diag_opts, &dev_opts, &reduce_opts),
                 error, end);

    // Create runtime objects based on user preference or default
    OCLERROR_PAR(device = cl_util_get_device(dev_opts.triplet.plat_index,
                                             dev_opts.triplet.dev_index,
                                             dev_opts.triplet.dev_type, &error),
                 error, end);
    OCLERROR_PAR(context =
                     clCreateContext(NULL, 1, &device, NULL, NULL, &error),
                 error, end);
    OCLERROR_RET(clGetDeviceInfo(device, CL_DEVICE_PLATFORM,
                                 sizeof(cl_platform_id), &platform, NULL),
                 error, cont);
#if CL_HPP_TARGET_OPENCL_VERSION >= 200
    cl_command_queue_properties props[] = { CL_QUEUE_PROPERTIES,
                                            CL_QUEUE_PROFILING_ENABLE, 0 };
    OCLERROR_PAR(queue = clCreateCommandQueueWithProperties(context, device,
                                                            props, &error),
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

    // Query device and runtime capabilities
    int may_use_work_group_reduce;
    OCLERROR_PAR(may_use_work_group_reduce =
                     check_use_work_group_reduce(platform, device, &error),
                 error, que);

    int may_use_sub_group_reduce;
    OCLERROR_PAR(may_use_sub_group_reduce =
                     check_use_sub_group_reduce(platform, device, &error),
                 error, que);

    if (diag_opts.verbose)
    {
        if (may_use_work_group_reduce)
            printf("Device supports work-group reduction intrinsics.\n");
        else if (may_use_sub_group_reduce)
            printf("Device supports sub-group reduction intrinsics.\n");
        else
            printf("Device doesn't support any reduction intrinsics.\n");
    }

    // User defined input
    char kernel_op[1023] = "";
    strcat(kernel_op,
           !strcmp(reduce_opts.op, "min")
               ? "int op(int lhs, int rhs) { return min(lhs, rhs); }\n"
               : "int op(int lhs, int rhs) { return lhs + rhs; }\n");
    if (may_use_work_group_reduce)
        strcat(kernel_op,
               !strcmp(reduce_opts.op, "min")
                   ? "int work_group_reduce_op(int val) { return "
                     "work_group_reduce_min(val); }\n"
                   : "int work_group_reduce_op(int val) { return "
                     "work_group_reduce_add(val); }\n");
    else if (may_use_sub_group_reduce)
        strcat(kernel_op,
               !strcmp(reduce_opts.op, "min")
                   ? "int sub_group_reduce_op(int val) { return "
                     "sub_group_reduce_min(val); }\n"
                   : "int sub_group_reduce_op(int val) { return "
                     "sub_group_reduce_add(val); }\n");
    cl_int (*host_op)(cl_int, cl_int) =
        !strcmp(reduce_opts.op, "min") ? &min_op : &plus_op;
    cl_int zero_elem = !strcmp(reduce_opts.op, "min") ? CL_INT_MAX : 0;

    // Compile kernel
    const char *kernel_location = "./reduce.cl";
    char *kernel = NULL, *tmp = NULL;
    size_t program_size = 0;
    cl_program program = NULL;

    OCLERROR_PAR(
        kernel = cl_util_read_text_file(kernel_location, &program_size, &error),
        error, que);
    // Note append of definitions
    program_size += 1 + strlen(kernel_op);
    MEM_CHECK(tmp = (char *)realloc(kernel, program_size), error, ker);
    strcat(tmp, kernel_op);

    kernel = tmp;
    // printf("%s", kernel);

    OCLERROR_PAR(program = clCreateProgramWithSource(
                     context, 1, (const char **)&kernel, &program_size, &error),
                 error, ker);
    kernel_op[0] = '\0';
    if (may_use_work_group_reduce == 2)
        strcat(kernel_op, "-D USE_WORK_GROUP_REDUCE -cl-std=CL2.0 ");
    else if (may_use_work_group_reduce == 3)
        strcat(kernel_op, "-D USE_WORK_GROUP_REDUCE -cl-std=CL3.0 ");
    else if (may_use_sub_group_reduce)
        strcat(kernel_op, "-D USE_SUB_GROUP_REDUCE -cl-std=CL3.0 ");

    OCLERROR_RET(cl_util_build_program(program, device, kernel_op), error, prg);

    cl_kernel reduce;
    OCLERROR_PAR(reduce = clCreateKernel(program, "reduce", &error), error,
                 prg);

    // Query maximum supported WGS of kernel on device based on private mem
    // (register) constraints
    size_t wgs, psm;
    OCLERROR_RET(clGetKernelWorkGroupInfo(reduce, device,
                                          CL_KERNEL_WORK_GROUP_SIZE,
                                          sizeof(size_t), &wgs, NULL),
                 error, red);
    OCLERROR_RET(
        clGetKernelWorkGroupInfo(reduce, device,
                                 CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
                                 sizeof(size_t), &psm, NULL),
        error, red);

    // Further constrain (reduce) WGS based on shared mem size on device
    cl_ulong loc_mem;
    OCLERROR_RET(clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_SIZE,
                                 sizeof(cl_ulong), &loc_mem, NULL),
                 error, red);
    while (loc_mem < wgs * 2 * sizeof(cl_int)) wgs -= psm;

    if (wgs == 0)
    {
        printf("Not enough local memory to serve a single sub-group.\n");
        error = CL_OUT_OF_RESOURCES;
        goto red;
    }

    cl_ulong factor = wgs * 2;

    // Initialize host-side storage
    const size_t length = reduce_opts.length;

    pcg32_srandom_r(&rng, 11111, -2222);
    cl_int *arr;
    MEM_CHECK(arr = (cl_int *)malloc(sizeof(cl_int) * length), error, red);

    if (diag_opts.verbose)
        printf("Generating %zu random numbers for reduction.\n", length);
    cl_sdk_fill_with_random_ints_range(&rng, arr, length, -1000, 1000);

    // Initialize device-side storage
    cl_mem front, back;
    OCLERROR_PAR(front = clCreateBuffer(
                     context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                     sizeof(cl_int) * length, arr, &error),
                 error, harr);
    OCLERROR_PAR(back = clCreateBuffer(
                     context, CL_MEM_READ_WRITE,
                     sizeof(cl_int) * new_size(length, factor), NULL, &error),
                 error, buff);

    // Launch kernels
    if (diag_opts.verbose)
    {
        printf("Executing on device... ");
        fflush(stdout);
    }

    cl_ulong curr = length;
    cl_uint steps = 0;
    while (curr > 1)
    {
        curr = new_size(curr, factor);
        ++steps;
    }
    cl_event *passes = NULL, *pass = NULL;
    MEM_CHECK(passes = (cl_event *)malloc(sizeof(cl_event) * steps), error,
              bufb);

    OCLERROR_RET(clSetKernelArg(reduce, 2, factor * sizeof(cl_int), NULL),
                 error, pas);
    OCLERROR_RET(clSetKernelArg(reduce, 4, sizeof(cl_int), &zero_elem), error,
                 pas);

    GET_CURRENT_TIMER(dev_start)
    curr = length;
    pass = passes;
    while (curr > 1)
    {
        OCLERROR_RET(clSetKernelArg(reduce, 0, sizeof(cl_mem), &front), error,
                     pas);
        OCLERROR_RET(clSetKernelArg(reduce, 1, sizeof(cl_mem), &back), error,
                     pas);
        OCLERROR_RET(clSetKernelArg(reduce, 3, sizeof(cl_ulong), &curr), error,
                     pas);

        size_t gl = global(curr, factor, wgs);
        OCLERROR_RET(clEnqueueNDRangeKernel(queue, reduce, 1, NULL, &gl, &wgs,
                                            0, NULL, pass),
                     error, pas);

        curr = new_size(curr, factor);
        ++pass;
        if (curr > 1)
        {
            cl_mem tmp = front;
            front = back;
            back = tmp;
        }
    }

    OCLERROR_RET(clWaitForEvents(steps, passes), error, pas);

    GET_CURRENT_TIMER(dev_end)
    cl_ulong dev_time;
    TIMER_DIFFERENCE(dev_time, dev_start, dev_end)
    if (diag_opts.verbose) printf("done.\n\n");

    // calculate reference dataset
    GET_CURRENT_TIMER(host_start)
    cl_int seq_ref = accumulate(arr, length, zero_elem, host_op);
    GET_CURRENT_TIMER(host_end)
    cl_ulong host_time;
    TIMER_DIFFERENCE(host_time, host_start, host_end)

    // Fetch results
    cl_int dev_res;
    OCLERROR_RET(clEnqueueReadBuffer(queue, back, CL_BLOCKING, 0,
                                     sizeof(cl_int), (void *)&dev_res, 0, NULL,
                                     NULL),
                 error, pas);

    // Validate
    if (dev_res != seq_ref)
        fprintf(stderr,
                "Sequential reference: %i\nDevice result: %i\nValidation "
                "failed!\n\n",
                seq_ref, dev_res);
    else
        printf("Validation passed!\n\n");

    if (!diag_opts.quiet)
    {
        printf("Total device execution as seen by host: %llu us.\n",
               (unsigned long long)(dev_time + 500) / 1000);
        printf("Reduction steps as measured by device :\n");
        for (size_t i = 0; i < steps; ++i)
            printf(
                "\t%llu us.\n",
                (unsigned long long)(cl_util_get_event_duration(
                                         passes[i], CL_PROFILING_COMMAND_START,
                                         CL_PROFILING_COMMAND_END, &error)
                                     + 500)
                    / 1000);
        printf("Reference execution as seen by host: %llu us.\n",
               (unsigned long long)(host_time + 500) / 1000);
    }

pas:
    free(passes);
bufb:
    OCLERROR_RET(clReleaseMemObject(back), end_error, buff);
buff:
    OCLERROR_RET(clReleaseMemObject(front), end_error, harr);
harr:
    free(arr);
red:
    OCLERROR_RET(clReleaseKernel(reduce), end_error, prg);
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
