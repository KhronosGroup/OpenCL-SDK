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

// OpenCL SDK includes
#include <CL/Utils/Utils.h>
#include <CL/SDK/CLI.h>
#include <CL/SDK/Image.h>
#include <CL/Utils/Event.h>

// standard includes
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdatomic.h>
#include <string.h>
#include <time.h>
#include <threads.h>

#define THRDERROR(func, err, label)                                            \
    do                                                                         \
    {                                                                          \
        if (thrd_success != (func))                                            \
        {                                                                      \
            err = -1;                                                          \
            goto label;                                                        \
        }                                                                      \
    } while (0)

typedef struct
{
    cl_device_id device;
    cl_context context;
    cl_event compute_event;
    cl_event copy_event;
    cl_event read_event;
    cl_command_queue compute_queue;
    cl_command_queue copy_queue;
    cl_command_queue read_queue;
    cl_platform_id platform;
    cl_program program;
    cl_kernel kernel;
    cl_mem img_a;
    cl_mem img_b;
    cl_mem copy_buffer;
} state_t;

typedef struct
{
    cnd_t image_save_finished_signal;
    mtx_t image_save_finished_mtx;
    size_t image_save_finished_count;
} global_state_t;

static global_state_t g_state;

typedef struct image_save_task
{
    cl_sdk_image image;
    size_t index;
} image_save_task;

static int image_save_consumer_thread(void* data)
{
    int error = 0;
    image_save_task* task = (image_save_task*)data;
    char image_filename[FILENAME_MAX];
    memset(image_filename, 0, FILENAME_MAX);
    sprintf(image_filename, "callback_out_%zu.png", task->index);
    OCLERROR_RET(cl_sdk_write_image(image_filename, &task->image), error, end);
    printf("Written image to %s\n", image_filename);

    // signal completion
    THRDERROR(mtx_lock(&g_state.image_save_finished_mtx), error, end);
    g_state.image_save_finished_count++;
    THRDERROR(mtx_unlock(&g_state.image_save_finished_mtx), error, end);
    THRDERROR(cnd_signal(&g_state.image_save_finished_signal), error, end);

end:
    free(task->image.pixels);
    free(task);
    if (error != 0)
    {
        printf("image_save_consumer_thread encountered an error. Exiting");
        exit(error);
    }
    return error;
}

static void start_image_save_task(cl_event event, cl_int status,
                                  void* user_data)
{
    (void)event;
    (void)status;
    int error = 0;
    // While it's not optimal to launch a thread for each saved image,
    // for the purpose of demonstration it is sufficient.
    thrd_t thread_handle;
    THRDERROR(
        thrd_create(&thread_handle, image_save_consumer_thread, user_data),
        error, end);
    THRDERROR(thrd_detach(thread_handle), error, end);

end:
    if (error != 0)
    {
        printf("An error occurred in callback, exiting");
        exit(error);
    }
}

static cl_int enqueue_buffer_read(cl_command_queue queue, cl_mem buf,
                                  size_t side, size_t idx,
                                  const cl_event* wait_event, cl_event* event)
{
    cl_int error = CL_SUCCESS;
    image_save_task* task;
    MEM_CHECK(task = (image_save_task*)malloc(sizeof(image_save_task)), error,
              error_end);
    task->image.width = side;
    task->image.height = side;
    task->image.pixel_size = sizeof(cl_uchar4);
    MEM_CHECK(task->image.pixels =
                  (unsigned char*)malloc(side * side * sizeof(cl_uchar4)),
              error, free_task);
    task->index = idx;

    OCLERROR_RET(clEnqueueReadBuffer(queue, buf, CL_FALSE, 0,
                                     side * side * sizeof(cl_uchar4),
                                     task->image.pixels, 1, wait_event, event),
                 error, free_pixels);
    OCLERROR_RET(
        clSetEventCallback(*event, CL_COMPLETE, start_image_save_task, task),
        error, free_pixels);
    return error;

free_pixels:
    free(task->image.pixels);
free_task:
    free(task);
error_end:
    return error;
}

// Sample-specific option
struct options_Callback
{
    size_t side;
    size_t iterations;
    size_t write_iter;
};

cag_option CallbackOptions[] = {
    { .identifier = 's',
      .access_letters = "s",
      .access_name = "side",
      .value_name = "(positive int)",
      .description = "Side length of the generated image in pixels" },

    { .identifier = 'i',
      .access_letters = "i",
      .access_name = "iter",
      .value_name = "(positive int)",
      .description = "Number of iterations in the simulation" },

    { .identifier = 'w',
      .access_letters = "w",
      .access_name = "write_iter",
      .value_name = "(positive int)",
      .description = "Controls after how many iterations the intermediate "
                     "result is written to file" },
};

ParseState parse_CallbackOptions(const char identifier,
                                 cag_option_context* cag_context,
                                 struct options_Callback* opts)
{
    const char* value;

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
        case 's': IF_ERR(opts->side = (size_t)atoi(value))
        case 'i': IF_ERR(opts->iterations = (size_t)atoi(value))
        case 'w': IF_ERR(opts->write_iter = (size_t)atoi(value))
    }
    return NotParsed;
}

static cl_int parse_options(int argc, char* argv[],
                            struct cl_sdk_options_SingleDevice* dev_opts,
                            struct options_Callback* cb_opts)
{
    cl_int error = CL_SUCCESS;
    struct cag_option *opts = NULL, *tmp = NULL;
    size_t n = 0;

    /* Prepare all options array. */
    MEM_CHECK(opts = add_CLI_options(opts, &n, SingleDeviceOptions,
                                     CAG_ARRAY_SIZE(SingleDeviceOptions)),
              error, end);
    MEM_CHECK(tmp = add_CLI_options(opts, &n, DiagnosticOptions,
                                    CAG_ARRAY_SIZE(DiagnosticOptions)),
              error, end);
    opts = tmp;
    MEM_CHECK(tmp = add_CLI_options(opts, &n, CallbackOptions,
                                    CAG_ARRAY_SIZE(CallbackOptions)),
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

        PARS_OPTIONS(
            parse_SingleDeviceOptions(identifier, &cag_context, dev_opts),
            state);
        PARS_OPTIONS(parse_CallbackOptions(identifier, &cag_context, cb_opts),
                     state);

        if (identifier == 'h')
        {
            printf("Usage: callback [OPTION]...\n");
            printf("Option name and value should be separated by '=' or a "
                   "space\n");
            cag_option_print(opts, n, stdout);
            exit((state == ParseError) ? CL_INVALID_ARG_VALUE : CL_SUCCESS);
        }
    }

end:
    free(opts);
    return error;
}

int main(int argc, char* argv[])
{
    state_t state;
    srand(time(NULL));
    g_state.image_save_finished_count = 0;
    int error = 0;
    THRDERROR(mtx_init(&g_state.image_save_finished_mtx, mtx_plain), error,
              end);
    THRDERROR(cnd_init(&g_state.image_save_finished_signal), error, save_mtx);

    struct cl_sdk_options_SingleDevice dev_opts = {
        .triplet = { 0, 0, CL_DEVICE_TYPE_ALL }
    };
    struct options_Callback cb_opts = {
        .side = 512,
        .iterations = 10000,
        .write_iter = 1000,
    };

    OCLERROR_RET(parse_options(argc, argv, &dev_opts, &cb_opts), error,
                 save_cnd);
    /// Create runtime objects based on user preference or default
    OCLERROR_PAR(state.device = cl_util_get_device(
                     dev_opts.triplet.plat_index, dev_opts.triplet.dev_index,
                     dev_opts.triplet.dev_type, &error),
                 error, save_cnd);
    cl_util_print_device_info(state.device);
    OCLERROR_PAR(state.context = clCreateContext(NULL, 1, &state.device, NULL,
                                                 NULL, &error),
                 error, dev);
    OCLERROR_RET(clGetDeviceInfo(state.device, CL_DEVICE_PLATFORM,
                                 sizeof(cl_platform_id), &state.platform, NULL),
                 error, cont);

    size_t kernel_source_length;
    char* kernel_source;
    OCLERROR_PAR(kernel_source = cl_util_read_text_file(
                     "./reaction_diffusion.cl", &kernel_source_length, &error),
                 error, cont);
    OCLERROR_PAR(state.program = clCreateProgramWithSource(
                     state.context, 1, (const char**)&kernel_source,
                     &kernel_source_length, &error),
                 error, ksource);
    OCLERROR_RET(cl_util_build_program(state.program, state.device, NULL),
                 error, prog);
    OCLERROR_PAR(state.kernel = clCreateKernel(
                     state.program, "reaction_diffusion_step", &error),
                 error, prog);
    cl_command_queue_properties props[] = { CL_QUEUE_PROPERTIES,
                                            CL_QUEUE_PROFILING_ENABLE, 0 };
    OCLERROR_PAR(state.compute_queue = clCreateCommandQueueWithProperties(
                     state.context, state.device, props, &error),
                 error, kern);
    OCLERROR_PAR(state.copy_queue = clCreateCommandQueueWithProperties(
                     state.context, state.device, props, &error),
                 error, comp_queue);
    OCLERROR_PAR(state.read_queue = clCreateCommandQueueWithProperties(
                     state.context, state.device, props, &error),
                 error, copy_queue);
    cl_uint num_image_formats;
    OCLERROR_RET(clGetSupportedImageFormats(state.context, CL_MEM_READ_WRITE,
                                            CL_MEM_OBJECT_IMAGE2D, 0, NULL,
                                            &num_image_formats),
                 error, read_queue);
    cl_image_format* supported_image_formats;
    MEM_CHECK(supported_image_formats = (cl_image_format*)malloc(
                  sizeof(cl_image_format) * num_image_formats),
              error, read_queue);

    OCLERROR_RET(clGetSupportedImageFormats(
                     state.context, CL_MEM_READ_WRITE, CL_MEM_OBJECT_IMAGE2D,
                     num_image_formats, supported_image_formats, NULL),
                 error, free_image_formats);
    cl_image_format image_format = {
        .image_channel_order = CL_RGBA,
        .image_channel_data_type = CL_UNORM_INT8,
    };
    bool found_required_image_format = false;
    for (cl_uint image_format_idx = 0; image_format_idx < num_image_formats;
         ++image_format_idx)
    {
        if (supported_image_formats[image_format_idx].image_channel_data_type
                == image_format.image_channel_data_type
            && supported_image_formats[image_format_idx].image_channel_order
                == image_format.image_channel_order)
        {
            found_required_image_format = true;
            break;
        }
    }
    if (!found_required_image_format)
    {
        error = -1;
        printf("Error: the required OpenCL image format is not supported by "
               "the selected device");
        goto free_image_formats;
    }

    cl_image_desc image_desc = {
        .image_height = cb_opts.side,
        .image_width = cb_opts.side,
        .image_type = CL_MEM_OBJECT_IMAGE2D,
    };
    OCLERROR_PAR(state.img_a =
                     clCreateImage(state.context, CL_MEM_READ_WRITE,
                                   &image_format, &image_desc, NULL, &error),
                 error, free_image_formats);
    OCLERROR_PAR(state.img_b =
                     clCreateImage(state.context, CL_MEM_READ_WRITE,
                                   &image_format, &image_desc, NULL, &error),
                 error, free_image_a);
    cl_float4 fill_color = { { 1.F, 0.F, 0.F, 1.F } };
    size_t fill_origin[3] = { 0, 0, 0 };
    size_t fill_region[3] = { cb_opts.side, cb_opts.side, 1 };
    OCLERROR_RET(clEnqueueFillImage(state.compute_queue, state.img_a,
                                    &fill_color, fill_origin, fill_region, 0,
                                    NULL, NULL),
                 error, free_image_b);
    fill_color.y = 1.F;
    fill_origin[0] = cb_opts.side / 2;
    fill_origin[1] = cb_opts.side / 2;
    fill_region[0] = cb_opts.side / 100;
    fill_region[1] = cb_opts.side / 100;
    OCLERROR_RET(clEnqueueFillImage(state.compute_queue, state.img_a,
                                    &fill_color, fill_origin, fill_region, 0,
                                    NULL, NULL),
                 error, free_image_b);
    OCLERROR_PAR(state.copy_buffer = clCreateBuffer(
                     state.context, CL_MEM_READ_WRITE,
                     cb_opts.side * cb_opts.side * sizeof(cl_uchar4), NULL,
                     &error),
                 error, free_image_b);

    state.compute_event = NULL;
    state.copy_event = NULL;
    state.read_event = NULL;
    for (size_t iter = 0; iter < cb_opts.iterations; ++iter)
    {
        cl_uint work_dim = 2;
        size_t global_work_size[2] = { cb_opts.side, cb_opts.side };
        OCLERROR_RET(
            clSetKernelArg(state.kernel, 0, sizeof(cl_mem), &state.img_a),
            error, copy_buf);
        OCLERROR_RET(
            clSetKernelArg(state.kernel, 1, sizeof(cl_mem), &state.img_b),
            error, copy_buf);
        if (state.compute_event)
        {
            OCLERROR_RET(clReleaseEvent(state.compute_event), error, copy_buf);
        }
        OCLERROR_RET(clEnqueueNDRangeKernel(
                         state.compute_queue, state.kernel, work_dim, NULL,
                         global_work_size, NULL,
                         state.read_event == NULL ? 0 : 1,
                         state.read_event == NULL ? NULL : &state.read_event,
                         &state.compute_event),
                     error, copy_buf);

        if (iter % cb_opts.write_iter == 0)
        {
            size_t copy_origin[3] = { 0, 0, 0 };
            size_t copy_region[3] = { cb_opts.side, cb_opts.side, 1 };
            if (state.copy_event)
            {
                OCLERROR_RET(clReleaseEvent(state.copy_event), error,
                             compute_ev);
            }
            OCLERROR_RET(clEnqueueCopyImageToBuffer(
                             state.copy_queue, state.img_a, state.copy_buffer,
                             copy_origin, copy_region, 0, 1u,
                             &state.compute_event, &state.copy_event),
                         error, compute_ev);
            if (state.read_event)
            {
                OCLERROR_RET(clReleaseEvent(state.read_event), error, copy_ev);
            }
            OCLERROR_RET(
                enqueue_buffer_read(state.read_queue, state.copy_buffer,
                                    cb_opts.side, iter / cb_opts.write_iter,
                                    &state.copy_event, &state.read_event),
                error, copy_ev);
        }

        cl_mem tmp = state.img_a;
        state.img_a = state.img_b;
        state.img_b = tmp;
    }

    size_t num_saved_images =
        (cb_opts.iterations + cb_opts.write_iter - 1) / cb_opts.write_iter;

    for (;;)
    {
        THRDERROR(mtx_lock(&g_state.image_save_finished_mtx), error,
                  free_image_b);
        bool wait_for_saving =
            num_saved_images != g_state.image_save_finished_count;
        if (wait_for_saving)
        {
            cnd_wait(&g_state.image_save_finished_signal,
                     &g_state.image_save_finished_mtx);
            THRDERROR(mtx_unlock(&g_state.image_save_finished_mtx), error,
                      free_image_b);
        }
        else
        {
            THRDERROR(mtx_unlock(&g_state.image_save_finished_mtx), error,
                      free_image_b);
            break;
        }
    }

    cl_int end_error = CL_SUCCESS;
    // read_ev:
    OCLERROR_RET(clReleaseEvent(state.read_event), end_error, copy_ev);
copy_ev:
    OCLERROR_RET(clReleaseEvent(state.copy_event), end_error, compute_ev);
compute_ev:
    OCLERROR_RET(clReleaseEvent(state.compute_event), end_error, copy_buf);
copy_buf:
    OCLERROR_RET(clReleaseMemObject(state.copy_buffer), end_error,
                 free_image_b);
free_image_b:
    OCLERROR_RET(clReleaseMemObject(state.img_b), end_error, free_image_a);
free_image_a:
    OCLERROR_RET(clReleaseMemObject(state.img_a), end_error,
                 free_image_formats);
free_image_formats:
    free(supported_image_formats);
read_queue:
    OCLERROR_RET(clReleaseCommandQueue(state.read_queue), end_error,
                 comp_queue);
copy_queue:
    OCLERROR_RET(clReleaseCommandQueue(state.copy_queue), end_error,
                 comp_queue);
comp_queue:
    OCLERROR_RET(clReleaseCommandQueue(state.compute_queue), end_error, kern);
kern:
    OCLERROR_RET(clReleaseKernel(state.kernel), end_error, prog);
prog:
    OCLERROR_RET(clReleaseProgram(state.program), end_error, ksource);
ksource:
    free(kernel_source);
cont:
    OCLERROR_RET(clReleaseContext(state.context), end_error, dev);
dev:
    OCLERROR_RET(clReleaseDevice(state.device), end_error, save_cnd);
save_cnd:
    cnd_destroy(&g_state.image_save_finished_signal);
save_mtx:
    mtx_destroy(&g_state.image_save_finished_mtx);
end:
    if (error) cl_util_print_error(error);
    return error;
}
