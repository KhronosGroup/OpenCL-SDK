#pragma once

#define CL_UTIL_INDEX_OUT_OF_RANGE -2000

// RET = function returns error code
// PAR = functions sets error code in the paremeter

#define OCLERROR_RET(func, err, label) \
do { err = func; if (err != CL_SUCCESS) goto label; } while (0)

#define OCLERROR_PAR(func, err, label) \
do { func; if (err != CL_SUCCESS) goto label; } while (0)

#define MEM_CHECK(func, err, label) \
do { if ((func) == NULL) {err = CL_OUT_OF_HOST_MEMORY; goto label;} } while (0)
