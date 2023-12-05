kernel void reaction_diffusion_step(read_only image2d_t in_data,
                                    write_only image2d_t out_data)
{
    const float DU = 1.F;
    const float DV = 0.3F;
    const float f = 0.055F;
    const float k = 0.062F;

    const sampler_t smplr = CLK_NORMALIZED_COORDS_FALSE | CLK_FILTER_NEAREST
        | CLK_ADDRESS_CLAMP_TO_EDGE;
    const size_t x = get_global_id(0);
    const size_t y = get_global_id(1);
    const float2 uv = read_imagef(in_data, smplr, (int2){ x, y }).xy;
    const float u = uv.x;
    const float v = uv.y;

    float u_diffuse = 0.F;
    float v_diffuse = 0.F;
    for (int dy = -1; dy <= 1; ++dy)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            const float2 _uv =
                read_imagef(in_data, smplr, (int2){ x + dx, y + dy }).xy;
            const float _u = _uv.x;
            const float _v = _uv.y;

            if (abs(dx) + abs(dy) == 0)
            {
                u_diffuse += _u * -1.F;
                v_diffuse += _v * -1.F;
            }
            else if (abs(dx) + abs(dy) == 1)
            {
                u_diffuse += _u * 0.2F;
                v_diffuse += _v * 0.2F;
            }
            else
            {
                u_diffuse += _u * 0.05F;
                v_diffuse += _v * 0.05F;
            }
        }
    }

    const float u_new = u + DU * u_diffuse - u * v * v + f * (1 - u);
    const float v_new = v + DV * v_diffuse + u * v * v - (k + f) * v;

    write_imagef(out_data, (int2){ x, y }, (float4){ u_new, v_new, 0, 1 });
}
