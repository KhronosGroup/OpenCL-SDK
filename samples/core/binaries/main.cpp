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
#include <CL/Utils/Utils.hpp>
#include <CL/SDK/Context.hpp>
#include <CL/SDK/Options.hpp>
#include <CL/SDK/CLI.hpp>

// STL includes
#include <climits>
#include <iostream>
#include <fstream>

// TCLAP includes
#include <tclap/CmdLine.h>

// Sample-specific option
struct BinariesOptions
{
    size_t start;
    size_t length;
};

// Add option to CLI parsing SDK utility
template <> auto cl::sdk::parse<BinariesOptions>()
{

    return std::make_tuple(
        std::make_shared<TCLAP::ValueArg<size_t>>(
            "s", "start", "Starting number", false, 1, "positive integral"),
        std::make_shared<TCLAP::ValueArg<size_t>>("l", "length",
                                                  "Length of input", false,
                                                  100000, "positive integral"));
}
template <>
BinariesOptions cl::sdk::comprehend<BinariesOptions>(
    std::shared_ptr<TCLAP::ValueArg<size_t>> start_arg,
    std::shared_ptr<TCLAP::ValueArg<size_t>> length_arg)
{
    return BinariesOptions{ start_arg->getValue(), length_arg->getValue() };
}

int main(int argc, char* argv[])
{
    try
    {
        // Parse command-line options
        auto opts =
            cl::sdk::parse_cli<cl::sdk::options::Diagnostic,
                               cl::sdk::options::SingleDevice, BinariesOptions>(
                argc, argv);
        const auto& diag_opts = std::get<0>(opts);
        const auto& dev_opts = std::get<1>(opts);
        const auto& binaries_opts = std::get<2>(opts);

        // Create context
        cl::Context context = cl::sdk::get_context(dev_opts.triplet);

        cl_int error;
        std::vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();

        cl::CommandQueue queue{ context, devices[0],
                                cl::QueueProperties::Profiling };

        cl::Platform platform{
            devices.at(0).getInfo<CL_DEVICE_PLATFORM>()
        }; // https://github.com/KhronosGroup/OpenCL-CLHPP/issues/150

        if (!diag_opts.quiet)
        {
            std::cout << "Selected platform: "
                      << platform.getInfo<CL_PLATFORM_VENDOR>() << "\n"
                      << "Selected device: "
                      << devices.at(0).getInfo<CL_DEVICE_NAME>() << "\n"
                      << std::endl;
        }

        cl::Program::Binaries binaries;
        try
        {
            /// Try to read binary
            binaries = cl::util::read_binary_files(devices, "Collatz", &error);
        } catch (cl::util::Error& e)
        {
            // if binary not present, compile and save
            std::cout << e.what() << "\n";

            std::string program_cl =
                cl::util::read_text_file("./Collatz.cl", &error);
            cl::Program program{ context, program_cl };

            program.build(devices.at(0));

            binaries = program.getInfo<CL_PROGRAM_BINARIES>(&error);
            cl::util::write_binaries(binaries, devices, "Collatz");
        }

        // if the binary is already present - calculate
        std::cout << "File found or constructed properly!"
                  << "\n";

        /// Create all remaining runtime object
        cl::Program program{ context, devices, binaries };
        program.build(devices[0]);

        auto collatz = cl::KernelFunctor<cl::Buffer>(program, "Collatz");
        const size_t length = binaries_opts.length;
        const size_t start = binaries_opts.start - 1;

        /// Prepare vector of values to extract results
        std::vector<cl_int> v(length);

        /// Initialize device-side storage
        cl::Buffer buf{ context, std::begin(v), std::end(v), false };

        /// Run kernel
        if (diag_opts.verbose)
        {
            std::cout << "Executing on device... ";
            std::cout.flush();
        }
        std::vector<cl::Event> passes;
        cl::NDRange offset(start);
        cl::NDRange global(length);

        auto dev_start = std::chrono::high_resolution_clock::now();
        passes.push_back(collatz(
            cl::EnqueueArgs{ queue, offset, global, cl::NullRange }, buf));

        cl::WaitForEvents(passes);
        auto dev_end = std::chrono::high_resolution_clock::now();
        if (diag_opts.verbose) std::cout << "done." << std::endl;

        // print timings
        if (diag_opts.verbose)
        {
            std::cout << "Execution time as seen by host: "
                      << std::chrono::duration_cast<std::chrono::microseconds>(
                             dev_end - dev_start)
                             .count()
                      << " us"
                      << ", by device: ";


            std::cout << cl::util::get_duration<CL_PROFILING_COMMAND_START,
                                                CL_PROFILING_COMMAND_END,
                                                std::chrono::microseconds>(
                             passes[0])
                             .count()
                      << " us" << std::endl;
        }

        // Fetch results
        cl::copy(queue, buf, std::begin(v), std::end(v));

        /// Show results
        int max_steps = 0;
        size_t max_ind = -1;
        for (size_t i = 0; i < length; ++i)
            if (v[i] < 0)
            {
                std::cerr << "Number " << start + 1 + i
                          << " gets out of 64 bits at step " << -v[i]
                          << std::endl;
            }
            else if ((v[i] == 0) && (start + i != 0))
            {
                std::cerr << "Number " << start + 1 + i
                          << " did not converge to 1 at step " << INT_MAX - 2
                          << std::endl;
            }
            else if (v[i] > max_steps)
            {
                max_steps = v[i];
                max_ind = start + 1 + i;
            }
        std::cout << "From " << length << " numbers checked starting from "
                  << start + 1 << ", maximum " << max_steps
                  << " steps was needed to get to 1 for number " << max_ind
                  << std::endl;

        return 0;

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
