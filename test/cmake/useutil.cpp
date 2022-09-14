#include <CL/opencl.hpp>

#include <CL/Utils/Utils.hpp>

#include <vector> // std::vector
#include <exception> // std::runtime_error, std::exception
#include <iostream> // std::cout
#include <cstdlib> // EXIT_FAILURE

int main(int argc, char* argv[])
{
    try
    {
        cl::Context context = cl::util::get_context(
            argc > 1 ? std::atoi(argv[1]) : 0,
            argc > 2 ? std::atoi(argv[2]) : 0, CL_DEVICE_TYPE_ALL);
    } catch (cl::Error& error) // If any OpenCL error occurs
    {
        if (error.err() == CL_PLATFORM_NOT_FOUND_KHR)
        {
            std::cout << "No OpenCL platform found." << std::endl;
            std::exit(EXIT_SUCCESS);
        }
        else
        {
            std::cerr << error.what() << "(" << error.err() << ")" << std::endl;
            std::exit(error.err());
        }
    } catch (std::exception& error) // If STL/CRT error occurs
    {
        std::cerr << error.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }
}
