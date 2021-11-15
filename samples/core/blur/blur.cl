kernel void blur_box(
    read_only image2d_t input_image,
    write_only image2d_t output_image,
    int size
)
{
    const int width = get_image_width(input_image);
    const int height = get_image_height(input_image);
    // coordinates of the pixel to work on
    const int2 coord = { get_global_id(0), get_global_id(1) };

    uint4 sum = 0;
    uint num = 0;
    int2 shift;
    for (shift.x = -size; shift.x <= size; ++shift.x)
        for (shift.y = -size; shift.y <= size; ++shift.y) {
            int2 cur = coord + shift;
            if ((0 <= cur.x) && (cur.x < width) && (0 <= cur.y) && (cur.y < height))
            {
                ++num;
                sum += read_imageui(input_image, cur);
            }
        }
    write_imageui(output_image, coord, (sum + num / 2) / num);
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

    uint4 sum = 0;
    uint num = 0;
    int2 shift = 0;
    for (shift.x = -size; shift.x <= size; ++shift.x)
    {
        int2 cur = coord + shift;
        if ((0 <= cur.x) && (cur.x < width))
        {
            ++num;
            sum += read_imageui(input_image, cur);
        }
    }
    write_imageui(output_image, coord, (sum + num / 2) / num);
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

    uint4 sum = 0;
    uint num = 0;
    int2 shift = 0;
    for (shift.y = -size; shift.y <= size; ++shift.y)
    {
        int2 cur = coord + shift;
        if ((0 <= cur.y) && (cur.y < height))
        {
            ++num;
            sum += read_imageui(input_image, cur);
        }
    }
    write_imageui(output_image, coord, (sum + num / 2) / num);
}
