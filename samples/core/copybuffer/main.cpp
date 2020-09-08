/*
 * Copyright (c) 2020 The Khronos Group Inc.
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
 *
 * OpenCL is a trademark of Apple Inc. used under license by Khronos.
 */

#include <CL/cl2.hpp>

cl::CommandQueue commandQueue;
cl::Buffer deviceMemSrc;
cl::Buffer deviceMemDst;

size_t gwx = 1024 * 1024;

static void init(void)
{
    cl_uint* pSrc = (cl_uint*)commandQueue.enqueueMapBuffer(
        deviceMemSrc, CL_TRUE, CL_MAP_WRITE_INVALIDATE_REGION, 0,
        gwx * sizeof(cl_uint));

    for (size_t i = 0; i < gwx; i++)
    {
        pSrc[i] = (cl_uint)(i);
    }

    commandQueue.enqueueUnmapMemObject(deviceMemSrc, pSrc);
}

static void go()
{
    commandQueue.enqueueCopyBuffer(deviceMemSrc, deviceMemDst, 0, 0,
                                   gwx * sizeof(cl_uint));
}

static void checkResults()
{
    const cl_uint* pDst = (const cl_uint*)commandQueue.enqueueMapBuffer(
        deviceMemDst, CL_TRUE, CL_MAP_READ, 0, gwx * sizeof(cl_uint));

    unsigned int mismatches = 0;

    for (size_t i = 0; i < gwx; i++)
    {
        if (pDst[i] != i)
        {
            if (mismatches < 16)
            {
                fprintf(stderr, "MisMatch!  dst[%d] == %08X, want %08X\n",
                        (unsigned int)i, pDst[i], (unsigned int)i);
            }
            mismatches++;
        }
    }

    if (mismatches)
    {
        fprintf(stderr, "Error: Found %d mismatches / %d values!!!\n",
                mismatches, (unsigned int)gwx);
    }
    else
    {
        printf("Success.\n");
    }

    commandQueue.enqueueUnmapMemObject(
        deviceMemDst,
        (void*)pDst); // TODO: Why isn't this a const void* in the API?
}

int main(int argc, char** argv)
{
    bool printUsage = false;
    int platformIndex = 0;
    int deviceIndex = 0;

    if (argc < 1)
    {
        printUsage = true;
    }
    else
    {
        for (size_t i = 1; i < argc; i++)
        {
            if (!strcmp(argv[i], "-d"))
            {
                ++i;
                if (i < argc)
                {
                    deviceIndex = strtol(argv[i], NULL, 10);
                }
            }
            else if (!strcmp(argv[i], "-p"))
            {
                ++i;
                if (i < argc)
                {
                    platformIndex = strtol(argv[i], NULL, 10);
                }
            }
            else
            {
                printUsage = true;
            }
        }
    }
    if (printUsage)
    {
        fprintf(stderr,
                "Usage: copybuffer      [options]\n"
                "Options:\n"
                "      -d: Device Index (default = 0)\n"
                "      -p: Platform Index (default = 0)\n");

        return -1;
    }

    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);

    printf("Running on platform: %s\n",
           platforms[platformIndex].getInfo<CL_PLATFORM_NAME>().c_str());

    std::vector<cl::Device> devices;
    platforms[platformIndex].getDevices(CL_DEVICE_TYPE_ALL, &devices);

    printf("Running on device: %s\n",
           devices[deviceIndex].getInfo<CL_DEVICE_NAME>().c_str());

    cl::Context context{ devices[deviceIndex] };
    commandQueue = cl::CommandQueue{ context, devices[deviceIndex] };

    deviceMemSrc =
        cl::Buffer{ context, CL_MEM_ALLOC_HOST_PTR, gwx * sizeof(cl_uint) };

    deviceMemDst =
        cl::Buffer{ context, CL_MEM_ALLOC_HOST_PTR, gwx * sizeof(cl_uint) };

    init();
    go();
    checkResults();

    return 0;
}