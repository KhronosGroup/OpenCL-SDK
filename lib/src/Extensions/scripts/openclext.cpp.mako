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

GL_Extensions = {
    'cl_khr_gl_depth_images',
    'cl_khr_gl_event',
    'cl_khr_gl_msaa_sharing',
    'cl_khr_gl_sharing',
    'cl_intel_sharing_format_query_gl',
    }

EGL_Extensions = {
    'cl_khr_egl_event',
    'cl_khr_egl_image',
    }

DX9_Extensions = {
    'cl_khr_dx9_media_sharing',
    'cl_intel_dx9_media_sharing',
    'cl_intel_sharing_format_query_dx9',
    }

D3D10_Extensions = {
    'cl_khr_d3d10_sharing',
    'cl_intel_sharing_format_query_d3d10',
    }

D3D11_Extensions = {
    'cl_khr_d3d11_sharing',
    'cl_intel_sharing_format_query_d3d11',
    }

VA_API_Extensions = {
    'cl_intel_va_api_media_sharing',
    'cl_intel_sharing_format_query_va_api',
    }

commonExtensions = {
    'cl_loader_info',
    }

# Extensions to include in this file:
def shouldGenerate(name):
    if name in genExtensions:
        return True
    elif not genExtensions and not name in skipExtensions:
        return True
    return False

# Common Extensions (not per-platform):
def isCommonExtension(name):
    return name in commonExtensions

# ifdef condition for an extension:
def getIfdefCondition(name):
    if name in GL_Extensions:
        return 'CLEXT_INCLUDE_GL'
    elif name in EGL_Extensions:
        return 'CLEXT_INCLUDE_EGL'
    elif name in DX9_Extensions:
        return 'CLEXT_INCLUDE_DX9'
    elif name in D3D10_Extensions:
        return 'CLEXT_INCLUDE_D3D10'
    elif name in D3D11_Extensions:
        return 'CLEXT_INCLUDE_D3D11'
    elif name in VA_API_Extensions:
        return 'CLEXT_INCLUDE_VA_API'
    return None

# XML blocks for extensions with functions to include:
def shouldEmit(block):
    for func in block.findall('command'):
        return True
    return False

# Extensions with functions to include:
def hasFunctions(extension):
    for block in extension.findall('require'):
        if shouldEmit(block):
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
def getCParameterStrings(params):
    strings = []
    if len(params) == 0:
        strings.append("void")
    else:
        for param in params:
            paramstr = param.Type + ' ' + param.Name + param.TypeEnd
            strings.append(paramstr)
    return strings

%>/*******************************************************************************
// Copyright (c) 2021-2023 Ben Ashbaugh
//
// SPDX-License-Identifier: MIT or Apache-2.0
*/

/*
// This file is generated from the Khronos OpenCL XML API Registry.
*/

// clang-format off


#if defined _WIN32 || defined __CYGWIN__
    #ifdef __GNUC__
        #define CL_API_ENTRY __attribute__((dllexport))
    #else
        #define CL_API_ENTRY __declspec(dllexport)
    #endif
#else
    #if __GNUC__ >= 4
        #define CL_API_ENTRY __attribute__((visibility("default")))
    #else
        #define CL_API_ENTRY
    #endif
#endif

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
// Some versions of the headers to not define cl_khr_gl_event.
#ifndef cl_khr_gl_event
#define cl_khr_gl_event 1
#endif
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

#include <stdlib.h>

#include <vector>

static inline cl_platform_id _get_platform(cl_platform_id platform)
{
    return platform;
}

static inline cl_platform_id _get_platform(cl_device_id device)
{
    if (device == nullptr) return nullptr;

    cl_platform_id platform = nullptr;
    clGetDeviceInfo(
        device,
        CL_DEVICE_PLATFORM,
        sizeof(platform),
        &platform,
        nullptr);
    return platform;
}

static inline cl_platform_id _get_platform(cl_command_queue command_queue)
{
    if (command_queue == nullptr) return nullptr;

    cl_device_id device = nullptr;
    clGetCommandQueueInfo(
        command_queue,
        CL_QUEUE_DEVICE,
        sizeof(device),
        &device,
        nullptr);
    return _get_platform(device);
}

static inline cl_platform_id _get_platform(cl_context context)
{
    if (context == nullptr) return nullptr;

    cl_uint numDevices = 0;
    clGetContextInfo(
        context,
        CL_CONTEXT_NUM_DEVICES,
        sizeof(numDevices),
        &numDevices,
        nullptr );

    if (numDevices == 1) {  // fast path, no dynamic allocation
        cl_device_id    device = nullptr;
        clGetContextInfo(
            context,
            CL_CONTEXT_DEVICES,
            sizeof(cl_device_id),
            &device,
            nullptr );
        return _get_platform(device);
    }

    // slower path, dynamic allocation
    std::vector<cl_device_id> devices(numDevices);
    clGetContextInfo(
        context,
        CL_CONTEXT_DEVICES,
        numDevices * sizeof(cl_device_id),
        devices.data(),
        nullptr );
    return _get_platform(devices[0]);
}

static inline cl_platform_id _get_platform(cl_kernel kernel)
{
    if (kernel == nullptr) return nullptr;

    cl_context context = nullptr;
    clGetKernelInfo(
        kernel,
        CL_KERNEL_CONTEXT,
        sizeof(context),
        &context,
        nullptr);
    return _get_platform(context);
}

static inline cl_platform_id _get_platform(cl_mem memobj)
{
    if (memobj == nullptr) return nullptr;

    cl_context context = nullptr;
    clGetMemObjectInfo(
        memobj,
        CL_MEM_CONTEXT,
        sizeof(context),
        &context,
        nullptr);
    return _get_platform(context);
}

/***************************************************************
* Function Pointer Typedefs
***************************************************************/

%for extension in sorted(spec.findall('extensions/extension'), key=getExtensionSortKey):
%  if shouldGenerate(extension.get('name')) and hasFunctions(extension):
%    if getIfdefCondition(extension.get('name')):
#if defined(${getIfdefCondition(extension.get('name'))})
%    endif
#if defined(${extension.get('name')})
%for block in extension.findall('require'):
%  if shouldEmit(block):
%    if block.get('condition'):

#if ${block.get('condition')}
%    endif
%    for func in block.findall('command'):
<%
    api = apisigs[func.get('name')]
%>
typedef ${api.RetType} (CL_API_CALL* ${api.Name}_clextfn)(
%      for i, paramStr in enumerate(getCParameterStrings(api.Params)):
%        if i < len(api.Params)-1:
    ${paramStr},
%        else:
    ${paramStr});
%        endif
%      endfor
%    endfor
%    if block.get('condition'):

#endif // ${block.get('condition')}
%    endif
%  endif
%endfor

#else
#pragma message("Define for ${extension.get('name')} was not found!  Please update your headers.")
#endif // defined(${extension.get('name')})
%    if getIfdefCondition(extension.get('name')):
#endif // defined(${getIfdefCondition(extension.get('name'))})
%    endif

%  endif
%endfor

/***************************************************************
* Extension Function Pointer Dispatch Table
***************************************************************/

struct openclext_dispatch_table {
    cl_platform_id platform;

%for extension in sorted(spec.findall('extensions/extension'), key=getExtensionSortKey):
%  if shouldGenerate(extension.get('name')) and hasFunctions(extension) and not isCommonExtension(extension.get('name')):
%    if getIfdefCondition(extension.get('name')):
#if defined(${getIfdefCondition(extension.get('name'))})
%    endif
#if defined(${extension.get('name')})
%for block in extension.findall('require'):
%  if shouldEmit(block):
%    if block.get('condition'):
#if ${block.get('condition')}
%    endif
%    for func in block.findall('command'):
<%
    api = apisigs[func.get('name')]
%>    ${api.Name}_clextfn ${api.Name};
%    endfor
%    if block.get('condition'):
#endif // ${block.get('condition')}
%    endif
%  endif
%endfor
#endif // defined(${extension.get('name')})
%    if getIfdefCondition(extension.get('name')):
#endif // defined(${getIfdefCondition(extension.get('name'))})
%    endif

%  endif
%endfor
};

struct openclext_dispatch_table_common {
%for extension in sorted(spec.findall('extensions/extension'), key=getExtensionSortKey):
%  if shouldGenerate(extension.get('name')) and hasFunctions(extension) and isCommonExtension(extension.get('name')):
%    if getIfdefCondition(extension.get('name')):
#if defined(${getIfdefCondition(extension.get('name'))})
%    endif
#if defined(${extension.get('name')})
%for block in extension.findall('require'):
%  if shouldEmit(block):
%    if block.get('condition'):
#if ${block.get('condition')}
%    endif
%    for func in block.findall('command'):
<%
    api = apisigs[func.get('name')]
%>    ${api.Name}_clextfn ${api.Name};
%    endfor
%    if block.get('condition'):
#endif // ${block.get('condition')}
%    endif
%  endif
%endfor
#endif // defined(${extension.get('name')})
%    if getIfdefCondition(extension.get('name')):
#endif // defined(${getIfdefCondition(extension.get('name'))})
%    endif

%  endif
%endfor
};

/***************************************************************
* Dispatch Table Initialization
***************************************************************/

static void _init(cl_platform_id platform, openclext_dispatch_table* dispatch_ptr)
{
    dispatch_ptr->platform = platform;

#define CLEXT_GET_EXTENSION(_funcname)                                         ${"\\"}
    dispatch_ptr->_funcname =                                                  ${"\\"}
        (_funcname##_clextfn)clGetExtensionFunctionAddressForPlatform(         ${"\\"}
            platform, #_funcname);

%for extension in sorted(spec.findall('extensions/extension'), key=getExtensionSortKey):
%  if shouldGenerate(extension.get('name')) and hasFunctions(extension) and not isCommonExtension(extension.get('name')):
%    if getIfdefCondition(extension.get('name')):
#if defined(${getIfdefCondition(extension.get('name'))})
%    endif
#if defined(${extension.get('name')})
%for block in extension.findall('require'):
%  if shouldEmit(block):
%    if block.get('condition'):
#if ${block.get('condition')}
%    endif
%    for func in block.findall('command'):
<%
    api = apisigs[func.get('name')]
%>    CLEXT_GET_EXTENSION(${api.Name});
%    endfor
%    if block.get('condition'):
#endif // ${block.get('condition')}
%    endif
%  endif
%endfor
#endif // defined(${extension.get('name')})
%    if getIfdefCondition(extension.get('name')):
#endif // defined(${getIfdefCondition(extension.get('name'))})
%    endif

%  endif
%endfor
#undef CLEXT_GET_EXTENSION
}

static void _init_common(openclext_dispatch_table_common* dispatch_ptr)
{
#define CLEXT_GET_EXTENSION(_funcname)                                         ${"\\"}
    dispatch_ptr->_funcname =                                                  ${"\\"}
        (_funcname##_clextfn)clGetExtensionFunctionAddress(#_funcname);

%for extension in sorted(spec.findall('extensions/extension'), key=getExtensionSortKey):
%  if shouldGenerate(extension.get('name')) and hasFunctions(extension) and isCommonExtension(extension.get('name')):
%    if getIfdefCondition(extension.get('name')):
#if defined(${getIfdefCondition(extension.get('name'))})
%    endif
#if defined(${extension.get('name')})
%for block in extension.findall('require'):
%  if shouldEmit(block):
%    if block.get('condition'):
#if ${block.get('condition')}
%    endif
%    for func in block.findall('command'):
<%
    api = apisigs[func.get('name')]
%>    CLEXT_GET_EXTENSION(${api.Name});
%    endfor
%    if block.get('condition'):
#endif // ${block.get('condition')}
%    endif
%  endif
%endfor
#endif // defined(${extension.get('name')})
%    if getIfdefCondition(extension.get('name')):
#endif // defined(${getIfdefCondition(extension.get('name'))})
%    endif

%  endif
%endfor
#undef CLEXT_GET_EXTENSION
}

#if defined(CLEXT_SINGLE_PLATFORM_ONLY)

static openclext_dispatch_table _dispatch = {};
static openclext_dispatch_table* _dispatch_ptr = nullptr;

template<typename T>
static inline openclext_dispatch_table* _get_dispatch(T object)
{
    if (object == nullptr) return nullptr;

    if (_dispatch_ptr == nullptr) {
        cl_platform_id platform = _get_platform(object);
        _init(platform, &_dispatch);
        _dispatch_ptr = &_dispatch;
    }

    return _dispatch_ptr;
}

// For some extension objects we cannot reliably query a platform ID without
// infinitely recursing.  For these objects we cannot initialize the dispatch
// table if it is not already initialized.

#if defined(cl_khr_semaphore)
template<>
inline openclext_dispatch_table* _get_dispatch<cl_semaphore_khr>(cl_semaphore_khr)
{
    return _dispatch_ptr;
}
#endif // defined(cl_khr_semaphore)

#if defined(cl_khr_command_buffer)
template<>
inline openclext_dispatch_table* _get_dispatch<cl_command_buffer_khr>(cl_command_buffer_khr)
{
    return _dispatch_ptr;
}
#endif // defined(cl_khr_command_buffer)

#if defined(cl_khr_command_buffer_mutable_dispatch)
template<>
inline openclext_dispatch_table* _get_dispatch<cl_mutable_command_khr>(cl_mutable_command_khr)
{
    return _dispatch_ptr;
}
#endif // defined(cl_khr_command_buffer)

#if defined(cl_intel_accelerator)
template<>
inline openclext_dispatch_table* _get_dispatch<cl_accelerator_intel>(cl_accelerator_intel)
{
    return _dispatch_ptr;
}
#endif // defined(cl_intel_accelerator)

#else // defined(CLEXT_SINGLE_PLATFORM_ONLY)

static size_t _num_platforms = 0;
static openclext_dispatch_table* _dispatch_array = nullptr;

template<typename T>
static inline openclext_dispatch_table* _get_dispatch(T object)
{
    if (_num_platforms == 0 && _dispatch_array == nullptr) {
        cl_uint numPlatforms = 0;
        clGetPlatformIDs(0, nullptr, &numPlatforms);
        if (numPlatforms == 0) {
            return nullptr;
        }

        openclext_dispatch_table* dispatch =
            (openclext_dispatch_table*)malloc(
                numPlatforms * sizeof(openclext_dispatch_table));
        if (dispatch == nullptr) {
            return nullptr;
        }

        std::vector<cl_platform_id> platforms(numPlatforms);
        clGetPlatformIDs(numPlatforms, platforms.data(), nullptr);

        for (size_t i = 0; i < numPlatforms; i++) {
            _init(platforms[i], dispatch + i);
        }

        _num_platforms = numPlatforms;
        _dispatch_array = dispatch;
    }

    cl_platform_id platform = _get_platform(object);
    for (size_t i = 0; i < _num_platforms; i++) {
        openclext_dispatch_table* dispatch_ptr =
            _dispatch_array + i;
        if (dispatch_ptr->platform == platform) {
            return dispatch_ptr;
        }
    }

    return nullptr;
}

// For some extension objects we cannot reliably query a platform ID without
// infinitely recursing.  For these objects we cannot initialize the dispatch
// table if it is not already initialized, and we need to use other methods
// to find the right dispatch table.

#if defined(cl_khr_semaphore)
template<>
inline openclext_dispatch_table* _get_dispatch<cl_semaphore_khr>(cl_semaphore_khr semaphore)
{
    if (semaphore == nullptr) return nullptr;
    if (_num_platforms <= 1) return _dispatch_array;

    for (size_t i = 0; i < _num_platforms; i++) {
        openclext_dispatch_table* dispatch_ptr =
            _dispatch_array + i;
        if (dispatch_ptr->clGetSemaphoreInfoKHR) {
            cl_uint refCount = 0;
            cl_int errorCode = dispatch_ptr->clGetSemaphoreInfoKHR(
                semaphore,
                CL_SEMAPHORE_REFERENCE_COUNT_KHR,
                sizeof(refCount),
                &refCount,
                nullptr);
            if (errorCode == CL_SUCCESS) {
                return dispatch_ptr;
            }
        }
    }

    return nullptr;
}
#endif // defined(cl_khr_semaphore)

#if defined(cl_khr_command_buffer)
template<>
inline openclext_dispatch_table* _get_dispatch<cl_command_buffer_khr>(cl_command_buffer_khr cmdbuf)
{
    if (cmdbuf == nullptr) return nullptr;
    if (_num_platforms <= 1) return _dispatch_array;

    for (size_t i = 0; i < _num_platforms; i++) {
        openclext_dispatch_table* dispatch_ptr =
            _dispatch_array + i;
        if (dispatch_ptr->clGetCommandBufferInfoKHR) {
            cl_uint refCount = 0;
            cl_int errorCode = dispatch_ptr->clGetCommandBufferInfoKHR(
                cmdbuf,
                CL_COMMAND_BUFFER_REFERENCE_COUNT_KHR,
                sizeof(refCount),
                &refCount,
                nullptr);
            if (errorCode == CL_SUCCESS) {
                return dispatch_ptr;
            }
        }
    }

    return nullptr;
}
#endif // defined(cl_khr_command_buffer)

#if defined(cl_khr_command_buffer_mutable_dispatch)
template<>
inline openclext_dispatch_table* _get_dispatch<cl_mutable_command_khr>(cl_mutable_command_khr command)
{
    if (command == nullptr) return nullptr;
    if (_num_platforms <= 1) return _dispatch_array;

    for (size_t i = 0; i < _num_platforms; i++) {
        openclext_dispatch_table* dispatch_ptr =
            _dispatch_array + i;
        if (dispatch_ptr->clGetMutableCommandInfoKHR) {
            // Alternatively, this could query the command queue from the
            // command, then get the dispatch table from the command queue.
            cl_command_buffer_khr cmdbuf = nullptr;
            cl_int errorCode = dispatch_ptr->clGetMutableCommandInfoKHR(
                command,
                CL_MUTABLE_COMMAND_COMMAND_BUFFER_KHR,
                sizeof(cmdbuf),
                &cmdbuf,
                nullptr);
            if (errorCode == CL_SUCCESS) {
                return dispatch_ptr;
            }
        }
    }

    return nullptr;
}
#endif // defined(cl_khr_command_buffer_mutable_dispatch)

#if defined(cl_intel_accelerator)
template<>
inline openclext_dispatch_table* _get_dispatch<cl_accelerator_intel>(cl_accelerator_intel accelerator)
{
    if (accelerator == nullptr) return nullptr;
    if (_num_platforms <= 1) return _dispatch_array;

    for (size_t i = 0; i < _num_platforms; i++) {
        openclext_dispatch_table* dispatch_ptr =
            _dispatch_array + i;
        if (dispatch_ptr->clGetAcceleratorInfoINTEL) {
            cl_uint refCount = 0;
            cl_int errorCode = dispatch_ptr->clGetAcceleratorInfoINTEL(
                accelerator,
                CL_ACCELERATOR_REFERENCE_COUNT_INTEL,
                sizeof(refCount),
                &refCount,
                nullptr);
            if (errorCode == CL_SUCCESS) {
                return dispatch_ptr;
            }
        }
    }

    return nullptr;
}
#endif // defined(cl_intel_accelerator)

#endif // defined(CLEXT_SINGLE_PLATFORM_ONLY)

static openclext_dispatch_table_common _dispatch_common = {};
static openclext_dispatch_table_common* _dispatch_ptr_common = nullptr;

static inline openclext_dispatch_table_common* _get_dispatch(void)
{
    if (_dispatch_ptr_common == nullptr) {
        _init_common(&_dispatch_common);
        _dispatch_ptr_common = &_dispatch_common;
    }

    return _dispatch_ptr_common;
}

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
* Extension Functions
***************************************************************/

%for extension in sorted(spec.findall('extensions/extension'), key=getExtensionSortKey):
%  if shouldGenerate(extension.get('name')) and hasFunctions(extension):
%    if getIfdefCondition(extension.get('name')):
#if defined(${getIfdefCondition(extension.get('name'))})
%    endif
#if defined(${extension.get('name')})
%for block in extension.findall('require'):
%  if shouldEmit(block):
%    if block.get('condition'):

#if ${block.get('condition')}
%    endif
%    for func in block.findall('command'):
<%
    api = apisigs[func.get('name')]
%>
${api.RetType} CL_API_CALL ${api.Name}(
%      for i, paramStr in enumerate(getCParameterStrings(api.Params)):
%        if i < len(api.Params)-1:
    ${paramStr},
%        else:
    ${paramStr})
%        endif
%      endfor
{
%      if isCommonExtension(extension.get('name')) and hasFunctions(extension):
    struct openclext_dispatch_table_common* dispatch_ptr = _get_dispatch();
%      elif api.Name == "clCreateCommandBufferKHR":
    struct openclext_dispatch_table* dispatch_ptr = _get_dispatch(${api.Params[0].Name} > 0 && ${api.Params[1].Name} ? ${api.Params[1].Name}[0] : nullptr);
%      elif api.Name == "clEnqueueCommandBufferKHR":
    struct openclext_dispatch_table* dispatch_ptr = _get_dispatch(${api.Params[2].Name});
%      else:
    struct openclext_dispatch_table* dispatch_ptr = _get_dispatch(${api.Params[0].Name});
%      endif
    if (dispatch_ptr == nullptr || dispatch_ptr->${api.Name} == nullptr) {
%      if api.RetType == "cl_int":
        return CL_INVALID_OPERATION;
%      elif api.Params[len(api.Params)-1].Name == "errcode_ret":
        if (errcode_ret) *errcode_ret = CL_INVALID_OPERATION;
        return nullptr;
%      elif api.RetType == "void*":
        return nullptr;
%      elif api.RetType == "void":
        return;
%      else:
        // not sure how to return an error in this case!
%      endif
    }
%      if api.RetType == "void":
    dispatch_ptr->${api.Name}(
%      else:
    return dispatch_ptr->${api.Name}(
%      endif
%      for i, arg in enumerate(api.Params):
%        if i < len(api.Params)-1:
        ${arg.Name},
%        else:
        ${arg.Name});
%        endif
%      endfor
}
%    endfor
%    if block.get('condition'):

#endif // ${block.get('condition')}
%    endif
%  endif
%endfor

#endif // defined(${extension.get('name')})
%    if getIfdefCondition(extension.get('name')):
#endif // defined(${getIfdefCondition(extension.get('name'))})

%    endif

%  endif
%endfor
#ifdef __cplusplus
}
#endif
