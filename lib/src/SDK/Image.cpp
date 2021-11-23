// OpenCL SDK includes
#include <CL/SDK/Image.hpp>

// stb includes
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

// STL includes
#include <vector>   // std::vector
#include <algorithm>

#if __cplusplus < 201703L
#include <string.h>
#else
#include <filesystem>
#endif

// OpenCL includes
#include <CL/opencl.hpp>

namespace cl
{
namespace sdk
{
    Image read_image(const char* file_name, cl_int * err)
    {
        Image im;
        unsigned char *data = stbi_load(file_name, &im.width, &im.height, &im.pixel_size, 0);
        im.pixels.insert(im.pixels.end(), data, data + im.width * im.height * im.pixel_size);

        if (im.width && im.height && im.pixel_size && im.pixels.size() == im.width * im.height * im.pixel_size)
            *err = CL_SUCCESS;
        else
            cl::util::detail::errHandler(CL_INVALID_ARG_VALUE, err, "File read error!");

        return im;
    }

    void write_image(const char * file_name, const Image& image, cl_int * err)
    {
        *err = CL_SUCCESS;

#if __cplusplus >= 201703L
        std::filesystem::path fn(file_name);
        std::string extension = fn.extension().string();
#else
        const char * extpos = strrchr(file_name, '.');
        if (extpos == nullptr)
            cl::util::detail::errHandler(CL_INVALID_ARG_VALUE, err, "No file extension!");
        std::string extension(extpos);
#endif

        // solution by Timmmm https://stackoverflow.com/questions/11635
        auto str_compare = [](const string& a, const string& b)
        {
            return std::equal(a.begin(), a.end(),
                        b.begin(), b.end(),
                        [](char a, char b) {
                            return tolower(a) == tolower(b);
                        });
        };

        if (str_compare(extension, ".png")) {
            if (!stbi_write_png(file_name, image.width, image.height, image.pixel_size, image.pixels.data(), 0))
                cl::util::detail::errHandler(CL_INVALID_ARG_VALUE, err, "Not possible to write PNG file!");
        }
        else if (str_compare(extension, ".bmp")) {
            if (!stbi_write_bmp(file_name, image.width, image.height, image.pixel_size, image.pixels.data()))
                cl::util::detail::errHandler(CL_INVALID_ARG_VALUE, err, "Not possible to write BMP file!");
        }
        else if (str_compare(extension, ".jpg")) {
            if (!stbi_write_jpg(file_name, image.width, image.height, image.pixel_size, image.pixels.data(), 80))
                cl::util::detail::errHandler(CL_INVALID_ARG_VALUE, err, "Not possible to write JPG file!");
        }
        else
            cl::util::detail::errHandler(CL_IMAGE_FORMAT_NOT_SUPPORTED, err, "Unknown file extension!");

        return;
    }
}
}
