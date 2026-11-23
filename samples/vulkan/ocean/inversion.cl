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

kernel void inversion( int2 patch_info, read_only image2d_t src0,
    read_only image2d_t src1, read_only image2d_t src2, write_only image2d_t dst )
{
    int2 uv = (int2)((int)get_global_id(0), (int)get_global_id(1));
    int res2 = patch_info.y * patch_info.y;

    float x = read_imagef(src0, sampler, uv).x;
    float y = read_imagef(src1, sampler, uv).x;
    float z = read_imagef(src2, sampler, uv).x;

    write_imagef(dst, uv, (float4)(x/res2, y/res2, z/res2, 1));
}
