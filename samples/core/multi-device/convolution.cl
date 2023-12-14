/*
 * Copyright (c) 2023 The Khronos Group Inc.
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
 */

kernel void convolution_3x3(const global float* in, global float* out,
                            const global float* mask, const uint2 out_dim)
{
    const uint2 gid = (uint2)(get_global_id(0), get_global_id(1));
    const uint mask_dim = 3;
    const uint pad_width = mask_dim / 2;

    // Padded constants.
    const uint2 in_dim = out_dim + pad_width * 2;

    // Check possible out of bounds.
    if (!(gid.x < out_dim.x && gid.y < out_dim.y))
    {
        return;
    }

    // Perform convolution. Fix one column at a time and iterate over each
    // element of it, as data is stored column-major.
    float result = 0.0f;
    #if __OPENCL_C_VERSION__ >= 200
    __attribute__((opencl_unroll_hint))
    #endif
    for(uint y = 0; y < mask_dim; ++y)
    {
        #if __OPENCL_C_VERSION__ >= 200
        __attribute__((opencl_unroll_hint))
        #endif
        for(uint x = 0; x < mask_dim; ++x)
        {
            result += mask[y * mask_dim + x] * in[(gid.y + y) * in_dim.x + (gid.x + x)];
        }
    }

    // Write result to correspoding output cell.
    out[gid.y * out_dim.x + gid.x] = result;
}
