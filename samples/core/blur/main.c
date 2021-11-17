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
#include<math.h>

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
        IF_ERR(opts->in = value)
    case 'o':
        IF_ERR(opts->out = value)
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

#define PARS_OPTIONS(parser)                        \
if ((state = parser) == ParsedOK)                   \
    continue;                                       \
else if (state == ParseError)                       \
    {printf("Parse error\n"); identifier = 'h';}

        PARS_OPTIONS(parse_DiagnosticOptions(identifier, diag_opts))
        PARS_OPTIONS(parse_SingleDeviceOptions(identifier, &cag_context, dev_opts))
        PARS_OPTIONS(parse_BlurOptions(identifier, &cag_context, blur_opts))

        if (identifier == 'h') {
            printf("Usage: blur [OPTION]...\n");
            printf("Option name and value should be separated by '=' or a space\n");
            /*printf("Demonstrates how to query various OpenCL extensions applicable "
                "in the context of a reduction algorithm and to touch up kernel sources "
                "at runtime to select the best kernel implementation for the task.\n\n");*/
            cag_option_print(opts, n, stdout);
            exit((state == ParseError) ? CL_INVALID_ARG_VALUE : CL_SUCCESS);
        }
#undef PARS_OPTIONS
    }

end:    free(opts);
    return error;
}

cl_image_format set_image_format(cl_sdk_image * const input_image, cl_sdk_image * const output_image,
    const cl_context context, const bool verbose, cl_int * const error)
{
    // this format is always supported
    cl_image_format res = {
        .image_channel_order = CL_RGBA,
        .image_channel_data_type = CL_UNSIGNED_INT8
    };
    *error = CL_SUCCESS;

    if ((input_image->pixel_size == 1) || (input_image->pixel_size == 3)) {
        cl_image_format * formats = NULL;
        cl_uint formats_number = 0;
        OCLERROR_RET(clGetSupportedImageFormats(context, CL_MEM_READ_ONLY, CL_MEM_OBJECT_IMAGE2D,
            0, NULL, &formats_number), *error, end);
        MEM_CHECK(formats = (cl_image_format *)malloc(sizeof(cl_image_format) * formats_number), *error, end);
        OCLERROR_RET(clGetSupportedImageFormats(context, CL_MEM_READ_ONLY, CL_MEM_OBJECT_IMAGE2D,
            formats_number, formats, NULL), *error, end);
        // search for 3/1 channels of uint8_t
        for (cl_uint i = 0; i < formats_number; ++i)
            if (    ((input_image->pixel_size == 3)
                    && (formats[i].image_channel_order == CL_RGB)
                    && (formats[i].image_channel_data_type == CL_UNSIGNED_INT8))
                ||  ((input_image->pixel_size == 1)
                    && (formats[i].image_channel_order == CL_R)
                    && (formats[i].image_channel_data_type == CL_UNSIGNED_INT8)))
            {
                return formats[i];
            }

        // if not found, default to 4 channels of uint8_t
        if (verbose)
            printf("Converting picture into supported format... ");

        const size_t
            pixels = input_image->width * input_image->height,
            new = sizeof(unsigned char) * pixels * 4;
        unsigned char * tmp = NULL;
        MEM_CHECK(tmp = (unsigned char *)realloc(input_image->pixels, new), *error, end);
        input_image->pixels = tmp;
        MEM_CHECK(tmp = (unsigned char *)realloc(output_image->pixels, new), *error, end);
        output_image->pixels = tmp;

        // change picture
        const size_t pixel_size = input_image->pixel_size;
        for (size_t i = pixels - 1; i != 0; --i) {
            memcpy(input_image->pixels + 4 * i, input_image->pixels + pixel_size * i, pixel_size);
            memset(input_image->pixels + 4 * i + pixel_size, 0, 4 - pixel_size);
        }
        memset(input_image->pixels + pixel_size, 0, 4 - pixel_size);
        input_image->pixel_size = 4;
        // store initial pixel_size in output_image->pixel_size
        if (verbose)
            printf("done.\n");
    }
    else if (input_image->pixel_size != 4) {
        fprintf(stderr, "Unknown image format!\n");
        *error = CL_INVALID_IMAGE_FORMAT_DESCRIPTOR;
        return res;
    }

    // show image format used
    if (verbose) {
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

end:    return res;
}

cl_int finalize_blur(cl_sdk_image * const input_image, cl_sdk_image * const output_image,
    const char * const filename, const cl_uint step)
{
    cl_int error = CL_SUCCESS;

    // restore image type if needed
    if (input_image->pixel_size != output_image->pixel_size) {
        const size_t
            pixels = input_image->width * input_image->height,
            pixel_size = output_image->pixel_size;
        for (size_t i = 1; i < pixels; ++i)
            memcpy(output_image->pixels + pixel_size * i, output_image->pixels + 4 * i, pixel_size);
    }

    char name[1024];
    sprintf(name, "%u", step);
    strncat(name, filename, sizeof(name) - 2);
    cl_sdk_write_image(name, output_image, &error);
    if (error == CL_SUCCESS)
        printf("Image %s written.\n\n", name);

    return error;
}

cl_int print_timings(struct timespec start, struct timespec end, cl_event * event_list, cl_uint event_number)
{
    cl_int error = CL_SUCCESS;

    cl_ulong time_host, time_device = 0;
    TIMER_DIFFERENCE(time_host, start, end)
    for (cl_uint i = 0; i < event_number; ++i) {
        time_device += cl_util_get_event_duration(event_list[i], CL_PROFILING_COMMAND_START, CL_PROFILING_COMMAND_END, &error);
        if (error != CL_SUCCESS)
            return error;
    }
    printf("Execution time as seen by host: %llu us, by device: %llu us\n",
        (unsigned long long)(time_host + 500) / 1000,
        (unsigned long long)(time_device + 500) / 1000);

    return error;
}

int main(int argc, char* argv[])
{
    cl_int error = CL_SUCCESS,
        end_error = CL_SUCCESS;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_event pass[3]; // events to measure execution time
    cl_uint im = 1;

    /// Parse command-line options
    struct cl_sdk_options_Diagnostic diag_opts = { .quiet = false, .verbose = false };
    struct cl_sdk_options_SingleDevice dev_opts = { .triplet = { 0, 0, CL_DEVICE_TYPE_ALL } };
    struct options_Blur blur_opts = { .size = 1, .op = "box", .in = NULL, .out = "out.png" };

    OCLERROR_RET(parse_options(argc, argv, &diag_opts, &dev_opts, &blur_opts), error, end);
    if (!blur_opts.in) {
        error = CL_INVALID_IMAGE_DESCRIPTOR;
        fprintf(stderr, "No input image name!\n");
        goto end;
    }

    /// Create runtime objects based on user preference or default
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

    /// Read input image and prepare output image
    cl_sdk_image input_image = { .width = 0, .height = 0, .pixel_size = 0, .pixels = NULL };
    OCLERROR_PAR(input_image = cl_sdk_read_image(blur_opts.in, &error), error, que);

    cl_sdk_image output_image = { .width = input_image.width, .height = input_image.height,
        .pixel_size = input_image.pixel_size, .pixels = NULL };
    MEM_CHECK(output_image.pixels = (unsigned char *)malloc(sizeof(unsigned char) *
        output_image.width * output_image.height * output_image.pixel_size), error, inim);

    /// Query device and runtime capabilities
    // 1) query image support
    cl_bool image_support = false;
    OCLERROR_RET(clGetDeviceInfo(device, CL_DEVICE_IMAGE_SUPPORT, sizeof(cl_bool), &image_support, NULL), error, outim);
    if (!image_support) {
        fprintf(stderr, "No image support on device!\n");
        error = CL_INVALID_DEVICE;
        goto outim;
    }

    // 2) query if the image format is supported and change image if not
    cl_image_format format;
    OCLERROR_PAR(format = set_image_format(&input_image, &output_image, context, diag_opts.verbose, &error), error, outim);

    // 3) query if device have local memory and its size
    bool use_local_mem = false;
    cl_device_local_mem_type mt;
    OCLERROR_RET(clGetDeviceInfo(device,
        CL_DEVICE_LOCAL_MEM_TYPE, sizeof(cl_device_local_mem_type), &mt, NULL), error, outim);
    if (mt == CL_LOCAL)
        use_local_mem = true;

    cl_ulong loc_mem;
    OCLERROR_RET(clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &loc_mem, NULL), error, outim);

    // 4) query if device allow subgroup shuffle operations
    bool use_subgroup_exchange = false, use_subgroup_exchange_relative = false;
    {
        char * name = NULL, * tmp1 = NULL, * tmp2 = NULL;
        OCLERROR_PAR(name = cl_util_get_device_info(device, CL_DEVICE_EXTENSIONS, &error), error, nam);
        printf("%s\n\n", name);
        tmp1 = strstr(name, "cl_khr_subgroup_shuffle_relative");
        if (tmp1)
            use_subgroup_exchange_relative = true;
        tmp2 = strstr(name, "cl_khr_subgroup_shuffle");
        if (tmp2 && (tmp2 == tmp1)) // exclude relative
            tmp2 = strstr(tmp2 + 1, "cl_khr_subgroup_shuffle");
        if (tmp2)
            use_subgroup_exchange = true;

nam:    free(name);
    }

    /// Create image buffers
    cl_mem input_image_buf, output_image_buf, temp_image_buf;
    const cl_image_desc desc = {
        .image_type      = CL_MEM_OBJECT_IMAGE2D,
        .image_width     = input_image.width,
        .image_height    = input_image.height,
        .image_row_pitch = 0,
        .num_mip_levels  = 0,
        .num_samples     = 0,
        .mem_object      = NULL
    };
    OCLERROR_PAR(input_image_buf = clCreateImage(context, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY,
        &format, &desc, NULL, &error), error, outim);
    OCLERROR_PAR(output_image_buf = clCreateImage(context, CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY,
        &format, &desc, NULL, &error), error, inbuf);

    size_t image_size[3] = { input_image.width, input_image.height, 1 };
    size_t origin[3] = { 0, 0, 0 };

    OCLERROR_RET(clEnqueueWriteImage(queue, input_image_buf, CL_NON_BLOCKING, origin, image_size, 0, 0,
        input_image.pixels, 0, NULL, NULL), error, outbuf);

    /// Create OpenCL program
    const char * kernel_location = "./blur.cl";
    char * kernel = NULL, * tmp = NULL;
    size_t program_size = 0;
    cl_program program = NULL;
    char kernel_op[1024] = ""; // here we can put some dynamic definitions

    OCLERROR_PAR(kernel = cl_util_read_text_file(kernel_location, &program_size, &error), error, outbuf);
    //program_size += 1 + strlen(kernel_op);
    //MEM_CHECK(tmp = (char *)realloc(kernel, program_size), error, ker);
    //strcat(tmp, kernel_op);
    //kernel = tmp;
    //printf("%s", kernel);

    OCLERROR_PAR(program = clCreateProgramWithSource(context, 1,
        (const char **)&kernel, &program_size, &error), error, ker);
    OCLERROR_RET(cl_util_build_program(program, device, kernel_op), error, prg);
    kernel_op[0] = '\0';

    /// Single-pass blur
    printf("Single-pass blur\n");

    // compile kernel
    cl_kernel blur;
    OCLERROR_PAR(blur = clCreateKernel(program, "blur_box", &error), error, prg);

    // set kernel parameters
    OCLERROR_RET(clSetKernelArg(blur, 0, sizeof(cl_mem), &input_image_buf), error, blr);
    OCLERROR_RET(clSetKernelArg(blur, 1, sizeof(cl_mem), &output_image_buf), error, blr);
    OCLERROR_RET(clSetKernelArg(blur, 2, sizeof(cl_int), &blur_opts.size), error, blr);

    // blur
    GET_CURRENT_TIMER(single_start)
    OCLERROR_RET(clEnqueueNDRangeKernel(queue, blur, 2, origin, image_size, NULL, 0, NULL, pass), error, blr);
    OCLERROR_RET(clWaitForEvents(1, pass), error, blr);
    GET_CURRENT_TIMER(single_end)

    OCLERROR_RET(clEnqueueReadImage(queue, output_image_buf, CL_BLOCKING, origin, image_size, 0, 0,
        output_image.pixels, 0, NULL, NULL), error, blr);

    if (diag_opts.verbose)
        print_timings(single_start, single_end, pass, 1);

    // write output file
    OCLERROR_RET(finalize_blur(&input_image, &output_image, blur_opts.out, im), error, blr);

    /// Dual-pass blur
    printf("Dual-pass blur\n");
    ++im;

    // create temporary buffer
    OCLERROR_PAR(temp_image_buf = clCreateImage(context, CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY,
        &format, &desc, NULL, &error), error, blr);

    // create kernels
    cl_kernel blur1, blur2;
    OCLERROR_PAR(blur1 = clCreateKernel(program, "blur_box_horizontal", &error), error, tmpbuf);
    OCLERROR_PAR(blur2 = clCreateKernel(program, "blur_box_vertical", &error), error, blr1);

    // set kernel parameters
    OCLERROR_RET(clSetKernelArg(blur1, 0, sizeof(cl_mem), &input_image_buf), error, blr2);
    OCLERROR_RET(clSetKernelArg(blur1, 1, sizeof(cl_mem), &temp_image_buf), error, blr2);
    OCLERROR_RET(clSetKernelArg(blur1, 2, sizeof(cl_int), &blur_opts.size), error, blr2);

    OCLERROR_RET(clSetKernelArg(blur2, 0, sizeof(cl_mem), &temp_image_buf), error, blr2);
    OCLERROR_RET(clSetKernelArg(blur2, 1, sizeof(cl_mem), &output_image_buf), error, blr2);
    OCLERROR_RET(clSetKernelArg(blur2, 2, sizeof(cl_int), &blur_opts.size), error, blr2);

    // blur
    GET_CURRENT_TIMER(dual_start)
    OCLERROR_RET(clEnqueueNDRangeKernel(queue, blur1, 2, origin, image_size, NULL, 0, NULL, pass + 1), error, blr2);
    OCLERROR_RET(clEnqueueNDRangeKernel(queue, blur2, 2, origin, image_size, NULL, 0, NULL, pass + 2), error, blr2);
    OCLERROR_RET(clWaitForEvents(2, pass + 1), error, blr2);
    GET_CURRENT_TIMER(dual_end)

    OCLERROR_RET(clEnqueueReadImage(queue, output_image_buf, CL_BLOCKING, origin, image_size, 0, 0,
        output_image.pixels, 0, NULL, NULL), error, blr2);

    if (diag_opts.verbose)
        print_timings(dual_start, dual_end, pass + 1, 2);

    // write output file
    OCLERROR_RET(finalize_blur(&input_image, &output_image, blur_opts.out, im), error, blr2);

    /// Use local memory exchange in dual-pass blur
    if (use_local_mem) {
        printf("Dual-pass local memory exchange blur\n");
        ++im;

        clReleaseKernel(blur2);
        clReleaseKernel(blur1);

        OCLERROR_PAR(blur1 = clCreateKernel(program, "blur_box_horizontal_exchange", &error), error, tmpbuf);
        OCLERROR_PAR(blur2 = clCreateKernel(program, "blur_box_vertical_exchange", &error), error, blr1);

        // 4) query maximum supported WGS of kernel on device based on private mem (register) constraints
        size_t wgs1, psm1, wgs2, psm2;
        OCLERROR_RET(clGetKernelWorkGroupInfo(blur1, device,
            CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &wgs1, NULL), error, blr2);
        OCLERROR_RET(clGetKernelWorkGroupInfo(blur1, device,
            CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(size_t), &psm1, NULL), error, blr2);
        OCLERROR_RET(clGetKernelWorkGroupInfo(blur2, device,
            CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &wgs2, NULL), error, blr2);
        OCLERROR_RET(clGetKernelWorkGroupInfo(blur2, device,
            CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(size_t), &psm2, NULL), error, blr2);

        // Further constrain (reduce) WGS based on shared mem size on device
        if (loc_mem >= (max(psm1, psm2) + 2 * blur_opts.size) * sizeof(cl_uchar4)) {
            while (loc_mem < (wgs1 + 2 * blur_opts.size) * sizeof(cl_uchar4))
                wgs1 -= psm1;
            while (loc_mem < (wgs2 + 2 * blur_opts.size) * sizeof(cl_uchar4))
                wgs2 -= psm2;
        }
        else {
            printf("Not enough local memory to serve a single sub-group.\n");
            error = CL_OUT_OF_RESOURCES;
            goto blr2;
        }

        // set kernel parameters
        OCLERROR_RET(clSetKernelArg(blur1, 0, sizeof(cl_mem), &input_image_buf), error, blr2);
        OCLERROR_RET(clSetKernelArg(blur1, 1, sizeof(cl_mem), &temp_image_buf), error, blr2);
        OCLERROR_RET(clSetKernelArg(blur1, 2, sizeof(cl_int), &blur_opts.size), error, blr2);
        OCLERROR_RET(clSetKernelArg(blur1, 3, sizeof(cl_uchar4) * (wgs1 + 2 * blur_opts.size), NULL), error, blr2);

        OCLERROR_RET(clSetKernelArg(blur2, 0, sizeof(cl_mem), &temp_image_buf), error, blr2);
        OCLERROR_RET(clSetKernelArg(blur2, 1, sizeof(cl_mem), &output_image_buf), error, blr2);
        OCLERROR_RET(clSetKernelArg(blur2, 2, sizeof(cl_int), &blur_opts.size), error, blr2);
        OCLERROR_RET(clSetKernelArg(blur2, 3, sizeof(cl_uchar4) * (wgs2 + 2 * blur_opts.size), NULL), error, blr2);

        // blur
        GET_CURRENT_TIMER(dual_start)
        size_t work_size1[3] = { (input_image.width + wgs1 - 1) / wgs1 * wgs1, input_image.height, 1 };
        size_t wgsf[3] = { wgs1, 1, 1 };
        OCLERROR_RET(clEnqueueNDRangeKernel(queue, blur1, 2, origin, work_size1, wgsf, 0, NULL, pass + 1), error, blr2);
        size_t work_size2[3] = { input_image.width, (input_image.height + wgs2 - 1) / wgs2 * wgs2, 1 };
        size_t wgss[3] = { 1, wgs2, 1 };
        OCLERROR_RET(clEnqueueNDRangeKernel(queue, blur2, 2, origin, work_size2, wgss, 0, NULL, pass + 2), error, blr2);
        OCLERROR_RET(clWaitForEvents(2, pass + 1), error, blr2);
        GET_CURRENT_TIMER(dual_end)

        OCLERROR_RET(clEnqueueReadImage(queue, output_image_buf, CL_BLOCKING, origin, image_size, 0, 0,
            output_image.pixels, 0, NULL, NULL), error, blr2);

        if (diag_opts.verbose)
            print_timings(dual_start, dual_end, pass + 1, 2);

        // write output file
        OCLERROR_RET(finalize_blur(&input_image, &output_image, blur_opts.out, im), error, blr2);
    }

    /// Subgroup exchange
    while (use_subgroup_exchange_relative || use_subgroup_exchange) {
        if (use_subgroup_exchange_relative)
            printf("Dual-pass subgroup relative exchange blur\n");
        else if (use_subgroup_exchange)
            printf("Dual-pass subgroup exchange blur\n");
        ++im;

        clReleaseKernel(blur2);
        clReleaseKernel(blur1);
        cl_program pr;

        // TODO error handling
        kernel_op[0] = '\0';
        if (use_subgroup_exchange_relative)
            strcat(kernel_op, "-D USE_SUBGROUP_EXCHANGE_RELATIVE ");
        else if (use_subgroup_exchange)
            strcat(kernel_op, "-D USE_SUBGROUP_EXCHANGE ");
        OCLERROR_PAR(pr = clCreateProgramWithSource(context, 1,
            (const char **)&kernel, &program_size, &error), error, tmpbuf);
        OCLERROR_RET(cl_util_build_program(pr, device, kernel_op), error, tmpbuf);

        // create kernels
        OCLERROR_PAR(blur1 = clCreateKernel(pr, "blur_box_horizontal_subgroup_exchange", &error), error, tmpbuf);
        OCLERROR_PAR(blur2 = clCreateKernel(pr, "blur_box_vertical_subgroup_exchange", &error), error, blr1);

        // set kernel parameters
        OCLERROR_RET(clSetKernelArg(blur1, 0, sizeof(cl_mem), &input_image_buf), error, blr2);
        OCLERROR_RET(clSetKernelArg(blur1, 1, sizeof(cl_mem), &temp_image_buf), error, blr2);
        OCLERROR_RET(clSetKernelArg(blur1, 2, sizeof(cl_int), &blur_opts.size), error, blr2);

        OCLERROR_RET(clSetKernelArg(blur2, 0, sizeof(cl_mem), &temp_image_buf), error, blr2);
        OCLERROR_RET(clSetKernelArg(blur2, 1, sizeof(cl_mem), &output_image_buf), error, blr2);
        OCLERROR_RET(clSetKernelArg(blur2, 2, sizeof(cl_int), &blur_opts.size), error, blr2);

        // 5) query preferred subgroup size of kernel on device
        size_t wgs1, wgs2;
        OCLERROR_RET(clGetKernelWorkGroupInfo(blur1, device,
            CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(size_t), &wgs1, NULL), error, blr2);
        OCLERROR_RET(clGetKernelWorkGroupInfo(blur2, device,
            CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(size_t), &wgs2, NULL), error, blr2);

        // blur
        GET_CURRENT_TIMER(rel_start)
        size_t work_size1[3] = { (input_image.width + wgs1 - 1) / wgs1 * wgs1, input_image.height, 1 };
        size_t wgsf[3] = { wgs1, 1, 1 };
        OCLERROR_RET(clEnqueueNDRangeKernel(queue, blur1, 2, origin, work_size1, wgsf, 0, NULL, pass + 1), error, blr2);
        size_t work_size2[3] = { input_image.width, (input_image.height + wgs2 - 1) / wgs2 * wgs2, 1 };
        size_t wgss[3] = { 1, wgs2, 1 };
        OCLERROR_RET(clEnqueueNDRangeKernel(queue, blur2, 2, origin, work_size2, wgss, 0, NULL, pass + 2), error, blr2);
        OCLERROR_RET(clWaitForEvents(2, pass + 1), error, blr2);
        GET_CURRENT_TIMER(rel_end)

        OCLERROR_RET(clEnqueueReadImage(queue, output_image_buf, CL_BLOCKING, origin, image_size, 0, 0,
            output_image.pixels, 0, NULL, NULL), error, blr2);

        if (diag_opts.verbose)
            print_timings(rel_start, rel_end, pass + 1, 2);

        // write output file
        OCLERROR_RET(finalize_blur(&input_image, &output_image, blur_opts.out, im), error, blr2);

        if (use_subgroup_exchange_relative) {
            use_subgroup_exchange_relative = false;
            continue;
        }
        else if (use_subgroup_exchange) {
            use_subgroup_exchange = false;
            continue;
        }
    }

    /// Cleanup
blr2:   OCLERROR_RET(clReleaseKernel(blur2), end_error, blr1);
blr1:   OCLERROR_RET(clReleaseKernel(blur1), end_error, tmpbuf);
tmpbuf: OCLERROR_RET(clReleaseMemObject(temp_image_buf), end_error, blr);
blr:    OCLERROR_RET(clReleaseKernel(blur), end_error, prg);
prg:    OCLERROR_RET(clReleaseProgram(program), end_error, ker);
ker:    free(kernel);
outbuf: OCLERROR_RET(clReleaseMemObject(output_image_buf), end_error, inbuf);
inbuf:  OCLERROR_RET(clReleaseMemObject(input_image_buf), end_error, outim);
outim:  free(output_image.pixels);
inim:   free(input_image.pixels);
que:    OCLERROR_RET(clReleaseCommandQueue(queue), end_error, cont);
cont:   OCLERROR_RET(clReleaseContext(context), end_error, end);
end:    if (error) cl_util_print_error(error);
    return error;
}
