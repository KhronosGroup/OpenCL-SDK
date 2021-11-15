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
#include<stdlib.h>
#include<stdio.h>
#include<stdbool.h>

// Sample-specific option
struct options_Blur {
    unsigned int size;
    const char * in, * out, * op;
};

cag_option BlurOptions[] = {
 {.identifier = 'i',
  .access_letters = "i",
  .access_name = "in",
  .value_name = "(name)",
  .description = "Input image file (obligatory)"},

 {.identifier = 'o',
  .access_letters = "o",
  .access_name = "out",
  .value_name = "(name)",
  .description = "Output image file"},

 {.identifier = 's',
  .access_letters = "s",
  .access_name = "size",
  .value_name = "(positive integer)",
  .description = "Size of blur kernel"},

 {.identifier = 'b',
  .access_letters = "b",
  .access_name = "blur",
  .value_name = "(box|gauss)",
  .description = "Operation of blur to perform"}
};

ParseState parse_BlurOptions(const char identifier, cag_option_context * cag_context, struct options_Blur * opts)
{
    const char * value;

#define IF_ERR(op) \
if ((value = cag_option_get_value(cag_context))) \
    { op; return ParsedOK; } \
else return ParseError;

    switch (identifier) {
    case 'i':
    case 'o':
        IF_ERR(opts->in = value)
    case 's':
        IF_ERR(opts->size = strtoul(value, NULL, 0))
    case 'b':
        if ((value = cag_option_get_value(cag_context))
            && (!strcmp(value, "box") || !strcmp(value, "gauss")))
        {
            opts->op = value;
            return ParsedOK;
        }
        else return ParseError;
    }
    return NotParsed;
}

cl_int parse_options(int argc,
                   char* argv[],
                   struct cl_sdk_options_Diagnostic * diag_opts,
                   struct cl_sdk_options_SingleDevice * dev_opts,
                   struct options_Blur * blur_opts)
{
    cl_int error = CL_SUCCESS;
    struct cag_option * opts = NULL, * tmp = NULL;
    size_t n = 0;

    /* Prepare all options array. */
    MEM_CHECK(opts = add_CLI_options(opts, &n, DiagnosticOptions,   CAG_ARRAY_SIZE(DiagnosticOptions)),   error, end);
    MEM_CHECK(tmp  = add_CLI_options(opts, &n, SingleDeviceOptions, CAG_ARRAY_SIZE(SingleDeviceOptions)), error, end);
    opts = tmp;
    MEM_CHECK(tmp  = add_CLI_options(opts, &n, BlurOptions,         CAG_ARRAY_SIZE(BlurOptions)),         error, end);
    opts = tmp;

    char identifier;
    cag_option_context cag_context;

    /* Prepare the context and iterate over all options. */
    cag_option_prepare(&cag_context, opts, n, argc, argv);
    while (cag_option_fetch(&cag_context)) {
        ParseState state = NotParsed;
        identifier = cag_option_get(&cag_context);

        state = parse_DiagnosticOptions(identifier, diag_opts);
        if (state == ParsedOK) continue;
        state = parse_SingleDeviceOptions(identifier, &cag_context, dev_opts);
        if (state == ParsedOK) continue;
        state = parse_BlurOptions(identifier, &cag_context, blur_opts);
        if (state == ParsedOK) continue;

        if ((identifier == 'h') || (state == ParseError)) {
            printf("Usage: blur [OPTION]...\n");
            /*printf("Demonstrates how to query various OpenCL extensions applicable "
                "in the context of a reduction algorithm and to touch up kernel sources "
                "at runtime to select the best kernel implementation for the task.\n\n");*/
            cag_option_print(opts, n, stdout);
            exit((state == ParseError) ? CL_INVALID_ARG_VALUE : CL_SUCCESS);
        }
    }

end:    free(opts);
    return error;
}

int check_use_work_group_reduce(cl_platform_id platform, cl_device_id device, cl_int * error)
{
    int res = 0;
    return res;
}

int check_use_sub_group_reduce(cl_platform_id platform, cl_device_id device, cl_int * error)
{
    int res = 0;
    return res;
}

int main(int argc, char* argv[])
{
    cl_int error = CL_SUCCESS,
        end_error = CL_SUCCESS;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;

    // Parse command-line options
    struct cl_sdk_options_Diagnostic diag_opts = { .quiet = false, .verbose = false };
    struct cl_sdk_options_SingleDevice dev_opts = { .triplet = { 0, 0, CL_DEVICE_TYPE_ALL } };
    struct options_Blur blur_opts = { .size = 1, .op = "box", .in = NULL, .out = "out.png" };

    OCLERROR_RET(parse_options(argc, argv, &diag_opts, &dev_opts, &blur_opts), error, end);
    if (!blur_opts.in) {
        error = CL_INVALID_IMAGE_DESCRIPTOR;
        fprintf(stderr, "No input image name!\n");
        goto end;
    }
    if (!diag_opts.quiet) {
        printf("Blur size: %u\n", blur_opts.size);
    }

    // Create runtime objects based on user preference or default
    OCLERROR_PAR(device = cl_util_get_device(dev_opts.triplet.plat_index,
        dev_opts.triplet.dev_index, dev_opts.triplet.dev_type, &error), error, end);
    OCLERROR_PAR(context = clCreateContext(NULL, 1, &device, NULL, NULL, &error), error, end);
    OCLERROR_RET(clGetDeviceInfo(device, CL_DEVICE_PLATFORM, sizeof(cl_platform_id), &platform, NULL), error, cont);
#if CL_HPP_TARGET_OPENCL_VERSION >= 200
    cl_command_queue_properties props[] = {CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0};
    OCLERROR_PAR(queue = clCreateCommandQueueWithProperties(context, device, props, &error), error, cont);
#else
    OCLERROR_PAR(queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &error), error, cont);
#endif

    if (!diag_opts.quiet) {
        cl_util_print_device_info(device);
    }

    // Query device and runtime capabilities
    // 1) query image support
    cl_bool image_support = false;
    OCLERROR_RET(clGetDeviceInfo(device, CL_DEVICE_IMAGE_SUPPORT, sizeof(cl_bool), &image_support, NULL), error, que);
    if (!image_support) {
        fprintf(stderr, "No image support on device!\n");
        error = CL_INVALID_DEVICE;
        goto que;
    }

    // Compile kernel
    const char * kernel_location = "./blur.cl";
    char * kernel = NULL, * tmp = NULL;
    size_t program_size = 0;
    cl_program program = NULL;
    char kernel_op[1024] = "";

    OCLERROR_PAR(kernel = cl_util_read_text_file(kernel_location, &program_size, &error), error, que);
    // Note append of definitions
    program_size += 1 + strlen(kernel_op);
    MEM_CHECK(tmp = (char *)realloc(kernel, program_size), error, ker);
    strcat(tmp, kernel_op);

    kernel = tmp;
    //printf("%s", kernel);

    OCLERROR_PAR(program = clCreateProgramWithSource(context, 1, (const char **)&kernel, &program_size, &error), error, ker);
    kernel_op[0] = '\0';
    OCLERROR_RET(cl_util_build_program(program, device, kernel_op), error, prg);

    cl_kernel blur;
    OCLERROR_PAR(blur = clCreateKernel(program, "blur_box", &error), error, prg);

    cl_sdk_image input_image = { .width = 0, .height = 0, .pixel_size = 0, .pixels = NULL };
    OCLERROR_PAR(input_image = cl_sdk_read_image(blur_opts.in, &error), error, blr);

    cl_sdk_image output_image = { .width = input_image.width, .height = input_image.height,
        .pixel_size = input_image.pixel_size, .pixels = NULL };
    MEM_CHECK(output_image.pixels = (unsigned char *)malloc(sizeof(unsigned char) *
        output_image.width * output_image.height * output_image.pixel_size), error, inim);

    // query if the image format is supported and change image if not
    cl_image_format * formats = NULL, * format = NULL;
    if ((input_image.pixel_size == 1) || (input_image.pixel_size == 3)) {
        cl_uint formats_number = 0;
        OCLERROR_RET(clGetSupportedImageFormats(context, CL_MEM_READ_ONLY, CL_MEM_OBJECT_IMAGE2D,
            0, NULL, &formats_number), error, outim);
        MEM_CHECK(formats = (cl_image_format *)malloc(sizeof(cl_image_format) * formats_number), error, outim);
        OCLERROR_RET(clGetSupportedImageFormats(context, CL_MEM_READ_ONLY, CL_MEM_OBJECT_IMAGE2D,
            formats_number, formats, NULL), error, frmt);
        // search for 3/1 channels of uint8_t
        for (cl_uint i = 0; i < formats_number; ++i)
            if (    ((input_image.pixel_size == 3)
                    && (formats[i].image_channel_order == CL_RGB)
                    && (formats[i].image_channel_data_type == CL_UNSIGNED_INT8))
                ||  ((input_image.pixel_size == 1)
                    && (formats[i].image_channel_order == CL_R)
                    && (formats[i].image_channel_data_type == CL_UNSIGNED_INT8)))
            {
                format = formats + i;
                break;
            }
        // if not found, default to 4 channels of uint8_t
        if (!format) {
            if (diag_opts.verbose)
                printf("Converting picture into supported format... ");
            format = formats;
            format->image_channel_order = CL_RGBA;
            formats->image_channel_data_type = CL_UNSIGNED_INT8;

            const size_t
                pixels = input_image.width * input_image.height,
                new = sizeof(unsigned char) * pixels * 4;
            unsigned char * tmp = NULL;
            MEM_CHECK(tmp = (unsigned char *)realloc(input_image.pixels, new), error, frmt);
            input_image.pixels = tmp;
            MEM_CHECK(tmp = (unsigned char *)realloc(output_image.pixels, new), error, frmt);
            output_image.pixels = tmp;

            // change picture
            const size_t pixel_size = input_image.pixel_size;
            for (size_t i = pixels - 1; i != 0; --i) {
                memcpy(input_image.pixels + 4 * i, input_image.pixels + pixel_size * i, pixel_size);
                memset(input_image.pixels + 4 * i + pixel_size, 0, 4 - pixel_size);
            }
            memset(input_image.pixels + pixel_size, 0, 4 - pixel_size);
            input_image.pixel_size = 4;
            // store initial pixel_size in output_image.pixel_size
            if (diag_opts.verbose)
                printf("done.\n");
        }
    }
    else if (input_image.pixel_size == 4) {
        MEM_CHECK(formats = (cl_image_format *)malloc(sizeof(cl_image_format)), error, outim);
        format = formats;
        format->image_channel_order = CL_RGBA;
        formats->image_channel_data_type = CL_UNSIGNED_INT8;
    }
    else {
        fprintf(stderr, "Unknown image format!\n");
        error = CL_INVALID_IMAGE_FORMAT_DESCRIPTOR;
        goto outim;
    }

    if (format->image_channel_order == CL_R)
        printf("CL_R ");
    else if (format->image_channel_order == CL_RGB)
        printf("CL_RBG ");
    else if (format->image_channel_order == CL_RGBA)
        printf("CL_RBGA ");
    if (format->image_channel_data_type == CL_UNSIGNED_INT8)
        printf("CL_UNSIGNED_INT8\n");

    cl_mem input_image_buf, output_image_buf;
    const cl_image_desc desc = {
        .image_type      = CL_MEM_OBJECT_IMAGE2D,
        .image_width     = input_image.width,
        .image_height    = input_image.height,
        .image_row_pitch = 0,
        .num_mip_levels  = 0,
        .num_samples     = 0,
        .mem_object      = NULL
    };
    OCLERROR_PAR(input_image_buf = clCreateImage(context, 0 /*CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY*/,
        format, &desc, NULL, &error), error, frmt);
    OCLERROR_PAR(output_image_buf = clCreateImage(context, 0 /*CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY*/,
        format, &desc, NULL, &error), error, inbuf);

    OCLERROR_RET(clSetKernelArg(blur, 0, sizeof(cl_mem), &input_image_buf), error, outbuf);
    OCLERROR_RET(clSetKernelArg(blur, 1, sizeof(cl_mem), &output_image_buf), error, outbuf);
    OCLERROR_RET(clSetKernelArg(blur, 2, sizeof(cl_int), &blur_opts.size), error, outbuf);

    size_t image_size[3] = { input_image.width, input_image.height, 1 };
    size_t origin[3] = { 0, 0, 0 };

    OCLERROR_RET(clEnqueueWriteImage(queue, input_image_buf, CL_NON_BLOCKING, origin, image_size, 0, 0,
        input_image.pixels, 0, NULL, NULL), error, outbuf);

    clFinish(queue);

    size_t shift[3] = {0, 0, 0};
    size_t size[3] = { input_image.width * 1 - shift[0] - 0, input_image.height * 1 - shift[1] - 0, 1 };
    OCLERROR_RET(clEnqueueNDRangeKernel(queue, blur, 2, shift, size, NULL, 0, NULL, NULL), error, outbuf);

    clFinish(queue);

    OCLERROR_RET(clEnqueueReadImage(queue, output_image_buf, CL_BLOCKING, origin, image_size, 0, 0,
        output_image.pixels, 0, NULL, NULL), error, outbuf);

    if (input_image.pixel_size != output_image.pixel_size) {
        const size_t
            pixels = input_image.width * input_image.height,
            pixel_size = output_image.pixel_size;
        for (size_t i = 1; i < pixels; ++i)
            memcpy(output_image.pixels + pixel_size * i, output_image.pixels + 4 * i, pixel_size);
    }

    OCLERROR_PAR(cl_sdk_write_image(blur_opts.out, &output_image, &error), error, outbuf);
    printf("File %s written.", blur_opts.out);

outbuf: OCLERROR_RET(clReleaseMemObject(output_image_buf), end_error, inbuf);
inbuf:  OCLERROR_RET(clReleaseMemObject(input_image_buf), end_error, outim);
frmt:   free(formats);
outim:  free(output_image.pixels);
inim:   free(input_image.pixels);
blr:    OCLERROR_RET(clReleaseKernel(blur), end_error, prg);
prg:    OCLERROR_RET(clReleaseProgram(program), end_error, ker);
ker:    free(kernel);
que:    OCLERROR_RET(clReleaseCommandQueue(queue), end_error, cont);
cont:   OCLERROR_RET(clReleaseContext(context), end_error, end);
end:    if (error) cl_util_print_error(error);
    return error;
}
