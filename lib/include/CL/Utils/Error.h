#pragma once

#include<stdio.h>

#define CL_UTIL_INDEX_OUT_OF_RANGE -2000

// RET = function returns error code
// PAR = functions sets error code in the paremeter

#ifdef _DEBUG

#define OCLERROR_RET(func, err, label) \
do { err = func; if (err != CL_SUCCESS) { cl_util_print_error(err); printf( "on line %d, in file %s\n%s\n", __LINE__, __FILE__, #func); goto label;} } while (0)

#define OCLERROR_PAR(func, err, label) \
do { func; if (err != CL_SUCCESS) { cl_util_print_error(err); printf( "on line %d, in file %s\n%s\n", __LINE__, __FILE__, #func); goto label;} } while (0)

#define MEM_CHECK(func, err, label) \
do { if ((func) == NULL) {err = CL_OUT_OF_HOST_MEMORY; cl_util_print_error(err); printf( "on line %d, in file %s\n%s\n", __LINE__, __FILE__, #func); goto label;} } while (0)

#else

#define OCLERROR_RET(func, err, label) \
do { err = func; if (err != CL_SUCCESS) goto label; } while (0)

#define OCLERROR_PAR(func, err, label) \
do { func; if (err != CL_SUCCESS) goto label; } while (0)

#define MEM_CHECK(func, err, label) \
do { if ((func) == NULL) {err = CL_OUT_OF_HOST_MEMORY; goto label;} } while (0)

#endif

void cl_util_print_error(cl_int error)
{
    switch (error) {
        case CL_SUCCESS:
            break;
        case CL_DEVICE_NOT_FOUND:
            printf("\nError: CL_DEVICE_NOT_FOUND\n");
            break;
        case CL_DEVICE_NOT_AVAILABLE:
            printf("\nError: CL_DEVICE_NOT_AVAILABLE\n");
            break;
        case CL_COMPILER_NOT_AVAILABLE:
            printf("\nError: CL_COMPILER_NOT_AVAILABLE\n");
            break;
        case CL_MEM_OBJECT_ALLOCATION_FAILURE:
            printf("\nError: CL_MEM_OBJECT_ALLOCATION_FAILURE\n");
            break;
        case CL_OUT_OF_RESOURCES:
            printf("\nError: CL_OUT_OF_RESOURCES\n");
            break;
        case CL_OUT_OF_HOST_MEMORY:
            printf("\nError: CL_OUT_OF_HOST_MEMORY\n");
            break;
        case CL_PROFILING_INFO_NOT_AVAILABLE:
            printf("\nError: CL_PROFILING_INFO_NOT_AVAILABLE\n");
            break;
        case CL_MEM_COPY_OVERLAP:
            printf("\nError: CL_MEM_COPY_OVERLAP\n");
            break;
        case CL_IMAGE_FORMAT_MISMATCH:
            printf("\nError: CL_IMAGE_FORMAT_MISMATCH\n");
            break;
        case CL_IMAGE_FORMAT_NOT_SUPPORTED:
            printf("\nError: CL_IMAGE_FORMAT_NOT_SUPPORTED\n");
            break;
        case CL_BUILD_PROGRAM_FAILURE:
            printf("\nError: CL_BUILD_PROGRAM_FAILURE\n");
            break;
        case CL_MAP_FAILURE:
            printf("\nError: CL_MAP_FAILURE\n");
            break;
#ifdef CL_VERSION_1_1
        case CL_MISALIGNED_SUB_BUFFER_OFFSET:
            printf("\nError: CL_MISALIGNED_SUB_BUFFER_OFFSET\n");
            break;
        case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST:
            printf("\nError: CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST\n");
            break;
#endif
#ifdef CL_VERSION_1_2
        case CL_COMPILE_PROGRAM_FAILURE:
            printf("\nError: CL_COMPILE_PROGRAM_FAILURE\n");
            break;
        case CL_LINKER_NOT_AVAILABLE:
            printf("\nError: CL_LINKER_NOT_AVAILABLE\n");
            break;
        case CL_LINK_PROGRAM_FAILURE:
            printf("\nError: CL_LINK_PROGRAM_FAILURE\n");
            break;
        case CL_DEVICE_PARTITION_FAILED:
            printf("\nError: CL_DEVICE_PARTITION_FAILED\n");
            break;
        case CL_KERNEL_ARG_INFO_NOT_AVAILABLE:
            printf("\nError: CL_KERNEL_ARG_INFO_NOT_AVAILABLE\n");
            break;
#endif
        case CL_INVALID_VALUE:
            printf("\nError: CL_INVALID_VALUE\n");
            break;
        case CL_INVALID_DEVICE_TYPE:
            printf("\nError: CL_INVALID_DEVICE_TYPE\n");
            break;
        case CL_INVALID_PLATFORM:
            printf("\nError: CL_INVALID_PLATFORM\n");
            break;
        case CL_INVALID_DEVICE:
            printf("\nError: CL_INVALID_DEVICE\n");
            break;
        case CL_INVALID_CONTEXT:
            printf("\nError: CL_INVALID_CONTEXT\n");
            break;
        case CL_INVALID_QUEUE_PROPERTIES:
            printf("\nError: CL_INVALID_QUEUE_PROPERTIES\n");
            break;
        case CL_INVALID_COMMAND_QUEUE:
            printf("\nError: CL_INVALID_COMMAND_QUEUE\n");
            break;
        case CL_INVALID_HOST_PTR:
            printf("\nError: CL_INVALID_HOST_PTR\n");
            break;
        case CL_INVALID_MEM_OBJECT:
            printf("\nError: CL_INVALID_MEM_OBJECT\n");
            break;
        case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
            printf("\nError: CL_INVALID_IMAGE_FORMAT_DESCRIPTOR\n");
            break;
        case CL_INVALID_IMAGE_SIZE:
            printf("\nError: CL_INVALID_IMAGE_SIZE\n");
            break;
        case CL_INVALID_SAMPLER:
            printf("\nError: CL_INVALID_SAMPLER\n");
            break;
        case CL_INVALID_BINARY:
            printf("\nError: CL_INVALID_BINARY\n");
            break;
        case CL_INVALID_BUILD_OPTIONS:
            printf("\nError: CL_INVALID_BUILD_OPTIONS\n");
            break;
        case CL_INVALID_PROGRAM:
            printf("\nError: CL_INVALID_PROGRAM\n");
            break;
        case CL_INVALID_PROGRAM_EXECUTABLE:
            printf("\nError: CL_INVALID_PROGRAM_EXECUTABLE\n");
            break;
        case CL_INVALID_KERNEL_NAME:
            printf("\nError: CL_INVALID_KERNEL_NAME\n");
            break;
        case CL_INVALID_KERNEL_DEFINITION:
            printf("\nError: CL_INVALID_KERNEL_DEFINITION\n");
            break;
        case CL_INVALID_KERNEL:
            printf("\nError: CL_INVALID_KERNEL\n");
            break;
        case CL_INVALID_ARG_INDEX:
            printf("\nError: CL_INVALID_ARG_INDEX\n");
            break;
        case CL_INVALID_ARG_VALUE:
            printf("\nError: CL_INVALID_ARG_VALUE\n");
            break;
        case CL_INVALID_ARG_SIZE:
            printf("\nError: CL_INVALID_ARG_SIZE\n");
            break;
        case CL_INVALID_KERNEL_ARGS:
            printf("\nError: CL_INVALID_KERNEL_ARGS\n");
            break;
        case CL_INVALID_WORK_DIMENSION:
            printf("\nError: CL_INVALID_WORK_DIMENSION\n");
            break;
        case CL_INVALID_WORK_GROUP_SIZE:
            printf("\nError: CL_INVALID_WORK_GROUP_SIZE\n");
            break;
        case CL_INVALID_WORK_ITEM_SIZE:
            printf("\nError: CL_INVALID_WORK_ITEM_SIZE\n");
            break;
        case CL_INVALID_GLOBAL_OFFSET:
            printf("\nError: CL_INVALID_GLOBAL_OFFSET\n");
            break;
        case CL_INVALID_EVENT_WAIT_LIST:
            printf("\nError: CL_INVALID_EVENT_WAIT_LIST\n");
            break;
        case CL_INVALID_EVENT:
            printf("\nError: CL_INVALID_EVENT\n");
            break;
        case CL_INVALID_OPERATION:
            printf("\nError: CL_INVALID_OPERATION\n");
            break;
        case CL_INVALID_GL_OBJECT:
            printf("\nError: CL_INVALID_GL_OBJECT\n");
            break;
        case CL_INVALID_BUFFER_SIZE:
            printf("\nError: CL_INVALID_BUFFER_SIZE\n");
            break;
        case CL_INVALID_MIP_LEVEL:
            printf("\nError: CL_INVALID_MIP_LEVEL\n");
            break;
        case CL_INVALID_GLOBAL_WORK_SIZE:
            printf("\nError: CL_INVALID_GLOBAL_WORK_SIZE\n");
            break;
#ifdef CL_VERSION_1_1
        case CL_INVALID_PROPERTY:
            printf("\nError: CL_INVALID_PROPERTY\n");
            break;
#endif
#ifdef CL_VERSION_1_2
        case CL_INVALID_IMAGE_DESCRIPTOR:
            printf("\nError: CL_INVALID_IMAGE_DESCRIPTOR\n");
            break;
        case CL_INVALID_COMPILER_OPTIONS:
            printf("\nError: CL_INVALID_COMPILER_OPTIONS\n");
            break;
        case CL_INVALID_LINKER_OPTIONS:
            printf("\nError: CL_INVALID_LINKER_OPTIONS\n");
            break;
        case CL_INVALID_DEVICE_PARTITION_COUNT:
            printf("\nError: CL_INVALID_DEVICE_PARTITION_COUNT\n");
            break;
#endif
#ifdef CL_VERSION_2_0
        case CL_INVALID_PIPE_SIZE:
            printf("\nError: CL_INVALID_PIPE_SIZE\n");
            break;
        case CL_INVALID_DEVICE_QUEUE:
            printf("\nError: CL_INVALID_DEVICE_QUEUE\n");
            break;
#endif
#ifdef CL_VERSION_2_2
        case CL_INVALID_SPEC_ID:
            printf("\nError: CL_INVALID_SPEC_ID\n");
            break;
        case CL_MAX_SIZE_RESTRICTION_EXCEEDED:
            printf("\nError: CL_MAX_SIZE_RESTRICTION_EXCEEDED\n");
            break;
#endif
        // SDK errors
        case CL_UTIL_INDEX_OUT_OF_RANGE:
            printf("\nError: CL_UTIL_INDEX_OUT_OF_RANGE\n");
            break;
        // end of SDK errors
        default:
            printf("\nUnknown error: %i\n", error);
            break;
    }
}