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
#include <CL/SDK/CLI.hpp>
#include <CL/SDK/Context.hpp>
#include <CL/SDK/Options.hpp>
#include <CL/SDK/Random.hpp>

// OpenCL Utils includes.
#include <CL/Utils/Error.hpp>
#include <CL/Utils/Event.hpp>
#include <CL/Utils/Utils.hpp>

// TCLAP includes.
#include <tclap/CmdLine.h>

// Standard header includes.
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <math.h>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

// Sample-specific options.
struct ConvolutionOptions
{
    cl_uint x_dim;
    cl_uint y_dim;
};

// Add option to CLI-parsing SDK utility for input dimensions.
template <> auto cl::sdk::parse<ConvolutionOptions>()
{
    return std::make_tuple(std::make_shared<TCLAP::ValueArg<cl_uint>>(
                               "x", "x_dim", "x dimension of input", false,
                               4'096, "positive integral"),
                           std::make_shared<TCLAP::ValueArg<cl_uint>>(
                               "y", "y_dim", "y dimension of input", false,
                               4'096, "positive integral"));
}
template <>
ConvolutionOptions cl::sdk::comprehend<ConvolutionOptions>(
    std::shared_ptr<TCLAP::ValueArg<cl_uint>> x_dim_arg,
    std::shared_ptr<TCLAP::ValueArg<cl_uint>> y_dim_arg)
{
    return ConvolutionOptions{ x_dim_arg->getValue(), y_dim_arg->getValue() };
}

// Host-side implementation of the convolution for verification. Padded input
// assumed.
void host_convolution(const std::vector<cl_float> in,
                      std::vector<cl_float>& out,
                      const std::vector<cl_float> mask, const cl_uint x_dim,
                      const cl_uint y_dim)
{
    constexpr cl_uint mask_dim = 3;
    constexpr cl_uint pad_width = mask_dim / 2;
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

int main(int argc, char* argv[])
{
    try
    {
        // Parse command-line options.
        auto opts = cl::sdk::parse_cli<cl::sdk::options::Diagnostic,
                                       cl::sdk::options::SingleDevice,
                                       ConvolutionOptions>(argc, argv);
        const auto& diag_opts = std::get<0>(opts);
        const auto& dev_opts = std::get<1>(opts);
        const auto& conv_opts = std::get<2>(opts);

        // Create runtime objects based on user preference or default.
        cl::Device dev = cl::sdk::get_context(dev_opts.triplet)
                             .getInfo<CL_CONTEXT_DEVICES>()
                             .at(0);

        cl::Platform platform{
            dev.getInfo<CL_DEVICE_PLATFORM>()
        }; // https://github.com/KhronosGroup/OpenCL-CLHPP/issues/150

        if (!diag_opts.quiet)
        {
            std::cout << "Selected device: " << dev.getInfo<CL_DEVICE_NAME>()
                      << "\n"
                      << "from " << platform.getInfo<CL_PLATFORM_VENDOR>()
                      << " platform\n"
                      << std::endl;
        }

        if (diag_opts.verbose)
        {
            std::cout << "Creating sub-devices...";
            std::cout.flush();
        }

#if CL_HPP_TARGET_OPENCL_VERSION < 120
        std::cerr
            << "Error: OpenCL subdevices not supported before version 1.2 "
            << std::endl;
        exit(EXIT_FAILURE);
#endif

        // Create subdevices, each with half of the compute units available.
        cl_uint max_compute_units = dev.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
        cl_device_partition_property subdevices_properties[] = {
            CL_DEVICE_PARTITION_EQUALLY,
            static_cast<cl_device_partition_property>(max_compute_units / 2), 0
        };
        std::vector<cl::Device> subdevices{};
        dev.createSubDevices(subdevices_properties, &subdevices);

        if (subdevices.size() < 2)
        {
            std::cerr << "Error: OpenCL cannot create subdevices" << std::endl;
            exit(EXIT_FAILURE);
        }

        cl::Context context(subdevices);

        // Read kernel file.
        const char* kernel_location = "./convolution.cl";
        std::ifstream kernel_stream{ kernel_location };
        if (!kernel_stream.is_open())
            throw std::runtime_error{
                std::string{ "Cannot open kernel source: " } + kernel_location
            };

        // Compile kernel.
        if (diag_opts.verbose)
        {
            std::cout << "done.\nCompiling kernel...";
            std::cout.flush();
        }
        cl::Program program(
            context,
            std::string{ std::istreambuf_iterator<char>{ kernel_stream },
                         std::istreambuf_iterator<char>{} });

        // Query device and runtime capabilities.
        // If no -cl-std option is specified then the highest 1.x version
        // supported by each device is used to compile the program. Therefore,
        // it's only necessary to add the -cl-std option for 2.0 and 3.0 OpenCL
        // versions.
        cl::string compiler_options;
        constexpr int max_major_version = 3;
        for (auto i = 2; i <= max_major_version; ++i)
        {
            std::string version_str = std::to_string(i) + "."; // "i."
            std::string compiler_opt_str =
                "-cl-std=CL" + std::to_string(i) + ".0 "; // -cl-std=CLi.0

            compiler_options += cl::string{ cl::util::opencl_c_version_contains(
                                                dev, version_str)
                                                ? compiler_opt_str
                                                : "" };
        }
        program.build(subdevices, compiler_options.c_str());

        // Initialize host-side storage.
        constexpr cl_uint mask_dim = 3;
        constexpr cl_uint pad_width = mask_dim / 2;
        const cl_uint x_dim = conv_opts.x_dim;
        const cl_uint y_dim = conv_opts.y_dim;
        const cl_uint pad_x_dim = x_dim + 2 * pad_width;
        const cl_uint pad_y_dim = y_dim + 2 * pad_width;

        const size_t input_size = pad_x_dim * pad_y_dim;
        const size_t output_size = x_dim * y_dim;
        const size_t mask_size = mask_dim * mask_dim;
        const size_t input_bytes = sizeof(cl_float) * input_size;
        const size_t output_bytes = sizeof(cl_float) * output_size;
        const size_t mask_bytes = sizeof(cl_float) * mask_size;

        if (diag_opts.verbose)
        {
            std::cout << "done.\nInitializing host-side storage...";
            std::cout.flush();
        }

        // Random number generator.
        auto prng = [engine = std::default_random_engine{},
                     dist = std::uniform_real_distribution<cl_float>{
                         -1.0, 1.0 }]() mutable { return dist(engine); };

        // Initialize input matrix. The input will be padded to remove
        // conditional branches from the convolution kernel for determining
        // out-of-bounds.
        std::vector<cl_float> h_input_grid(input_size);
        if (diag_opts.verbose)
        {
            std::cout << "\n  Generating " << output_size
                      << " random numbers for convolution input grid...";
            std::cout.flush();
        }
        cl::sdk::fill_with_random(prng, h_input_grid);

        // Fill with 0s the extra rows and columns added for padding.
        for (cl_uint y = 0; y < pad_y_dim; ++y)
        {
            for (cl_uint x = 0; x < pad_x_dim; ++x)
            {
                if (x == 0 || y == 0 || x == (pad_x_dim - 1)
                    || y == (pad_y_dim - 1))
                {
                    h_input_grid[y * pad_x_dim + x] = 0;
                }
            }
        }

        // Declare output matrix. Output will not be padded.
        std::vector<cl_float> h_output_grid(output_size, 0);

        // Initialize convolution mask.
        std::vector<cl_float> h_mask(mask_size);
        if (diag_opts.verbose)
        {
            std::cout << "done.  \nGenerating " << mask_size
                      << " random numbers for convolution mask...";
            std::cout.flush();
        }
        cl::sdk::fill_with_random(prng, h_mask);

        // Create device buffers, from which we will create the subbuffers for
        // the subdevices.
        const cl_uint grid_midpoint = y_dim / 2;
        const cl_uint pad_grid_midpoint = pad_y_dim / 2;

        if (diag_opts.verbose)
        {
            std::cout << "done.\nInitializing device-side storage...";
            std::cout.flush();
        }

        cl::Buffer dev_input_grid(context,
                                  CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR
                                      | CL_MEM_HOST_NO_ACCESS,
                                  input_bytes, h_input_grid.data());
        cl::Buffer dev_output_grid(context,
                                   CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR
                                       | CL_MEM_HOST_READ_ONLY,
                                   output_bytes);
        cl::Buffer dev_mask(context,
                            CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR
                                | CL_MEM_HOST_NO_ACCESS,
                            mask_bytes, h_mask.data());

        if (diag_opts.verbose)
        {
            std::cout << "done.\nSetting up sub-devices...";
            std::cout.flush();
        }

        // Set up subdevices for kernel execution.
        const size_t half_input_bytes =
            sizeof(cl_float) * pad_x_dim * (pad_grid_midpoint + 1);
        const size_t input_offset =
            sizeof(cl_float) * pad_x_dim * (pad_grid_midpoint - 1);
        const size_t half_output_bytes =
            sizeof(cl_float) * x_dim * grid_midpoint;

        std::vector<
            cl::KernelFunctor<cl::Buffer, cl::Buffer, cl::Buffer, cl_uint2>>
            convolutions{};
        std::vector<cl::CommandQueue> sub_queues{};
        std::vector<cl::Buffer> sub_input_grids{}, sub_output_grids{};

        for (size_t i = 0; i < subdevices.size(); ++i)
        {
            auto subdevice = subdevices[i];

            if (diag_opts.verbose)
            {
                std::cout
                    << "\n  Creating kernel and command queue of sub-device "
                    << i << "...";
                std::cout.flush();
            }

            auto convolution =
                cl::KernelFunctor<cl::Buffer, cl::Buffer, cl::Buffer, cl_uint2>(
                    program, "convolution_3x3")
                    .getKernel();

            cl::CommandQueue queue(context, subdevice,
                                   cl::QueueProperties::Profiling);

            // Initialize device-side storage.
            // First device performs the convolution in the upper half and
            // second device in the lower half (middle borders included).
            if (diag_opts.verbose)
            {
                std::cout << "done.\n  Initializing device-side storage of "
                             "sub-device "
                          << i << "...";
                std::cout.flush();
            }

            cl_buffer_region input_region = { i * input_offset,
                                              half_input_bytes },
                             output_region = { i * half_output_bytes,
                                               half_output_bytes };

            const cl_uint align =
                subdevice.getInfo<CL_DEVICE_MEM_BASE_ADDR_ALIGN>();
            if (input_region.origin % align || output_region.origin % align)
            {
                std::cerr << "Error: Memory should be aligned to "
                          << subdevice.getInfo<CL_DEVICE_MEM_BASE_ADDR_ALIGN>()
                          << std::endl;
                exit(EXIT_FAILURE);
            }

            cl::Buffer sub_input_grid = dev_input_grid.createSubBuffer(
                CL_MEM_READ_ONLY, CL_BUFFER_CREATE_TYPE_REGION, &input_region);
            cl::Buffer sub_output_grid = dev_output_grid.createSubBuffer(
                CL_MEM_WRITE_ONLY, CL_BUFFER_CREATE_TYPE_REGION,
                &output_region);

            if (diag_opts.verbose)
            {
                std::cout << "done.";
                std::cout.flush();
            }

            convolutions.push_back(convolution.clone());
            sub_queues.push_back(queue);
            sub_input_grids.push_back(sub_input_grid);
            sub_output_grids.push_back(sub_output_grid);
        }

        // Launch kernels.
        if (diag_opts.verbose)
        {
            std::cout << "\nExecuting on device... ";
            std::cout.flush();
        }

        // Initialize global and local buffers for device execution.
        const cl::NDRange global{ x_dim, y_dim };

        // Enqueue kernel calls and wait for them to finish.
        std::vector<cl::Event> dev1_kernel_runs;
        dev1_kernel_runs.reserve(1);
        std::vector<cl::Event> dev2_kernel_runs;
        dev2_kernel_runs.reserve(1);
        auto dev_start = std::chrono::high_resolution_clock::now();

        dev1_kernel_runs.push_back(convolutions[0](
            cl::EnqueueArgs{ sub_queues[0], global }, sub_input_grids[0],
            sub_output_grids[0], dev_mask, { { x_dim, grid_midpoint } }));

        dev2_kernel_runs.push_back(convolutions[1](
            cl::EnqueueArgs{ sub_queues[1], global }, sub_input_grids[1],
            sub_output_grids[1], dev_mask, { { x_dim, grid_midpoint } }));

        cl::WaitForEvents(dev1_kernel_runs);
        cl::WaitForEvents(dev2_kernel_runs);
        auto dev_end = std::chrono::high_resolution_clock::now();

        // Compute reference host-side convolution.
        if (diag_opts.verbose)
        {
            std::cout << " done.\nExecuting on host... ";
            std::cout.flush();
        }
        auto host_start = std::chrono::high_resolution_clock::now();

        host_convolution(h_input_grid, h_output_grid, h_mask, x_dim, y_dim);

        auto host_end = std::chrono::high_resolution_clock::now();

        if (diag_opts.verbose)
        {
            std::cout << "done." << std::endl;
        }

        // Fetch and combine results from devices.
        std::vector<cl_float> concatenated_results(output_size);
        cl::copy(sub_queues.front(), dev_output_grid,
                 concatenated_results.begin(), concatenated_results.end());

        // Validate device-side solution.
        cl_float deviation = 0.f;
        const cl_float tolerance = 1e-6;

        for (size_t i = 0; i < concatenated_results.size(); ++i)
        {
            deviation += std::fabs(concatenated_results[i] - h_output_grid[i]);
        }
        deviation /= concatenated_results.size();

        if (deviation > tolerance)
        {
            std::cerr << "Failed convolution! Normalized deviation "
                      << deviation
                      << " between host and device exceeds tolerance "
                      << tolerance << std::endl;
        }
        else
        {
            std::cout << "Successful convolution!" << std::endl;
        }

        if (!diag_opts.quiet)
        {
            std::cout << "Kernels execution time as seen by host: "
                      << std::chrono::duration_cast<std::chrono::microseconds>(
                             dev_end - dev_start)
                             .count()
                      << " us." << std::endl;
            std::cout << "Kernels execution time as measured by devices: "
                      << std::endl;
            for (auto& pass : dev1_kernel_runs)
                std::cout << "  - "
                          << cl::util::get_duration<CL_PROFILING_COMMAND_START,
                                                    CL_PROFILING_COMMAND_END,
                                                    std::chrono::microseconds>(
                                 pass)
                                 .count()
                          << " us." << std::endl;
            for (auto& pass : dev2_kernel_runs)
                std::cout << "  - "
                          << cl::util::get_duration<CL_PROFILING_COMMAND_START,
                                                    CL_PROFILING_COMMAND_END,
                                                    std::chrono::microseconds>(
                                 pass)
                                 .count()
                          << " us." << std::endl;
            std::cout << "Reference execution as seen by host: "
                      << std::chrono::duration_cast<std::chrono::microseconds>(
                             host_end - host_start)
                             .count()
                      << " us." << std::endl;
        }
    } catch (cl::BuildError& e)
    {
        std::cerr << "OpenCL build error: " << e.what() << std::endl;
        for (auto& build_log : e.getBuildLog())
        {
            std::cerr << "\tBuild log for device: "
                      << build_log.first.getInfo<CL_DEVICE_NAME>() << "\n"
                      << std::endl;
            std::cerr << build_log.second << "\n" << std::endl;
        }
        std::exit(e.err());
    } catch (cl::util::Error& e)
    {
        std::cerr << "OpenCL utils error: " << e.what() << std::endl;
        std::exit(e.err());
    } catch (cl::Error& e)
    {
        std::cerr << "OpenCL runtime error: " << e.what() << std::endl;
        std::exit(e.err());
    } catch (std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    return 0;
}
