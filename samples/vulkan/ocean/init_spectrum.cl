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

constant float PI = 3.14159265359f;
constant sampler_t sampler = CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE;
constant float GRAVITY = 9.81f;

float4 gaussRND(float4 rnd)
{
    float u0 = 2.0*PI*rnd.x;
    float v0 = sqrt(-2.0 * log(rnd.y));
    float u1 = 2.0*PI*rnd.z;
    float v1 = sqrt(-2.0 * log(rnd.w));

    float4 ret = (float4)(v0 * cos(u0), v0 * sin(u0), v1 * cos(u1), v1 * sin(u1));
    return ret;
}

// patch_info.x - ocean patch size
// patch_info.y - ocean texture unified resolution
// params.x - wind x
// params.y - wind.y
// params.z - amplitude
// params.w - capillar supress factor

kernel void init_spectrum( int2 patch_info, float4 params, read_only image2d_t noise, write_only image2d_t dst )
{
    int2 uv = (int2)((int)get_global_id(0), (int)get_global_id(1));
    int res = patch_info.y;

    float2 fuv = (float2)(get_global_id(0), get_global_id(1)) - (float)(res)/2.f;
    float2 k = (2.f * PI * fuv) / patch_info.x;
    float k_mag = length(k);

    if (k_mag < 0.00001) k_mag = 0.00001;

    float wind_speed = length((float2)(params.x, params.y));
    float4 params_n = params;
    params_n.xy = (float2)(params.x/wind_speed, params.y/wind_speed);
    float l_phillips = (wind_speed * wind_speed) / GRAVITY;
    float4 rnd = clamp(read_imagef(noise, sampler, uv), 0.001f, 1.f);

    float magSq = k_mag * k_mag;
    float h0k = sqrt((params.z/(magSq*magSq)) * pow(dot(normalize(k), params_n.xy), 2.f) *
                exp(-(1.0/(magSq * l_phillips * l_phillips))) * exp(-magSq*pow(params.w, 2.f)))/ sqrt(2.0);
    float h0minusk = sqrt((params.z/(magSq*magSq)) * pow(dot(normalize(-k), params_n.xy), 2.f) *
                exp(-(1.0/(magSq * l_phillips * l_phillips))) * exp(-magSq*pow(params.w, 2.f)))/ sqrt(2.0);
    float4 gauss_random = gaussRND(rnd);
    write_imagef(dst, uv, (float4)(gauss_random.xy*h0k, gauss_random.zw*h0minusk));
}
