// OpenCL SDK includes
#include <CL/SDK/Image.h>

// OpenCL Utils includes
#include <CL/Utils/Utils.h>

// STL includes
#include <stdio.h> // fprintf
#include <ctype.h> // tolower

// stb includes
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

// OpenCL includes
#include <CL/cl.h>

cl_sdk_image cl_sdk_read_image(const char* const file_name, cl_int* const error)
{
    cl_int err = CL_SUCCESS;

    cl_sdk_image im = {
        .width = 0, .height = 0, .pixel_size = 1, .pixels = NULL
    };
    im.pixels = stbi_load(file_name, &im.width, &im.height, &im.pixel_size, 0);

    if (im.width && im.height && im.pixel_size && im.pixels)
        err = CL_SUCCESS;
    else
    {
        fprintf(stderr, "File read error!");
        err = CL_INVALID_ARG_VALUE;
    }

    if (error != NULL) *error = err;
    return im;
}

static char* to_lowercase(const char* const s, char* const d, const size_t n)
{
    for (size_t i = 0; i < n; ++i) d[i] = tolower(s[i]);
    return d;
}

cl_int cl_sdk_write_image(const char* const file_name,
                          const cl_sdk_image* const im)
{
    cl_int error = CL_SUCCESS;
    char fext[5] = { 0, 0, 0, 0, 0 };

#define IF_EXT(ext, func, err_text)                                            \
    if (!strcmp(to_lowercase(file_name + strlen(file_name) - 4, fext, 4),      \
                ext))                                                          \
    {                                                                          \
        if (!func)                                                             \
        {                                                                      \
            fprintf(stderr, err_text);                                         \
            error = CL_INVALID_ARG_VALUE;                                      \
        }                                                                      \
    }

    IF_EXT(".png",
           stbi_write_png(file_name, im->width, im->height, im->pixel_size,
                          im->pixels, 0),
           "Not possible to write PNG file!\n")
    else IF_EXT(
        ".bmp",
        stbi_write_bmp(file_name, im->width, im->height, im->pixel_size,
                       im->pixels),
        "Not possible to write BMP file!\n") else IF_EXT(".jpg",
                                                         stbi_write_jpg(
                                                             file_name,
                                                             im->width,
                                                             im->height,
                                                             im->pixel_size,
                                                             im->pixels, 80),
                                                         "Not possible to "
                                                         "write JPG "
                                                         "file!\n") else
    {
        fprintf(stderr, "Unknown file extension!\n");
        error = CL_IMAGE_FORMAT_NOT_SUPPORTED;
    }

#undef IF_EXT
    return error;
}
