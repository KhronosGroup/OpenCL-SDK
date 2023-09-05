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
#include <CL/SDK/CLI.hpp>
#include <CL/SDK/Context.hpp>
#include <CL/SDK/Image.hpp>
#include <CL/SDK/Options.hpp>
#include <CL/Utils/Context.hpp>

// standard header includes
#include <cassert>
#include <fstream>
#include <future>
#include <iostream>
#include <map>
#include <mutex>
#include <tuple> // std::make_tuple
#include <vector>

// TCLAP includes
#include <tclap/CmdLine.h>

namespace {

struct ReadJob
{
    // Although the buffer is not used in the callback, we keep a reference to
    // the OpenCL object to keep it alive while the read is performed.
    cl::Buffer buffer;
    std::vector<cl_uchar> data;
    std::size_t side;
};

std::mutex read_mutex;
std::map<std::intptr_t, ReadJob> read_jobs;
std::vector<std::future<void>> image_write_tasks;

template <typename T> struct DoubleBuffer
{
    T read, write;

    void swap() { std::swap(read, write); }
};

void CL_CALLBACK read_complete_callback(cl_event, cl_int, void* user_data)
{
    // Acquire the job associated with the received ID and remove it from the
    // map. These operations have to be protected, because the main thread or
    // other callbacks might modify the map concurrently.
    const auto job_id = reinterpret_cast<std::intptr_t>(user_data);
    ReadJob job;
    {
        std::lock_guard<std::mutex> copy_lock(read_mutex);
        const auto iter = read_jobs.find(job_id);
        job = std::move(iter->second);
        read_jobs.erase(iter);
    }

    // Launch an asynchronous task (on the CPU) which writes the read contents
    // of the buffer to a PNG file. Notice, how the relevant members of the job
    // are moved to the closure.
    // Since no std::launch is passed, the task will either run deferred (on the
    // main thread calling .wait()), or truly asynchronously on a different
    // thread. The main point is that it is not running on the OpenCL runtime's
    // thread.
    image_write_tasks[static_cast<std::size_t>(job_id)] =
        std::async([pixels = std::move(job.data), side = job.side, job_id] {
            const cl::sdk::Image image{
                static_cast<int>(side),
                static_cast<int>(side),
                static_cast<int>(sizeof(cl_uchar4)),
                std::move(pixels),
            };
            const std::string filename =
                "callbackcpp_out" + std::to_string(job_id) + ".png";
            cl::sdk::write_image(filename.c_str(), image);
        });

    // job.buffer goes out of scope here, releasing the OpenCL buffer.
}

struct CallbackOptions
{
    std::size_t side;
    std::size_t iterations;
    std::size_t write_iter;
};

} // namespace

template <> auto cl::sdk::parse<CallbackOptions>()
{
    return std::make_tuple(
        std::make_shared<TCLAP::ValueArg<std::size_t>>(
            "s", "side", "Side length of the generated image in pixels", false,
            512, "positive integral"),
        std::make_shared<TCLAP::ValueArg<std::size_t>>(
            "i", "iter", "Number of iterations in the simulation", false, 10000,
            "positive integral"),
        std::make_shared<TCLAP::ValueArg<std::size_t>>(
            "w", "write_iter",
            "Controls after how many iterations the intermediate result is "
            "written to file",
            false, 1000, "positive integral"));
}

template <>
CallbackOptions cl::sdk::comprehend<CallbackOptions>(
    std::shared_ptr<TCLAP::ValueArg<size_t>> side_arg,
    std::shared_ptr<TCLAP::ValueArg<size_t>> iter_arg,
    std::shared_ptr<TCLAP::ValueArg<size_t>> write_iter_arg)
{
    return CallbackOptions{ side_arg->getValue(), iter_arg->getValue(),
                            write_iter_arg->getValue() };
}

int main(int argc, char* argv[])
{
    try
    {
        // Parse command-line options
        auto opts =
            cl::sdk::parse_cli<cl::sdk::options::Diagnostic,
                               cl::sdk::options::SingleDevice, CallbackOptions>(
                argc, argv);
        const auto& diag_opts = std::get<0>(opts);
        const auto& dev_opts = std::get<1>(opts);
        const auto& alg_opts = std::get<2>(opts);

        // Create runtime objects based on user preference or default
        cl::Context context = cl::sdk::get_context(dev_opts.triplet);
        cl::Device device = context.getInfo<CL_CONTEXT_DEVICES>().at(0);

        // There is a separate command queue for kernel launches,
        // device->device copies and device->host copies.
        // Synchronization is ensured via events (see later).
        cl::CommandQueue compute_queue{ context, device };
        cl::CommandQueue copy_queue{ context, device };
        cl::CommandQueue read_queue{ context, device };

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

        // Compile kernel
        const char* kernel_location = "./reaction_diffusion.cl";
        std::ifstream kernel_stream{ kernel_location };
        if (!kernel_stream.is_open())
            throw std::runtime_error{
                std::string{ "Cannot open kernel source: " } + kernel_location
            };

        cl::Program program{ context,
                             std::string{ std::istreambuf_iterator<char>{
                                              kernel_stream },
                                          std::istreambuf_iterator<char>{} } };
        program.build(device);
        cl::KernelFunctor<cl::Image2D, cl::Image2D> reaction_diffusion_step(
            program, "reaction_diffusion_step");

        // Check if the used image format is supported on the device
        const cl::ImageFormat required_image_format(CL_RGBA, CL_UNORM_INT8);
        std::vector<cl::ImageFormat> supported_formats;
        context.getSupportedImageFormats(
            CL_MEM_READ_WRITE, CL_MEM_OBJECT_IMAGE2D, &supported_formats);
        auto found_format = std::find_if(
            supported_formats.begin(), supported_formats.end(),
            [&](const cl::ImageFormat& format) {
                return format.image_channel_order
                    == required_image_format.image_channel_order
                    && format.image_channel_data_type
                    == required_image_format.image_channel_data_type;
            });
        if (found_format == supported_formats.end())
        {
            throw std::runtime_error("Required image format is not supported "
                                     "on the selected runtime");
        }

        // Options provided on the command line
        const std::size_t side = alg_opts.side;
        const std::size_t iterations = alg_opts.iterations;
        const std::size_t save_at_every = alg_opts.write_iter;

        // Create two equivalent images. In a single iteration one serves as the
        // source, the other as the destination, and then the roles are swapped.
        DoubleBuffer<cl::Image2D> images{
            cl::Image2D(context, CL_MEM_READ_WRITE, required_image_format, side,
                        side),
            cl::Image2D(context, CL_MEM_READ_WRITE, required_image_format, side,
                        side),
        };

        // In each pixel of the images, the concentration of the
        //   - U (reaction source) component is stored in the R channel,
        //   - V (reaction result) component is stored in the G channel.
        // The B channel is unused, and the A (alpha) channel must be set to 1,
        // so the resulting image is visible in the image viewer.
        //
        // First, fill the source image with chemical U (the source component of
        // the reaction)
        compute_queue.enqueueFillImage(images.read,
                                       cl_float4{ { 1.F, 0.F, 0.F, 1.F } },
                                       { 0, 0 }, { side, side, 1 });

        // In addition, fill a small rectangle in the middle of the source image
        // with chemical V (the result of the reaction)
        compute_queue.enqueueFillImage(
            images.read, cl_float4{ { 1.F, 1.F, 0.F, 1.F } },
            { side / 2, side / 2 }, { side / 100, side / 100, 1 });

        // The copy_event is initialized with a completed user event,
        // so it doesn't block the first kernel launch.
        cl::UserEvent completed_event(context);
        completed_event.setStatus(CL_COMPLETE);
        cl::Event copy_event = completed_event;
        cl::Event prev_compute_event = completed_event;

        const std::size_t num_images_writes =
            (iterations + save_at_every - 1) / save_at_every;
        image_write_tasks.resize(num_images_writes);
        std::intptr_t copy_job_id{};
        for (std::size_t iter = 0; iter < iterations; ++iter)
        {
            // Enqueue the next step of the iteration and swap the source and
            // destination images. This synchronizes with the previous copy of
            // the input image, ensuring that the copy is finished before the
            // current kernel overwrites the data.
            auto compute_event = reaction_diffusion_step(
                cl::EnqueueArgs(compute_queue, copy_event,
                                cl::NDRange(side, side)),
                images.read, images.write);

            // For every Nth iteration, the current state of the simulation is
            // written to a PNG file
            if (iter % save_at_every == 0)
            {
                // Add a new entry to the job map, protected by a mutex.
                // The job is associated with a job ID.
                auto& job = [&]() -> ReadJob& {
                    std::lock_guard<std::mutex> lock(read_mutex);
                    return read_jobs[copy_job_id] = {};
                }();
                // We should allocate outside of the critical section
                job.buffer = cl::Buffer(context, CL_MEM_READ_WRITE,
                                        side * side * sizeof(cl_uchar4));
                job.data = std::vector<cl_uchar>(side * side * 4);
                job.side = side;

                // Enqueue the copy of the last destination image to a buffer.
                // This is a device->device copy, which is probably faster than
                // a device->host copy. The copy synchronizes with the previous
                // kernel launch, however, the next kernel can be launched,
                // while the copy is in progress, because both read only from
                // images.front.
                const std::vector<cl::Event> compute_events{
                    prev_compute_event
                };
                copy_queue.enqueueCopyImageToBuffer(
                    images.read, job.buffer, { 0, 0 }, { side, side, 1 }, 0,
                    &compute_events, &copy_event);

                // When the device->device copy is finished, a device->host read
                // can start. The resulting data on the host is accessible
                // through the job object.
                const std::vector<cl::Event> copy_events{ copy_event };
                cl::Event read_event;
                read_queue.enqueueReadBuffer(
                    job.buffer, false, 0, side * side * sizeof(cl_uchar4),
                    job.data.data(), &copy_events, &read_event);

                // When the device->host read is finished, a callback is called.
                // It spawns an asynchronous CPU job that writes the data to
                // file. We use the pointer-sized user-data argument to identify
                // the job in the callback.
                read_event.setCallback(CL_COMPLETE, read_complete_callback,
                                       reinterpret_cast<void*>(copy_job_id));
                ++copy_job_id;
            }
            prev_compute_event = compute_event;
            images.swap();
        }
        // Wait for every read finished callback to fire.
        read_queue.finish();

        // Eventually, wait for each image file writing job to finish.
        for (const auto& task : image_write_tasks)
        {
            task.wait();
        }
        // We can expect that all jobs have been consumed at this point.
        assert(read_jobs.empty());
    } catch (cl::util::Error& e)
    {
        std::cerr << "OpenCL Utils error: " << e.what() << std::endl;
        std::exit(e.err());
    } catch (cl::BuildError& e)
    {
        std::cerr << "OpenCL runtime error: " << e.what() << std::endl;
        for (auto& build_log : e.getBuildLog())
        {
            std::cerr << "\tBuild log for device: "
                      << build_log.first.getInfo<CL_DEVICE_NAME>() << "\n"
                      << std::endl;
            std::cerr << build_log.second << "\n" << std::endl;
        }
        std::exit(e.err());
    } catch (cl::Error& e)
    {
        std::cerr << "OpenCL runtime error: " << e.what() << " (" << e.err()
                  << ")" << std::endl;
        std::exit(e.err());
    } catch (std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }
}
