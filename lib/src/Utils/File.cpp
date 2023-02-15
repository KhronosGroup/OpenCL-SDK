// OpenCL SDK includes
#include <CL/Utils/File.hpp>

// STL includes
#include <fstream>
#include <iterator>
#include <algorithm>
#include <iostream>

std::string cl::util::read_text_file(const char* const filename,
                                     cl_int* const error)
{
    std::ifstream in(filename);
    if (in.good())
    {
        try
        {
            std::string red((std::istreambuf_iterator<char>(in)),
                            std::istreambuf_iterator<char>());

            *error = CL_SUCCESS;
            return red;

        } catch (std::bad_alloc& ex)
        {
            (void)ex;
            detail::errHandler(CL_OUT_OF_RESOURCES, error, "Bad allocation!");
            return std::string();
        }
    }
    else
    {
        detail::errHandler(CL_INVALID_VALUE, error, "No file!");
        return std::string();
    }
}

std::vector<unsigned char>
cl::util::read_binary_file(const char* const filename, cl_int* const error)
{
    std::ifstream in(filename, std::ios::binary);
    if (in.good())
    {
        try
        {
            std::vector<unsigned char> buffer(
                std::istreambuf_iterator<char>(in), {});
            return buffer;
        } catch (std::bad_alloc& ex)
        {
            (void)ex;
            detail::errHandler(CL_OUT_OF_RESOURCES, error, "Bad allocation!");
            return std::vector<unsigned char>();
        }
    }
    else
    {
        *error = CL_INVALID_VALUE;
        return std::vector<unsigned char>();
    }
}


cl::Program::Binaries
cl::util::read_binary_files(const std::vector<cl::Device>& devices,
                            const char* const program_file_name,
                            cl_int* const error)
{
    cl::Program::Binaries binaries(0);
    size_t num_devices = devices.size();

    for (const auto& device : devices)
    {
        string device_name = device.getInfo<CL_DEVICE_NAME>();
        string binary_name =
            string(program_file_name) + "-" + device_name + ".bin";


        binaries.push_back(
            cl::util::read_binary_file(binary_name.c_str(), error));
        if (*error == CL_INVALID_VALUE) return binaries;
    }
    *error = CL_SUCCESS;
    return binaries;
}

cl_int cl::util::write_binaries(const cl::Program::Binaries& binaries,
                                const std::vector<cl::Device>& devices,
                                const char* const program_file_name)
{
    cl_int error = CL_SUCCESS;
    if (binaries.size() == devices.size())
    {
        try
        {
            for (auto i = 0; i < binaries.size(); ++i)
            {
                string binary_name = string(program_file_name) + "-"
                    + devices.at(i).getInfo<CL_DEVICE_NAME>() + ".bin";

                std::ofstream out(binary_name, std::ios::binary);
                out.write((const char*)binaries[i].data(),
                          binaries[i].size() * sizeof(char));
            }
            return error;
        } catch (std::bad_alloc& ex)
        {
            (void)ex;
            detail::errHandler(CL_OUT_OF_RESOURCES, &error, "Bad allocation!");
            return error;
        }
    }
    else
    {
        detail::errHandler(CL_INVALID_VALUE, &error,
                           "binaries and devices don't match");
        return error;
    }
}
