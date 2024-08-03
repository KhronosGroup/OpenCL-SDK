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

constant sampler_t sampler = CLK_ADDRESS_REPEAT | CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_TRUE;
constant float normal_scale_fac = 3.f;
// patch_info.x - ocean patch size
// patch_info.y - ocean texture unified resolution
// scale_fac.x - choppines
// scale_fac.y - altitude scale
kernel void normals( int2 patch_info, float2 scale_fac, read_only image2d_t noise,
     read_only image2d_t src, write_only image2d_t dst )
{
    int2 uv = (int2)((int)get_global_id(0), (int)get_global_id(1));
    float2 fuv = convert_float2(uv) / patch_info.y;

    float texel = 1.f / patch_info.y;

    float dz_c = read_imagef(src, sampler, fuv).y;
    float dz_cr = read_imagef(src, sampler, (float2)(fuv.x + texel, fuv.y)).y;
    float dz_ct = read_imagef(src, sampler, (float2)(fuv.x, fuv.y + texel)).y;
    float dz_cl = read_imagef(src, sampler, (float2)(fuv.x - texel, fuv.y)).y;
    float dz_cb = read_imagef(src, sampler, (float2)(fuv.x, fuv.y - texel)).y;
    float dz_tr = read_imagef(src, sampler, (float2)(fuv.x + texel, fuv.y + texel)).y;
    float dz_br = read_imagef(src, sampler, (float2)(fuv.x + texel, fuv.y - texel)).y;
    float dz_tl = read_imagef(src, sampler, (float2)(fuv.x - texel, fuv.y + texel)).y;
    float dz_bl = read_imagef(src, sampler, (float2)(fuv.x - texel, fuv.y - texel)).y;

    float3 normal = (float3)(0.f, 0.f, 1.f / normal_scale_fac);
    normal.y = dz_c + 2.f * dz_cb + dz_br - dz_tl - 2.f * dz_ct - dz_tr;
    normal.x = dz_c + 2.f * dz_cl + dz_tl - dz_br - 2.f * dz_cr - dz_tr;

    float4 n = read_imagef(noise, sampler, fuv*(float2)(4.0));

    write_imagef(dst, uv, (float4)(normalize(normal), n[(uv.x+uv.y)%4]));
}
