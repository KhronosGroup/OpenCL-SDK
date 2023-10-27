# External Memory Sample

## Sample purpose
External devices resources can be shared across GPU APIs. This can specially come in handy when developing graphical applications, as usually we have specialized APIs for graphics (like OpenGL or the lower-level-API Vulkan) that are used for rendering and the more general APIs (like OpenCL, SYCL, etc). This sample showcases an OpenCL program that interacts with the Vulkan API by sharing buffers. For one that actually does rendering, the [open_cl_interop](https://github.com/KhronosGroup/Vulkan-Samples/tree/main/samples/extensions/open_cl_interop) sample should be consulted.

## Key APIs and Concepts
### Kernel logic
The kernel used in this sample is a saxpy, i.e. performs the vector operation $a*x+y$ where $x$ and $y$ are the input vectors and $a$ is a scalar. This simple kernel was chosen because the main purpose of the example is to showcase the buffer sharing between the OpenCL and Vulkan APIs, rather than showing off some complex kernel implementation.

### Create Vulkan instance with the necessary extensions enabled
The Vulkan function `vkCreateInstance` creates a new Vulkan instance (object gathering the application's state), which later can be used to query the physical devices available on the system for our program. When calling to this function, a `VkInstanceCreateInfo` object must be passed in order to tell the Vulkan API some characteristics of the application. In this sample, one of the main pieces of information passed to the named function is a list of Vulkan instance extensions to be enabled:
- `VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME` for exporting non-Vulkan handles from Vulkan buffers.
- `VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME` for also being able to query the properties of physical devices (needed for obtaining the devices' UUIDs).

### Find an OpenCL device Vulkan-compatible
In the context of a given OpenCL program, for a device to be compatible with the Vulkan API there are three main requirements:
- It has to be recognized by Vulkan as a physical device, that is, Vulkan must report the existence of a physical device with the same UUID than the selected OpenCL device's. In Vulkan, with `vkGetPhysicalDeviceProperties2` we can get the properties of a physical device, among which is included the `deviceUUID` attribute storing the UUID of the corresponding device. For OpenCL, we can query the device's UUID by calling `clGetDeviceInfo` (or the C++ wrapper `cl::Device::getInfo<>()`) with the `CL_DEVICE_UUID_KHR` value as `cl_device_info` parameter.
  - Beware the query of the UUID in OpenCL/Vulkan cannot be done without the device supporting the `cl_khr_device_uuid`/`VK_KHR_get_physical_device_properties2`.
- It must support the Vulkan device extensions needed for the program at hand. In this occasion, we need the Vulkan device to support exporting non-Vulkan handles from Vulkan memory objects (e.g. buffers). The `vkEnumerateDeviceExtensionProperties` function is used for querying the Vulkan device extensions supported by a given physical device.
- It also needs to support the Khronos extension `cl_khr_external_memory_opaque_fd` for Linux systems or `cl_khr_external_memory_win32` for Windows. With the C API, The function `clGetDeviceInfo` called with the parameter `CL_DEVICE_EXTENSIONS` provides information about whether this extension is supported by the OpenCL device. The C++ API (Utils library) provides the function `cl::util::supports_extension`, with which this check can be done easier.

  _Note: The `cl_khr_external_memory` extension requires OpenCL 3.0, which we make sure to check that is indeed supported on the device before compiling the OpenCL kernel._

Once a suitable Vulkan physical device (and its correspondent OpenCL device) has been found, we can create a Vulkan device object from it with `vkCreateDevice`. We must set the `ppEnabledExtensionNames` attribute of the `VkDeviceCreateInfo` passed to the said function with the names of the required Vulkan device extensions (that we already checked the device supports) in order for them to be enabled on the device.

### Create Vulkan buffers for external sharing
When creating the Vulkan buffer objects for our application, we must make explicit that those buffers are going to be shared with an external API. The way of doing this can be summarized into the following steps:
- Before starting to allocate Vulkan memory objects, we need to ensure that the external memory handle type needed for importing Vulkan memory objects is supported by the device, both in OpenCL and in Vulkan APIs.
The mapping between Vulkan and OpenCL handle types is as follows:

  | Vulkan external memory handle type                                 | OpenCL external memory handle type                            |
  | ------------------------------------------------------------------ | ------------------------------------------------------------- |
  | `VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR`                 | `CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR`                     |
  | `VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR`              | `CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_WIN32_KHR`                  |
  | `VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHR`          | `CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_WIN32_KMT_BIT_KHR`          |

  The first row contains the handle types used for Linux, while for Windows platforms the handle types used are either the ones from the second or third row.

  To check whether the OpenCL device supports the memory handle we use `clGetDeviceInfo` with the `CL_DEVICE_EXTERNAL_MEMORY_IMPORT_HANDLE_TYPES_KHR` value as `cl_device_info` parameter in order to get a list of supported external memory handle types.

  For Vulkan, we can request a `VkExternalBufferProperties` object containing this information by calling to `vkGetPhysicalDeviceExternalBufferProperties`.

- We now create our Vulkan buffer objects. We first initialize a `VkExternalMemoryBufferCreateInfo` structure with the necessary information for the buffers bounded to the exported memory. It is **mandatory** when creating a Vulkan buffer that will be bound to exported/imported memory to pass a **non-null** value for the **`handleTypes`** field of this info structure. A pointer to this object is then added as the `pNext` field of a `VkBufferCreateInfo` structure, which contains the information for creating Vulkan buffers (that are not necessarily bounded to external memory). We finally create our buffers by calling `vkCreateBuffer`.

- The next step is to allocate device memory. This is done with the function `vkAllocateMemory`, which needs a `VkMemoryAllocateInfo` parameter. The key information to set up when allocating external memory is the `pNext` field, pointing to a `VkExportMemoryAllocateInfo` structure which `handleTypes` field specifies the handle types that may be exported.

- After allocating the device memory, it is only left to bind it to the buffer objects with `vkBindBufferMemory` and to map the latter into the application address space with `vkMapMemory`. If the buffer objects are to be mapped in their entirety, we can use `VK_WHOLE_SIZE` as the `size` parameter of `vkMapMemory`. After mapping the buffer objects we obtain host-accessible pointers to the beginning of the mapped ranges and we can just copy the contents of the host arrays to those ranges.

### Initialize OpenCL buffers from external API
The key point when initializing OpenCL buffers from external memory is that we need a file descriptor associated to this external memory in order to access it from the OpenCL API. In the Vulkan API we can get such file descriptor by making use of the function `vkGetMemoryFdKHR` provided by the `VK_KHR_external_memory_fd` extension.

Being provided by an extension, we need to obtain a function pointer to it by calling to `vkGetDeviceProcAddr`. We can then call `vkGetMemoryFdKHR` with a `VkMemoryGetFdInfoKHR` parameter containing the information about the memory range for which we want to obtain a file descriptor:
 - `memory` field containing the pointer to the said range
 - `handleType` field with the same Vulkan external memory handle type used in the `VkExportMemoryAllocateInfo` structure when memory was allocated.

Once we have the file descriptor, we can initialize an array of `cl_mem_properties` with the following entries:
- The OpenCL external memory handle type to use.
- The file descriptor previously obtained for the Vulkan memory range.
- A list of devices to which these properties apply. This list must start with an entry containing the macro `CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR`, followed by as many entries as devices in the list containing the corresponding `cl_device_id` objects. The list must end with an entry containing the macro `CL_DEVICE_HANDLE_LIST_END_KHR`.
- A $0$ indicating the end of the array.

_Note: With the C++ API we can obtain the `cl_device_id` object from a `cl::Device device` wrapper by using the `()` operator._

This array of properties is then passed to `clCreateBufferWithProperties` (or to the C++ constructor of `cl::Buffer`). When creating OpenCL buffer objects from external memory there are a couple of restrictions in the parameters allowed for `clCreateBufferWithProperties`/`cl::Buffer::Buffer()`, namely:
- The `flags` parameter used to specify usage information for the buffer must not include `CL_MEM_USE_HOST_PTR`, `CL_MEM_ALLOC_HOST_PTR`, or `CL_MEM_COPY_HOST_PTR`.
- The `host_ptr` argument must be null.

From this point on the OpenCL API functions are called as usual.

## Application flow
### Overview
1. Parse user options.
2. Initialize Vulkan instance.
3. Find an OpenCL Vulkan-compatible device.
4. Create a Vulkan device object from the physical device selected enabling the required extensions on it.
5. Check that the OpenCL device supports the necessary Khronos extensions.
6. Create Vulkan's buffer objects for sharing them with an external API.
7. Query the requirements for memory to be exportable. Allocate memory, bind buffers to memory and map the former to the Vulkan address space. Copy input from host to Vulkan memory objects.
8. Query the file descriptors correspondent to Vulkan's memory ranges mapped and initialize OpenCL buffers from them.
9. Enqueue kernel call to saxpy.
10. Fetch and validate result.
11. Free resources.

## Used API surface
### C
```c
CL_BLOCKING
CL_CONTEXT_PLATFORM
CL_DEVICE_EXTENSIONS
CL_DEVICE_EXTERNAL_MEMORY_IMPORT_HANDLE_TYPES_KHR
CL_DEVICE_HANDLE_LIST_KHR
CL_DEVICE_HANDLE_LIST_END_KHR
CL_DEVICE_NAME
CL_DEVICE_PLATFORM
CL_DEVICE_TYPE_ALL
CL_HPP_TARGET_OPENCL_VERSION
CL_INVALID_ARG_VALUE
CL_INVALID_VALUE
CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR
CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_WIN32_KMT_KHR
CL_KERNEL_WORK_GROUP_SIZE
CL_KHR_EXTERNAL_MEMORY_OPAQUE_FD_EXTENSION_NAME
CL_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME
CL_MEM_READ_ONLY
CL_MEM_READ_WRITE
CL_PLATFORM_VENDOR
CL_PROFILING_COMMAND_END
CL_PROFILING_COMMAND_START
CL_QUEUE_PROFILING_ENABLE
CL_QUEUE_PROPERTIES
CL_SUCCESS
CL_UUID_SIZE_KHR
cl_command_queue
cl_command_queue_properties
cl_context
cl_context_properties
cl_device_id
cl_event
cl_float
cl_int
cl_kernel
cl_external_memory_handle_type_khr
cl_khr_external_memory_opaque_fd
cl_khr_external_memory_win32
cl_mem
cl_mem_properties
cl_platform_id
cl_program
cl_sdk_fill_with_random_ints_range(pcg32_random_t*, cl_int*, size_t, cl_int, cl_int)
cl_sdk_options_Diagnostic
cl_sdk_options_SingleDevice
cl_uint
cl_uchar
cl_ulong
cl_util_build_program(cl_program, cl_device_id, char*)
cl_util_get_device(cl_uint, cl_uint, cl_device_type, cl_int*)
cl_util_get_event_duration(cl_event, cl_profiling_info, cl_profiling_info, cl_int*)
cl_util_print_device_info*(cl_device_id)
cl_util_print_error(cl_int)
cl_util_read_text_file(char*const, size_t*const, cl_int*)
clCreateBufferWithProperties(cl_context, cl_mem_properties*, cl_mem_flags, size_t, void*, cl_int*)
clCreateCommandQueueWithProperties(cl_context, cl_device_id, cl_queue_properties*, cl_int*) -> OpenCL >= 2.0
clCreateContext(cl_context_properties*, cl_uint, cl_device_id*, void *(char*, void*,size_t, void*), void*, cl_int*)
clCreateKernel(cl_program, char*, cl_int*)
clGetKernelWorkGroupInfo(cl_kernel, cl_device_id, cl_kernel_work_group_info, size_t, void*, size_t*)
clCreateProgramWithSource(cl_context, cl_uint, char**, size_t*, cl_int*)
clEnqueueNDRangeKernel(cl_command_queue, cl_kernel, cl_uint, size_t*, size_t*, size_t*, cl_uint, cl_event*, cl_event*)
clEnqueueReadBuffer(cl_command_queue, cl_mem, cl_bool, size_t, size_t, void*, cl_uint, cl_event*, cl_event*)
clGetDeviceIDs(cl_platform_id, cl_device_type, cl_uint, cl_device_id*, cl_uint*)
clGetDeviceInfo(cl_device_id, cl_device_info, size_t, void*, size_t*)
clGetPlatformIDs(cl_uint, cl_platform_id*, cl_uint*)
clReleaseCommandQueue(cl_command_queue)
clReleaseContext(cl_context)
clReleaseKernel(cl_kernel)
clReleaseMemObject(cl_mem)
clReleaseProgram(cl_program)
clSetKernelArg(cl_kernel, cl_uint, size_t, void *)
clWaitForEvents(cl_uint, cl_event*)
```

### C++
```c++
cl::Buffer::Buffer(const Context&, const vector<cl_mem_properties>&, IteratorType, IteratorType, bool, bool=false, cl_int*=NULL)
cl::BuildError
cl::CommandQueue::CommandQueue(const cl::Context&, const Device&,cl::QueueProperties, cl_int*=NULL)
cl::Context
cl::Device::Device()
cl::EnqueueArgs::EnqueueArgs(cl::CommandQueue&, cl::NDRange, cl::NDRange)
cl::Error
cl::Event
cl::KernelFunctor::KernelFunctor(const Program&, const string, cl_int*=NULL)
cl::NDRange::NDRange(size_t, size_t)
cl::Platform::Platform()
cl::Platform::Platform(cl::Platform)
cl::Platform::get(vector<cl::Platform>*)
cl::Program::Program(cl::Program)
cl::WaitForEvents(const vector<cl::Event>&)
cl::copy(const CommandQueue&, const cl::Buffer&, IteratorType, IteratorType)
cl::sdk::comprehend()
cl::sdk::fill_with_random()
cl::sdk::get_context(cl_uint, cl_uint, cl_device_type, cl_int*)
cl::sdk::parse()
cl::sdk::parse_cli()
cl::sdk::options::Diagnostic
cl::sdk::options::SingleDevice
cl::string::string(cl::string)
cl::util::Error
cl::util::get_duration(cl::Event&)
cl::util::supports_extension(const cl::Device&, const cl::string&)
```

### Vulkan
```c
PFN_vkCreateDevice(VkPhysicalDevice, const VkDeviceCreateInfo*, const VkAllocationCallbacks*, VkDevice*)
VK_BUFFER_USAGE_TRANSFER_DST_BIT
VK_BUFFER_USAGE_TRANSFER_SRC_BIT
VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR
VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR
VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME
VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME
VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME
VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME
VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
VK_MAKE_VERSION
VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
VK_SHARING_MODE_EXCLUSIVE
VK_STRUCTURE_TYPE_APPLICATION_INFO
VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO
VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO
VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO
VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO
VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO
VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR
VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES_KHR
VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR
VK_SUCCESS
VK_WHOLE_SIZE
VkApplicationInfo
VkBuffer
VkBufferCreateInfo
VkDevice
VkDeviceMemory
VkDeviceQueueCreateInfo
VkExportMemoryAllocateInfo
VkExtensionProperties
VkExternalMemoryBufferCreateInfo
VkExternalMemoryHandleTypeFlagBits
VkInstance
VkInstanceCreateInfo
VkMemoryAllocateInfo
VkMemoryGetFdInfoKHR
VkMemoryPropertyFlags
VkMemoryRequirements
VkPhysicalDevice
VkPhysicalDeviceIDPropertiesKHR
VkPhysicalDeviceMemoryProperties
VkPhysicalDeviceProperties2KHR
VkPhysicalDeviceProperties
VkResult
vkAllocateMemory(VkDevice, const VkMemoryAllocateInfo*, const VkAllocationCallbacks*, VkDeviceMemory*)
vkBindBufferMemory(VkDevice, VkBuffer, VkDeviceMemory, VkDeviceSize)
vkCreateBuffer(VkDevice, const VkBufferCreateInfo*, const VkAllocationCallbacks*, VkBuffer*)
vkCreateInstance(const VkInstanceCreateInfo*, const VkAllocationCallbacks*, VkInstance*)
vkDestroyBuffer(VkDevice, VkBuffer, const VkAllocationCallbacks*)
vkEnumerateDeviceExtensionProperties(VkPhysicalDevice, const char*, uint32_t*, VkExtensionProperties*)
vkEnumeratePhysicalDevices(VkInstance, uint32_t*, VkPhysicalDevice*)
vkFreeMemory(VkDevice, VkDeviceMemory, const VkAllocationCallbacks*)
vkGetBufferMemoryRequirements(VkDevice, VkBuffer, VkMemoryRequirements*)
vkGetDeviceProcAddr(VkDevice, const char*)
vkGetMemoryFdKHR(VkDevice, const VkMemoryGetFdInfoKHR*, int*)
vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice, VkPhysicalDeviceMemoryProperties*)
vkGetPhysicalDeviceProperties2(VkPhysicalDevice, VkPhysicalDeviceProperties2)
vkMapMemory(VkDevice, VkDeviceMemory, VkDeviceSize, VkDeviceSize, VkMemoryMapFlags, void**)
vkUnmapMemory(VkDevice, VkDeviceMemory)
```
