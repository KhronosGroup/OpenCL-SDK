#pragma once

#include <stdlib.h>
#include <stdio.h>

// reads all the text file contents securely in ANSI C89
// returns pointer to C-string with file contents
// based on https://stackoverflow.com/questions/14002954/ by Nominal Animal
char * cl_sdk_read_file(char * filename)
{
    char * data = NULL, * temp;
    size_t size = 0;
    size_t used = 0;
    size_t n;
    FILE * in;

    #define  READALL_CHUNK 2097152

    /* File name can not be NULL. */
    if (filename == NULL)
        return NULL;

    /* Open file. */
    in = fopen(filename, "r");
    if (in == NULL)
        return NULL;

    /* A read error already occurred? */
    if (ferror(in))
        return NULL;

    while (1) {
        if (used + READALL_CHUNK + 1 > size) {
            size = used + READALL_CHUNK + 1;

            /* Overflow check. */
            if (size <= used) {
                free(data);
                return NULL;
            }

            temp = (char *)realloc(data, size);
            if (temp == NULL) {
                free(data);
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
        return NULL;
    }

    temp = (char *)realloc(data, used + 1);
    if (temp == NULL) {
        free(data);
        return NULL;
    }
    data = temp;
    data[used] = '\0';

    return data;
}