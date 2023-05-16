// OpenCL SDK includes
#include <CL/SDK/Image.hpp>

// OpenCL Utils includes
#include <CL/Utils/Utils.hpp>

// stb includes
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

// STL includes
#include <vector> // std::vector
#include <algorithm>

#if __cplusplus < 201703L
#include <string.h>
#else
#include <filesystem>
#endif

// OpenCL includes
#include <CL/opencl.hpp>

namespace cl {
namespace sdk {
    Image read_image(const char* const file_name, cl_int* const error = nullptr)
    {
        cl_int err = CL_SUCCESS;

        Image im;
        unsigned char* data =
            stbi_load(file_name, &im.width, &im.height, &im.pixel_size, 0);

        if (data == nullptr)
        {
            std::string err_msg{ "Not possible to read file" };
            const char* load_msg = stbi_failure_reason();

            if (load_msg) err_msg += std::string(": ") + load_msg;

            cl::util::detail::errHandler(CL_INVALID_ARG_VALUE, &err,
                                         err_msg.c_str());
        }

        im.pixels.insert(im.pixels.end(), data,
                         data + im.width * im.height * im.pixel_size);

        if (im.width && im.height && im.pixel_size
            && im.pixels.size() == im.width * im.height * im.pixel_size)
            err = CL_SUCCESS;
        else
            cl::util::detail::errHandler(CL_INVALID_ARG_VALUE, &err,
                                         "File read error!");

        if (error != nullptr) *error = err;
        return im;
    }

    cl_int write_image(const char* file_name, const Image& image)
    {
        cl_int error = CL_SUCCESS;

#if __cplusplus >= 201703L
        std::filesystem::path fn(file_name);
        std::string extension = fn.extension().string();
#else
        const char* extpos = strrchr(file_name, '.');
        if (extpos == nullptr)
            cl::util::detail::errHandler(CL_INVALID_ARG_VALUE, &error,
                                         "No file extension!");
        std::string extension(extpos);
#endif

        // solution by Timmmm https://stackoverflow.com/questions/11635
        auto str_compare = [](const string& a, const string& b) {
            return std::equal(
                a.begin(), a.end(), b.begin(), b.end(),
                [](char a, char b) { return tolower(a) == tolower(b); });
        };

        if (str_compare(extension, ".png"))
        {
            if (!stbi_write_png(file_name, image.width, image.height,
                                image.pixel_size, image.pixels.data(), 0))
                cl::util::detail::errHandler(CL_INVALID_ARG_VALUE, &error,
                                             "Not possible to write PNG file!");
        }
        else if (str_compare(extension, ".bmp"))
        {
            if (!stbi_write_bmp(file_name, image.width, image.height,
                                image.pixel_size, image.pixels.data()))
                cl::util::detail::errHandler(CL_INVALID_ARG_VALUE, &error,
                                             "Not possible to write BMP file!");
        }
        else if (str_compare(extension, ".jpg"))
        {
            if (!stbi_write_jpg(file_name, image.width, image.height,
                                image.pixel_size, image.pixels.data(), 80))
                cl::util::detail::errHandler(CL_INVALID_ARG_VALUE, &error,
                                             "Not possible to write JPG file!");
        }
        else
            cl::util::detail::errHandler(CL_IMAGE_FORMAT_NOT_SUPPORTED, &error,
                                         "Unknown file extension!");

        return error;
    }
}
}
