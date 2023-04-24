// OpenCL SDK includes
#include <CL/Utils/File.hpp>

// STL includes
#include <fstream>
#include <iterator>
#include <algorithm>
#include <iostream>

// whereami includes
#include <whereami.h>

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

            if (error != nullptr) *error = CL_SUCCESS;
            return red;

        } catch (std::bad_alloc&)
        {
            detail::errHandler(CL_OUT_OF_RESOURCES, error, "Bad allocation!");
            return std::string();
        }
    }
    else
    {
        detail::errHandler(CL_UTIL_FILE_OPERATION_ERROR, error,
                           (std::string("Unable to read ") + filename).c_str());
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
            if (error != nullptr) *error = CL_SUCCESS;
            return buffer;
        } catch (std::bad_alloc&)
        {
            detail::errHandler(CL_OUT_OF_RESOURCES, error, "Bad allocation!");
            return std::vector<unsigned char>();
        }
    }
    else
    {
        detail::errHandler(CL_UTIL_FILE_OPERATION_ERROR, error,
                           (std::string("Unable to read ") + filename).c_str());
        return std::vector<unsigned char>();
    }
}


cl::Program::Binaries
cl::util::read_binary_files(const std::vector<cl::Device>& devices,
                            const char* const program_base_name,
                            cl_int* const error)
{
    cl::Program::Binaries binaries(0);

    for (const auto& device : devices)
    {
        string device_name = device.getInfo<CL_DEVICE_NAME>();
        string binary_name =
            string(program_base_name) + "-" + device_name + ".bin";

        binaries.push_back(
            cl::util::read_binary_file(binary_name.c_str(), error));
        if (error != nullptr)
        {
            if (*error != CL_SUCCESS)
            {
                detail::errHandler(CL_UTIL_FILE_OPERATION_ERROR, error,
                                   "Not all binaries found!");
                return cl::Program::Binaries();
            }
        }
    }
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
        } catch (std::bad_alloc&)
        {
            detail::errHandler(CL_OUT_OF_RESOURCES, &error, "Bad allocation!");
            return error;
        }
    }
    else
    {
        detail::errHandler(CL_INVALID_VALUE, &error,
                           "Binaries and devices don't match!");
        return error;
    }
}

std::string cl::util::executable_folder(cl_int* const error)
{
    int wai_length = wai_getExecutablePath(NULL, 0, NULL);
    if (wai_length == -1)
    {
        detail::errHandler(CL_UTIL_FILE_OPERATION_ERROR, error,
                           "Unable to query executable path length!");
        return "";
    }
    std::string wai_filename(wai_length, '\0');
    int wai_dirname_length = -1;
    wai_length = wai_getExecutablePath(&wai_filename.front(), wai_length,
                                       &wai_dirname_length);
    if (wai_length == -1 || wai_dirname_length == -1)
    {
        detail::errHandler(CL_UTIL_FILE_OPERATION_ERROR, error,
                           "Unable to query executable path or folder length!");
        return "";
    }

    return wai_filename.substr(0, wai_dirname_length);
}

std::string cl::util::read_exe_relative_text_file(const char* const filename,
                                                  cl_int* const error)
{
    cl_int err = CL_SUCCESS;
    std::string exe_folder = executable_folder(&err);
    if (err != CL_SUCCESS)
    {
        detail::errHandler(CL_UTIL_FILE_OPERATION_ERROR, error,
                           "Failed to query exe folder!");
        return "";
    }
    std::string result =
        read_text_file((exe_folder + "/" + filename).c_str(), &err);
    if (err != CL_SUCCESS)
    {
        detail::errHandler(CL_UTIL_FILE_OPERATION_ERROR, error,
                           "Unable to read file!");
        return "";
    }
    return result;
}
