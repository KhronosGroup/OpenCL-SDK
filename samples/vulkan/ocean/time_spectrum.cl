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

constant float PI = 3.14159265359;
constant float G = 9.81;
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

complex conj(complex c)
{
    return (complex)(c.x, -c.y);
}

kernel void spectrum( float dt, int2 patch_info,
    read_only image2d_t src, write_only image2d_t dst_x,
    write_only image2d_t dst_y, write_only image2d_t dst_z )
{
    int2 uv = (int2)((int)get_global_id(0), (int)get_global_id(1));
    int res = patch_info.y;
    float2 wave_vec = (float2)(uv.x - res / 2.f, uv.y - res / 2.f);
    float2 k = (2.f * PI * wave_vec) / patch_info.x;
    float k_mag = length(k);

    float w = sqrt(G * k_mag);

    float4 h0k = read_imagef(src, sampler, uv);
    complex fourier_amp = (complex)(h0k.x, h0k.y);
    complex fourier_amp_conj = conj((complex)(h0k.z, h0k.w));

    float cos_wt = cos(w*dt);
    float sin_wt = sin(w*dt);

    // euler formula
    complex exp_iwt = (complex)(cos_wt, sin_wt);
    complex exp_iwt_inv = (complex)(cos_wt, -sin_wt);

    // dy
    complex h_k_t_dy = add(mul(fourier_amp, exp_iwt), (mul(fourier_amp_conj, exp_iwt_inv)));

    // dx
    complex dx = (complex)(0.0,-k.x/k_mag);
    complex h_k_t_dx = mul(dx, h_k_t_dy);

    // dz
    complex dz = (complex)(0.0,-k.y/k_mag);
    complex h_k_t_dz = mul(dz, h_k_t_dy);

    // amplitude
    write_imagef(dst_y, uv, (float4)(h_k_t_dy.x, h_k_t_dy.y, 0, 1));

    // choppiness
    write_imagef(dst_x, uv, (float4)(h_k_t_dx.x, h_k_t_dx.y, 0, 1));
    write_imagef(dst_z, uv, (float4)(h_k_t_dz.x, h_k_t_dz.y, 0, 1));
}
