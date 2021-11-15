kernel void blur_box(
    read_only image2d_t input_image,
    write_only image2d_t output_image,
    int size
)
{
    const int width = get_global_size(0);
    const int height = get_global_size(1);
    const int2 coord = { get_global_id(0), get_global_id(1) };
    const sampler_t s = CLK_NORMALIZED_COORDS_FALSE
        | CLK_ADDRESS_NONE // CLK_ADDRESS_CLAMP_TO_EDGE //
        | CLK_FILTER_NEAREST;

    float4 sum = 0.0;
    uint num = 0;
    int2 shift;
    for (shift.x = -size; shift.x <= size; ++shift.x)
        for (shift.y = -size; shift.y <= size; ++shift.y) {
            int2 cur = coord + shift;
            if ((0 <= cur.x) && (cur.x < width)
                && (0 <= cur.y) && (cur.y < height))
            {
                ++num;
                sum += as_float4(read_imageui(input_image, s, cur));
                //printf("%v4hlX%s%v2hli\n", sum, " ", cur);
            }
        }
    write_imageui(output_image, coord, as_uint4(sum / num));
}


kernel void blur_box_horizontal(
    read_only image2d_t input_image,
    write_only image2d_t output_image,
    int size
)
{
    const int width = get_global_size(0);
    const int height = get_global_size(1);
    const int2 coord = { get_global_id(0), get_global_id(1) };
    const sampler_t s = CLK_NORMALIZED_COORDS_FALSE
        | CLK_ADDRESS_NONE // CLK_ADDRESS_CLAMP_TO_EDGE //
        | CLK_FILTER_NEAREST;

    float4 sum = 0.0;
    uint num = 0;
    int2 shift = 0;
    for (shift.x = -size; shift.x <= size; ++shift.x)
    {
        int2 cur = coord + shift;
        if ((0 <= cur.x) && (cur.x < width))
        {
            ++num;
            sum += as_float4(read_imageui(input_image, s, cur));
        }
    }
    write_imageui(output_image, coord, as_uint4(sum / num));
}

kernel void blur_box_vertical(
    read_only image2d_t input_image,
    write_only image2d_t output_image,
    int size
)
{
    const int width = get_global_size(0);
    const int height = get_global_size(1);
    const int2 coord = { get_global_id(0), get_global_id(1) };
    const sampler_t s = CLK_NORMALIZED_COORDS_FALSE
        | CLK_ADDRESS_NONE // CLK_ADDRESS_CLAMP_TO_EDGE //
        | CLK_FILTER_NEAREST;

    float4 sum = 0.0;
    uint num = 0;
    int2 shift = 0;
    for (shift.y = -size; shift.y <= size; ++shift.y)
    {
        int2 cur = coord + shift;
        if ((0 <= cur.y) && (cur.y < height))
        {
            ++num;
            sum += as_float4(read_imageui(input_image, s, cur));
        }
    }
    write_imageui(output_image, coord, as_uint4(sum / num));
}
