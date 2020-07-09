/*
// Copyright (c) 2019-2020 Ben Ashbaugh
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
*/

#include <stdio.h>
#include <vector>

#include <CL/cl.h>

static cl_int AllocateAndGetPlatformInfoString(
    cl_platform_id platformId,
    cl_platform_info param_name,
    char*& param_value )
{
    cl_int  errorCode = CL_SUCCESS;
    size_t  size = 0;

    if( errorCode == CL_SUCCESS )
    {
        if( param_value != NULL )
        {
            delete [] param_value;
            param_value = NULL;
        }
    }

    if( errorCode == CL_SUCCESS )
    {
        errorCode = clGetPlatformInfo(
            platformId,
            param_name,
            0,
            NULL,
            &size );
    }

    if( errorCode == CL_SUCCESS )
    {
        if( size != 0 )
        {
            param_value = new char[ size ];
            if( param_value == NULL )
            {
                errorCode = CL_OUT_OF_HOST_MEMORY;
            }
        }
    }

    if( errorCode == CL_SUCCESS )
    {
        errorCode = clGetPlatformInfo(
            platformId,
            param_name,
            size,
            param_value,
            NULL );
    }

    if( errorCode != CL_SUCCESS )
    {
        delete [] param_value;
        param_value = NULL;
    }

    return errorCode;
}

static cl_int PrintPlatformInfoSummary(
    cl_platform_id platformId )
{
    cl_int  errorCode = CL_SUCCESS;

    char*           platformName = NULL;
    char*           platformVendor = NULL;
    char*           platformVersion = NULL;

    errorCode |= AllocateAndGetPlatformInfoString(
        platformId,
        CL_PLATFORM_NAME,
        platformName );
    errorCode |= AllocateAndGetPlatformInfoString(
        platformId,
        CL_PLATFORM_VENDOR,
        platformVendor );
    errorCode |= AllocateAndGetPlatformInfoString(
        platformId,
        CL_PLATFORM_VERSION,
        platformVersion );

    printf("\tName:           %s\n", platformName );
    printf("\tVendor:         %s\n", platformVendor );
    printf("\tDriver Version: %s\n", platformVersion );

    delete [] platformName;
    delete [] platformVendor;
    delete [] platformVersion;

    platformName = NULL;
    platformVendor = NULL;
    platformVersion = NULL;

    return errorCode;
}

static cl_int AllocateAndGetDeviceInfoString(
    cl_device_id    device,
    cl_device_info  param_name,
    char*&          param_value )
{
    cl_int  errorCode = CL_SUCCESS;
    size_t  size = 0;

    if( errorCode == CL_SUCCESS )
    {
        if( param_value != NULL )
        {
            delete [] param_value;
            param_value = NULL;
        }
    }

    if( errorCode == CL_SUCCESS )
    {
        errorCode = clGetDeviceInfo(
            device,
            param_name,
            0,
            NULL,
            &size );
    }

    if( errorCode == CL_SUCCESS )
    {
        if( size != 0 )
        {
            param_value = new char[ size ];
            if( param_value == NULL )
            {
                errorCode = CL_OUT_OF_HOST_MEMORY;
            }
        }
    }

    if( errorCode == CL_SUCCESS )
    {
        errorCode = clGetDeviceInfo(
            device,
            param_name,
            size,
            param_value,
            NULL );
    }

    if( errorCode != CL_SUCCESS )
    {
        delete [] param_value;
        param_value = NULL;
    }

    return errorCode;
}

static cl_int PrintDeviceInfoSummary(
    cl_device_id* devices,
    size_t numDevices )
{
    cl_int  errorCode = CL_SUCCESS;

    cl_device_type  deviceType;
    char*           deviceName = NULL;
    char*           deviceVendor = NULL;
    char*           deviceVersion = NULL;
    char*           driverVersion = NULL;

    size_t  i = 0;
    for( i = 0; i < numDevices; i++ )
    {
        errorCode |= clGetDeviceInfo(
            devices[i],
            CL_DEVICE_TYPE,
            sizeof( deviceType ),
            &deviceType,
            NULL );
        errorCode |= AllocateAndGetDeviceInfoString(
            devices[i],
            CL_DEVICE_NAME,
            deviceName );
        errorCode |= AllocateAndGetDeviceInfoString(
            devices[i],
            CL_DEVICE_VENDOR,
            deviceVendor );
        errorCode |= AllocateAndGetDeviceInfoString(
            devices[i],
            CL_DEVICE_VERSION,
            deviceVersion );
        errorCode |= AllocateAndGetDeviceInfoString(
            devices[i],
            CL_DRIVER_VERSION,
            driverVersion );

        if( errorCode == CL_SUCCESS )
        {
            printf("Device[%d]:\n", (int)i );

            switch( deviceType )
            {
            case CL_DEVICE_TYPE_DEFAULT:    printf("\tType:           %s\n", "DEFAULT" );      break;
            case CL_DEVICE_TYPE_CPU:        printf("\tType:           %s\n", "CPU" );          break;
            case CL_DEVICE_TYPE_GPU:        printf("\tType:           %s\n", "GPU" );          break;
            case CL_DEVICE_TYPE_ACCELERATOR:printf("\tType:           %s\n", "ACCELERATOR" );  break;
            default:                        printf("\tType:           %s\n", "***UNKNOWN***" );break;
            }

            printf("\tName:           %s\n", deviceName );
            printf("\tVendor:         %s\n", deviceVendor );
            printf("\tDevice Version: %s\n", deviceVersion );
            printf("\tDriver Version: %s\n", driverVersion );
        }
        else
        {
            fprintf(stderr, "Error getting device info for device %d.\n", (int)i );
        }

        delete [] deviceName;
        delete [] deviceVendor;
        delete [] deviceVersion;
        delete [] driverVersion;

        deviceName = NULL;
        deviceVendor = NULL;
        deviceVersion = NULL;
        driverVersion = NULL;
    }

    return errorCode;
}

int main(
    int argc,
    char** argv )
{
    bool printUsage = false;

    int i = 0;

    if( argc < 1 )
    {
        printUsage = true;
    }
    else
    {
        for( i = 1; i < argc; i++ )
        {
            {
                printUsage = true;
            }
        }
    }
    if( printUsage )
    {
        fprintf(stderr,
            "Usage: enumopencl      [options]\n"
            "Options:\n"
            );

        return -1;
    }

    cl_uint numPlatforms = 0;
    clGetPlatformIDs( 0, NULL, &numPlatforms );
    printf( "Enumerated %u platforms.\n\n", numPlatforms );

    std::vector<cl_platform_id> platforms;
    platforms.resize( numPlatforms );
    clGetPlatformIDs( numPlatforms, platforms.data(), NULL );

    for( auto& platform : platforms )
    {
        printf( "Platform:\n" );
        PrintPlatformInfoSummary( platform );

        cl_uint numDevices = 0;
        clGetDeviceIDs( platform, CL_DEVICE_TYPE_ALL, 0, NULL, &numDevices );

        std::vector<cl_device_id> devices;
        devices.resize( numDevices );
        clGetDeviceIDs( platform, CL_DEVICE_TYPE_ALL, numDevices, devices.data(), NULL );

        PrintDeviceInfoSummary( devices.data(), numDevices );
        printf( "\n" );
    }

    printf( "Done.\n" );

    return 0;
}
