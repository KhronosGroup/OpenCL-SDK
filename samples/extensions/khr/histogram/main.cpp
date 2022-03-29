/*
 * Copyright (c) 2020 The Khronos Group Inc.
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
#include <CL/Utils/Context.hpp>
#include <CL/SDK/Context.hpp>
#include <CL/SDK/Options.hpp>
#include <CL/SDK/CLI.hpp>
#include <CL/SDK/Random.hpp>

// STL includes
#include <iostream>
#include <valarray>
#include <random>
#include <algorithm>
#include <fstream>
#include <tuple> // std::make_tuple

// TCLAP includes
#include <tclap/CmdLine.h>

// Sample-specific option
struct HistogramOptions
{
    size_t length;
    size_t bins;
};

// Add option to CLI parsing SDK utility
template <> auto cl::sdk::parse<HistogramOptions>()
{
    return std::make_tuple(
        std::make_shared<TCLAP::ValueArg<size_t>>(
            "l", "length", "Length of input", false, 1'048'576,
            "positive integral"),
        std::make_shared<TCLAP::ValueArg<size_t>>(
            "b", "bins", "Bins of histogram", false, 100, "positive integral"));
}
template <>
HistogramOptions cl::sdk::comprehend<HistogramOptions>(
    std::shared_ptr<TCLAP::ValueArg<size_t>> length_arg,
    std::shared_ptr<TCLAP::ValueArg<size_t>> bins_arg)
{
    return HistogramOptions{ length_arg->getValue(), bins_arg->getValue() };
}

int main(int argc, char* argv[])
{
    try
    {
        // Parse command-line options
        auto opts = cl::sdk::parse_cli<cl::sdk::options::Diagnostic,
                                       cl::sdk::options::SingleDevice,
                                       HistogramOptions>(argc, argv);
        const auto& diag_opts = std::get<0>(opts);
        const auto& dev_opts = std::get<1>(opts);
        const auto& histogram_opts = std::get<2>(opts);

        // Create runtime objects based on user preference or default
        cl::Context context = cl::sdk::get_context(dev_opts.triplet);
        cl::Device device = context.getInfo<CL_CONTEXT_DEVICES>().at(0);
        cl::CommandQueue queue{ context, device };
        cl::Platform platform{
            device.getInfo<CL_DEVICE_PLATFORM>()
        }; // https://github.com/KhronosGroup/OpenCL-CLHPP/issues/150

        if (!diag_opts.quiet)
        {
            std::cout << "Selected platform: "
                      << platform.getInfo<CL_PLATFORM_VENDOR>() << "\n"
                      << "Selected device: " << device.getInfo<CL_DEVICE_NAME>()
                      << "\n"
                      << "Selected local memory size: "
                      << device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() << "\n"
                      << std::endl;
        }

        // Compile kernel
        const char* kernel_location = "./histogram.cl";
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

        auto histogram_shared =
            cl::KernelFunctor<cl_uint, cl_uint, cl_uint, cl::Buffer, cl::Buffer,
                              cl::LocalSpaceArg, cl::Buffer>(
                program, "histogram_shared");
        auto histogram_global =
            cl::KernelFunctor<cl_uint, cl_uint, cl::Buffer, cl::Buffer,
                              cl::Buffer>(program, "histogram_global");

        // Initialize host-side storage
        const auto length = histogram_opts.length;
        const auto bins = histogram_opts.bins;

        const float min = -100.0f;
        const float max = 100.0f;

        auto prng = [engine = std::default_random_engine{},
                     dist = std::uniform_real_distribution<cl_float>{
                         min, max }]() mutable { return dist(engine); };

        std::valarray<cl_float> input(length);
        std::valarray<cl_float> levels(bins + 1);
        std::valarray<cl_uint> histogram(length);

        // Initialize input variables
        cl::sdk::fill_with_random(prng, input);
        cl_float epsilon = (max - min) / bins;
        for (cl_uint index = 0; index < bins; index++)
        {
            levels[index] = min + epsilon * index;
        }
        levels[bins] = max;

        // Initialize device-side storage
        cl::Buffer buf_input{ context, std::begin(input), std::end(input),
                              true },
            buf_levels{ context, std::begin(levels), std::end(levels), true },
            buf_histogram{ context, std::begin(histogram), std::end(histogram),
                           false };

        // Execute kernel
        if (bins * sizeof(cl_uint)
            <= device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>())
        {
            size_t items_per_thread = 32;
            size_t grid_size =
                (length + items_per_thread - 1) / items_per_thread;
            histogram_shared(cl::EnqueueArgs{ queue, cl::NDRange{ grid_size } },
                             (cl_uint)length, (cl_uint)bins,
                             (cl_uint)items_per_thread, buf_input, buf_levels,
                             cl::Local(bins * sizeof(cl_uint)), buf_histogram);
        }
        else
        {
            histogram_global(cl::EnqueueArgs{ queue, cl::NDRange{ length } },
                             (cl_uint)length, (cl_uint)bins, buf_input,
                             buf_levels, buf_histogram);
        }

        // Concurrently calculate reference dataset
        std::vector<cl_uint> histogram_expected(bins, 0);
        for (const auto value : input)
        {
            if (value >= levels[0] && value < levels[bins])
            {
                const auto bin_iter = std::upper_bound(std::begin(levels),
                                                       std::end(levels), value);
                histogram_expected[bin_iter - std::begin(levels) - 1]++;
            }
        }

        // Fetch results
        cl::copy(queue, buf_histogram, std::begin(histogram),
                 std::end(histogram));

        // Validate
        if (std::equal(std::begin(histogram_expected),
                       std::end(histogram_expected), std::begin(histogram)))
            std::cout << "Verification passed." << std::endl;
        else
            throw std::runtime_error{ "Verification FAILED!" };

        return 0;
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
        std::cerr << "OpenCL rutnime error: " << e.what() << std::endl;
        std::exit(e.err());
    } catch (std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    return 0;
}
