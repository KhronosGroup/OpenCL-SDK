// OpenCL includes
#include <CL/cl.h>

// C standard includes
#include <stdio.h>

#ifdef _MSC_VER
#define print(...) printf_s(__VA_ARGS__)
#else
#define print(...) printf(__VA_ARGS__)
#endif

int main()
{
    cl_int CL_err = CL_SUCCESS;
    cl_uint numPlatforms = 0;

    CL_err = clGetPlatformIDs(0, NULL, &numPlatforms);

    if (CL_err == CL_SUCCESS)
        print("%u platform(s) found\n", numPlatforms);
    else
        print("clGetPlatformIDs(%i)\n", CL_err);

    return 0;
}
