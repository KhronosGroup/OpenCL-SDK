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
 */

#include <CL/opencl.hpp>

struct Sample
{
    cl::CommandQueue commandQueue;
    cl::Kernel kernel;
    cl::Buffer deviceMemSrc;
    cl::Buffer deviceMemDst;
};

constexpr size_t bufferSize = 1024 * 1024;

static const char kernelString[] = R"CLC(
kernel void CopyBuffer( global uint* dst, global uint* src )
{
    uint id = get_global_id(0);
    dst[id] = src[id];
}
)CLC";

static void init(Sample& sample)
{
    cl_uint* pSrc = (cl_uint*)sample.commandQueue.enqueueMapBuffer(
        sample.deviceMemSrc, CL_TRUE, CL_MAP_WRITE_INVALIDATE_REGION, 0,
        bufferSize * sizeof(cl_uint));

    for (size_t i = 0; i < bufferSize; i++)
    {
        pSrc[i] = (cl_uint)(i);
    }

    sample.commandQueue.enqueueUnmapMemObject(sample.deviceMemSrc, pSrc);
}

static void go(Sample& sample)
{
    sample.kernel.setArg(0, sample.deviceMemDst);
    sample.kernel.setArg(1, sample.deviceMemSrc);

    sample.commandQueue.enqueueNDRangeKernel(sample.kernel, cl::NullRange,
                                             cl::NDRange{ bufferSize });
}

static void checkResults(Sample& sample)
{
    const cl_uint* pDst = (const cl_uint*)sample.commandQueue.enqueueMapBuffer(
        sample.deviceMemDst, CL_TRUE, CL_MAP_READ, 0,
        bufferSize * sizeof(cl_uint));

    unsigned int mismatches = 0;

    for (size_t i = 0; i < bufferSize; i++)
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
                mismatches, (unsigned int)bufferSize);
    }
    else
    {
        printf("Success.\n");
    }

    sample.commandQueue.enqueueUnmapMemObject(
        sample.deviceMemDst,
        (void*)pDst);
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
                "Usage: copybufferkernel    [options]\n"
                "Options:\n"
                "      -d: Device Index (default = 0)\n"
                "      -p: Platform Index (default = 0)\n");

        return -1;
    }

    try
    {
        Sample sample;

        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);

        printf("Running on platform: %s\n",
               platforms[platformIndex].getInfo<CL_PLATFORM_NAME>().c_str());

        std::vector<cl::Device> devices;
        platforms[platformIndex].getDevices(CL_DEVICE_TYPE_ALL, &devices);

        printf("Running on device: %s\n",
               devices[deviceIndex].getInfo<CL_DEVICE_NAME>().c_str());

        cl::Context context{ devices[deviceIndex] };
        sample.commandQueue = cl::CommandQueue{ context, devices[deviceIndex] };

        cl::Program program{ context, kernelString };
        program.build();

        sample.kernel = cl::Kernel{ program, "CopyBuffer" };

        sample.deviceMemSrc = cl::Buffer{ context, CL_MEM_ALLOC_HOST_PTR,
                                          bufferSize * sizeof(cl_uint) };

        sample.deviceMemDst = cl::Buffer{ context, CL_MEM_ALLOC_HOST_PTR,
                                          bufferSize * sizeof(cl_uint) };

        init(sample);
        go(sample);
        checkResults(sample);
    } catch (cl::BuildError& e)
    {
        for (const auto& log : e.getBuildLog())
        {
            printf("Build log for device %s:\n",
                   log.first.getInfo<CL_DEVICE_NAME>().c_str());
            printf("%s\n", log.second.c_str());
        }
    } catch (cl::Error& e)
    {
        printf("OpenCL Error: %s returned %d\n", e.what(), e.err());
    }

    return 0;
}