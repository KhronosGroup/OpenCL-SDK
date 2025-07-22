/* Include OpenCL header, and define OpenCL extensions, since what is and is not
 * available in the official headers is very system-dependent */

#ifndef EXT_H
#define EXT_H

/* Khronos now provides unified headers for all OpenCL versions, and
 * it should be included after defining a target OpenCL version
 * (otherwise, the maximum version will simply be used, but a message
 * will be printed).
 *
 * TODO: until 3.0 gets finalized, we only target 2.2 because the 3.0
 * defines etc are still changing, so users may have an older version
 * of the 3.0 headers lying around, which may prevent clinfo from being
 * compilable.
 */
#ifndef CL_TARGET_OPENCL_VERSION
#define CL_TARGET_OPENCL_VERSION 220
#endif

/* We will use the deprecated clGetExtensionFunctionAddress,
 * so let the headers know that we don't care about it being deprecated.
 * The standard CL_USE_DEPRECATED_OPENCL_1_1_APIS define apparently
 * doesn't work for macOS, so we'll just tell the compiler to not
 * warn about deprecated functions.
 * A more correct solution would be to suppress the warning only around the
 * clGetExtensionFunctionAddress call, but honestly I just cleaned up that
 * piece of code. And I'm actually wondering if it even makes sense to
 * build that part of the code on macOS: does anybody actually use
 * ocl-icd as OpenCL dispatcher on macOS?
 */

#ifdef __APPLE__
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <OpenCL/opencl.h>
#else
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_ENABLE_BETA_EXTENSIONS
#include <CL/cl.h>
#include <CL/cl_ext.h>
#endif

/* cl_amd_object_metadata */
#define CL_PLATFORM_MAX_KEYS_AMD			0x403C

/* cl_khr_terminate_context */
#define CL_DEVICE_TERMINATE_CAPABILITY_KHR_1x		0x200F
#define CL_DEVICE_TERMINATE_CAPABILITY_KHR		0x2031

/* cl_nv_device_attribute_query */
#define CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV		0x4000
#define CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV		0x4001
#define CL_DEVICE_REGISTERS_PER_BLOCK_NV		0x4002
#define CL_DEVICE_WARP_SIZE_NV				0x4003
#define CL_DEVICE_GPU_OVERLAP_NV			0x4004
#define CL_DEVICE_KERNEL_EXEC_TIMEOUT_NV		0x4005
#define CL_DEVICE_INTEGRATED_MEMORY_NV			0x4006
#define CL_DEVICE_ATTRIBUTE_ASYNC_ENGINE_COUNT_NV	0x4007
#define CL_DEVICE_PCI_BUS_ID_NV				0x4008
#define CL_DEVICE_PCI_SLOT_ID_NV			0x4009
#define CL_DEVICE_PCI_DOMAIN_ID_NV			0x400A

/* cl_ext_atomic_counters_{32,64} */
#define CL_DEVICE_MAX_ATOMIC_COUNTERS_EXT		0x4032

/* cl_amd_device_attribute_query */
#define CL_DEVICE_PROFILING_TIMER_OFFSET_AMD		0x4036
#define CL_DEVICE_TOPOLOGY_AMD				0x4037
#define CL_DEVICE_BOARD_NAME_AMD			0x4038
#define CL_DEVICE_GLOBAL_FREE_MEMORY_AMD		0x4039
#define CL_DEVICE_SIMD_PER_COMPUTE_UNIT_AMD		0x4040
#define CL_DEVICE_SIMD_WIDTH_AMD			0x4041
#define CL_DEVICE_SIMD_INSTRUCTION_WIDTH_AMD		0x4042
#define CL_DEVICE_WAVEFRONT_WIDTH_AMD			0x4043
#define CL_DEVICE_GLOBAL_MEM_CHANNELS_AMD		0x4044
#define CL_DEVICE_GLOBAL_MEM_CHANNEL_BANKS_AMD		0x4045
#define CL_DEVICE_GLOBAL_MEM_CHANNEL_BANK_WIDTH_AMD	0x4046
#define CL_DEVICE_LOCAL_MEM_SIZE_PER_COMPUTE_UNIT_AMD	0x4047
#define CL_DEVICE_LOCAL_MEM_BANKS_AMD			0x4048
#define CL_DEVICE_THREAD_TRACE_SUPPORTED_AMD		0x4049
#define CL_DEVICE_GFXIP_MAJOR_AMD			0x404A
#define CL_DEVICE_GFXIP_MINOR_AMD			0x404B
#define CL_DEVICE_AVAILABLE_ASYNC_QUEUES_AMD		0x404C
/* These two are undocumented */
#define CL_DEVICE_MAX_REAL_TIME_COMPUTE_QUEUES_AMD	0x404D
#define CL_DEVICE_MAX_REAL_TIME_COMPUTE_UNITS_AMD	0x404E
/* These were added in v4 of the extension, but have values lower than
 * than the older ones, and spanning around the cl_ext_atomic_counters_*
 * define
 */
#define CL_DEVICE_PREFERRED_WORK_GROUP_SIZE_AMD         0x4030
#define CL_DEVICE_MAX_WORK_GROUP_SIZE_AMD               0x4031
#define CL_DEVICE_PREFERRED_CONSTANT_BUFFER_SIZE_AMD    0x4033
#define CL_DEVICE_PCIE_ID_AMD                           0x4034

#ifndef CL_DEVICE_TOPOLOGY_TYPE_PCIE_AMD
#define CL_DEVICE_TOPOLOGY_TYPE_PCIE_AMD		1

typedef union
{
	struct { cl_uint type; cl_uint data[5]; } raw;
	struct { cl_uint type; cl_char unused[17]; cl_char bus; cl_char device; cl_char function; } pcie;
} cl_device_topology_amd;
#endif

/* cl_amd_offline_devices */
#define CL_CONTEXT_OFFLINE_DEVICES_AMD			0x403F

/* cl_amd_copy_buffer_p2p */
#define CL_DEVICE_NUM_P2P_DEVICES_AMD			0x4088
#define CL_DEVICE_P2P_DEVICES_AMD			0x4089

/* cl_altera_device_temperature */
#define CL_DEVICE_CORE_TEMPERATURE_ALTERA		0x40F3

#endif
