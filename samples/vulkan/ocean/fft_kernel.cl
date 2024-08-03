/*
 * Copyright (c) 2024 Mobica Limited, Marcin Hajder
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

constant sampler_t sampler = CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE;

typedef float2 complex;

complex mul(complex c0, complex c1)
{
    return (complex)(c0.x * c1.x - c0.y * c1.y, c0.x * c1.y + c0.y * c1.x);
}

complex add(complex c0, complex c1)
{
    return (complex)(c0.x + c1.x, c0.y + c1.y);
}

// mode.x - 0-horizontal, 1-vertical
// mode.y - subsequent count

__kernel void fft_1D( int2 mode, int2 patch_info,
    read_only image2d_t twiddle, read_only image2d_t src, write_only image2d_t dst )
{
    int2 uv = (int2)((int)get_global_id(0), (int)get_global_id(1));

    int2 data_coords = (int2)(mode.y, uv.x * (1-mode.x) + uv.y * mode.x);
    float4 data = read_imagef(twiddle, sampler, data_coords);

    int2 pp_coords0 = (int2)(data.z, uv.y) * (1-mode.x) + (int2)(uv.x, data.z) * mode.x;
    float2 p = read_imagef(src, sampler, pp_coords0).xy;

    int2 pp_coords1 = (int2)(data.w, uv.y) * (1-mode.x) + (int2)(uv.x, data.w) * mode.x;
    float2 q = read_imagef(src, sampler, pp_coords1).xy;

    float2 w = (float2)(data.x, data.y);

    //Butterfly operation
    complex H = add(p,mul(w,q));

    write_imagef(dst, uv, (float4)(H.x, H.y, 0, 1));
}
