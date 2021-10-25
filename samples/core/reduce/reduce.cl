float op(float lhs, float rhs);

float read_local(local float* shared, size_t count, float zero, size_t i)
{
    return i < count ? shared[i] : zero;
}

kernel void reduce(
    global float* front,
    global float* back,
    local float* shared,
    unsigned int length,
    float zero_elem
)
{
    const size_t lid = get_local_id(0),
                 lsi = get_local_size(0),
                 wid = get_group_id(0),
                 wsi = get_num_groups(0);

    const size_t wg_stride = lsi * 2,
                 valid_count = wid != wsi - 1 ? // If not last group
                    wg_stride :                 // as much as possible
                    length - wid * wg_stride;   // only the remaining

    // Copy real data to local
    event_t read;
    async_work_group_copy(
        shared,
        front + wid * wg_stride,
        valid_count,
        read);
    wait_group_events(1, &read);
    barrier(CLK_LOCAL_MEM_FENCE);

    for (int i = lsi; i != 0; i /= 2)
    {
        if (lid < i)
            shared[lid] =
                op(
                    read_local(shared, valid_count, zero_elem, lid),
                    read_local(shared, valid_count, zero_elem, lid + i)
                );
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    if (lid == 0) back[wid] = shared[0];
}
