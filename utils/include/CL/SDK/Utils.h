#pragma once

#include<stdlib.h>
#include<stdio.h>
#include<CL/cl.h>

// reads all the text file contents securely in ANSI C89
// returns pointer to C-string with file contents
// can handle streams with no known size and no support for fseek
// based on https://stackoverflow.com/questions/14002954/ by Nominal Animal
char * cl_utils_read_text_file(char * filename, size_t * length, cl_int * errcode_ret)
{
    char * data = NULL, * temp;
    size_t size = 0;
    size_t used = 0;
    size_t n;
    FILE * in;

    /* Size of each input chunk to be read and allocate for. */
    #define READALL_CHUNK 2097152

    /* File name can not be NULL. */
    if (errcode_ret == NULL) {
        return NULL;
    }
    if (filename == NULL) {
        *errcode_ret = CL_INVALID_ARG_VALUE;
        return NULL;
    }

    /* Open file. */
    in = fopen(filename, "r");
    if (in == NULL) {
        *errcode_ret = CL_INVALID_VALUE;
        return NULL;
    }

    /* A read error already occurred? */
    if (ferror(in)) {
        *errcode_ret = CL_INVALID_VALUE;
        return NULL;
    }

    while (1) {
        if (used + READALL_CHUNK + 1 > size) {
            size = used + READALL_CHUNK + 1;

            /* Overflow check. */
            if (size <= used) {
                free(data);
                *errcode_ret = CL_OUT_OF_RESOURCES;
                return NULL;
            }

            temp = (char *)realloc(data, size);
            if (temp == NULL) {
                free(data);
                *errcode_ret = CL_OUT_OF_HOST_MEMORY;
                return NULL;
            }
            data = temp;
        }

        n = fread(data + used, 1, READALL_CHUNK, in);
        if (n == 0)
            break;

        used += n;
    }

    if (ferror(in)) {
        free(data);
        *errcode_ret = CL_INVALID_VALUE;
        return NULL;
    }

    temp = (char *)realloc(data, used + 1);
    if (temp == NULL) {
        free(data);
        *errcode_ret = CL_OUT_OF_HOST_MEMORY;
        return NULL;
    }
    data = temp;
    data[used] = '\0';
    if (length != NULL)
        *length = used;

    *errcode_ret = CL_SUCCESS;
    return data;
}
