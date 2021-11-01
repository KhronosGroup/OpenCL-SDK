#include <CL/Utils/Utils.h>

#include<stdlib.h>
#include<stdio.h>

// reads all the text file contents securely in ANSI C89
// returns pointer to C-string with file contents
// can handle streams with no known size and no support for fseek
// based on https://stackoverflow.com/questions/14002954/ by Nominal Animal
UTILS_EXPORT char * cl_utils_read_text_file(const char * filename, size_t * length, cl_int * errcode_ret)
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
do { if (func) {*errcode_ret = err; goto label;} } while (0)

    if (errcode_ret == NULL) {
        errcode_ret = &err;
    }

    /* File name can not be NULL. */
    IF_ERR(!filename, CL_INVALID_ARG_VALUE, end);

    /* Open file. */
    IF_ERR(fopen_s(&in, filename, "r"), CL_INVALID_VALUE, end);

    /* A read error already occurred? */
    IF_ERR(ferror(in), CL_INVALID_VALUE, fl);

    while (1) {
        if (used + READALL_CHUNK + 1 > size) {
            size = used + READALL_CHUNK + 1;

            /* Overflow check. */
            IF_ERR(size <= used, CL_OUT_OF_RESOURCES, dt);

            MEM_CHECK(temp = (char *)realloc(data, size), *errcode_ret, dt);
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
    MEM_CHECK(temp = (char *)realloc(data, used + 1), *errcode_ret, dt);
    data = temp;
    data[used] = '\0';
    if (length != NULL)
        *length = used;

    *errcode_ret = CL_SUCCESS;
fl:     fclose(in);
end:    return data;

dt:     fclose(in);
        free(data);
        return NULL;
}
