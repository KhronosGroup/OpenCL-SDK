#pragma once

// OpenCL SDK includes
#include "OpenCLSDK_Export.h"
#include <CL/SDK/Options.h>

// ISO C includes
#include <stddef.h> // size_t

// cargs includes
#include <cargs.h>

typedef struct cag_option cag_option;

SDK_EXPORT cag_option *add_CLI_options(cag_option *opts, size_t *const num_opts,
                                       cag_option *add_opts,
                                       size_t add_num_opts);

enum ParseState
{
    ParsedOK,
    NotParsed,
    ParseError
};

typedef enum ParseState ParseState;

SDK_EXPORT extern cag_option DiagnosticOptions[3];

SDK_EXPORT ParseState parse_DiagnosticOptions(
    const char identifier, struct cl_sdk_options_Diagnostic *diag_opts);

SDK_EXPORT extern cag_option SingleDeviceOptions[3];

// TODO: error handling
SDK_EXPORT cl_device_type get_dev_type(const char *in);

SDK_EXPORT ParseState parse_SingleDeviceOptions(
    const char identifier, cag_option_context *cag_context,
    struct cl_sdk_options_SingleDevice *dev_opts);

#define PARS_OPTIONS(parser, state)                                            \
    do                                                                         \
    {                                                                          \
        if (state == NotParsed) state = parser;                                \
        if (state == ParseError)                                               \
        {                                                                      \
            fprintf(stderr, "Parse error\n");                                  \
            identifier = 'h';                                                  \
            state = ParsedOK;                                                  \
        }                                                                      \
    } while (0)
