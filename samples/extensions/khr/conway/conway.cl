__kernel void conway(float a,
                    __global float* x,
                    __global float* y)
{
    int gid = get_global_id(0);

    y[gid] = a * x[gid] + y[gid];
}
