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

#include <stdio.h>
#include <vector>

#include <CL/cl.h>

static cl_int AllocateAndGetPlatformInfoString(cl_platform_id platformId,
                                               cl_platform_info param_name,
                                               char*& param_value)
{
    cl_int errorCode = CL_SUCCESS;
    size_t size = 0;

    if (errorCode == CL_SUCCESS)
    {
        if (param_value != NULL)
        {
            delete[] param_value;
            param_value = NULL;
        }
    }

    if (errorCode == CL_SUCCESS)
    {
        errorCode = clGetPlatformInfo(platformId, param_name, 0, NULL, &size);
    }

    if (errorCode == CL_SUCCESS)
    {
        if (size != 0)
        {
            param_value = new char[size];
            if (param_value == NULL)
            {
                errorCode = CL_OUT_OF_HOST_MEMORY;
            }
        }
    }

    if (errorCode == CL_SUCCESS)
    {
        errorCode =
            clGetPlatformInfo(platformId, param_name, size, param_value, NULL);
    }

    if (errorCode != CL_SUCCESS)
    {
        delete[] param_value;
        param_value = NULL;
    }

    return errorCode;
}

static cl_int PrintPlatformInfoSummary(cl_platform_id platformId)
{
    cl_int errorCode = CL_SUCCESS;

    char* platformName = NULL;
    char* platformVendor = NULL;
    char* platformVersion = NULL;

    errorCode |= AllocateAndGetPlatformInfoString(platformId, CL_PLATFORM_NAME,
                                                  platformName);
    errorCode |= AllocateAndGetPlatformInfoString(
        platformId, CL_PLATFORM_VENDOR, platformVendor);
    errorCode |= AllocateAndGetPlatformInfoString(
        platformId, CL_PLATFORM_VERSION, platformVersion);

    printf("\tName:           %s\n", platformName);
    printf("\tVendor:         %s\n", platformVendor);
    printf("\tDriver Version: %s\n", platformVersion);

    delete[] platformName;
    delete[] platformVendor;
    delete[] platformVersion;

    platformName = NULL;
    platformVendor = NULL;
    platformVersion = NULL;

    return errorCode;
}

static cl_int AllocateAndGetDeviceInfoString(cl_device_id device,
                                             cl_device_info param_name,
                                             char*& param_value)
{
    cl_int errorCode = CL_SUCCESS;
    size_t size = 0;

    if (errorCode == CL_SUCCESS)
    {
        if (param_value != NULL)
        {
            delete[] param_value;
            param_value = NULL;
        }
    }

    if (errorCode == CL_SUCCESS)
    {
        errorCode = clGetDeviceInfo(device, param_name, 0, NULL, &size);
    }

    if (errorCode == CL_SUCCESS)
    {
        if (size != 0)
        {
            param_value = new char[size];
            if (param_value == NULL)
            {
                errorCode = CL_OUT_OF_HOST_MEMORY;
            }
        }
    }

    if (errorCode == CL_SUCCESS)
    {
        errorCode =
            clGetDeviceInfo(device, param_name, size, param_value, NULL);
    }

    if (errorCode != CL_SUCCESS)
    {
        delete[] param_value;
        param_value = NULL;
    }

    return errorCode;
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

static cl_int PrintDeviceInfoSummary(cl_device_id* devices, size_t numDevices)
{
    cl_int errorCode = CL_SUCCESS;

    cl_device_type deviceType;
    char* deviceName = NULL;
    char* deviceVendor = NULL;
    char* deviceVersion = NULL;
    char* deviceProfile = NULL;
    char* driverVersion = NULL;

    size_t i = 0;
    for (i = 0; i < numDevices; i++)
    {
        errorCode |= clGetDeviceInfo(devices[i], CL_DEVICE_TYPE,
                                     sizeof(deviceType), &deviceType, NULL);
        errorCode |= AllocateAndGetDeviceInfoString(devices[i], CL_DEVICE_NAME,
                                                    deviceName);
        errorCode |= AllocateAndGetDeviceInfoString(
            devices[i], CL_DEVICE_VENDOR, deviceVendor);
        errorCode |= AllocateAndGetDeviceInfoString(
            devices[i], CL_DEVICE_VERSION, deviceVersion);
        errorCode |= AllocateAndGetDeviceInfoString(
            devices[i], CL_DEVICE_PROFILE, deviceProfile);
        errorCode |= AllocateAndGetDeviceInfoString(
            devices[i], CL_DRIVER_VERSION, driverVersion);

        if (errorCode == CL_SUCCESS)
        {
            printf("Device[%d]:\n", (int)i);

            PrintDeviceType("\tType:           ", deviceType);

            printf("\tName:           %s\n", deviceName);
            printf("\tVendor:         %s\n", deviceVendor);
            printf("\tDevice Version: %s\n", deviceVersion);
            printf("\tDevice Profile: %s\n", deviceProfile);
            printf("\tDriver Version: %s\n", driverVersion);
        }
        else
        {
            fprintf(stderr, "Error getting device info for device %d.\n",
                    (int)i);
        }

        delete[] deviceName;
        delete[] deviceVendor;
        delete[] deviceVersion;
        delete[] deviceProfile;
        delete[] driverVersion;

        deviceName = NULL;
        deviceVendor = NULL;
        deviceVersion = NULL;
        deviceProfile = NULL;
        driverVersion = NULL;
    }

    return errorCode;
}

int main(int argc, char** argv)
{
    bool printUsage = false;

    int i = 0;

    if (argc < 1)
    {
        printUsage = true;
    }
    else
    {
        for (i = 1; i < argc; i++)
        {
            {
                printUsage = true;
            }
        }
    }
    if (printUsage)
    {
        fprintf(stderr,
                "Usage: enumopencl      [options]\n"
                "Options:\n");

        return -1;
    }

    cl_uint numPlatforms = 0;
    clGetPlatformIDs(0, NULL, &numPlatforms);
    printf("Enumerated %u platforms.\n\n", numPlatforms);

    std::vector<cl_platform_id> platforms;
    platforms.resize(numPlatforms);
    clGetPlatformIDs(numPlatforms, platforms.data(), NULL);

    for (auto& platform : platforms)
    {
        printf("Platform:\n");
        PrintPlatformInfoSummary(platform);

        cl_uint numDevices = 0;
        clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, NULL, &numDevices);

        std::vector<cl_device_id> devices;
        devices.resize(numDevices);
        clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, numDevices, devices.data(),
                       NULL);

        PrintDeviceInfoSummary(devices.data(), numDevices);
        printf("\n");
    }

    printf("Done.\n");

    return 0;
}
