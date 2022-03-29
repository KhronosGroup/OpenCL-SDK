#pragma once

// OpenCL SDK includes
#include "OpenCLSDK_Export.h"

// Standard C includes
#include <stddef.h> // size_t
#include <stdint.h> // uint64_t

// OpenCL includes
#include <CL/cl.h> // cl_int, cl_float

/*
 * PCG Random Number Generation for C.
 *
 * Copyright 2014 Melissa O'Neill <oneill@pcg-random.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For additional information about the PCG random number generation scheme,
 * including its license and other licensing options, visit
 *
 *       http://www.pcg-random.org
 */

// *Really* minimal PCG32 code / (c) 2014 M.E. O'Neill / pcg-random.org
// Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)

typedef struct
{
    uint64_t state;
    uint64_t inc;
} pcg32_random_t;

// pcg32_srandom(initstate, initseq)
// pcg32_srandom_r(rng, initstate, initseq):
//     Seed the rng.  Specified in two parts, state initializer and a
//     sequence selection constant (a.k.a. stream id)

SDK_EXPORT void pcg32_srandom_r(pcg32_random_t* rng, uint64_t initstate,
                                uint64_t initseq);

// fill array with random floats in [0, 1)
SDK_EXPORT void cl_sdk_fill_with_random_floats(pcg32_random_t* rng,
                                               cl_float* arr, const size_t len);

SDK_EXPORT void cl_sdk_fill_with_random_floats_range(pcg32_random_t* rng,
                                                     cl_float* arr,
                                                     const size_t len,
                                                     const cl_float low,
                                                     const cl_float hi);

// return uniformly distributed numbers in the range [low, hi]
// use rejection sampling from uniform bit distribution
SDK_EXPORT void cl_sdk_fill_with_random_ints_range(pcg32_random_t* rng,
                                                   cl_int* arr,
                                                   const size_t len,
                                                   const cl_int low,
                                                   const cl_int hi);
