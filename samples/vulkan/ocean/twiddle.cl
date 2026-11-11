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

typedef float2 complex;

kernel void generate( int resolution, global int * bit_reversed, write_only image2d_t dst )
{
    int2 uv = (int2)((int)get_global_id(0), (int)get_global_id(1));
    float k = fmod(uv.y * ((float)(resolution) / pow(2.f, (float)(uv.x+1))), resolution);
    complex twiddle = (complex)( cos(2.0*PI*k/(float)(resolution)), sin(2.0*PI*k/(float)(resolution)));

    int butterflyspan = (int)(pow(2.f, (float)(uv.x)));
    int butterflywing;

    if (fmod(uv.y, pow(2.f, (float)(uv.x + 1))) < pow(2.f, (float)(uv.x)))
        butterflywing = 1;
    else
        butterflywing = 0;

    // first stage, bit reversed indices
    if (uv.x == 0) {
        // top butterfly wing
        if (butterflywing == 1)
            write_imagef(dst, uv, (float4)(twiddle.x, twiddle.y, bit_reversed[(int)(uv.y)], bit_reversed[(int)(uv.y + 1)]));
        // bot butterfly wing
        else
            write_imagef(dst, uv, (float4)(twiddle.x, twiddle.y, bit_reversed[(int)(uv.y - 1)], bit_reversed[(int)(uv.y)]));
    }
    // second to log2(resolution) stage
    else {
        // top butterfly wing
        if (butterflywing == 1)
            write_imagef(dst, uv, (float4)(twiddle.x, twiddle.y, uv.y, uv.y + butterflyspan));
        // bot butterfly wing
        else
            write_imagef(dst, uv, (float4)(twiddle.x, twiddle.y, uv.y - butterflyspan, uv.y));
    }
}
