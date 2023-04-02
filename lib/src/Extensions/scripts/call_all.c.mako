<%
skipExtensions = {
    # cl_khr_gl_sharing is a special case because it is implemented in the ICD
    # loader and is called into via the ICD dispatch table.
    'cl_khr_gl_sharing',
    # cl_khr_icd is used by the ICD loader only.
    'cl_khr_icd',
    # cl_loader_layers is used by the ICD loader only.
    'cl_loader_layers',
    # cl_APPLE_ContextLoggingFunctions is not passed a dispatchable object so
    # we cannot generate functions for it.
    'cl_APPLE_ContextLoggingFunctions',
    # cl_APPLE_SetMemObjectDestructor could work but there is a discrepancy
    # in the headers for the pfn_notify function.
    'cl_APPLE_SetMemObjectDestructor',
    }

defaultValueForType = {
    # Handle Types
    'cl_accelerator_intel'              : 'NULL',
    'cl_command_buffer_khr'             : 'NULL',
    'cl_command_queue'                  : 'NULL',
    'cl_context'                        : 'NULL',
    'cl_device_id'                      : 'NULL',
    'cl_event'                          : 'NULL',
    'cl_GLsync'                         : 'NULL',
    'cl_kernel'                         : 'NULL',
    'cl_mem'                            : 'NULL',
    'cl_mutable_command_khr'            : 'NULL',
    'cl_platform_id'                    : 'NULL',
    'cl_program'                        : 'NULL',
    'cl_sampler'                        : 'NULL',
    'cl_semaphore_khr'                  : 'NULL',
    'CLeglDisplayKHR'                   : 'NULL',
    'CLeglImageKHR'                     : 'NULL',
    'CLeglSyncKHR'                      : 'NULL',
    'ID3D10Texture2D'                   : 'NULL',
    'ID3D10Texture3D'                   : 'NULL',
    'ID3D11Texture2D'                   : 'NULL',
    'ID3D11Texture3D'                   : 'NULL',
    'HANDLE'                            : 'NULL',
    # Enum Types
    'cl_accelerator_info_intel'         : 'CL_ACCELERATOR_DESCRIPTOR_INTEL',
    'cl_accelerator_type_intel'         : '0',
    'cl_bool'                           : 'CL_FALSE',
    'cl_command_buffer_info_khr'        : 'CL_COMMAND_BUFFER_REFERENCE_COUNT_KHR',
    'cl_d3d10_device_source_khr'        : 'CL_D3D10_DEVICE_KHR',
    'cl_d3d10_device_set_khr'           : 'CL_ALL_DEVICES_FOR_D3D10_KHR',
    'cl_d3d11_device_source_khr'        : 'CL_D3D11_DEVICE_KHR',
    'cl_d3d11_device_set_khr'           : 'CL_ALL_DEVICES_FOR_D3D11_KHR',
    'cl_dx9_device_source_intel'        : 'CL_D3D9_DEVICE_INTEL',
    'cl_dx9_device_set_intel'           : 'CL_ALL_DEVICES_FOR_DX9_INTEL',
    'cl_dx9_media_adapter_set_khr'      : 'CL_ALL_DEVICES_FOR_DX9_MEDIA_ADAPTER_KHR',
    'cl_dx9_media_adapter_type_khr'     : 'CL_ADAPTER_D3D9_KHR',
    'cl_external_semaphore_handle_type_khr' : '0',
    'cl_icdl_info'                      : 'CL_ICDL_OCL_VERSION',
    'cl_image_pitch_info_qcom'          : 'CL_IMAGE_ROW_ALIGNMENT_QCOM',
    'cl_image_requirements_info_ext'    : 'CL_IMAGE_REQUIREMENTS_SIZE_EXT',
    'cl_kernel_exec_info_arm'           : 'CL_KERNEL_EXEC_INFO_SVM_PTRS_ARM',
    'cl_kernel_sub_group_info'          : 'CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE_KHR',
    'cl_mem_advice_intel'               : '0',
    'cl_mutable_command_info_khr'       : 'CL_MUTABLE_COMMAND_COMMAND_BUFFER_KHR',
    'cl_map_flags'                      : 'CL_MAP_READ',
    'cl_mem_flags'                      : 'CL_MEM_READ_WRITE',
    'cl_mem_info_intel'                 : 'CL_MEM_ALLOC_TYPE_INTEL',
    'cl_mem_migration_flags'            : 'CL_MIGRATE_MEM_OBJECT_HOST',
    'cl_mem_migration_flags_ext'        : 'CL_MIGRATE_MEM_OBJECT_HOST_EXT',
    'cl_mem_object_type'                : 'CL_MEM_OBJECT_IMAGE2D',
    'cl_mipmap_filter_mode_img'         : 'CL_MIPMAP_FILTER_ANY_IMG',
    'cl_semaphore_info_khr'             : 'CL_SEMAPHORE_CONTEXT_KHR',
    'cl_svm_mem_flags_arm'              : 'CL_MEM_SVM_FINE_GRAIN_BUFFER_ARM',
    'cl_va_api_device_source_intel'     : 'CL_VA_API_DISPLAY_INTEL',
    'cl_va_api_device_set_intel'        : 'CL_ALL_DEVICES_FOR_VA_API_INTEL',
    # Integral Types
    'cl_int'                            : '0',
    'cl_uint'                           : '0',
    'size_t'                            : '0',
    'UINT'                              : '0',
    }

# Extensions to include in this file:
def shouldGenerate(extension):
    if extension in genExtensions:
        return True
    elif not genExtensions and not extension in skipExtensions:
        return True
    return False

# XML blocks with funcitons to include:
def shouldEmit(block):
    for func in block.findall('command'):
        return True
    return False

# Order the extensions should be emitted in the headers.
# KHR -> EXT -> Vendor Extensions
def getExtensionSortKey(item):
    name = item.get('name')
    if name.startswith('cl_khr'):
        return 0, name
    if name.startswith('cl_ext'):
        return 1, name
    return 99, name

# Gets C function parameter strings for the specified API params:
def getCallArgs(params):
    callstr = ""
    for i, param in enumerate(params):
        if i != 0:
            callstr += ', '
        if param.Type.endswith('*'):
            callstr += 'NULL'
        elif param.Type in defaultValueForType:
            callstr += defaultValueForType[param.Type]
        else:
            print('Found unknown type: ' + param.Type);
            callstr += '???'
    return callstr

%>/*******************************************************************************
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
%for extension in sorted(spec.findall('extensions/extension'), key=getExtensionSortKey):
%  if shouldGenerate(extension.get('name')):
%for block in extension.findall('require'):
%  if shouldEmit(block):
#ifdef ${extension.get('name')}
%    if block.get('condition'):
#if ${block.get('condition')}
%    endif
%    for func in block.findall('command'):
<%
    api = apisigs[func.get('name')]
%>    ${api.Name}(${getCallArgs(api.Params)});
%    endfor
%    if block.get('condition'):
#endif // ${block.get('condition')}
%    endif
#endif // ${extension.get('name')}

%  endif
%endfor
%  endif
%endfor
}
