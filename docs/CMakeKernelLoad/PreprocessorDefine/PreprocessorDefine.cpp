// OpenCL includes
#include <CL/cl2.hpp>   // namespace cl::

// STL includes
#include <vector>       // std::vector
#include <iostream>     // std::cout
#include <stdexcept>    // std::runtime_error
#include <fstream>      // std::ifstream
#include <filesystem>   // std::filesystem
#include <string>       // std::string
#include <iterator>     // std::istreambuf_iterator
#include <cstddef>      // std::size_t
#include <cmath>        // std::pow
#include <valarray>     // std::valarray
#include <random>       // std::default_random_engine, std::uniform_real_distribution
#include <algorithm>    // std::generate_n, std::equal
#include <exception>    // std::exception

namespace cl
{
    namespace util
    {
        template <cl_int From, cl_int To, typename Dur = std::chrono::nanoseconds>
        auto get_duration(cl::Event& ev)
        {
            return std::chrono::duration_cast<Dur>(std::chrono::nanoseconds{ ev.getProfilingInfo<To>() - ev.getProfilingInfo<From>() });
        }
    }
}

int main(int, char**)
{
    try // Any error results in program termination
    {
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);

        if (platforms.empty()) throw std::runtime_error{ "No OpenCL platforms found." };

        std::cout << "Found platform" << (platforms.size() > 1 ? "s" : "") << ":\n";
        for (const auto& platform : platforms)
            std::cout << "\t" << platform.getInfo<CL_PLATFORM_VENDOR>() << std::endl;

        cl::Platform plat = platforms.at(0);
        std::cout << "Selected platform: " << plat.getInfo<CL_PLATFORM_VENDOR>() << std::endl;

        std::vector<cl::Device> devices;
        plat.getDevices(CL_DEVICE_TYPE_ALL, &devices);

        std::cout << "Found device" << (devices.size() > 1 ? "s" : "") << ":\n";
        for (const auto& device : devices)
            std::cout << "\t" << device.getInfo<CL_DEVICE_NAME>() << std::endl;

        cl::Device device = devices.at(0);
        std::cout << "Selected device: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;

        // Create context and queue
        std::vector<cl_context_properties> props{
            CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>((plat)()),
            0
        };
        cl::Context context{ devices, props.data() };

        cl::CommandQueue queue{ context, device, cl::QueueProperties::Profiling };

        // Load program source
        std::ifstream source_file{ KERNEL_PATH };
        if (!source_file.is_open())
            throw std::runtime_error{ std::string{ "Cannot open kernel source: " } + "./saxpy.cl" };

        // Create program and kernel
        cl::Program program{ context, std::string{ std::istreambuf_iterator<char>{ source_file },
                                                   std::istreambuf_iterator<char>{} } };
        program.build({ device });

        auto saxpy = cl::KernelFunctor<cl_float, cl::Buffer, cl::Buffer>(program, "saxpy");

        // Init computation
        const std::size_t chainlength = std::size_t(std::pow(2u, 20u)); // 1M, cast denotes floating-to-integral conversion,
                                                                        //     promises no data is lost, silences compiler warning
        std::valarray<cl_float> vec_x(chainlength),
                                vec_y(chainlength);
        cl_float a = 2.0;

        // Fill arrays with random values between 0 and 100
        auto prng = [engine = std::default_random_engine{},
                     distribution = std::uniform_real_distribution<cl_float>{ -100.0, 100.0 }]() mutable { return distribution(engine); };

        std::generate_n(std::begin(vec_x), chainlength, prng);
        std::generate_n(std::begin(vec_y), chainlength, prng);

        cl::Buffer buf_x{ queue, std::begin(vec_x), std::end(vec_x), true },
                   buf_y{ queue, std::begin(vec_y), std::end(vec_y), false };

        // Launch kernels
        cl::Event kernel_event{ saxpy(cl::EnqueueArgs{ queue, cl::NDRange{ chainlength } }, a, buf_x, buf_y) };
        kernel_event.wait();

        std::cout <<
            "Device (kernel) execution took: " <<
            cl::util::get_duration<CL_PROFILING_COMMAND_START,
                                   CL_PROFILING_COMMAND_END,
                                   std::chrono::microseconds>(kernel_event).count() <<
            " us." << std::endl;

        // Compute validation set on host
        auto start = std::chrono::high_resolution_clock::now();

        vec_y = a * vec_x + vec_y;

        auto finish = std::chrono::high_resolution_clock::now();

        std::cout <<
            "Host (validation) execution took: " <<
            std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count() <<
            " us." << std::endl;

        // (Blocking) fetch of results (reuse storage of vec_x)
        cl::copy(queue, buf_y, std::begin(vec_x), std::end(vec_x));

        // Validate (compute saxpy on host and match results)
        if (!std::equal(std::begin(vec_x), std::end(vec_x),
                        std::begin(vec_y), std::end(vec_y))) throw std::runtime_error{ "Validation failed." };
        else
            std::cout << "Validation succeeded!" << std::endl;
    }
    catch (cl::BuildError& error) // If kernel failed to build
    {
        std::cerr << error.what() << "(" << error.err() << ")" << std::endl;

        for (const auto& log : error.getBuildLog())
        {
            std::cerr <<
                "\tBuild log for device: " <<
                log.first.getInfo<CL_DEVICE_NAME>() <<
                std::endl << std::endl <<
                log.second <<
                std::endl << std::endl;
        }

        std::exit(error.err());
    }
    catch (cl::Error& error) // If any OpenCL error occurs
    {
        std::cerr << error.what() << "(" << error.err() << ")" << std::endl;

        std::exit(error.err());
    }
    catch (std::exception& error) // If STL/CRT error occurs
    {
        std::cerr << error.what() << std::endl;

        std::exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
