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
#include <CL/Utils/Event.hpp>
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
#include <tuple>        // std::make_tuple
#include <numeric>      // std::accumulate

// TCLAP includes
#include <tclap/CmdLine.h>

// Sample-specific option
struct ReduceOptions {
    size_t length;
    std::string op;
};

// Add option to CLI parsing SDK utility
template <> auto cl::sdk::parse<ReduceOptions>(){
    std::vector<std::string> valid_op_strings{ "min", "sum" };
    TCLAP::ValuesConstraint<std::string> valid_op_constraint{ valid_op_strings };

    return std::make_tuple(
        std::make_shared<TCLAP::ValueArg<size_t>>("l", "length", "Length of input", false, 1'048'576, "positive integral"),
        std::make_shared<TCLAP::ValueArg<std::string>>("o", "op", "Operation to perform", false, "min", &valid_op_constraint)
    );
}
template <> ReduceOptions cl::sdk::comprehend<ReduceOptions>(
    std::shared_ptr<TCLAP::ValueArg<size_t>> length_arg,
    std::shared_ptr<TCLAP::ValueArg<std::string>> op_arg){
    return ReduceOptions{
        length_arg->getValue(),
        op_arg->getValue()
    };
}


int main(int argc, char* argv[])
{
    try
    {
        // Parse command-line options
        auto opts = cl::sdk::parse_cli<
                        cl::sdk::options::Diagnostic,
                        cl::sdk::options::SingleDevice,
                        ReduceOptions>(argc, argv);
        const auto& diag_opts   = std::get<0>(opts);
        const auto& dev_opts    = std::get<1>(opts);
        const auto& reduce_opts = std::get<2>(opts);

        // Create runtime objects based on user preference or default
        cl::Context context = cl::sdk::get_context(dev_opts.triplet);
        cl::Device device = context.getInfo<CL_CONTEXT_DEVICES>().at(0);
        auto valami = device.getInfo<CL_DEVICE_QUEUE_ON_HOST_PROPERTIES>();
        cl::CommandQueue queue{ context, device, cl::QueueProperties::Profiling };
        cl::Platform platform{ device.getInfo<CL_DEVICE_PLATFORM>() }; // https://github.com/KhronosGroup/OpenCL-CLHPP/issues/150

        if (!diag_opts.quiet)
            std::cout <<
                "Selected platform: " << platform.getInfo<CL_PLATFORM_VENDOR>() << "\n" <<
                "Selected device: " << device.getInfo<CL_DEVICE_NAME>() << "\n" <<
                std::endl;

        // Query device and runtime capabilities
        auto may_use_work_group_reduce = [&]() // IILE
        {
            if (platform.getInfo<CL_PLATFORM_VERSION>().find("OpenCL 2.") != cl::string::npos)
            {
                return device.getInfo<CL_DEVICE_OPENCL_C_VERSION>().find("OpenCL C 2.") != cl::string::npos;
            }
            else if (platform.getInfo<CL_PLATFORM_VERSION>().find("OpenCL 3.") != cl::string::npos)
            {
                auto c_features = device.getInfo<CL_DEVICE_OPENCL_C_FEATURES>();
                auto feature_is_work_group_reduce = [](const cl_name_version& name_ver)
                {
                    return cl::string{name_ver.name} == "__opencl_c_work_group_collective_functions";
                };
                return device.getInfo<CL_DEVICE_WORK_GROUP_COLLECTIVE_FUNCTIONS_SUPPORT>() &&
                    std::find_if(c_features.cbegin(), c_features.cend(), feature_is_work_group_reduce) != c_features.cend();
            }
            else return false;
        }();
        auto may_use_sub_group_reduce =
            platform.getInfo<CL_PLATFORM_VERSION>().find("OpenCL 3.") != cl::string::npos &&
                    device.getInfo<CL_DEVICE_EXTENSIONS>().find("cl_khr_subgroups") != cl::string::npos;
        if (diag_opts.verbose)
        {
            if (may_use_work_group_reduce)
                std::cout << "Device supports work-group reduction intrinsics." << std::endl;
            else if (may_use_sub_group_reduce)
                std::cout << "Device supports sub-group reduction intrinsics." << std::endl;
            else
                std::cout << "Device doesn't support any reduction intrinsics." << std::endl;
        }

        // User defined input
        std::string kernel_op = reduce_opts.op == "min" ?
            "int op(int lhs, int rhs) { return min(lhs, rhs); }\n":
            "int op(int lhs, int rhs) { return lhs + rhs; }\n";
        if (may_use_work_group_reduce)
            kernel_op += reduce_opts.op == "min" ?
                "int work_group_reduce_op(int val) { return work_group_reduce_min(val); }":
                "int work_group_reduce_op(int val) { return work_group_reduce_add(val); }";
        else if (may_use_sub_group_reduce)
            kernel_op += reduce_opts.op == "min" ?
                "int sub_group_reduce_op(int val) { return sub_group_reduce_min(val); }":
                "int sub_group_reduce_op(int val) { return sub_group_reduce_add(val); }";
        auto host_op = reduce_opts.op == "min" ?
            std::function<int(int, int)>{ [](int lhs, int rhs){ return std::min(lhs, rhs); } } :
            std::function<int(int, int)>{ std::plus<int>{} };
        auto zero_elem = reduce_opts.op == "min" ?
            std::numeric_limits<cl_int>().max() :
            static_cast<cl_int>(0);

        // Compile kernel
        const char* kernel_location = "./reduce.cl";
        std::ifstream kernel_stream{ kernel_location };
        if (!kernel_stream.is_open())
            throw std::runtime_error{ std::string{ "Cannot open kernel source: " } + kernel_location };

        cl::Program program{ context, std::string{ std::istreambuf_iterator<char>{ kernel_stream },
                                                   std::istreambuf_iterator<char>{} }.append(kernel_op) }; // Note append
        cl::string compiler_options =
            cl::string{may_use_work_group_reduce ? "-D USE_WORK_GROUP_REDUCE " : "" } +
            cl::string{may_use_sub_group_reduce ? "-D USE_SUB_GROUP_REDUCE " : "" };
        program.build( device, compiler_options.c_str() );

        auto reduce = cl::KernelFunctor<cl::Buffer, cl::Buffer, cl::LocalSpaceArg, cl_uint, cl_int>(program, "reduce");

        // Query maximum supported WGS of kernel on device based on private mem (register) constraints
        auto wgs = reduce.getKernel().getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(device);

        // Further constrain (reduce) WGS based on shared mem size on device
        while (device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() < wgs * 2 * sizeof(cl_int))
            wgs -= reduce.getKernel().getWorkGroupInfo<CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE>(device);

        if (wgs == 0) throw std::runtime_error{"Not enough local memory to serve a single sub-group."};

        auto factor = wgs * 2;
        // Every pass reduces input length by 'factor'.
        // If actual size is not divisible by factor,
        // an extra output element is produced using some
        // number of zero_elem inputs.
        auto new_size = [factor](const std::size_t actual)
        {
            return actual / factor + (actual % factor == 0 ? 0 : 1);
        };
        // NOTE: because one work-group produces one output
        //       new_size == number_of_work_groups
        auto global = [=](const std::size_t actual){ return new_size(actual) * wgs; };

        // Initialize host-side storage
        const auto length = reduce_opts.length;

        auto prng = [engine = std::default_random_engine{},
                     dist = std::uniform_int_distribution<cl_int>{ -1000, 1000 }]() mutable {
            return dist(engine);
        };

        std::vector<cl_int> arr(length);
        if (diag_opts.verbose)
            std::cout << "Generating " << length << " random numbers for reduction." << std::endl;
        cl::sdk::fill_with_random(prng, arr);

        // Initialize device-side storage
        cl::Buffer front{ queue, std::begin(arr), std::end(arr), false },
                   back{ context, CL_MEM_READ_WRITE, new_size(arr.size()) * sizeof(cl_int) };

        // Launch kernels
        if (diag_opts.verbose)
            std::cout << "Executing on device... "; std::cout.flush();
        std::vector<cl::Event> passes;
        cl_uint curr = static_cast<cl_uint>(arr.size());
        auto dev_start = std::chrono::high_resolution_clock::now();
        while ( curr > 1 )
        {
            passes.push_back(
                reduce(
                    cl::EnqueueArgs{
                        queue,
                        global(curr),
                        wgs
                    },
                    front,
                    back,
                    cl::Local(factor * sizeof(cl_int)),
                    curr,
                    zero_elem
                )
            );

            curr = static_cast<cl_uint>(new_size(curr));
            if (curr > 1) std::swap(front, back);
        }
        cl::WaitForEvents(passes);
        auto dev_end = std::chrono::high_resolution_clock::now();
        if (diag_opts.verbose)
            std::cout << "done." << std::endl;

        // calculate reference dataset
        auto host_start = std::chrono::high_resolution_clock::now();
        auto seq_ref = std::accumulate(arr.cbegin(), arr.cend(), zero_elem, host_op);
        auto host_end = std::chrono::high_resolution_clock::now();

        // Fetch results
        cl_int dev_res;
        cl::copy(queue, back, &dev_res, &dev_res + 1);

        // Validate
        if (dev_res != seq_ref)
        {
            std::cerr << "Sequential reference: " << seq_ref << std::endl;
            std::cerr << "Device result: " << dev_res << std::endl;
            throw std::runtime_error{ "Validation failed!" };
        }

        if (!diag_opts.quiet)
        {
            std::cout << "Total device execution as seen by host: " << std::chrono::duration_cast<std::chrono::microseconds>(dev_end - dev_start).count() << " us." << std::endl;
            std::cout << "Reduction steps as measured by device :\n";
            for (auto& pass : passes)
                std::cout << "\t" << cl::util::get_duration<CL_PROFILING_COMMAND_START, CL_PROFILING_COMMAND_END, std::chrono::microseconds>(pass).count() << " us." << std::endl;
            std::cout << "Reference execution as seen by host   : " << std::chrono::duration_cast<std::chrono::microseconds>(host_end - host_start).count() << " us." << std::endl;
        }
    }
    catch(cl::util::Error& e)
    {
        std::cerr << "OpenCL Utils error: " << e.what() << std::endl;
        std::exit(e.err());
    }
    catch(cl::BuildError& e)
    {
        std::cerr << "OpenCL runtime error: " << e.what() << std::endl;
        for (auto& build_log : e.getBuildLog())
        {
            std::cerr << "\tBuild log for device: " << build_log.first.getInfo<CL_DEVICE_NAME>() << "\n" << std::endl;
            std::cerr << build_log.second << "\n" << std::endl;
        }
        std::exit(e.err());
    }
    catch(cl::Error& e)
    {
        std::cerr << "OpenCL rutnime error: " << e.what() << std::endl;
        std::exit(e.err());
    }
    catch(std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    return 0;
}
