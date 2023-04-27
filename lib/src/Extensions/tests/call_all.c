/*******************************************************************************
// Copyright (c) 2021-2023 Ben Ashbaugh
//
// SPDX-License-Identifier: MIT or Apache-2.0
*/

/*
// This file is generated from the Khronos OpenCL XML API Registry.
*/

// clang-format off

#define CL_USE_DEPRECATED_OPENCL_1_0_APIS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#define CL_USE_DEPRECATED_OPENCL_2_1_APIS
#define CL_USE_DEPRECATED_OPENCL_2_2_APIS
#define CL_USE_DEPRECATED_OPENCL_3_0_APIS
#include <CL/cl.h>
#include <CL/cl_ext.h>
#if defined(CLEXT_INCLUDE_GL)
#include <CL/cl_gl.h>
#endif
#if defined(CLEXT_INCLUDE_EGL)
#include <CL/cl_egl.h>
#endif
#if defined(CLEXT_INCLUDE_DX9)
#include <CL/cl_dx9_media_sharing.h>
#endif
// Note: If both D3D10 and D3D11 are supported, the D3D11 header must be
// included first.
#if defined(CLEXT_INCLUDE_D3D11)
#include <CL/cl_d3d11.h>
#endif
#if defined(CLEXT_INCLUDE_D3D10)
#include <CL/cl_d3d10.h>
#endif
#if defined(CLEXT_INCLUDE_VA_API)
#include <CL/cl_va_api_media_sharing_intel.h>
#endif

// Some headers do not include function prototypes for the DX sharing extensions.
#include "dx_sharing_prototypes.h"

// Some headers do not include function prototypes for the loader info extension.
#include "loader_info_prototypes.h"

void call_all(void)
{
#ifdef cl_khr_command_buffer
    clCreateCommandBufferKHR(0, NULL, NULL, NULL);
    clFinalizeCommandBufferKHR(NULL);
    clRetainCommandBufferKHR(NULL);
    clReleaseCommandBufferKHR(NULL);
    clEnqueueCommandBufferKHR(0, NULL, NULL, 0, NULL, NULL);
    clCommandBarrierWithWaitListKHR(NULL, NULL, 0, NULL, NULL, NULL);
    clCommandCopyBufferKHR(NULL, NULL, NULL, NULL, 0, 0, 0, 0, NULL, NULL, NULL);
    clCommandCopyBufferRectKHR(NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL);
    clCommandCopyBufferToImageKHR(NULL, NULL, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, NULL);
    clCommandCopyImageKHR(NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, NULL, NULL, NULL);
    clCommandCopyImageToBufferKHR(NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, NULL, NULL, NULL);
    clCommandFillBufferKHR(NULL, NULL, NULL, NULL, 0, 0, 0, 0, NULL, NULL, NULL);
    clCommandFillImageKHR(NULL, NULL, NULL, NULL, NULL, NULL, 0, NULL, NULL, NULL);
    clCommandNDRangeKernelKHR(NULL, NULL, NULL, NULL, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL);
    clGetCommandBufferInfoKHR(NULL, CL_COMMAND_BUFFER_REFERENCE_COUNT_KHR, 0, NULL, NULL);
#endif // cl_khr_command_buffer

#ifdef cl_khr_command_buffer_mutable_dispatch
    clUpdateMutableCommandsKHR(NULL, NULL);
    clGetMutableCommandInfoKHR(NULL, CL_MUTABLE_COMMAND_COMMAND_BUFFER_KHR, 0, NULL, NULL);
#endif // cl_khr_command_buffer_mutable_dispatch

#ifdef cl_khr_create_command_queue
    clCreateCommandQueueWithPropertiesKHR(NULL, NULL, NULL, NULL);
#endif // cl_khr_create_command_queue

#ifdef cl_khr_d3d10_sharing
    clGetDeviceIDsFromD3D10KHR(NULL, CL_D3D10_DEVICE_KHR, NULL, CL_ALL_DEVICES_FOR_D3D10_KHR, 0, NULL, NULL);
    clCreateFromD3D10BufferKHR(NULL, CL_MEM_READ_WRITE, NULL, NULL);
    clCreateFromD3D10Texture2DKHR(NULL, CL_MEM_READ_WRITE, NULL, 0, NULL);
    clCreateFromD3D10Texture3DKHR(NULL, CL_MEM_READ_WRITE, NULL, 0, NULL);
    clEnqueueAcquireD3D10ObjectsKHR(NULL, 0, NULL, 0, NULL, NULL);
    clEnqueueReleaseD3D10ObjectsKHR(NULL, 0, NULL, 0, NULL, NULL);
#endif // cl_khr_d3d10_sharing

#ifdef cl_khr_d3d11_sharing
    clGetDeviceIDsFromD3D11KHR(NULL, CL_D3D11_DEVICE_KHR, NULL, CL_ALL_DEVICES_FOR_D3D11_KHR, 0, NULL, NULL);
    clCreateFromD3D11BufferKHR(NULL, CL_MEM_READ_WRITE, NULL, NULL);
    clCreateFromD3D11Texture2DKHR(NULL, CL_MEM_READ_WRITE, NULL, 0, NULL);
    clCreateFromD3D11Texture3DKHR(NULL, CL_MEM_READ_WRITE, NULL, 0, NULL);
    clEnqueueAcquireD3D11ObjectsKHR(NULL, 0, NULL, 0, NULL, NULL);
    clEnqueueReleaseD3D11ObjectsKHR(NULL, 0, NULL, 0, NULL, NULL);
#endif // cl_khr_d3d11_sharing

#ifdef cl_khr_dx9_media_sharing
    clGetDeviceIDsFromDX9MediaAdapterKHR(NULL, 0, NULL, NULL, CL_ALL_DEVICES_FOR_DX9_MEDIA_ADAPTER_KHR, 0, NULL, NULL);
    clCreateFromDX9MediaSurfaceKHR(NULL, CL_MEM_READ_WRITE, CL_ADAPTER_D3D9_KHR, NULL, 0, NULL);
    clEnqueueAcquireDX9MediaSurfacesKHR(NULL, 0, NULL, 0, NULL, NULL);
    clEnqueueReleaseDX9MediaSurfacesKHR(NULL, 0, NULL, 0, NULL, NULL);
#endif // cl_khr_dx9_media_sharing

#ifdef cl_khr_egl_event
    clCreateEventFromEGLSyncKHR(NULL, NULL, NULL, NULL);
#endif // cl_khr_egl_event

#ifdef cl_khr_egl_image
    clCreateFromEGLImageKHR(NULL, NULL, NULL, CL_MEM_READ_WRITE, NULL, NULL);
    clEnqueueAcquireEGLObjectsKHR(NULL, 0, NULL, 0, NULL, NULL);
    clEnqueueReleaseEGLObjectsKHR(NULL, 0, NULL, 0, NULL, NULL);
#endif // cl_khr_egl_image

#ifdef cl_khr_external_memory
    clEnqueueAcquireExternalMemObjectsKHR(NULL, 0, NULL, 0, NULL, NULL);
    clEnqueueReleaseExternalMemObjectsKHR(NULL, 0, NULL, 0, NULL, NULL);
#endif // cl_khr_external_memory

#ifdef cl_khr_external_semaphore
    clGetSemaphoreHandleForTypeKHR(NULL, NULL, 0, 0, NULL, NULL);
#endif // cl_khr_external_semaphore

#ifdef cl_khr_gl_event
    clCreateEventFromGLsyncKHR(NULL, NULL, NULL);
#endif // cl_khr_gl_event

#ifdef cl_khr_il_program
    clCreateProgramWithILKHR(NULL, NULL, 0, NULL);
#endif // cl_khr_il_program

#ifdef cl_khr_semaphore
    clCreateSemaphoreWithPropertiesKHR(NULL, NULL, NULL);
    clEnqueueWaitSemaphoresKHR(NULL, 0, NULL, NULL, 0, NULL, NULL);
    clEnqueueSignalSemaphoresKHR(NULL, 0, NULL, NULL, 0, NULL, NULL);
    clGetSemaphoreInfoKHR(NULL, CL_SEMAPHORE_CONTEXT_KHR, 0, NULL, NULL);
    clReleaseSemaphoreKHR(NULL);
    clRetainSemaphoreKHR(NULL);
#endif // cl_khr_semaphore

#ifdef cl_khr_subgroups
    clGetKernelSubGroupInfoKHR(NULL, NULL, CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE_KHR, 0, NULL, 0, NULL, NULL);
#endif // cl_khr_subgroups

#ifdef cl_khr_suggested_local_work_size
    clGetKernelSuggestedLocalWorkSizeKHR(NULL, NULL, 0, NULL, NULL, NULL);
#endif // cl_khr_suggested_local_work_size

#ifdef cl_khr_terminate_context
    clTerminateContextKHR(NULL);
#endif // cl_khr_terminate_context

#ifdef cl_ext_device_fission
    clReleaseDeviceEXT(NULL);
    clRetainDeviceEXT(NULL);
    clCreateSubDevicesEXT(NULL, NULL, 0, NULL, NULL);
#endif // cl_ext_device_fission

#ifdef cl_ext_image_requirements_info
    clGetImageRequirementsInfoEXT(NULL, NULL, CL_MEM_READ_WRITE, NULL, NULL, CL_IMAGE_REQUIREMENTS_SIZE_EXT, 0, NULL, NULL);
#endif // cl_ext_image_requirements_info

#ifdef cl_ext_migrate_memobject
    clEnqueueMigrateMemObjectEXT(NULL, 0, NULL, CL_MIGRATE_MEM_OBJECT_HOST_EXT, 0, NULL, NULL);
#endif // cl_ext_migrate_memobject

#ifdef cl_arm_import_memory
    clImportMemoryARM(NULL, CL_MEM_READ_WRITE, NULL, NULL, 0, NULL);
#endif // cl_arm_import_memory

#ifdef cl_arm_shared_virtual_memory
    clSVMAllocARM(NULL, CL_MEM_SVM_FINE_GRAIN_BUFFER_ARM, 0, 0);
    clSVMFreeARM(NULL, NULL);
    clEnqueueSVMFreeARM(NULL, 0, NULL, NULL, NULL, 0, NULL, NULL);
    clEnqueueSVMMemcpyARM(NULL, CL_FALSE, NULL, NULL, 0, 0, NULL, NULL);
    clEnqueueSVMMemFillARM(NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    clEnqueueSVMMapARM(NULL, CL_FALSE, CL_MAP_READ, NULL, 0, 0, NULL, NULL);
    clEnqueueSVMUnmapARM(NULL, NULL, 0, NULL, NULL);
    clSetKernelArgSVMPointerARM(NULL, 0, NULL);
    clSetKernelExecInfoARM(NULL, CL_KERNEL_EXEC_INFO_SVM_PTRS_ARM, 0, NULL);
#endif // cl_arm_shared_virtual_memory

#ifdef cl_img_generate_mipmap
    clEnqueueGenerateMipmapIMG(NULL, NULL, NULL, CL_MIPMAP_FILTER_ANY_IMG, NULL, NULL, 0, NULL, NULL);
#endif // cl_img_generate_mipmap

#ifdef cl_img_use_gralloc_ptr
    clEnqueueAcquireGrallocObjectsIMG(NULL, 0, NULL, 0, NULL, NULL);
    clEnqueueReleaseGrallocObjectsIMG(NULL, 0, NULL, 0, NULL, NULL);
#endif // cl_img_use_gralloc_ptr

#ifdef cl_intel_accelerator
    clCreateAcceleratorINTEL(NULL, 0, 0, NULL, NULL);
    clGetAcceleratorInfoINTEL(NULL, CL_ACCELERATOR_DESCRIPTOR_INTEL, 0, NULL, NULL);
    clRetainAcceleratorINTEL(NULL);
    clReleaseAcceleratorINTEL(NULL);
#endif // cl_intel_accelerator

#ifdef cl_intel_create_buffer_with_properties
    clCreateBufferWithPropertiesINTEL(NULL, NULL, CL_MEM_READ_WRITE, 0, NULL, NULL);
#endif // cl_intel_create_buffer_with_properties

#ifdef cl_intel_dx9_media_sharing
    clGetDeviceIDsFromDX9INTEL(NULL, CL_D3D9_DEVICE_INTEL, NULL, CL_ALL_DEVICES_FOR_DX9_INTEL, 0, NULL, NULL);
    clCreateFromDX9MediaSurfaceINTEL(NULL, CL_MEM_READ_WRITE, NULL, NULL, 0, NULL);
    clEnqueueAcquireDX9ObjectsINTEL(NULL, 0, NULL, 0, NULL, NULL);
    clEnqueueReleaseDX9ObjectsINTEL(NULL, 0, NULL, 0, NULL, NULL);
#endif // cl_intel_dx9_media_sharing

#ifdef cl_intel_program_scope_host_pipe
    clEnqueueReadHostPipeINTEL(NULL, NULL, NULL, CL_FALSE, NULL, 0, 0, NULL, NULL);
    clEnqueueWriteHostPipeINTEL(NULL, NULL, NULL, CL_FALSE, NULL, 0, 0, NULL, NULL);
#endif // cl_intel_program_scope_host_pipe

#ifdef cl_intel_sharing_format_query_d3d10
    clGetSupportedD3D10TextureFormatsINTEL(NULL, CL_MEM_READ_WRITE, CL_MEM_OBJECT_IMAGE2D, 0, NULL, NULL);
#endif // cl_intel_sharing_format_query_d3d10

#ifdef cl_intel_sharing_format_query_d3d11
    clGetSupportedD3D11TextureFormatsINTEL(NULL, CL_MEM_READ_WRITE, CL_MEM_OBJECT_IMAGE2D, 0, 0, NULL, NULL);
#endif // cl_intel_sharing_format_query_d3d11

#ifdef cl_intel_sharing_format_query_dx9
    clGetSupportedDX9MediaSurfaceFormatsINTEL(NULL, CL_MEM_READ_WRITE, CL_MEM_OBJECT_IMAGE2D, 0, 0, NULL, NULL);
#endif // cl_intel_sharing_format_query_dx9

#ifdef cl_intel_sharing_format_query_gl
    clGetSupportedGLTextureFormatsINTEL(NULL, CL_MEM_READ_WRITE, CL_MEM_OBJECT_IMAGE2D, 0, NULL, NULL);
#endif // cl_intel_sharing_format_query_gl

#ifdef cl_intel_sharing_format_query_va_api
    clGetSupportedVA_APIMediaSurfaceFormatsINTEL(NULL, CL_MEM_READ_WRITE, CL_MEM_OBJECT_IMAGE2D, 0, 0, NULL, NULL);
#endif // cl_intel_sharing_format_query_va_api

#ifdef cl_intel_unified_shared_memory
    clHostMemAllocINTEL(NULL, NULL, 0, 0, NULL);
    clDeviceMemAllocINTEL(NULL, NULL, NULL, 0, 0, NULL);
    clSharedMemAllocINTEL(NULL, NULL, NULL, 0, 0, NULL);
    clMemFreeINTEL(NULL, NULL);
    clMemBlockingFreeINTEL(NULL, NULL);
    clGetMemAllocInfoINTEL(NULL, NULL, CL_MEM_ALLOC_TYPE_INTEL, 0, NULL, NULL);
    clSetKernelArgMemPointerINTEL(NULL, 0, NULL);
    clEnqueueMemFillINTEL(NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    clEnqueueMemcpyINTEL(NULL, CL_FALSE, NULL, NULL, 0, 0, NULL, NULL);
    clEnqueueMemAdviseINTEL(NULL, NULL, 0, 0, 0, NULL, NULL);
#endif // cl_intel_unified_shared_memory

#ifdef cl_intel_unified_shared_memory
#if defined(CL_VERSION_1_2)
    clEnqueueMigrateMemINTEL(NULL, NULL, 0, CL_MIGRATE_MEM_OBJECT_HOST, 0, NULL, NULL);
#endif // defined(CL_VERSION_1_2)
#endif // cl_intel_unified_shared_memory

#ifdef cl_intel_unified_shared_memory
    clEnqueueMemsetINTEL(NULL, NULL, 0, 0, 0, NULL, NULL);
#endif // cl_intel_unified_shared_memory

#ifdef cl_intel_va_api_media_sharing
    clGetDeviceIDsFromVA_APIMediaAdapterINTEL(NULL, CL_VA_API_DISPLAY_INTEL, NULL, CL_ALL_DEVICES_FOR_VA_API_INTEL, 0, NULL, NULL);
    clCreateFromVA_APIMediaSurfaceINTEL(NULL, CL_MEM_READ_WRITE, NULL, 0, NULL);
    clEnqueueAcquireVA_APIMediaSurfacesINTEL(NULL, 0, NULL, 0, NULL, NULL);
    clEnqueueReleaseVA_APIMediaSurfacesINTEL(NULL, 0, NULL, 0, NULL, NULL);
#endif // cl_intel_va_api_media_sharing

#ifdef cl_loader_info
    clGetICDLoaderInfoOCLICD(CL_ICDL_OCL_VERSION, 0, NULL, NULL);
#endif // cl_loader_info

#ifdef cl_pocl_content_size
    clSetContentSizeBufferPoCL(NULL, NULL);
#endif // cl_pocl_content_size

#ifdef cl_qcom_ext_host_ptr
    clGetDeviceImageInfoQCOM(NULL, 0, 0, NULL, CL_IMAGE_ROW_ALIGNMENT_QCOM, 0, NULL, NULL);
#endif // cl_qcom_ext_host_ptr

}
