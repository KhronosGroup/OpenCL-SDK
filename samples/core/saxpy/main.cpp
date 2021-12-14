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
struct SaxpyOptions
{
    size_t length;
};

// Add option to CLI parsing SDK utility
template <> auto cl::sdk::parse<SaxpyOptions>()
{
    return std::make_tuple(std::make_shared<TCLAP::ValueArg<size_t>>(
        "l", "length", "Length of input", false, 1'048'576,
        "positive integral"));
}
template <>
SaxpyOptions cl::sdk::comprehend<SaxpyOptions>(
    std::shared_ptr<TCLAP::ValueArg<size_t>> length_arg)
{
    return SaxpyOptions{ length_arg->getValue() };
}

std::valarray<float> fma(float x, std::valarray<float> y,
                         std::valarray<float> z)
{
    if (y.size() == z.size())
    {
        size_t len = y.size();
        std::valarray<float> res(len);
        for (size_t i = 0; i < len; ++i) res[i] = fmaf(x, y[i], z[i]);
        return res;
    }
    else
        throw std::logic_error("Different sizes!");
}

int main(int argc, char* argv[])
{
    try
    {
        // Parse command-line options
        auto opts =
            cl::sdk::parse_cli<cl::sdk::options::Diagnostic,
                               cl::sdk::options::SingleDevice, SaxpyOptions>(
                argc, argv);
        const auto& diag_opts = std::get<0>(opts);
        const auto& dev_opts = std::get<1>(opts);
        const auto& saxpy_opts = std::get<2>(opts);

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
                      << std::endl;
        }

        // Compile kernel
        const char* kernel_location = "./saxpy.cl";
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

        auto saxpy = cl::KernelFunctor<cl_float, cl::Buffer, cl::Buffer>(
            program, "saxpy");

        // Initialize host-side storage
        const auto length = saxpy_opts.length;

        auto prng = [engine = std::default_random_engine{},
                     dist = std::uniform_real_distribution<cl_float>{
                         -100.0, 100.0 }]() mutable { return dist(engine); };

        cl_float a = prng();
        std::valarray<cl_float> arr_x(length), arr_y(length);
        cl::sdk::fill_with_random(prng, arr_x, arr_y);

        // Initialize device-side storage
        cl::Buffer buf_x{ context, std::begin(arr_x), std::end(arr_x), true },
            buf_y{ context, std::begin(arr_y), std::end(arr_y), false };

        // Execute kernel
        saxpy(cl::EnqueueArgs{ queue, cl::NDRange{ length } }, a, buf_x, buf_y);

        // Concurrently calculate reference dataset
        arr_y = fma(a, arr_x, arr_y); // a * arr_x + arr_y;

        // Fetch results
        cl::copy(queue, buf_y, std::begin(arr_x), std::end(arr_x));

        // Validate
        if (std::equal(std::begin(arr_x), std::end(arr_x), std::begin(arr_y),
                       std::end(arr_y)))
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