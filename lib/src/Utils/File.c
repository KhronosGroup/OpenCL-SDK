// OpenCL SDK includes
#include <CL/Utils/Utils.h>

// STL includes
#include <stdlib.h> // realloc, free
#include <stdio.h> // fopen, ferror, fread, fclose
#include <string.h> // memset

// read all the text file contents securely in ANSI C89
// return pointer to C-string with file contents
// can handle streams with no known size and no support for fseek
// based on https://stackoverflow.com/questions/14002954/ by Nominal Animal
UTILS_EXPORT
char *cl_util_read_text_file(const char *const filename, size_t *const length,
                             cl_int *const error)
{
    cl_int err = CL_SUCCESS;
    char *data = NULL, *temp;
    size_t size = 0;
    size_t used = 0;
    size_t n;
    FILE *in;

    /* Size of each input chunk to be read and allocate for. */
#define READALL_CHUNK 2097152

#define IF_ERR(func, error_type, label)                                        \
    do                                                                         \
    {                                                                          \
        if (func)                                                              \
        {                                                                      \
            err = error_type;                                                  \
            goto label;                                                        \
        }                                                                      \
    } while (0)

#ifndef _WIN32
#define fopen_s(fp, fmt, mode)                                                 \
    ({                                                                         \
        *(fp) = fopen((fmt), (mode));                                          \
        (*(fp)) ? 0 : -1;                                                      \
    })
#endif

    /* File name can not be NULL. */
    IF_ERR(!filename, CL_INVALID_ARG_VALUE, end);

    /* Open file. */
    IF_ERR((fopen_s(&in, filename, "r") != 0), CL_INVALID_VALUE, end);

    /* A read error already occurred? */
    IF_ERR(ferror(in), CL_UTIL_FILE_OPERATION_ERROR, fl);

    while (1)
    {
        if (used + READALL_CHUNK + 1 > size)
        {
            size = used + READALL_CHUNK + 1;

            /* Overflow check. */
            IF_ERR(size <= used, CL_OUT_OF_RESOURCES, nodata);

            MEM_CHECK(temp = (char *)realloc(data, size), err, nodata);
            data = temp;
        }

        /* Read file in chunks. */
        n = fread(data + used, 1, READALL_CHUNK, in);
        if (n == 0) break;

        used += n;
    }

    /* A read error already occurred? */
    IF_ERR(ferror(in), CL_UTIL_FILE_OPERATION_ERROR, nodata);

    /* Put null termination. */
    MEM_CHECK(temp = (char *)realloc(data, used + 1), err, nodata);
    data = temp;
    data[used] = '\0';
    if (length != NULL) *length = used;

fl:
    fclose(in);
end:
    if (error != NULL) *error = err;
    return data;

nodata:
    fclose(in);
    free(data);
    if (error != NULL) *error = err;
    return NULL;

#undef IF_ERR
}

UTILS_EXPORT
unsigned char *cl_util_read_binary_file(const char *const filename,
                                        size_t *const length,
                                        cl_int *const error)
{
    cl_int err = CL_SUCCESS;
    unsigned char *data = NULL, *temp;
    size_t size = 0;
    size_t used = 0;
    size_t n;
    FILE *in;

    /* Size of each input chunk to be read and allocate for. */
#define READALL_CHUNK 2097152

#define IF_ERR(func, error_type, label)                                        \
    do                                                                         \
    {                                                                          \
        if (func)                                                              \
        {                                                                      \
            err = error_type;                                                  \
            goto label;                                                        \
        }                                                                      \
    } while (0)

    /* File name can not be NULL. */
    IF_ERR(!filename, CL_INVALID_ARG_VALUE, end);

    /* Open file. */
    IF_ERR((fopen_s(&in, filename, "rb") != 0), CL_INVALID_VALUE, end);

    /* A read error already occurred? */
    IF_ERR(ferror(in), CL_UTIL_FILE_OPERATION_ERROR, fl);

    while (1)
    {
        if (used + READALL_CHUNK + 1 > size)
        {
            size = used + READALL_CHUNK + 1;

            /* Overflow check. */
            IF_ERR(size <= used, CL_OUT_OF_RESOURCES, nodata);

            MEM_CHECK(temp = (unsigned char *)realloc(data, size), err, nodata);
            data = temp;
        }

        /* Read file in chunks. */
        n = fread(data + used, 1, READALL_CHUNK, in);
        if (n == 0) break;

        used += n;
    }

    /* A read error already occurred? */
    IF_ERR(ferror(in), CL_UTIL_FILE_OPERATION_ERROR, nodata);

    /* Truncate to the real size. */
    MEM_CHECK(temp = (unsigned char *)realloc(data, used), err, nodata);
    data = temp;
    if (length != NULL) *length = used;

fl:
    fclose(in);
end:
    if (error != NULL) *error = err;
    return data;

nodata:
    fclose(in);
    free(data);
    if (error != NULL) *error = err;
    return NULL;

#undef IF_ERR
}

// function to write binaries of OpenCL compiled program
// binaries are written as separate files for each device
// with file name "(program_file_name)_(name of device).bin"
// based on variant of Logan
// http://logan.tw/posts/2014/11/22/pre-compile-the-opencl-kernel-program-part-2/
UTILS_EXPORT
cl_int cl_util_write_binaries(const cl_program program,
                              const char *const program_file_name)
{
    cl_int error = CL_SUCCESS;

    cl_uint num_devices = 0;
    size_t binaries_size_alloc_size = 0;
    size_t binaries_ptr_alloc_size = 0;
    size_t *binaries_size = NULL;
    unsigned char **binaries_ptr = NULL;
    cl_device_id *devices = NULL;

    // read number of devices
    OCLERROR_RET(clGetProgramInfo(program, CL_PROGRAM_NUM_DEVICES,
                                  sizeof(cl_uint), &num_devices, NULL),
                 error, end);
    if (num_devices == 0) return CL_INVALID_PROGRAM_EXECUTABLE;

    // read the binaries size
    binaries_size_alloc_size = sizeof(size_t) * num_devices;
    MEM_CHECK(binaries_size = (size_t *)malloc(binaries_size_alloc_size), error,
              end);

    OCLERROR_RET(clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES,
                                  binaries_size_alloc_size, binaries_size,
                                  NULL),
                 error, end);

    // read the binaries
    binaries_ptr_alloc_size = sizeof(unsigned char *) * num_devices;
    MEM_CHECK(binaries_ptr = (unsigned char **)malloc(binaries_ptr_alloc_size),
              error, end);
    memset(binaries_ptr, 0, binaries_ptr_alloc_size);

    for (cl_uint i = 0; i < num_devices; ++i)
        MEM_CHECK(binaries_ptr[i] = (unsigned char *)malloc(binaries_size[i]),
                  error, end);

    OCLERROR_RET(clGetProgramInfo(program, CL_PROGRAM_BINARIES,
                                  binaries_ptr_alloc_size, binaries_ptr, NULL),
                 error, end);

    // query devices
    MEM_CHECK(devices =
                  (cl_device_id *)malloc(sizeof(cl_device_id) * num_devices),
              error, end);
    OCLERROR_RET(clGetProgramInfo(program, CL_PROGRAM_DEVICES,
                                  sizeof(cl_device_id) * num_devices, devices,
                                  NULL),
                 error, end);

    // write the binaries to files
    for (cl_uint i = 0; i < num_devices; ++i)
    {
        // get device name
        char name_data[201];
        OCLERROR_RET(clGetDeviceInfo(devices[i], CL_DEVICE_NAME,
                                     sizeof(name_data) - 1, name_data, NULL),
                     error, end);
        name_data[200] = '\0';
        // create output file name
        char filename[256];
        snprintf(filename, sizeof(filename), "%s-%s.bin", program_file_name,
                 name_data);

        // write the binary to the output file
        FILE *f = NULL;
        if (fopen_s(&f, filename, "wb") != 0)
        {
            if (fwrite(binaries_ptr[i], sizeof(unsigned char), binaries_size[i],
                       f)
                != binaries_size[i])
                error = CL_UTIL_FILE_OPERATION_ERROR;
            fclose(f);
        }
        else
            error = CL_INVALID_VALUE;
    }

end: // cleanup
    if (binaries_ptr != NULL)
    {
        for (cl_uint i = 0; i < num_devices; ++i) free(binaries_ptr[i]);
        free(binaries_ptr);
    }
    free(binaries_size);

    return error;
}

// function to read binaries of OpenCL compiled program
// read binaries from files of file names "(program_file_name)_(name of
// device).bin"
UTILS_EXPORT
cl_program cl_util_read_binaries(const cl_context context,
                                 const cl_device_id *const devices,
                                 const cl_uint num_devices,
                                 const char *const program_file_name,
                                 cl_int *const error)
{
    cl_int err = CL_SUCCESS;
    size_t binaries_size_alloc_size = 0;
    size_t binaries_ptr_alloc_size = 0;
    size_t *binaries_size = NULL;
    unsigned char **binaries_ptr = NULL;
    cl_program program = NULL;

    // array for binaries' sizes
    binaries_size_alloc_size = sizeof(size_t) * num_devices;
    MEM_CHECK(binaries_size = (size_t *)malloc(binaries_size_alloc_size), err,
              end);

    // array for the binaries
    binaries_ptr_alloc_size = sizeof(unsigned char *) * num_devices;
    MEM_CHECK(binaries_ptr = (unsigned char **)malloc(binaries_ptr_alloc_size),
              err, end);
    memset(binaries_ptr, 0, binaries_ptr_alloc_size);

    // read binaries from files
    for (cl_uint i = 0; i < num_devices; ++i)
    {
        // get device name
        char name_data[201];
        OCLERROR_RET(clGetDeviceInfo(devices[i], CL_DEVICE_NAME,
                                     sizeof(name_data) - 1, name_data, NULL),
                     err, end);
        name_data[200] = '\0';
        // create input file name
        char filename[256];
        snprintf(filename, sizeof(filename), "%s-%s.bin", program_file_name,
                 name_data);

        binaries_ptr[i] =
            cl_util_read_binary_file(filename, binaries_size + i, &err);
        if (err == CL_INVALID_VALUE)
        {
            fprintf(stderr, "No suitable file %s found\n", filename);
            goto end;
        }
        else if (err != CL_SUCCESS)
            goto end;
    }

    OCLERROR_PAR(program = clCreateProgramWithBinary(
                     context, num_devices, devices, binaries_size,
                     (const unsigned char **)binaries_ptr, &err, NULL),
                 err, end);

end: // cleanup
    if (binaries_ptr != NULL)
    {
        for (cl_uint i = 0; i < num_devices; ++i) free(binaries_ptr[i]);
        free(binaries_ptr);
    }
    free(binaries_size);

    if (error != NULL) *error = err;
    return program;
}
