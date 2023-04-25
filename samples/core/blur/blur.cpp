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
#include "blur.hpp"

#include <CL/Utils/Utils.hpp>
#include <CL/SDK/Context.hpp>
#include <CL/SDK/CLI.hpp>

// STL includes
#include <iostream>
#include <algorithm>
#include <map>
#include <math.h>

// TCLAP includes
#include <tclap/CmdLine.h>

// Default image
#include "default_image.h"

std::unique_ptr<TCLAP::ValuesConstraint<std::string>> valid_op_constraint;

// Add option to CLI parsing SDK utility
template <> auto cl::sdk::parse<BlurCppExample::BlurOptions>()
{
    return std::make_tuple(
        std::make_shared<TCLAP::ValueArg<std::string>>(
            "i", "in", "Input image file", false, "", "name"),
        std::make_shared<TCLAP::ValueArg<std::string>>(
            "o", "out", "Output image file", false, "out.png", "name"),
        std::make_shared<TCLAP::ValueArg<float>>("s", "size",
                                                 "Size of blur kernel", false,
                                                 (float)1.0, "positive float"),
        std::make_shared<TCLAP::MultiArg<std::string>>(
            "b", "blur", "Operation of blur to perform: box or gauss", false,
            "box"));
}

template <>
BlurCppExample::BlurOptions cl::sdk::comprehend<BlurCppExample::BlurOptions>(
    std::shared_ptr<TCLAP::ValueArg<std::string>> in_arg,
    std::shared_ptr<TCLAP::ValueArg<std::string>> out_arg,
    std::shared_ptr<TCLAP::ValueArg<float>> size_arg,
    std::shared_ptr<TCLAP::MultiArg<std::string>> op_arg)
{
    return BlurCppExample::BlurOptions{ in_arg->getValue(), out_arg->getValue(),
                                        size_arg->getValue(),
                                        op_arg->getValue() };
}

void BlurCppExample::single_pass_box_blur()
{
    std::cout << "Single-pass blur" << std::endl;
    step++;

    auto size = static_cast<cl_int>(blur_opts.size);
    auto blur =
        cl::KernelFunctor<cl::Memory, cl::Memory, cl_int>(program, "blur_box");
    // blur
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<cl::Event> passes;

    auto event = blur(cl::EnqueueArgs{ queue, cl::NDRange{ width, height } },
                      input_image_buf, output_image_buf, size);

    passes.push_back(event);

    event.wait();

    auto end = std::chrono::high_resolution_clock::now();

    cl::enqueueReadImage(output_image_buf, CL_BLOCKING, origin, image_size, 0,
                         0, output_image.pixels.data());

    if (verbose) print_timings(end - start, passes);

    // write output file
    finalize_blur();
}

void BlurCppExample::dual_pass_box_blur()
{
    std::cout << "Dual-pass blur" << std::endl;
    step++;

    auto size = static_cast<cl_int>(blur_opts.size);
    auto blur =
        cl::KernelFunctor<cl::Memory, cl::Memory, cl_int>(program, "blur_box");

    // blur
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<cl::Event> passes;

    passes.push_back(
        blur(cl::EnqueueArgs{ queue, cl::NDRange{ width, height } },
             input_image_buf, temp_image_buf, size));

    passes.push_back(
        blur(cl::EnqueueArgs{ queue, cl::NDRange{ width, height } },
             temp_image_buf, output_image_buf, size));

    cl::WaitForEvents(passes);

    auto end = std::chrono::high_resolution_clock::now();

    cl::enqueueReadImage(output_image_buf, CL_BLOCKING, origin, image_size, 0,
                         0, output_image.pixels.data());

    if (verbose) print_timings(end - start, passes);

    // write output file
    finalize_blur();
}

void BlurCppExample::dual_pass_local_memory_exchange_box_blur()
{
    std::cout << "Dual-pass local memory exchange blur" << std::endl;
    step++;

    auto size = static_cast<cl_int>(blur_opts.size);

    cl::Kernel blur1(program, "blur_box_horizontal_exchange");
    cl::Kernel blur2(program, "blur_box_vertical_exchange");

    auto wgs1 = blur1.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(device);
    auto psm1 =
        blur1.getWorkGroupInfo<CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE>(
            device);
    auto wgs2 = blur2.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(device);
    auto psm2 =
        blur2.getWorkGroupInfo<CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE>(
            device);

    // Further constrain (reduce) WGS based on shared mem size on device
    auto loc_mem = device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>();

    if (loc_mem >= ((psm1 > psm2 ? psm1 : psm2) + 2 * size) * sizeof(cl_uchar4))
    {
        while (loc_mem < (wgs1 + 2 * size) * sizeof(cl_uchar4)) wgs1 -= psm1;
        while (loc_mem < (wgs2 + 2 * size) * sizeof(cl_uchar4)) wgs2 -= psm2;
    }
    else
    {
        cl::util::detail::errHandler(
            CL_OUT_OF_RESOURCES, nullptr,
            "Not enough local memory to serve a single sub-group.");
    }

    blur1.setArg(0, input_image_buf);
    blur1.setArg(1, temp_image_buf);
    blur1.setArg(2, size);
    blur1.setArg(3, sizeof(cl_uchar4) * (wgs1 + 2 * size), nullptr);

    blur2.setArg(0, temp_image_buf);
    blur2.setArg(1, output_image_buf);
    blur2.setArg(2, size);
    blur2.setArg(3, sizeof(cl_uchar4) * (wgs2 + 2 * size), nullptr);

    // blur
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<cl::Event> passes;
    cl::Event event;

    cl::NDRange wgsf{ wgs1, 1 };
    cl::NDRange work_size1{ (input_image.width + wgs1 - 1) / wgs1 * wgs1,
                            (cl::size_type)input_image.height };
    queue.enqueueNDRangeKernel(blur1, origin, work_size1, wgsf, nullptr,
                               &event);
    passes.push_back(event);

    cl::NDRange wgss{ 1, wgs2 };
    cl::NDRange work_size2{ (cl::size_type)input_image.width,
                            (input_image.height + wgs2 - 1) / wgs2 * wgs2 };
    queue.enqueueNDRangeKernel(blur2, origin, work_size2, wgss, nullptr,
                               &event);
    passes.push_back(event);

    cl::WaitForEvents(passes);

    auto end = std::chrono::high_resolution_clock::now();

    cl::enqueueReadImage(output_image_buf, CL_BLOCKING, origin, image_size, 0,
                         0, output_image.pixels.data());

    if (verbose) print_timings(end - start, passes);

    // write output file
    finalize_blur();
}

void BlurCppExample::dual_pass_subgroup_exchange_box_blur()
{
    step++;

    auto size = static_cast<cl_int>(blur_opts.size);
    // create kernels
    auto blur1 = cl::KernelFunctor<cl::Memory, cl::Memory, cl_int>(
        program, "blur_box_horizontal_subgroup_exchange");
    auto blur2 = cl::KernelFunctor<cl::Memory, cl::Memory, cl_int>(
        program, "blur_box_vertical_subgroup_exchange");

    auto wgs1 =
        blur1.getKernel()
            .getWorkGroupInfo<CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE>(
                device);
    auto wgs2 =
        blur2.getKernel()
            .getWorkGroupInfo<CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE>(
                device);

    // blur
    std::vector<cl::Event> passes;
    auto start = std::chrono::high_resolution_clock::now();

    cl::NDRange work_size1{ (width + wgs1 - 1) / wgs1 * wgs1, height };
    cl::NDRange wgsf{ wgs1, 1 };
    passes.push_back(blur1(cl::EnqueueArgs{ queue, work_size1, wgsf },
                           input_image_buf, temp_image_buf, size));

    cl::NDRange work_size2{ width, (height + wgs2 - 1) / wgs2 * wgs2 };
    cl::NDRange wgss{ 1, wgs2 };
    passes.push_back(blur2(cl::EnqueueArgs{ queue, work_size2, wgss },
                           temp_image_buf, output_image_buf, size));

    cl::WaitForEvents(passes);

    auto end = std::chrono::high_resolution_clock::now();

    cl::enqueueReadImage(output_image_buf, CL_BLOCKING, origin, image_size, 0,
                         0, output_image.pixels.data());

    if (verbose) print_timings(end - start, passes);

    // write output file
    finalize_blur();
}

void BlurCppExample::dual_pass_kernel_blur()
{
    step++;

    auto size = gauss_size;
    auto& kern = gauss_kernel_buf;

    // create kernels
    auto blur1 = cl::KernelFunctor<cl::Memory, cl::Memory, cl_int, cl::Buffer>(
        program, "blur_kernel_horizontal");
    auto blur2 = cl::KernelFunctor<cl::Memory, cl::Memory, cl_int, cl::Buffer>(
        program, "blur_kernel_vertical");

    // blur
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<cl::Event> passes;

    passes.push_back(
        blur1(cl::EnqueueArgs{ queue, cl::NDRange{ width, height } },
              input_image_buf, temp_image_buf, size, kern));

    passes.push_back(
        blur2(cl::EnqueueArgs{ queue, cl::NDRange{ width, height } },
              temp_image_buf, output_image_buf, size, kern));

    cl::WaitForEvents(passes);

    auto end = std::chrono::high_resolution_clock::now();

    cl::enqueueReadImage(output_image_buf, CL_BLOCKING, origin, image_size, 0,
                         0, output_image.pixels.data());

    if (verbose) print_timings(end - start, passes);

    // write output file
    finalize_blur();
}

void BlurCppExample::dual_pass_local_memory_exchange_kernel_blur()
{
    step++;

    auto size = gauss_size;
    auto& kern = gauss_kernel_buf;

    // create kernels
    auto blur1 = cl::KernelFunctor<cl::Memory, cl::Memory, cl_int, cl::Buffer,
                                   cl::LocalSpaceArg>(
        program, "blur_kernel_horizontal_exchange");
    auto blur2 = cl::KernelFunctor<cl::Memory, cl::Memory, cl_int, cl::Buffer,
                                   cl::LocalSpaceArg>(
        program, "blur_kernel_vertical_exchange");

    // (register) constraints
    auto wgs1 =
        blur1.getKernel().getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(device);
    auto psm1 =
        blur1.getKernel()
            .getWorkGroupInfo<CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE>(
                device);
    auto wgs2 =
        blur2.getKernel().getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(device);
    auto psm2 =
        blur2.getKernel()
            .getWorkGroupInfo<CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE>(
                device);
    auto loc_mem = device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>();

    if (loc_mem >= ((psm1 > psm2 ? psm1 : psm2) + 2 * size) * sizeof(cl_uchar4))
    {
        while (loc_mem < (wgs1 + 2 * size) * sizeof(cl_uchar4)) wgs1 -= psm1;
        while (loc_mem < (wgs2 + 2 * size) * sizeof(cl_uchar4)) wgs2 -= psm2;
    }
    else
    {
        cl::util::detail::errHandler(
            CL_OUT_OF_RESOURCES, nullptr,
            "Not enough local memory to serve a single sub-group.");
    }

    // blur
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<cl::Event> passes;
    cl::Event event;

    cl::NDRange work_size1{ (width + wgs1 - 1) / wgs1 * wgs1, height };
    cl::NDRange wgsf{ wgs1, 1 };
    auto local1 = cl::Local(sizeof(cl_uchar4) * (wgs1 + 2 * size));

    passes.push_back(blur1(cl::EnqueueArgs{ queue, work_size1, wgsf },
                           input_image_buf, temp_image_buf, size, kern,
                           local1));

    cl::NDRange work_size2{ width, (height + wgs2 - 1) / wgs2 * wgs2 };
    cl::NDRange wgss{ 1, wgs2 };
    auto local2 = cl::Local(sizeof(cl_uchar4) * (wgs2 + 2 * size));

    passes.push_back(blur2(cl::EnqueueArgs{ queue, work_size2, wgss },
                           temp_image_buf, output_image_buf, size, kern,
                           local2));

    cl::WaitForEvents(passes);

    auto end = std::chrono::high_resolution_clock::now();

    cl::enqueueReadImage(output_image_buf, CL_BLOCKING, origin, image_size, 0,
                         0, output_image.pixels.data());

    if (verbose) print_timings(end - start, passes);

    // write output file
    finalize_blur();
}

void BlurCppExample::dual_pass_subgroup_exchange_kernel_blur()
{
    step++;

    auto size = gauss_size;
    auto& kern = gauss_kernel_buf;

    // create kernels
    auto blur1 = cl::KernelFunctor<cl::Memory, cl::Memory, cl_int, cl::Buffer>(
        program, "blur_kernel_horizontal_subgroup_exchange");
    auto blur2 = cl::KernelFunctor<cl::Memory, cl::Memory, cl_int, cl::Buffer>(
        program, "blur_kernel_vertical_subgroup_exchange");

    // query preferred subgroup size of kernel on device
    auto wgs1 =
        blur1.getKernel()
            .getWorkGroupInfo<CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE>(
                device);
    auto wgs2 =
        blur2.getKernel()
            .getWorkGroupInfo<CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE>(
                device);
    // blur
    std::vector<cl::Event> passes;
    auto start = std::chrono::high_resolution_clock::now();

    cl::NDRange work_size1{ (width + wgs1 - 1) / wgs1 * wgs1, height };
    cl::NDRange wgsf{ wgs1, 1 };
    passes.push_back(blur1(cl::EnqueueArgs{ queue, work_size1, wgsf },
                           input_image_buf, temp_image_buf, size, kern));

    cl::NDRange work_size2{ width, (height + wgs2 - 1) / wgs2 * wgs2 };
    cl::NDRange wgss{ 1, wgs2 };
    passes.push_back(blur2(cl::EnqueueArgs{ queue, work_size2, wgss },
                           temp_image_buf, output_image_buf, size, kern));

    cl::WaitForEvents(passes);

    auto end = std::chrono::high_resolution_clock::now();

    cl::enqueueReadImage(output_image_buf, CL_BLOCKING, origin, image_size, 0,
                         0, output_image.pixels.data());

    if (verbose) print_timings(end - start, passes);

    // write output file
    finalize_blur();
}

void BlurCppExample::load_device()
{
    // Create context
    context = cl::sdk::get_context(dev_opts.triplet);

    device = context.getInfo<CL_CONTEXT_DEVICES>().at(0);
    queue = cl::CommandQueue(context, device, cl::QueueProperties::Profiling);
    cl::CommandQueue::setDefault(queue);

    cl::Platform platform{
        device.getInfo<CL_DEVICE_PLATFORM>()
    }; // https://github.com/KhronosGroup/OpenCL-CLHPP/issues/150

    if (!diag_opts.quiet)
    {
        std::cout << "Selected platform: "
                  << platform.getInfo<CL_PLATFORM_VENDOR>() << "\n"
                  << "Selected device: " << device.getInfo<CL_DEVICE_NAME>()
                  << "\n"
                  << std::endl;
    }
}

void BlurCppExample::read_input_image()
{
    /// If file not provided in command line, create a default one.
    if (blur_opts.in.empty())
    {
        std::string fname("andrew_svk_7oJ4D_ewB7c_unsplash.png");

        std::cout << "No file given, use standard image " << fname << std::endl;

        const char* fcont = (const char*)andrew_svk_7oJ4D_ewB7c_unsplash_png;
        const size_t fsize = andrew_svk_7oJ4D_ewB7c_unsplash_png_size;

        std::fstream f(fname, std::ios::out | std::ios::binary);
        if (!f.is_open())
        {
            throw std::runtime_error{ std::string{
                "Cannot create a default image: open "
                "andrew_svk_7oJ4D_ewB7c_unsplash_png" } };
        }
        f.write(fcont, fsize);
        f.close();

        blur_opts.in = fname;
    }

    input_image = cl::sdk::read_image(blur_opts.in.c_str(), nullptr);
    image_size = { (cl::size_type)input_image.width,
                   (cl::size_type)input_image.height };
    width = input_image.width;
    height = input_image.height;
}

void BlurCppExample::prepare_output_image()
{
    output_image.width = input_image.width;
    output_image.height = input_image.height;
    output_image.pixel_size = input_image.pixel_size;
    output_image.pixels.clear();
    output_image.pixels.reserve(sizeof(unsigned char) * output_image.width
                                * output_image.height
                                * output_image.pixel_size);
}

std::tuple<bool, bool, bool> BlurCppExample::query_capabilities()
{
    // 1) query image support
    if (!device.getInfo<CL_DEVICE_IMAGE_SUPPORT>())
    {
        cl::util::detail::errHandler(CL_INVALID_DEVICE, nullptr,
                                     "No image support on device!");
    }

    // 2) query if the image format is supported and change image if not
    format = set_image_format();

    // 3) query if device have local memory
    bool use_local_mem =
        (device.getInfo<CL_DEVICE_LOCAL_MEM_TYPE>() == CL_LOCAL);

    // 4) query if device allow subgroup shuffle operations
    bool use_subgroup_exchange =
        cl::util::supports_extension(device, "cl_khr_subgroup_shuffle");
    bool use_subgroup_exchange_relative = cl::util::supports_extension(
        device, "cl_khr_subgroup_shuffle_relative");

    return std::make_tuple(use_local_mem, use_subgroup_exchange,
                           use_subgroup_exchange_relative);
}

void BlurCppExample::create_image_buffers()
{
    input_image_buf = cl::Image2D(
        context, (cl_mem_flags)(CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY),
        format, (cl::size_type)input_image.width,
        (cl::size_type)input_image.height);

    output_image_buf = cl::Image2D(
        context, (cl_mem_flags)(CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY),
        format, (cl::size_type)input_image.width,
        (cl::size_type)input_image.height);

    temp_image_buf = cl::Image2D(
        context, (cl_mem_flags)(CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY),
        format, (cl::size_type)input_image.width,
        (cl::size_type)input_image.height);

    cl::enqueueWriteImage(input_image_buf, CL_NON_BLOCKING, origin, image_size,
                          0, 0, input_image.pixels.data());
}

void BlurCppExample::build_program(std::string options)
{
    // Open kernel stream if not already openned.
    if (!kernel_stream.is_open())
    {
        const char* kernel_location = "./blur.cl";

        kernel_stream = std::ifstream(kernel_location);

        if (!kernel_stream.is_open())
        {
            throw std::runtime_error{
                std::string{ "Cannot open kernel source: " } + kernel_location
            };
        }
    }

    // Scroll to the top
    kernel_stream.clear();
    kernel_stream.seekg(0, std::ios::beg);

    // Compile kernel
    program = cl::Program(
        context,
        std::string{ std::istreambuf_iterator<char>{ kernel_stream },
                     std::istreambuf_iterator<char>{} });

    program.build(device, options.c_str());
}

void BlurCppExample::create_gaussian_kernel()
{
    // create gaussian convolution kernel
    create_gaussian_kernel_(blur_opts.size, &gauss_kernel, &gauss_size);

    gauss_kernel_buf =
        cl::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                   sizeof(float) * (2 * gauss_size + 1), gauss_kernel);
}

// note that the kernel is not normalized and has size of 2*(*size)+1
// elements
void BlurCppExample::create_gaussian_kernel_(float radius, float** const kernel,
                                             int* const size)
{
    radius = fabsf(radius);
    *size = (int)ceilf(3 * radius);
    int span = 2 * (*size) + 1;

    *kernel = new float[span];

    for (int i = 0; i <= *size; ++i)
    {
        float gx = gaussian((float)i, radius);
        (*kernel)[*size + i] = gx;
        (*kernel)[*size - i] = gx;
    }
}

float BlurCppExample::gaussian(float x, float radius)
{
    const float pi = 3.141592653589793238462f;
    return expf(-x * x / (2 * radius * radius)) / (sqrtf(2 * pi) * radius);
}

void BlurCppExample::parse_command_line(int argc, char* argv[])
{
    auto opts = cl::sdk::parse_cli<cl::sdk::options::Diagnostic,
                                   cl::sdk::options::SingleDevice, BlurOptions>(
        argc, argv);
    diag_opts = std::get<0>(opts);
    dev_opts = std::get<1>(opts);
    blur_opts = std::get<2>(opts);

    verbose = diag_opts.verbose;
    filename = blur_opts.out;
    step = 0;

    if (blur_opts.op.empty())
        std::cout << "No blur option passed: box and gauss will be performed."
                  << std::endl;
}

void BlurCppExample::print_timings(std::chrono::duration<double> host_duration,
                                   std::vector<cl::Event>& events)
{
    std::chrono::duration<double> device_duration(0);

    for (cl::Event event : events)
    {
        device_duration +=
            cl::util::get_duration<CL_PROFILING_COMMAND_START,
                                   CL_PROFILING_COMMAND_END,
                                   std::chrono::microseconds>(event);
    }

    std::cout << "Execution time as seen by host: "
              << std::chrono::duration_cast<std::chrono::microseconds>(
                     host_duration)
                     .count()
              << " us, by device: "
              << std::chrono::duration_cast<std::chrono::microseconds>(
                     device_duration)
                     .count()
              << std::endl;
}

bool BlurCppExample::option_active(const std::string option)
{
    // If no option is selected, all options are assumed to be enabled.
    if (blur_opts.op.empty()) return true;

    return (std::any_of(blur_opts.op.begin(), blur_opts.op.end(),
                        [option](std::string op) {
                            return op.find(option) != std::string::npos;
                        }));
}

void BlurCppExample::show_format(cl::ImageFormat* format)
{
    if (verbose)
    {
        std::map<cl_channel_order, std::string> channel_order = {
            { CL_R, "CL_R" }, { CL_RGB, "CL_RBG" }, { CL_RGBA, "CL_RBGA" }
        };

        std::map<cl_channel_type, std::string> channel_type = {
            { CL_SIGNED_INT8, "CL_SIGNED_INT8" },
            { CL_SIGNED_INT16, "CL_SIGNED_INT16" },
            { CL_SIGNED_INT32, "CL_SIGNED_INT32" },
            { CL_UNSIGNED_INT8, "CL_UNSIGNED_INT8" },
            { CL_UNSIGNED_INT16, "CL_UNSIGNED_INT16" },
            { CL_UNSIGNED_INT32, "CL_UNSIGNED_INT32" },
        };

        std::cout << "Format: " << channel_order[format->image_channel_order]
                  << ", " << channel_type[format->image_channel_data_type]
                  << std::endl
                  << std::endl;
    }
}

cl::ImageFormat BlurCppExample::set_image_format()
{
    // this format is always supported
    cl::ImageFormat res = cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8);

    if ((input_image.pixel_size == 1) || (input_image.pixel_size == 3))
    {
        cl::vector<cl::ImageFormat> formats;

        context.getSupportedImageFormats(CL_MEM_READ_ONLY,
                                         CL_MEM_OBJECT_IMAGE2D, &formats);
        for (auto& format : formats)
        {
            if (((input_image.pixel_size == 3)
                 && (format.image_channel_order == CL_RGB)
                 && (format.image_channel_data_type == CL_UNSIGNED_INT8))
                || ((input_image.pixel_size == 1)
                    && (format.image_channel_order == CL_R)
                    && (format.image_channel_data_type == CL_UNSIGNED_INT8)))
            {
                show_format(&format);
                return format;
            }
        }

        // if not found, default to 4 channels of uint8_t
        if (verbose)
            std::cout << "Converting picture into supported format... ";

        const size_t pixels = input_image.width * input_image.height;
        const size_t new_size = sizeof(unsigned char) * pixels * 4;

        input_image.pixels.reserve(new_size);
        output_image.pixels.reserve(new_size);

        // change picture
        const size_t pixel_size = input_image.pixel_size;
        auto input_pixels = input_image.pixels.data();
        for (size_t i = pixels - 1; i != 0; --i)
        {
            memcpy(input_pixels + 4 * i, input_pixels + pixel_size * i,
                   pixel_size);
            memset(input_pixels + 4 * i + pixel_size, 0, 4 - pixel_size);
        }
        memset(input_pixels + pixel_size, 0, 4 - pixel_size);
        input_image.pixel_size = 4;
        // store initial pixel_size in output_image.pixel_size
        if (verbose) std::cout << "done." << std::endl;
    }
    else if (input_image.pixel_size != 4)
    {
        cl::util::detail::errHandler(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR,
                                     nullptr,
                                     "Not possible to write PNG file!");
    }

    show_format(&res);

    return res;
}

void BlurCppExample::finalize_blur()
{
    // restore image type if needed
    if (input_image.pixel_size != output_image.pixel_size)
    {
        const auto pixels = input_image.width * input_image.height,
                   pixel_size = output_image.pixel_size;
        for (size_t i = 1; i < pixels; ++i)
            memcpy(output_image.pixels.data() + pixel_size * i,
                   output_image.pixels.data() + 4 * i, pixel_size);
    }

    std::string name = std::to_string((unsigned int)step) + filename;

    cl::sdk::write_image(name.c_str(), output_image);

    std::cout << "Image " << name << " written." << std::endl << std::endl;
}
