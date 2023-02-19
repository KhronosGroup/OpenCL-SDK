/*
 * Copyright (c) 2020-2023 The Khronos Group Inc.
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

#include <stdio.h>
#include <vector>

#include <CL/opencl.hpp>

static cl_int PrintPlatformInfoSummary(cl::Platform platform)
{
    printf("\tName:           %s\n",
           platform.getInfo<CL_PLATFORM_NAME>().c_str());
    printf("\tVendor:         %s\n",
           platform.getInfo<CL_PLATFORM_VENDOR>().c_str());
    printf("\tDriver Version: %s\n",
           platform.getInfo<CL_PLATFORM_VERSION>().c_str());

    return CL_SUCCESS;
}

static void PrintDeviceType(const char* label, cl_device_type type)
{
    printf("%s%s%s%s%s%s\n", label,
           (type & CL_DEVICE_TYPE_DEFAULT) ? "DEFAULT " : "",
           (type & CL_DEVICE_TYPE_CPU) ? "CPU " : "",
           (type & CL_DEVICE_TYPE_GPU) ? "GPU " : "",
           (type & CL_DEVICE_TYPE_ACCELERATOR) ? "ACCELERATOR " : "",
           (type & CL_DEVICE_TYPE_CUSTOM) ? "CUSTOM " : "");
}

static cl_int PrintDeviceInfoSummary(const std::vector<cl::Device> devices)
{
    for (size_t i = 0; i < devices.size(); i++)
    {
        printf("Device[%zu]:\n", i);

        cl_device_type deviceType = devices[i].getInfo<CL_DEVICE_TYPE>();
        PrintDeviceType("\tType:           ", deviceType);

        printf("\tName:           %s\n",
               devices[i].getInfo<CL_DEVICE_NAME>().c_str());
        printf("\tVendor:         %s\n",
               devices[i].getInfo<CL_DEVICE_VENDOR>().c_str());
        printf("\tDevice Version: %s\n",
               devices[i].getInfo<CL_DEVICE_VERSION>().c_str());
        printf("\tDevice Profile: %s\n",
               devices[i].getInfo<CL_DEVICE_PROFILE>().c_str());
        printf("\tDriver Version: %s\n",
               devices[i].getInfo<CL_DRIVER_VERSION>().c_str());
    }

    return CL_SUCCESS;
}

int main(int argc, char** argv)
{
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);

    for (size_t i = 0; i < platforms.size(); i++)
    {
        printf("Platform[%zu]:\n", i);
        PrintPlatformInfoSummary(platforms[i]);

        std::vector<cl::Device> devices;
        platforms[i].getDevices(CL_DEVICE_TYPE_ALL, &devices);

        PrintDeviceInfoSummary(devices);
        printf("\n");
    }

    printf("Done.\n");

    return 0;
}
