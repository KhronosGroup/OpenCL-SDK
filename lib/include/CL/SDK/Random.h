#pragma once

// STL includes
#include <stdint.h>
#include <stdbool.h>
#include<math.h>

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

typedef struct { uint64_t state;  uint64_t inc; } pcg32_random_t;

// pcg32_random()
// pcg32_random_r(rng)
//     Generate a uniformly distributed 32-bit random number

uint32_t pcg32_random_r(pcg32_random_t * rng)
{
    uint64_t oldstate = rng->state;
    // Advance internal state
    rng->state = oldstate * 6364136223846793005ULL + (rng->inc | 1);
    // Calculate output function (XSH RR), uses old state for max ILP
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

cl_float pcg32_random_float(pcg32_random_t * rng)
{
    return ldexp(pcg32_random_r(rng), -32);
}

cl_float pcg32_random_float_range(pcg32_random_t * rng, cl_float low, cl_float hi)
{
    return ldexp(pcg32_random_r(rng), -32) * (hi - low) + low;
}

// pcg32_srandom(initstate, initseq)
// pcg32_srandom_r(rng, initstate, initseq):
//     Seed the rng.  Specified in two parts, state initializer and a
//     sequence selection constant (a.k.a. stream id)

void pcg32_srandom_r(pcg32_random_t * rng, uint64_t initstate, uint64_t initseq)
{
    rng->state = 0U;
    rng->inc = (initseq << 1u) | 1u;
    pcg32_random_r(rng);
    rng->state += initstate;
    pcg32_random_r(rng);
}

// fill array with random floats in [0, 1)
void cl_sdk_fill_with_random_floats(pcg32_random_t * rng, cl_float * arr, size_t len)
{
    for (; len > 0; arr[--len] = pcg32_random_float(rng));
}

void cl_sdk_fill_with_random_floats_range(pcg32_random_t * rng, cl_float * arr, size_t len, cl_float low, cl_float hi)
{
    cl_float diff = hi - low;
    for (; len > 0; arr[--len] = pcg32_random_float(rng) * diff + low);
}

// return uniformly distributed numbers in the range [low, hi]
// use rejection sampling from uniform bit distribution
void cl_sdk_fill_with_random_ints_range(pcg32_random_t * rng, cl_int * arr, size_t len, cl_int low, cl_int hi)
{
    const uint32_t
        diff = hi - low,
        bits = log2(diff),
        mask = (1u << (bits + 1)) - 1;
    while (len > 0) {
        --len;
        uint32_t res;
        bool bad = true;
        do {
            res = pcg32_random_r(rng);
            for (int i = 0; i < 32 / bits; ++i, res >>= bits)
                if ((res & mask) <= diff) {
                    res &= mask;
                    bad = false;
                    break;
                }
        } while (bad);
        arr[len] = low + (cl_int)res;
    }
}
