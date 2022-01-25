#pragma once

#include "OpenCLUtils_Export.h"

#include <CL/cl.h>
#include <CL/Utils/File.h>
#include <CL/Utils/Context.h>
#include <CL/Utils/cargs.h>

// RET = function returns error code
// PAR = functions sets error code in the paremeter

#define OCLERROR_RET(func, err, label) \
do { err = func; if (err != CL_SUCCESS) goto label; } while (0)

#define OCLERROR_PAR(func, err, label) \
do { func; if (err != CL_SUCCESS) goto label; } while (0)

#define MEM_CHECK(func, err, label) \
do { if ((func) == NULL) {err = CL_OUT_OF_HOST_MEMORY; goto label;} } while (0)

#define MAXOCLPLAT 65535
#define MAXOCLDEV  65535
