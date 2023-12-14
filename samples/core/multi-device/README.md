# Multi-device Convolution Example

## Sample purpose
This example showcases how to set up a multi-device execution of a given kernel using two OpenCL-compatible devices.

## Key APIs and Concepts
The main idea behind this example is that a given kernel can be run simultaneously by two (or potentially more) devices, therefore reducing its execution time. One can essentially think of two strategies for this workflow:
1. each device computes its proportional part of the solution at its own speed and the results are combined on the host's side when finished, and
2. each device executes the kernel at its own speed but after each iteration there is P2P communication between the devices to share the partial results.

This example implements the first approach.

### Kernel logic
The kernel is a simple $3 \times 3$ convolution, meaning that the convolution over the input matrix is performed using a $3 \times 3$ mask matrix.

In this implementation of the convolution kernel we assume that the input matrix is padded with 0s, so no extra conditional logic is necessary to ensure that the mask is not applied to out-of-bounds elements (e.g. when processing element $(0,0)$ of the output matrix).

### Device fission
In order to simplify the conditions under which the example can be executed, we introduced the use of OpenCL's device fission. This feature allows the user to partition a device into *sub-devices*. These sub-devices correspond physically to a certain region of the original device, but are virtually perceived as whole new devices. This partition of the device can be made in several ways.
- Partition equally by the number compute units (threads). After specifying the number of compute units that each sub-device should have, OpenCL creates as many sub-devices as possible under that restriction. If the number of compute units specified does not divide the total amount of compute units available, the leftovers do not get assigned to any sub-device. This option may be used when we want to enable task parallelism in our program, as tasks can be evenly distributed among the sub-devices.

- Partition by counts (of compute units). With this option we can specify the exact number of compute units that we want for each sub-device. This approach may be used when we want to isolate some part of the device for high priority tasks while preventing the lower priority ones from interrupting/interfering with them.

- Partition by affinity domain. The device is split into sub-devices containing compute units that share part of a cache hierarchy. For instance, when executing high-throughput jobs with little shared memory in a NUMA multiprocessor it could be beneficial for maximizing the throughput to partition the device so compute units from the same NUMA node are grouped together. That way each job can run on a sub-device (NUMA node) and get all of its resources without competing with the other jobs. On the other hand, if the program requires a great amount of shared memory, creating sub-devices that group compute units sharing the same cache can be the best option.

This sample tries to exploit task parallelism, so the first approach is the one used: from one device we create two sub-devices, each with half of the available compute units.

_Note: A device can be fissioned in more than one level, meaning that a sub-device of a device can also be partitioned into multiple (sub-)sub-devices._

#### Sub-buffers
Global buffer objects in OpenCL are one-dimensional collections of elements. From these objects can be obtained new buffer objects, known as sub-buffers. The main use-cases of these sub-buffers are the following:
- When we need accessing a buffer with different access flags than were specified in buffer creation. E.g. if we create a global buffer with `READ_WRITE` permissions, we then can create two sub-buffers from it, one with `READ_ONLY` permissions and other with `WRITE_ONLY` permissions. Therefore, being able to pass the same information with different permissions to different kernels, which can come in handy when one of the kernels that access the buffer does not perform writes or reads on the buffer as some internal coherence routines can be omitted when launching the kernels.
- When it's necessary to pass subsets of the same buffer to different kernels calls. E.g. in this sample we need to enqueue one kernel call to one of the sub-devices that convolutes the left half of the matrix and another one to the other sub-device which convolutes the right part of the matrix.

_Note: Unlike sub-devices, a sub-buffer of a global buffer cannot be partitioned again into (sub-)sub-buffers._

## Application flow
### Overview
1. Select a device. By default the application will select the first device available, but we provide a command-line option to let user specify which type of device prefers to use (e.g. "cpu" or "gpu").
2. Query compute units available on the device and create two sub-devices from it with half of the compute units each.
3. Compile kernel.
4. Initialize host-side input and output matrices. Pad input matrix with 0s so the convolution kernel does not access to out-of-bounds elements.
5. Initialize device-side global buffers.
6. Set up OpenCL objects for the sub-devices. In particular, create sub-buffers for input and output matrices.
7. Enqueue kernel calls on each device with the correspondent arguments and wait until they finish.
8. Run the host-side convolution algorithm.
9. Fetch and combine results from devices. Compare the solution obtained with the host's and print to the standard output the result of this validation.
10. Free memory and OpenCL resources.

### Device fission
Before creating sub-devices from a given device we must think about which partitioning approach is the most appropriate for the kernel/s at hand. In our case, we would like to exploit task parallelism, as the objective is to perform the convolution using 2 devices at the same time to speed it up. Therefore, the best approach is to create two sub-devices with equal number of compute units.

As we don't need to perform any other task, we can use all the compute units, so we query how many compute units are available in total using the `clGetDeviceInfo`/`cl::Device::getInfo` function with the `CL_DEVICE_MAX_COMPUTE_UNITS` parameter.

With this information we can then create an array of `cl_device_partition_property` containing the properties of the partition of the device. In our case, we must specify that we want to partition the device equally by adding the macro `CL_DEVICE_PARTITION_EQUALLY` and we must indicate how many compute units each device will get, which is half of the maximum available.

Lastly, we use the `clCreateSubDevices`/`cl::Device::createSubDevices` function to fission the device.

### Sub-buffers creation
For creating a sub-buffer from a global buffer object we first need to determine two important parameters:
- Which permissions will it have. It cannot have more permissions than the original buffer, e.g. if the global buffer was declared as read-only, the subsequent sub-buffers created from it cannot be write-only or read-write.

- What range from the global buffer will be mapped onto the sub-buffer. We need to consider which kernel is going to take the sub-buffer as input and/or output and determine which range from the global buffer must be mapped.

In our case we use two read-only input buffers, one for the input matrix and one for the mask, and one write-only output buffer. However, we only need sub-buffers for the input and output matrix, as the mask is the same for both kernel calls. Thus, we only create two sub-buffers from the input global buffer and two more from the output one.
The flags that we set when creating them are `CL_MEM_READ_ONLY` for the input sub-buffers and `CL_MEM_WRITE_ONLY` for the output ones.

For the ranges mapped into the sub-buffers, we take half of the input matrix[^1] for each sub-buffer and half of the output buffer for each too.

[^1]:_The input buffers are actually overlapped, as we need one extra column after/before the middle column when enqueuing the first/second call to the kernel for performing the convolution correctly._

### Kernel launch
The rest of the program does not differ much from the usual single-device kernel launch. The only difference is that each sub-device will need a separate set of runtime objects to be created: device objects, kernel functors, command queues and events.

Once everything is set up, a kernel call is enqueued to the command queue of each device with the correspondent input and output parameters, and two different events are used to wait for them to be finished. When the devices finish the computations, the results are combined in a single host matrix and compared to the host-side results.

## Used API surface
### C
```c
CL_BLOCKING
CL_DEVICE_MAX_COMPUTE_UNITS
CL_DEVICE_PARTITION_EQUALLY
CL_DEVICE_PLATFORM
CL_DEVICE_TYPE_ALL
CL_HPP_TARGET_OPENCL_VERSION
CL_INVALID_ARG_VALUE
CL_KERNEL_WORK_GROUP_SIZE
CL_MEM_COPY_HOST_PTR
CL_MEM_HOST_READ_ONLY
CL_MEM_READ_ONLY
CL_MEM_WRITE_ONLY
CL_PROFILING_COMMAND_END
CL_PROFILING_COMMAND_START
CL_QUEUE_PROFILING_ENABLE
CL_QUEUE_PROPERTIES
CL_SUCCESS
cl_buffer_create_type
cl_command_queue
cl_command_queue_properties
cl_context
cl_device_partition_property
cl_device_type
cl_event
cl_float
cl_int
cl_kernel
cl_mem
cl_mem_flags
cl_platform_id
cl_program
cl_sdk_fill_with_random_ints_range(pcg32_random_t*, cl_int*, size_t, cl_int, cl_int)
cl_sdk_options_DeviceTriplet
cl_sdk_options_Diagnostic
cl_sdk_options_SingleDevice
cl_uint
cl_uint2
cl_ulong
cl_util_build_program(cl_program, cl_device_id, char*)
cl_util_get_device(cl_uint, cl_uint, cl_device_type, cl_int*)
cl_util_get_event_duration(cl_event, cl_profiling_info, cl_profiling_info, cl_int*)
cl_util_print_device_info*(cl_device_id)
cl_util_print_error(cl_int)
cl_util_read_text_file(char*const, size_t*const, cl_int*)
get_dev_type(char*)
clCreateBuffer(cl_context, cl_mem_flags, size_t, void*, cl_int*)
clCreateSubBuffer(cl_mem, cl_mem_flags, cl_buffer_create_type, const void*, cl_int*)
clCreateCommandQueue(cl_context, cl_device_id, cl_command_queue_properties, cl_int*)
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
cl::Buffer::Buffer(const Context&, cl_mem_flags, size_type, void*, cl_int*=NULL)
cl::Buffer::createSubBuffer(cl_mem_flags, cl_buffer_create_type, const void*, cl_int*=NULL)
cl::BuildError
cl::CommandQueue::CommandQueue(const cl::Context&, cl::QueueProperties, cl_int*=NULL)
cl::CommandQueue::enqueueReadBuffer(const Buffer&, cl_bool, size_type, size_type, void*, const std::vector<cl::Event>*=nullptr, cl::Event*=nullptr)
cl::Context
cl::Device::Device()
cl::Device::createSubDevices(const cl_device_partition_property*, std::vector<cl::Device>*)
cl::EnqueueArgs::EnqueueArgs(cl::CommandQueue&, cl::NDRange, cl::NDRange)
cl::Error
cl::Event
cl::Kernel
cl::KernelFunctor::KernelFunctor(const Program&, const string, cl_int*=NULL)
cl::NDRange::NDRange(size_t, size_t)
cl::NullRange
cl::Platform::Platform()
cl::Platform::Platform(cl::Platform)
cl::Platform::get(vector<cl::Platform>*)
cl::Program::Program()
cl::Program::Program(cl::Program)
cl::WaitForEvents(const vector<cl::Event>&)
cl::copy(const CommandQueue&, const cl::Buffer&, IteratorType, IteratorType)
cl::sdk::comprehend()
cl::sdk::fill_with_random()
cl::sdk::get_context(cl_uint, cl_uint, cl_device_type, cl_int*)
cl::sdk::options::SingleDevice
cl::sdk::parse()
cl::sdk::parse_cli()
cl::sdk::options::DeviceTriplet
cl::sdk::options::Diagnostic
cl::sdk::options::SingleDevice
cl::string::string(cl::string)
cl::util::Error
cl::util::get_duration(cl::Event&)
cl::util::opencl_c_version_contains(const cl::Device&, const cl::string&)
```
