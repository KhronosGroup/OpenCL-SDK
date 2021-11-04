#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <cargs.h>

#include <CL/cl.h>

struct cl_sdk_options_Triplet
{
    int plat_index;
    int dev_index;
    cl_device_type dev_type;
};

struct cl_sdk_options_Diagnostic
{
    bool verbose;
    bool quiet;
};

struct cl_sdk_options_SingleDevice
{
    struct cl_sdk_options_Triplet triplet;
};

struct cl_sdk_options_MultiDevice
{
    struct cl_sdk_options_Triplet * triplets;
    size_t number;
};

typedef struct cag_option cag_option;

cag_option * add_CLI_options(cag_option * opts, size_t * const num_opts, cag_option * add_opts, size_t add_num_opts)
{
    cag_option * tmp = NULL;

    tmp = (cag_option *)realloc(opts, sizeof(cag_option) * (*num_opts + add_num_opts));
    if (tmp) {
        memcpy(tmp + *num_opts, add_opts, sizeof(cag_option) * add_num_opts);
        *num_opts += add_num_opts;
    }

    return tmp;
}

enum ParseState
{
    ParsedOK,
    NotParsed,
    ParseError
};

typedef enum ParseState ParseState;

cag_option DiagnosticOptions[] = {
 {.identifier = 'h',
  .access_letters = "h",
  .access_name = "help",
  .description = "Show this help"},

 {.identifier = 'q',
  .access_letters = "q",
  .access_name = "quiet",
  .description = "Suppress standard output"},

 {.identifier = 'v',
  .access_letters = "v",
  .access_name = "verbose",
  .description = "Extra informational output"}
};

ParseState parse_DiagnosticOptions(const char identifier, struct cl_sdk_options_Diagnostic * diag_opts)
{
    switch (identifier) {
    case 'q':
        diag_opts->quiet = true;
        return ParsedOK;
    case 'v':
        diag_opts->verbose = true;
        return ParsedOK;
    }
    return NotParsed;
}

cag_option SingleDeviceOptions[] = {
 {.identifier = 'p',
  .access_letters = "p",
  .access_name = "platform",
  .value_name = "(positive integer)",
  .description = "Index of platform to use"},

 {.identifier = 'd',
  .access_letters = "d",
  .access_name = "device",
  .value_name = "(positive integer)",
  .description = "Index of device to use"},

 {.identifier = 't',
  .access_letters = "t",
  .access_name = "type",
  .value_name = "(all|cpu|gpu|acc|def)",
  .description = "Type of device to use"}
};

// TODO: error handling
cl_device_type get_dev_type(const char * in)
{
    if (!strcmp(in, "all")) return CL_DEVICE_TYPE_ALL;
    else if (!strcmp(in, "cpu")) return CL_DEVICE_TYPE_CPU;
    else if (!strcmp(in, "gpu")) return CL_DEVICE_TYPE_GPU;
    else if (!strcmp(in, "acc")) return CL_DEVICE_TYPE_ACCELERATOR;
    else if (!strcmp(in, "def")) return CL_DEVICE_TYPE_DEFAULT;
    else return CL_DEVICE_TYPE_ALL;//CL_INVALID_DEVICE_TYPE;// "Unkown device type after cli parse. Should not have happened."
}

ParseState parse_SingleDeviceOptions(const char identifier, cag_option_context * cag_context, struct cl_sdk_options_SingleDevice * dev_opts)
{
    const char * value;
    switch (identifier) {
    case 'p':
        if ((value = cag_option_get_value(cag_context))) {
            dev_opts->triplet.plat_index = strtoul(value, NULL, 0);
            return ParsedOK;
        }
        else return ParseError;
    case 'd':
        if ((value = cag_option_get_value(cag_context))) {
            dev_opts->triplet.dev_index = strtoul(value, NULL, 0);
            return ParsedOK;
        }
        else return ParseError;
    case 't':
        if ((value = cag_option_get_value(cag_context))) {
            dev_opts->triplet.dev_type = get_dev_type(value);
            return 1;
        }
        else return ParseError;
    }
    return NotParsed;
}
