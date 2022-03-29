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
#include <CL/SDK/Image.h>
#include <CL/Utils/Event.h>

// STL includes
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

// Default image
#include "default_image.h"

// Sample-specific option
struct options_Blur
{
    float size;
    const char *in, *out, *op;
};

cag_option BlurOptions[] = { { .identifier = 'i',
                               .access_letters = "i",
                               .access_name = "in",
                               .value_name = "(name)",
                               .description = "Input image file" },

                             { .identifier = 'o',
                               .access_letters = "o",
                               .access_name = "out",
                               .value_name = "(name)",
                               .description = "Output image file" },

                             { .identifier = 's',
                               .access_letters = "s",
                               .access_name = "size",
                               .value_name = "(positive float)",
                               .description = "Size of blur kernel" },

                             { .identifier = 'b',
                               .access_letters = "b",
                               .access_name = "blur",
                               .value_name = "(box|gauss)",
                               .description =
                                   "Operation of blur to perform" } };

ParseState parse_BlurOptions(const char identifier,
                             cag_option_context *cag_context,
                             struct options_Blur *opts)
{
    const char *value;

#define IF_ERR(op)                                                             \
    if ((value = cag_option_get_value(cag_context)))                           \
    {                                                                          \
        op;                                                                    \
        return ParsedOK;                                                       \
    }                                                                          \
    else                                                                       \
        return ParseError;

    switch (identifier)
    {
        case 'i': IF_ERR(opts->in = value)
        case 'o': IF_ERR(opts->out = value)
        case 's': IF_ERR(opts->size = fabsf(strtof(value, NULL)))
        case 'b':
            if ((value = cag_option_get_value(cag_context))
                && (!strcmp(value, "box") || !strcmp(value, "gauss")))
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
                     struct options_Blur *blur_opts)
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
    MEM_CHECK(tmp = add_CLI_options(opts, &n, BlurOptions,
                                    CAG_ARRAY_SIZE(BlurOptions)),
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
        PARS_OPTIONS(parse_BlurOptions(identifier, &cag_context, blur_opts),
                     state);

        if (identifier == 'h')
        {
            printf("Usage: blur [OPTION]...\n");
            printf("Option name and value should be separated by '=' or a "
                   "space\n");
            /*printf("Demonstrates how to query various OpenCL extensions
               applicable " "in the context of a reduction algorithm and to
               touch up kernel sources " "at runtime to select the best kernel
               implementation for the task.\n\n");*/
            cag_option_print(opts, n, stdout);
            exit((state == ParseError) ? CL_INVALID_ARG_VALUE : CL_SUCCESS);
        }
    }

end:
    free(opts);
    return error;
}

float Gaussian(float x, float radius)
{
    const float pi = 3.141592653589793238462f;
    return expf(-x * x / (2 * radius * radius)) / (sqrtf(2 * pi) * radius);
}

// note that the kernel is not normalized and has size of 2*(*size)+1 elements
cl_int create_gaussian_kernel(float radius, float **const kernel,
                              int *const size)
{
    cl_int error = CL_SUCCESS;

    radius = fabsf(radius);
    *size = (int)ceilf(3 * radius);
    int span = 2 * (*size) + 1;
    *kernel = NULL;
    MEM_CHECK(*kernel = (float *)malloc(sizeof(float) * span), error, end);

    for (int i = 0; i <= *size; ++i)
    {
        float gx = Gaussian((float)i, radius);
        (*kernel)[*size + i] = gx;
        (*kernel)[*size - i] = gx;
    }

end:
    return error;
}

typedef struct state
{
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    char *kernel;
    size_t program_size;
    char *options;
    cl_program program;
    cl_uint step;
    cl_sdk_image input_image;
    cl_mem input_image_buf;
    cl_sdk_image output_image;
    cl_mem output_image_buf;
    cl_mem temp_image_buf;
    const char *filename;
    bool verbose;
} state;

cl_image_format set_image_format(state *const s, cl_int *const error)
{
    // this format is always supported
    cl_image_format res = { .image_channel_order = CL_RGBA,
                            .image_channel_data_type = CL_UNSIGNED_INT8 };
    *error = CL_SUCCESS;

    if ((s->input_image.pixel_size == 1) || (s->input_image.pixel_size == 3))
    {
        // search for 1/3 channels of uint8_t
        cl_image_format *formats = NULL;
        cl_uint formats_number = 0;
        OCLERROR_RET(clGetSupportedImageFormats(s->context, CL_MEM_READ_ONLY,
                                                CL_MEM_OBJECT_IMAGE2D, 0, NULL,
                                                &formats_number),
                     *error, end);
        MEM_CHECK(formats = (cl_image_format *)malloc(sizeof(cl_image_format)
                                                      * formats_number),
                  *error, end);
        OCLERROR_RET(clGetSupportedImageFormats(s->context, CL_MEM_READ_ONLY,
                                                CL_MEM_OBJECT_IMAGE2D,
                                                formats_number, formats, NULL),
                     *error, end);
        for (cl_uint i = 0; i < formats_number; ++i)
            if (((s->input_image.pixel_size == 3)
                 && (formats[i].image_channel_order == CL_RGB)
                 && (formats[i].image_channel_data_type == CL_UNSIGNED_INT8))
                || ((s->input_image.pixel_size == 1)
                    && (formats[i].image_channel_order == CL_R)
                    && (formats[i].image_channel_data_type
                        == CL_UNSIGNED_INT8)))
            {
                return formats[i];
            }

        // if not found, default to 4 channels of uint8_t
        if (s->verbose) printf("Converting picture into supported format... ");

        const size_t pixels = s->input_image.width * s->input_image.height;
        const size_t new = sizeof(unsigned char) * pixels * 4;
        unsigned char *tmp = NULL;
        MEM_CHECK(tmp = (unsigned char *)realloc(s->input_image.pixels, new),
                  *error, end);
        s->input_image.pixels = tmp;
        MEM_CHECK(tmp = (unsigned char *)realloc(s->output_image.pixels, new),
                  *error, end);
        s->output_image.pixels = tmp;

        // change picture
        const size_t pixel_size = s->input_image.pixel_size;
        for (size_t i = pixels - 1; i != 0; --i)
        {
            memcpy(s->input_image.pixels + 4 * i,
                   s->input_image.pixels + pixel_size * i, pixel_size);
            memset(s->input_image.pixels + 4 * i + pixel_size, 0,
                   4 - pixel_size);
        }
        memset(s->input_image.pixels + pixel_size, 0, 4 - pixel_size);
        s->input_image.pixel_size = 4;
        // store initial pixel_size in s->output_image.pixel_size
        if (s->verbose) printf("done.\n");
    }
    else if (s->input_image.pixel_size != 4)
    {
        fprintf(stderr, "Unknown image format!\n");
        *error = CL_INVALID_IMAGE_FORMAT_DESCRIPTOR;
        return res;
    }

    // show image format used
    if (s->verbose)
    {
        printf("Format: ");
        if (res.image_channel_order == CL_R)
            printf("CL_R, ");
        else if (res.image_channel_order == CL_RGB)
            printf("CL_RBG, ");
        else if (res.image_channel_order == CL_RGBA)
            printf("CL_RBGA, ");
        if (res.image_channel_data_type == CL_UNSIGNED_INT8)
            printf("CL_UNSIGNED_INT8\n\n");
    }

end:
    return res;
}

cl_int finalize_blur(state *const s)
{
    cl_int error = CL_SUCCESS;

    // restore image type if needed
    if (s->input_image.pixel_size != s->output_image.pixel_size)
    {
        const size_t pixels = s->input_image.width * s->input_image.height,
                     pixel_size = s->output_image.pixel_size;
        for (size_t i = 1; i < pixels; ++i)
            memcpy(s->output_image.pixels + pixel_size * i,
                   s->output_image.pixels + 4 * i, pixel_size);
    }

    char name[1024];
    sprintf(name, "%u", (unsigned int)s->step);
    strncat(name, s->filename, sizeof(name) - 2);
    error = cl_sdk_write_image(name, &s->output_image);
    if (error == CL_SUCCESS) printf("Image %s written.\n\n", name);

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

cl_int single_pass_box_blur(state *const s, cl_int size)
{
    cl_int error = CL_SUCCESS;
    printf("Single-pass blur\n");
    ++(s->step);

    size_t image_size[3] = { s->input_image.width, s->input_image.height, 1 };
    size_t origin[3] = { 0, 0, 0 };

    // compile kernel
    cl_kernel blur;
    OCLERROR_PAR(blur = clCreateKernel(s->program, "blur_box", &error), error,
                 end);

    // set kernel parameters
    OCLERROR_RET(clSetKernelArg(blur, 0, sizeof(cl_mem), &s->input_image_buf),
                 error, blr);
    OCLERROR_RET(clSetKernelArg(blur, 1, sizeof(cl_mem), &s->output_image_buf),
                 error, blr);
    OCLERROR_RET(clSetKernelArg(blur, 2, sizeof(cl_int), &size), error, blr);

    // blur
    cl_event pass;
    GET_CURRENT_TIMER(start)
    OCLERROR_RET(clEnqueueNDRangeKernel(s->queue, blur, 2, origin, image_size,
                                        NULL, 0, NULL, &pass),
                 error, blr);
    OCLERROR_RET(clWaitForEvents(1, &pass), error, blr);
    GET_CURRENT_TIMER(end)

    OCLERROR_RET(clEnqueueReadImage(s->queue, s->output_image_buf, CL_BLOCKING,
                                    origin, image_size, 0, 0,
                                    s->output_image.pixels, 0, NULL, NULL),
                 error, blr);

    if (s->verbose) print_timings(start, end, &pass, 1);

    // write output file
    OCLERROR_RET(finalize_blur(s), error, blr);

blr:
    clReleaseKernel(blur);
end:
    return error;
}

cl_int dual_pass_box_blur(state *const s, cl_int size)
{
    cl_int error = CL_SUCCESS, end_error = CL_SUCCESS;
    printf("Dual-pass blur\n");
    ++(s->step);

    size_t image_size[3] = { s->input_image.width, s->input_image.height, 1 };
    size_t origin[3] = { 0, 0, 0 };

    // create kernels
    cl_kernel blur1, blur2;
    OCLERROR_PAR(blur1 =
                     clCreateKernel(s->program, "blur_box_horizontal", &error),
                 error, end);
    OCLERROR_PAR(blur2 =
                     clCreateKernel(s->program, "blur_box_vertical", &error),
                 error, blr1);

    // set kernel parameters
    OCLERROR_RET(clSetKernelArg(blur1, 0, sizeof(cl_mem), &s->input_image_buf),
                 error, blr2);
    OCLERROR_RET(clSetKernelArg(blur1, 1, sizeof(cl_mem), &s->temp_image_buf),
                 error, blr2);
    OCLERROR_RET(clSetKernelArg(blur1, 2, sizeof(cl_int), &size), error, blr2);

    OCLERROR_RET(clSetKernelArg(blur2, 0, sizeof(cl_mem), &s->temp_image_buf),
                 error, blr2);
    OCLERROR_RET(clSetKernelArg(blur2, 1, sizeof(cl_mem), &s->output_image_buf),
                 error, blr2);
    OCLERROR_RET(clSetKernelArg(blur2, 2, sizeof(cl_int), &size), error, blr2);

    // blur
    cl_event pass[2];
    GET_CURRENT_TIMER(start)
    OCLERROR_RET(clEnqueueNDRangeKernel(s->queue, blur1, 2, origin, image_size,
                                        NULL, 0, NULL, pass + 0),
                 error, blr2);
    OCLERROR_RET(clEnqueueNDRangeKernel(s->queue, blur2, 2, origin, image_size,
                                        NULL, 0, NULL, pass + 1),
                 error, blr2);
    OCLERROR_RET(clWaitForEvents(2, pass), error, blr2);
    GET_CURRENT_TIMER(end)

    OCLERROR_RET(clEnqueueReadImage(s->queue, s->output_image_buf, CL_BLOCKING,
                                    origin, image_size, 0, 0,
                                    s->output_image.pixels, 0, NULL, NULL),
                 error, blr2);

    if (s->verbose) print_timings(start, end, pass, 2);

    // write output file
    OCLERROR_RET(finalize_blur(s), error, blr2);

blr2:
    OCLERROR_RET(clReleaseKernel(blur2), end_error, blr1);
blr1:
    OCLERROR_RET(clReleaseKernel(blur1), end_error, end);
end:
    return error;
}

cl_int dual_pass_local_memory_exchange_box_blur(state *const s, cl_int size)
{
    cl_int error = CL_SUCCESS, end_error = CL_SUCCESS;
    printf("Dual-pass local memory exchange blur\n");
    ++(s->step);

    size_t image_size[3] = { s->input_image.width, s->input_image.height, 1 };
    size_t origin[3] = { 0, 0, 0 };

    // create kernels
    cl_kernel blur1, blur2;
    OCLERROR_PAR(blur1 = clCreateKernel(s->program,
                                        "blur_box_horizontal_exchange", &error),
                 error, end);
    OCLERROR_PAR(blur2 = clCreateKernel(s->program,
                                        "blur_box_vertical_exchange", &error),
                 error, blr1);

    // query maximum supported WGS of kernel on device based on private mem
    // (register) constraints
    size_t wgs1, psm1, wgs2, psm2;
    OCLERROR_RET(clGetKernelWorkGroupInfo(blur1, s->device,
                                          CL_KERNEL_WORK_GROUP_SIZE,
                                          sizeof(size_t), &wgs1, NULL),
                 error, blr2);
    OCLERROR_RET(
        clGetKernelWorkGroupInfo(blur1, s->device,
                                 CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
                                 sizeof(size_t), &psm1, NULL),
        error, blr2);
    OCLERROR_RET(clGetKernelWorkGroupInfo(blur2, s->device,
                                          CL_KERNEL_WORK_GROUP_SIZE,
                                          sizeof(size_t), &wgs2, NULL),
                 error, blr2);
    OCLERROR_RET(
        clGetKernelWorkGroupInfo(blur2, s->device,
                                 CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
                                 sizeof(size_t), &psm2, NULL),
        error, blr2);

    // Further constrain (reduce) WGS based on shared mem size on device
    cl_ulong loc_mem;
    OCLERROR_RET(clGetDeviceInfo(s->device, CL_DEVICE_LOCAL_MEM_SIZE,
                                 sizeof(cl_ulong), &loc_mem, NULL),
                 error, blr2);

    if (loc_mem >= ((psm1 > psm2 ? psm1 : psm2) + 2 * size) * sizeof(cl_uchar4))
    {
        while (loc_mem < (wgs1 + 2 * size) * sizeof(cl_uchar4)) wgs1 -= psm1;
        while (loc_mem < (wgs2 + 2 * size) * sizeof(cl_uchar4)) wgs2 -= psm2;
    }
    else
    {
        printf("Not enough local memory to serve a single sub-group.\n");
        error = CL_OUT_OF_RESOURCES;
        goto blr2;
    }

    // set kernel parameters
    OCLERROR_RET(clSetKernelArg(blur1, 0, sizeof(cl_mem), &s->input_image_buf),
                 error, blr2);
    OCLERROR_RET(clSetKernelArg(blur1, 1, sizeof(cl_mem), &s->temp_image_buf),
                 error, blr2);
    OCLERROR_RET(clSetKernelArg(blur1, 2, sizeof(cl_int), &size), error, blr2);
    OCLERROR_RET(
        clSetKernelArg(blur1, 3, sizeof(cl_uchar4) * (wgs1 + 2 * size), NULL),
        error, blr2);

    OCLERROR_RET(clSetKernelArg(blur2, 0, sizeof(cl_mem), &s->temp_image_buf),
                 error, blr2);
    OCLERROR_RET(clSetKernelArg(blur2, 1, sizeof(cl_mem), &s->output_image_buf),
                 error, blr2);
    OCLERROR_RET(clSetKernelArg(blur2, 2, sizeof(cl_int), &size), error, blr2);
    OCLERROR_RET(
        clSetKernelArg(blur2, 3, sizeof(cl_uchar4) * (wgs2 + 2 * size), NULL),
        error, blr2);

    // blur
    cl_event pass[2];
    GET_CURRENT_TIMER(start)
    size_t work_size1[3] = { (s->input_image.width + wgs1 - 1) / wgs1 * wgs1,
                             s->input_image.height, 1 };
    size_t wgsf[3] = { wgs1, 1, 1 };
    OCLERROR_RET(clEnqueueNDRangeKernel(s->queue, blur1, 2, origin, work_size1,
                                        wgsf, 0, NULL, pass + 0),
                 error, blr2);
    size_t work_size2[3] = { s->input_image.width,
                             (s->input_image.height + wgs2 - 1) / wgs2 * wgs2,
                             1 };
    size_t wgss[3] = { 1, wgs2, 1 };
    OCLERROR_RET(clEnqueueNDRangeKernel(s->queue, blur2, 2, origin, work_size2,
                                        wgss, 0, NULL, pass + 1),
                 error, blr2);
    OCLERROR_RET(clWaitForEvents(2, pass), error, blr2);
    GET_CURRENT_TIMER(end)

    OCLERROR_RET(clEnqueueReadImage(s->queue, s->output_image_buf, CL_BLOCKING,
                                    origin, image_size, 0, 0,
                                    s->output_image.pixels, 0, NULL, NULL),
                 error, blr2);

    if (s->verbose) print_timings(start, end, pass, 2);

    // write output file
    OCLERROR_RET(finalize_blur(s), error, blr2);

blr2:
    OCLERROR_RET(clReleaseKernel(blur2), end_error, blr1);
blr1:
    OCLERROR_RET(clReleaseKernel(blur1), end_error, end);
end:
    return error;
}

cl_int dual_pass_subgroup_exchange_box_blur(state *const s, cl_int size)
{
    cl_int error = CL_SUCCESS, end_error = CL_SUCCESS;
    ++(s->step);

    size_t image_size[3] = { s->input_image.width, s->input_image.height, 1 };
    size_t origin[3] = { 0, 0, 0 };

    // build program with options
    cl_program program;
    OCLERROR_PAR(program = clCreateProgramWithSource(s->context, 1,
                                                     (const char **)&s->kernel,
                                                     &s->program_size, &error),
                 error, end);
    OCLERROR_RET(cl_util_build_program(program, s->device, s->options), error,
                 prg);

    // create kernels
    cl_kernel blur1, blur2;
    OCLERROR_PAR(blur1 = clCreateKernel(
                     program, "blur_box_horizontal_subgroup_exchange", &error),
                 error, prg);
    OCLERROR_PAR(blur2 = clCreateKernel(
                     program, "blur_box_vertical_subgroup_exchange", &error),
                 error, blr1);

    // set kernel parameters
    OCLERROR_RET(clSetKernelArg(blur1, 0, sizeof(cl_mem), &s->input_image_buf),
                 error, blr2);
    OCLERROR_RET(clSetKernelArg(blur1, 1, sizeof(cl_mem), &s->temp_image_buf),
                 error, blr2);
    OCLERROR_RET(clSetKernelArg(blur1, 2, sizeof(cl_int), &size), error, blr2);

    OCLERROR_RET(clSetKernelArg(blur2, 0, sizeof(cl_mem), &s->temp_image_buf),
                 error, blr2);
    OCLERROR_RET(clSetKernelArg(blur2, 1, sizeof(cl_mem), &s->output_image_buf),
                 error, blr2);
    OCLERROR_RET(clSetKernelArg(blur2, 2, sizeof(cl_int), &size), error, blr2);

    // query preferred subgroup size of kernel on device
    size_t wgs1, wgs2;
    OCLERROR_RET(
        clGetKernelWorkGroupInfo(blur1, s->device,
                                 CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
                                 sizeof(size_t), &wgs1, NULL),
        error, blr2);
    OCLERROR_RET(
        clGetKernelWorkGroupInfo(blur2, s->device,
                                 CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
                                 sizeof(size_t), &wgs2, NULL),
        error, blr2);

    // blur
    cl_event pass[2];
    GET_CURRENT_TIMER(start)
    size_t work_size1[3] = { (s->input_image.width + wgs1 - 1) / wgs1 * wgs1,
                             s->input_image.height, 1 };
    size_t wgsf[3] = { wgs1, 1, 1 };
    OCLERROR_RET(clEnqueueNDRangeKernel(s->queue, blur1, 2, origin, work_size1,
                                        wgsf, 0, NULL, pass + 0),
                 error, blr2);
    size_t work_size2[3] = { s->input_image.width,
                             (s->input_image.height + wgs2 - 1) / wgs2 * wgs2,
                             1 };
    size_t wgss[3] = { 1, wgs2, 1 };
    OCLERROR_RET(clEnqueueNDRangeKernel(s->queue, blur2, 2, origin, work_size2,
                                        wgss, 0, NULL, pass + 1),
                 error, blr2);
    OCLERROR_RET(clWaitForEvents(2, pass), error, blr2);
    GET_CURRENT_TIMER(end)

    OCLERROR_RET(clEnqueueReadImage(s->queue, s->output_image_buf, CL_BLOCKING,
                                    origin, image_size, 0, 0,
                                    s->output_image.pixels, 0, NULL, NULL),
                 error, blr2);

    if (s->verbose) print_timings(start, end, pass, 2);

    // write output file
    OCLERROR_RET(finalize_blur(s), error, blr2);

    // cleanup for error handling
blr2:
    OCLERROR_RET(clReleaseKernel(blur2), end_error, blr1);
blr1:
    OCLERROR_RET(clReleaseKernel(blur1), end_error, prg);
prg:
    OCLERROR_RET(clReleaseProgram(program), end_error, end);
    ;
end:
    return error;
}

cl_int dual_pass_kernel_blur(state *const s, cl_int size, cl_mem kern)
{
    cl_int error = CL_SUCCESS, end_error = CL_SUCCESS;
    ++(s->step);

    size_t image_size[3] = { s->input_image.width, s->input_image.height, 1 };
    size_t origin[3] = { 0, 0, 0 };

    cl_kernel blur1, blur2;
    OCLERROR_PAR(
        blur1 = clCreateKernel(s->program, "blur_kernel_horizontal", &error),
        error, end);
    OCLERROR_PAR(blur2 =
                     clCreateKernel(s->program, "blur_kernel_vertical", &error),
                 error, blr1);

    // set kernel parameters
    OCLERROR_RET(clSetKernelArg(blur1, 0, sizeof(cl_mem), &s->input_image_buf),
                 error, blr2);
    OCLERROR_RET(clSetKernelArg(blur1, 1, sizeof(cl_mem), &s->temp_image_buf),
                 error, blr2);
    OCLERROR_RET(clSetKernelArg(blur1, 2, sizeof(cl_int), &size), error, blr2);
    OCLERROR_RET(clSetKernelArg(blur1, 3, sizeof(cl_mem), &kern), error, blr2);

    OCLERROR_RET(clSetKernelArg(blur2, 0, sizeof(cl_mem), &s->temp_image_buf),
                 error, blr2);
    OCLERROR_RET(clSetKernelArg(blur2, 1, sizeof(cl_mem), &s->output_image_buf),
                 error, blr2);
    OCLERROR_RET(clSetKernelArg(blur2, 2, sizeof(cl_int), &size), error, blr2);
    OCLERROR_RET(clSetKernelArg(blur2, 3, sizeof(cl_mem), &kern), error, blr2);

    // blur
    cl_event pass[2];
    GET_CURRENT_TIMER(start)
    OCLERROR_RET(clEnqueueNDRangeKernel(s->queue, blur1, 2, origin, image_size,
                                        NULL, 0, NULL, pass + 0),
                 error, blr2);
    OCLERROR_RET(clEnqueueNDRangeKernel(s->queue, blur2, 2, origin, image_size,
                                        NULL, 0, NULL, pass + 1),
                 error, blr2);
    OCLERROR_RET(clWaitForEvents(2, pass), error, blr2);
    GET_CURRENT_TIMER(end)

    OCLERROR_RET(clEnqueueReadImage(s->queue, s->output_image_buf, CL_BLOCKING,
                                    origin, image_size, 0, 0,
                                    s->output_image.pixels, 0, NULL, NULL),
                 error, blr2);

    if (s->verbose) print_timings(start, end, pass, 2);

    // write output file
    OCLERROR_RET(finalize_blur(s), error, blr2);

blr2:
    OCLERROR_RET(clReleaseKernel(blur2), end_error, blr1);
blr1:
    OCLERROR_RET(clReleaseKernel(blur1), end_error, end);
end:
    return error;
}

cl_int dual_pass_local_memory_exchange_kernel_blur(state *const s, cl_int size,
                                                   cl_mem kern)
{
    cl_int error = CL_SUCCESS, end_error = CL_SUCCESS;
    ++(s->step);

    size_t image_size[3] = { s->input_image.width, s->input_image.height, 1 };
    size_t origin[3] = { 0, 0, 0 };

    // create kernels
    cl_kernel blur1, blur2;
    OCLERROR_PAR(blur1 = clCreateKernel(
                     s->program, "blur_kernel_horizontal_exchange", &error),
                 error, end);
    OCLERROR_PAR(blur2 = clCreateKernel(
                     s->program, "blur_kernel_vertical_exchange", &error),
                 error, blr1);

    // query maximum supported WGS of kernel on device based on private mem
    // (register) constraints
    size_t wgs1, psm1, wgs2, psm2;
    OCLERROR_RET(clGetKernelWorkGroupInfo(blur1, s->device,
                                          CL_KERNEL_WORK_GROUP_SIZE,
                                          sizeof(size_t), &wgs1, NULL),
                 error, blr2);
    OCLERROR_RET(
        clGetKernelWorkGroupInfo(blur1, s->device,
                                 CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
                                 sizeof(size_t), &psm1, NULL),
        error, blr2);
    OCLERROR_RET(clGetKernelWorkGroupInfo(blur2, s->device,
                                          CL_KERNEL_WORK_GROUP_SIZE,
                                          sizeof(size_t), &wgs2, NULL),
                 error, blr2);
    OCLERROR_RET(
        clGetKernelWorkGroupInfo(blur2, s->device,
                                 CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
                                 sizeof(size_t), &psm2, NULL),
        error, blr2);

    // Further constrain (reduce) WGS based on shared mem size on device
    cl_ulong loc_mem;
    OCLERROR_RET(clGetDeviceInfo(s->device, CL_DEVICE_LOCAL_MEM_SIZE,
                                 sizeof(cl_ulong), &loc_mem, NULL),
                 error, blr2);

    if (loc_mem >= ((psm1 > psm2 ? psm1 : psm2) + 2 * size) * sizeof(cl_uchar4))
    {
        while (loc_mem < (wgs1 + 2 * size) * sizeof(cl_uchar4)) wgs1 -= psm1;
        while (loc_mem < (wgs2 + 2 * size) * sizeof(cl_uchar4)) wgs2 -= psm2;
    }
    else
    {
        printf("Not enough local memory to serve a single sub-group.\n");
        error = CL_OUT_OF_RESOURCES;
        goto blr2;
    }

    // set kernel parameters
    OCLERROR_RET(clSetKernelArg(blur1, 0, sizeof(cl_mem), &s->input_image_buf),
                 error, blr2);
    OCLERROR_RET(clSetKernelArg(blur1, 1, sizeof(cl_mem), &s->temp_image_buf),
                 error, blr2);
    OCLERROR_RET(clSetKernelArg(blur1, 2, sizeof(cl_int), &size), error, blr2);
    OCLERROR_RET(clSetKernelArg(blur1, 3, sizeof(cl_mem), &kern), error, blr2);
    OCLERROR_RET(
        clSetKernelArg(blur1, 4, sizeof(cl_uchar4) * (wgs1 + 2 * size), NULL),
        error, blr2);

    OCLERROR_RET(clSetKernelArg(blur2, 0, sizeof(cl_mem), &s->temp_image_buf),
                 error, blr2);
    OCLERROR_RET(clSetKernelArg(blur2, 1, sizeof(cl_mem), &s->output_image_buf),
                 error, blr2);
    OCLERROR_RET(clSetKernelArg(blur2, 2, sizeof(cl_int), &size), error, blr2);
    OCLERROR_RET(clSetKernelArg(blur2, 3, sizeof(cl_mem), &kern), error, blr2);
    OCLERROR_RET(
        clSetKernelArg(blur2, 4, sizeof(cl_uchar4) * (wgs2 + 2 * size), NULL),
        error, blr2);

    // blur
    cl_event pass[2];
    GET_CURRENT_TIMER(start)
    size_t work_size1[3] = { (s->input_image.width + wgs1 - 1) / wgs1 * wgs1,
                             s->input_image.height, 1 };
    size_t wgsf[3] = { wgs1, 1, 1 };
    OCLERROR_RET(clEnqueueNDRangeKernel(s->queue, blur1, 2, origin, work_size1,
                                        wgsf, 0, NULL, pass + 0),
                 error, blr2);
    size_t work_size2[3] = { s->input_image.width,
                             (s->input_image.height + wgs2 - 1) / wgs2 * wgs2,
                             1 };
    size_t wgss[3] = { 1, wgs2, 1 };
    OCLERROR_RET(clEnqueueNDRangeKernel(s->queue, blur2, 2, origin, work_size2,
                                        wgss, 0, NULL, pass + 1),
                 error, blr2);
    OCLERROR_RET(clWaitForEvents(2, pass), error, blr2);
    GET_CURRENT_TIMER(end)

    OCLERROR_RET(clEnqueueReadImage(s->queue, s->output_image_buf, CL_BLOCKING,
                                    origin, image_size, 0, 0,
                                    s->output_image.pixels, 0, NULL, NULL),
                 error, blr2);

    if (s->verbose) print_timings(start, end, pass, 2);

    // write output file
    OCLERROR_RET(finalize_blur(s), error, blr2);

blr2:
    OCLERROR_RET(clReleaseKernel(blur2), end_error, blr1);
blr1:
    OCLERROR_RET(clReleaseKernel(blur1), end_error, end);
end:
    return error;
}

cl_int dual_pass_subgroup_exchange_kernel_blur(state *const s, cl_int size,
                                               cl_mem kern)
{
    cl_int error = CL_SUCCESS, end_error = CL_SUCCESS;
    ++(s->step);

    size_t image_size[3] = { s->input_image.width, s->input_image.height, 1 };
    size_t origin[3] = { 0, 0, 0 };

    // build program with options
    cl_program program;
    OCLERROR_PAR(program = clCreateProgramWithSource(s->context, 1,
                                                     (const char **)&s->kernel,
                                                     &s->program_size, &error),
                 error, end);
    OCLERROR_RET(cl_util_build_program(program, s->device, s->options), error,
                 prg);

    // create kernels
    cl_kernel blur1, blur2;
    OCLERROR_PAR(
        blur1 = clCreateKernel(
            program, "blur_kernel_horizontal_subgroup_exchange", &error),
        error, prg);
    OCLERROR_PAR(blur2 = clCreateKernel(
                     program, "blur_kernel_vertical_subgroup_exchange", &error),
                 error, blr1);

    // set kernel parameters
    OCLERROR_RET(clSetKernelArg(blur1, 0, sizeof(cl_mem), &s->input_image_buf),
                 error, blr2);
    OCLERROR_RET(clSetKernelArg(blur1, 1, sizeof(cl_mem), &s->temp_image_buf),
                 error, blr2);
    OCLERROR_RET(clSetKernelArg(blur1, 2, sizeof(cl_int), &size), error, blr2);
    OCLERROR_RET(clSetKernelArg(blur1, 3, sizeof(cl_mem), &kern), error, blr2);

    OCLERROR_RET(clSetKernelArg(blur2, 0, sizeof(cl_mem), &s->temp_image_buf),
                 error, blr2);
    OCLERROR_RET(clSetKernelArg(blur2, 1, sizeof(cl_mem), &s->output_image_buf),
                 error, blr2);
    OCLERROR_RET(clSetKernelArg(blur2, 2, sizeof(cl_int), &size), error, blr2);
    OCLERROR_RET(clSetKernelArg(blur2, 3, sizeof(cl_mem), &kern), error, blr2);

    // query preferred subgroup size of kernel on device
    size_t wgs1, wgs2;
    OCLERROR_RET(
        clGetKernelWorkGroupInfo(blur1, s->device,
                                 CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
                                 sizeof(size_t), &wgs1, NULL),
        error, blr2);
    OCLERROR_RET(
        clGetKernelWorkGroupInfo(blur2, s->device,
                                 CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
                                 sizeof(size_t), &wgs2, NULL),
        error, blr2);

    // blur
    cl_event pass[2];
    GET_CURRENT_TIMER(start)
    size_t work_size1[3] = { (s->input_image.width + wgs1 - 1) / wgs1 * wgs1,
                             s->input_image.height, 1 };
    size_t wgsf[3] = { wgs1, 1, 1 };
    OCLERROR_RET(clEnqueueNDRangeKernel(s->queue, blur1, 2, origin, work_size1,
                                        wgsf, 0, NULL, pass + 0),
                 error, blr2);
    size_t work_size2[3] = { s->input_image.width,
                             (s->input_image.height + wgs2 - 1) / wgs2 * wgs2,
                             1 };
    size_t wgss[3] = { 1, wgs2, 1 };
    OCLERROR_RET(clEnqueueNDRangeKernel(s->queue, blur2, 2, origin, work_size2,
                                        wgss, 0, NULL, pass + 1),
                 error, blr2);
    OCLERROR_RET(clWaitForEvents(2, pass), error, blr2);
    GET_CURRENT_TIMER(end)

    OCLERROR_RET(clEnqueueReadImage(s->queue, s->output_image_buf, CL_BLOCKING,
                                    origin, image_size, 0, 0,
                                    s->output_image.pixels, 0, NULL, NULL),
                 error, blr2);

    if (s->verbose) print_timings(start, end, pass, 2);

    // write output file
    OCLERROR_RET(finalize_blur(s), error, blr2);

    // cleanup for error handling
blr2:
    OCLERROR_RET(clReleaseKernel(blur2), end_error, blr1);
blr1:
    OCLERROR_RET(clReleaseKernel(blur1), end_error, prg);
prg:
    OCLERROR_RET(clReleaseProgram(program), end_error, end);
    ;
end:
    return error;
}


int main(int argc, char *argv[])
{
    cl_int error = CL_SUCCESS, end_error = CL_SUCCESS;
    state s;
    cl_platform_id platform;

    /// Parse command-line options
    struct cl_sdk_options_Diagnostic diag_opts = { .quiet = false,
                                                   .verbose = false };
    struct cl_sdk_options_SingleDevice dev_opts = {
        .triplet = { 0, 0, CL_DEVICE_TYPE_ALL }
    };
    struct options_Blur blur_opts = {
        .size = 1, .op = "box", .in = NULL, .out = "out.png"
    };

    OCLERROR_RET(parse_options(argc, argv, &diag_opts, &dev_opts, &blur_opts),
                 error, end);
    s.verbose = diag_opts.verbose;
    s.filename = blur_opts.out;
    s.step = 0;

    /// Create runtime objects based on user preference or default
    OCLERROR_PAR(s.device = cl_util_get_device(
                     dev_opts.triplet.plat_index, dev_opts.triplet.dev_index,
                     dev_opts.triplet.dev_type, &error),
                 error, end);
    OCLERROR_PAR(s.context =
                     clCreateContext(NULL, 1, &s.device, NULL, NULL, &error),
                 error, end);
    OCLERROR_RET(clGetDeviceInfo(s.device, CL_DEVICE_PLATFORM,
                                 sizeof(cl_platform_id), &platform, NULL),
                 error, cont);
#if CL_HPP_TARGET_OPENCL_VERSION >= 200
    cl_command_queue_properties props[] = { CL_QUEUE_PROPERTIES,
                                            CL_QUEUE_PROFILING_ENABLE, 0 };
    OCLERROR_PAR(s.queue = clCreateCommandQueueWithProperties(
                     s.context, s.device, props, &error),
                 error, cont);
#else
    OCLERROR_PAR(s.queue = clCreateCommandQueue(
                     s.context, s.device, CL_QUEUE_PROFILING_ENABLE, &error),
                 error, cont);
#endif

    if (!diag_opts.quiet) cl_util_print_device_info(s.device);

    /// Read input image and prepare output image
    const char fname[] = "andrew_svk_7oJ4D_ewB7c_unsplash.png";
    if (!blur_opts.in)
    {
        printf("No file given, use standard image %s\n", fname);
        const unsigned char *fcont = andrew_svk_7oJ4D_ewB7c_unsplash_png;
        const size_t fsize = andrew_svk_7oJ4D_ewB7c_unsplash_png_size;
        FILE *f;
        if ((f = fopen(fname, "wb")) != NULL)
        {
            if (fwrite(fcont, 1, fsize, f) != fsize)
            {
                fprintf(stderr, "File write error!\n");
                error = CL_UTIL_FILE_OPERATION_ERROR;
                fclose(f);
                goto que;
            }
            fclose(f);
        }
        else
        {
            fprintf(stderr, "Cannot open the file to write!\n");
            error = CL_INVALID_VALUE;
            goto que;
        }
        blur_opts.in = fname;
    }
    OCLERROR_PAR(s.input_image = cl_sdk_read_image(blur_opts.in, &error), error,
                 que);

    s.output_image = s.input_image;
    s.output_image.pixels = NULL;
    MEM_CHECK(s.output_image.pixels = (unsigned char *)malloc(
                  sizeof(unsigned char) * s.output_image.width
                  * s.output_image.height * s.output_image.pixel_size),
              error, inim);

    /// Query device and runtime capabilities
    // 1) query image support
    cl_bool image_support = false;
    OCLERROR_RET(clGetDeviceInfo(s.device, CL_DEVICE_IMAGE_SUPPORT,
                                 sizeof(cl_bool), &image_support, NULL),
                 error, outim);
    if (!image_support)
    {
        fprintf(stderr, "No image support on device!\n");
        error = CL_INVALID_DEVICE;
        goto outim;
    }

    // 2) query if the image format is supported and change image if not
    cl_image_format format;
    OCLERROR_PAR(format = set_image_format(&s, &error), error, outim);

    // 3) query if device have local memory and its size
    bool use_local_mem = false;
    cl_device_local_mem_type mt;
    OCLERROR_RET(clGetDeviceInfo(s.device, CL_DEVICE_LOCAL_MEM_TYPE,
                                 sizeof(cl_device_local_mem_type), &mt, NULL),
                 error, outim);
    if (mt == CL_LOCAL) use_local_mem = true;

    // 4) query if device allow subgroup shuffle operations
    bool use_subgroup_exchange = false, use_subgroup_exchange_relative = false;
    {
        char *name = NULL, *tmp1 = NULL, *tmp2 = NULL;
        OCLERROR_PAR(name = cl_util_get_device_info(
                         s.device, CL_DEVICE_EXTENSIONS, &error),
                     error, nam);
        tmp1 = strstr(name, "cl_khr_subgroup_shuffle_relative");
        if (tmp1) use_subgroup_exchange_relative = true;
        tmp2 = strstr(name, "cl_khr_subgroup_shuffle");
        if (tmp2 && (tmp2 == tmp1)) // exclude relative
            tmp2 = strstr(tmp2 + 1, "cl_khr_subgroup_shuffle");
        if (tmp2) use_subgroup_exchange = true;

    nam:
        free(name);
    }

    /// Create image buffers
    const cl_image_desc desc = { .image_type = CL_MEM_OBJECT_IMAGE2D,
                                 .image_width = s.input_image.width,
                                 .image_height = s.input_image.height,
                                 .image_row_pitch = 0,
                                 .num_mip_levels = 0,
                                 .num_samples = 0,
                                 .mem_object = NULL };
    OCLERROR_PAR(s.input_image_buf = clCreateImage(
                     s.context, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY,
                     &format, &desc, NULL, &error),
                 error, outim);
    OCLERROR_PAR(s.output_image_buf = clCreateImage(
                     s.context, CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY,
                     &format, &desc, NULL, &error),
                 error, inbuf);
    OCLERROR_PAR(s.temp_image_buf = clCreateImage(
                     s.context, CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY,
                     &format, &desc, NULL, &error),
                 error, outbuf);

    size_t image_size[3] = { s.input_image.width, s.input_image.height, 1 };
    size_t origin[3] = { 0, 0, 0 };

    OCLERROR_RET(clEnqueueWriteImage(s.queue, s.input_image_buf,
                                     CL_NON_BLOCKING, origin, image_size, 0, 0,
                                     s.input_image.pixels, 0, NULL, NULL),
                 error, tmpbuf);

    /// Create OpenCL program
    const char *kernel_location = "./blur.cl";
    s.kernel = NULL;
    s.program_size = 0;
    char kernel_op[1024] = ""; // here we put some dynamic definitions
    s.options = kernel_op;

    OCLERROR_PAR(s.kernel = cl_util_read_text_file(kernel_location,
                                                   &s.program_size, &error),
                 error, tmpbuf);

    OCLERROR_PAR(s.program = clCreateProgramWithSource(s.context, 1,
                                                       (const char **)&s.kernel,
                                                       &s.program_size, &error),
                 error, ker);
    OCLERROR_RET(cl_util_build_program(s.program, s.device, kernel_op), error,
                 prg);
    kernel_op[0] = '\0';

    /// Box blur
    if (strstr(blur_opts.op, "box"))
    {
        /// Single-pass blur
        OCLERROR_RET(single_pass_box_blur(&s, (cl_int)blur_opts.size), error,
                     prg);

        /// Dual-pass blur
        OCLERROR_RET(dual_pass_box_blur(&s, (cl_int)blur_opts.size), error,
                     prg);

        /// Use local memory exchange in dual-pass blur
        if (use_local_mem)
            OCLERROR_RET(dual_pass_local_memory_exchange_box_blur(
                             &s, (cl_int)blur_opts.size),
                         error, prg);

        /// Subgroup exchange in dual-pass blur
        if (use_subgroup_exchange_relative)
        {
            printf("Dual-pass subgroup relative exchange blur\n");

            kernel_op[0] = '\0';
            strcat(kernel_op, "-D USE_SUBGROUP_EXCHANGE_RELATIVE ");

            OCLERROR_RET(dual_pass_subgroup_exchange_box_blur(
                             &s, (cl_int)blur_opts.size),
                         error, prg);
        }
        if (use_subgroup_exchange)
        {
            printf("Dual-pass subgroup exchange blur\n");

            kernel_op[0] = '\0';
            strcat(kernel_op, "-D USE_SUBGROUP_EXCHANGE ");

            OCLERROR_RET(dual_pass_subgroup_exchange_box_blur(
                             &s, (cl_int)blur_opts.size),
                         error, prg);
        }
    } // Box blur

    /// Gaussian blur
    if (strstr(blur_opts.op, "gauss"))
    {
        // create Gaussian convolution kernel
        float *gauss = NULL;
        int gauss_size = 0;
        OCLERROR_RET(
            create_gaussian_kernel(blur_opts.size, &gauss, &gauss_size), error,
            gau);
        cl_mem gauss_kern;
        OCLERROR_PAR(gauss_kern = clCreateBuffer(
                         s.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                         sizeof(float) * (2 * gauss_size + 1), gauss, &error),
                     error, gau);

        /// Dual-pass Gaussian blur
        printf("Dual-pass Gaussian blur\n");
        OCLERROR_RET(dual_pass_kernel_blur(&s, gauss_size, gauss_kern), error,
                     gkrn);

        /// Local memory exchange Gaussian blur
        if (use_local_mem)
        {
            printf("Dual-pass local memory exchange Gaussian blur\n");
            OCLERROR_RET(dual_pass_local_memory_exchange_kernel_blur(
                             &s, gauss_size, gauss_kern),
                         error, gkrn);
        }

        /// Subgroup exchange in dual-pass Gaussian blur
        if (use_subgroup_exchange_relative)
        {
            printf("Dual-pass subgroup relative exchange Gaussian blur\n");

            kernel_op[0] = '\0';
            strcat(kernel_op, "-D USE_SUBGROUP_EXCHANGE_RELATIVE ");

            OCLERROR_RET(dual_pass_subgroup_exchange_kernel_blur(&s, gauss_size,
                                                                 gauss_kern),
                         error, gkrn);
        }
        if (use_subgroup_exchange)
        {
            printf("Dual-pass subgroup exchange Gaussian blur\n");

            kernel_op[0] = '\0';
            strcat(kernel_op, "-D USE_SUBGROUP_EXCHANGE ");

            OCLERROR_RET(dual_pass_subgroup_exchange_kernel_blur(&s, gauss_size,
                                                                 gauss_kern),
                         error, gkrn);
        }

    gkrn:
        OCLERROR_RET(clReleaseMemObject(gauss_kern), end_error, gau);
    gau:
        free(gauss);
    } // Gaussian blur

    /// Cleanup
prg:
    OCLERROR_RET(clReleaseProgram(s.program), end_error, ker);
ker:
    free(s.kernel);
tmpbuf:
    OCLERROR_RET(clReleaseMemObject(s.temp_image_buf), end_error, outbuf);
outbuf:
    OCLERROR_RET(clReleaseMemObject(s.output_image_buf), end_error, inbuf);
inbuf:
    OCLERROR_RET(clReleaseMemObject(s.input_image_buf), end_error, outim);
outim:
    free(s.output_image.pixels);
inim:
    free(s.input_image.pixels);
que:
    OCLERROR_RET(clReleaseCommandQueue(s.queue), end_error, cont);
cont:
    OCLERROR_RET(clReleaseContext(s.context), end_error, end);
end:
    if (error != CL_SUCCESS) cl_util_print_error(error);
    return error;
}
