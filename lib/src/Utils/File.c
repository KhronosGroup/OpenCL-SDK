// OpenCL Utils includes
#include <CL/Utils/File.h>

// STL includes
#include<stdlib.h> // realloc, free
#include<stdio.h>  // fopen, ferror, fread, fclose

// read all the text file contents securely in ANSI C89
// return pointer to C-string with file contents
// can handle streams with no known size and no support for fseek
// based on https://stackoverflow.com/questions/14002954/ by Nominal Animal
char * cl_util_read_text_file(const char * filename, size_t * length, cl_int * error)
{
    char * data = NULL, * temp;
    size_t size = 0;
    size_t used = 0;
    size_t n;
    FILE * in;
    cl_int err;

    /* Size of each input chunk to be read and allocate for. */
#define READALL_CHUNK 2097152

#define IF_ERR(func, err, label) \
do { if (func) {*error = err; goto label;} } while (0)

    if (error == NULL) {
        error = &err;
    }

    /* File name can not be NULL. */
    IF_ERR(!filename, CL_INVALID_ARG_VALUE, end);

    /* Open file. */
    IF_ERR(!(in = fopen(filename, "r")), CL_INVALID_VALUE, end);

    /* A read error already occurred? */
    IF_ERR(ferror(in), CL_INVALID_VALUE, fl);

    while (1) {
        if (used + READALL_CHUNK + 1 > size) {
            size = used + READALL_CHUNK + 1;

            /* Overflow check. */
            IF_ERR(size <= used, CL_OUT_OF_RESOURCES, dt);

            MEM_CHECK(temp = (char *)realloc(data, size), *error, dt);
            data = temp;
        }

        /* Read file in chunks. */
        n = fread(data + used, 1, READALL_CHUNK, in);
        if (n == 0)
            break;

        used += n;
    }

    /* A read error already occurred? */
    IF_ERR(ferror(in), CL_INVALID_VALUE, dt);

    /* Put null termination. */
    MEM_CHECK(temp = (char *)realloc(data, used + 1), *error, dt);
    data = temp;
    data[used] = '\0';
    if (length != NULL)
        *length = used;

    *error = CL_SUCCESS;
fl:     fclose(in);
end:    return data;

dt:     fclose(in);
        free(data);
        return NULL;

#undef IF_ERR
}
