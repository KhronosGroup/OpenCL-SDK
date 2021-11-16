#include <CL/opencl.hpp>

#include <vector>       // std::vector
#include <exception>    // std::runtime_error, std::exception
#include <iostream>     // std::cout
#include <cstdlib>      // EXIT_FAILURE

int main()
{
    try
    {
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);

        std::cout <<
            "Found " <<
            platforms.size() <<
            " platform" <<
            (platforms.size() > 1 ? "s.\n" : ".\n") <<
            std::endl;

        for (const auto& platform : platforms)
        {
            std::cout << platform.getInfo<CL_PLATFORM_VENDOR>() << std::endl;

            std::vector<cl::Device> devices;
            platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);

            for (const auto& device : devices)
                std::cout << "\t" << device.getInfo<CL_DEVICE_NAME>() << std::endl;
        }
    }
    catch (cl::Error& error) // If any OpenCL error occurs
    {
        if(error.err() == CL_PLATFORM_NOT_FOUND_KHR)
        {
            std::cout << "No OpenCL platform found." << std::endl;
            std::exit(EXIT_SUCCESS);
        }
        else
        {
            std::cerr << error.what() << "(" << error.err() << ")" << std::endl;
            std::exit(error.err());
        }
    }
    catch (std::exception& error) // If STL/CRT error occurs
    {
        std::cerr << error.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }
}
