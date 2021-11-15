kernel void blur_box(
    read_only image2d_t input_image,
    write_only image2d_t output_image,
    int size
)
{
    const int width = get_image_width(input_image); //get_global_size(0);//
    const int height = get_image_height(input_image); //get_global_size(1);//
    // coordinates of the pixel to work on
    const int2 coord = { get_global_id(0), get_global_id(1) };
    const sampler_t s = CLK_NORMALIZED_COORDS_FALSE
        | CLK_ADDRESS_NONE // CLK_ADDRESS_CLAMP_TO_EDGE //
        | CLK_FILTER_NEAREST;

    //if (coord.x == get_global_offset(0) && coord.y == get_global_offset(1))
    //    printf("%i%s%i\n\n", width, " ", height);

    uint4 sum = 0;//100 * 112 * 112;
    uint num = 0;
    // Intel CPU runtime bug?
    // gives memory access violations for size >= 0
    // even if there is no acceses out of bounds (checked by printf)
    // but works with any size if write_imageui is commented out

    // 1st variant
    int2 shift;
    for (shift.x = -size; shift.x <= size; ++shift.x)
        for (shift.y = -size; shift.y <= size; ++shift.y) {
            int2 cur = coord + shift;
            if ((0 <= cur.x) && (cur.x < width) && (0 <= cur.y) && (cur.y < height))
            {
                ++num;
                //if ((cur.x < 0) || (cur.x > width - 1) || (cur.y < 0) || (cur.y > height - 1))
                //    printf("%v2hli%s%v2hli\n", coord, " ", cur);
                //sum += read_imageui(input_image, s, cur); // works only with size=0 and does not with size>=0
                sum += read_imageui(input_image, cur); // this works always
                //printf("%v4hlX%s%v2hli\n", sum, " ", cur);
            }
        }

    // 2nd variant
    /*const int4 lims = { max(coord.x - size, 0), min(coord.x + size, width - 1),
        max(coord.y - size, 0), min(coord.y + size, height - 1) };
    int2 cur;
    for (cur.x = lims.s0; cur.x <= lims.s1; ++cur.x)
        for (cur.y = lims.s2; cur.y <= lims.s3; ++cur.y) {
            ++num;
            //if ((cur.x < 0) || (cur.x > width - 1) || (cur.y < 0) || (cur.y > height - 1))
            //    printf("%v2hli%s%v2hli\n", coord, " ", cur);
            sum += read_imageui(input_image, s, cur); // this works with size=0,1,2, but not with size>=3
            //sum += read_imageui(input_image, cur); // this works with any size
            //printf("%v4hlX%s%v2hli\n", sum, " ", cur);
        }*/

    //if (!num) printf("%i%s%i%s%u\n", coord.x, " ", coord.y, " ", num);
    //if ((0 <= coord.x) && (coord.x < width) && (0 <= coord.y) && (coord.y < height))
    //    write_imageui(output_image, (int2)(get_global_id(0), get_global_id(1))/*coord*/, (sum + num / 2) / num);
    //write_imageui(output_image, (int2)(0, 0), (sum + num / 2) / num);
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
    const sampler_t s = CLK_NORMALIZED_COORDS_FALSE
        | CLK_ADDRESS_NONE // CLK_ADDRESS_CLAMP_TO_EDGE //
        | CLK_FILTER_NEAREST;

    uint4 sum = 0.0;
    uint num = 0;
    int2 shift = 0;
    for (shift.x = -size; shift.x <= size; ++shift.x)
    {
        int2 cur = coord + shift;
        if ((0 <= cur.x) && (cur.x < width))
        {
            ++num;
            sum += read_imageui(input_image, s, cur);
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
    const sampler_t s = CLK_NORMALIZED_COORDS_FALSE
        | CLK_ADDRESS_NONE // CLK_ADDRESS_CLAMP_TO_EDGE //
        | CLK_FILTER_NEAREST;

    uint4 sum = 0.0;
    uint num = 0;
    int2 shift = 0;
    for (shift.y = -size; shift.y <= size; ++shift.y)
    {
        int2 cur = coord + shift;
        if ((0 <= cur.y) && (cur.y < height))
        {
            ++num;
            sum += read_imageui(input_image, s, cur);
        }
    }
    write_imageui(output_image, coord, (sum + num / 2) / num);
}
