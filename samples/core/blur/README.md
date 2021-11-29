# Blur

## Sample Purpose

This sample intends to demonstrate how to use different techniques of data exchange between workitems in workgroup, query various extensions applicable and use compile options to touch up kernel sources at runtime to produce the best kernel implementation for the task.

## Key APIs and Concepts

The tasks that are memory bound need reduction of memory accesses to the minimum possible. It can be achieved by exchange of the data between the workitems in the same workgroup.

The most important aspect of this sample is to demonstrate different ways of data exchange: through the use of local memory and through the use of OpenCL 3.0 extensions of `cl_khr_subgroup_shuffle` and `cl_khr_subgroup_shuffle_relative`.

### Application flow

The application once a device is selected queries its supported image formats and uses the format closest to the format of image to apply blur to. Then to find out what variants of blur are possible to execute on the device it is queried about several features.

First the type of local memory is checked and whether local memory is actually not different from global, then local memory exchange blur step is skipped. Second the application queries whether the device is capable of executing built-in `sub_group_shuffle` and/or `sub_group_shuffle_up/down` intrinsics. If not, the corresponding steps are also skipped.

Image buffers are created, and the OpenCL program is compiled with different options for general steps of no data exchange and local memory data exchange and for specialized steps of sub-group data exchange. Kernels used for box and Gaussian blur allow decomposition into 1D convolutions and it is used for speedup of the blur calculations.

Local memory exchange algorithm proceeds by first collectively loading all the pixels needed for workgroup into local memory and then read the pixels sequentially into each of workitems.

Subgroup exchange algorithm does not use local memory but instead exchanges data directly between different workitems in the subgroup. Note that for the subgroup data exchange to work correctly all the workitems sourcing data for exchange should encounter `sub_group_shuffle[_up/down]` operations. The kernels otherwise are conceptually simple, they read data first and then exchange it shuffling up in the workgroup while 0-th subgroup workitem reads new pixel from the image.

#### Event profiling

While events are multi-purpose in OpenCL, they are the handles through which synchronization and profiling can be done, here we only demonstrate their profiling capabilities. Event profiling is turned on by creating the queue as such:
```c
cl_command_queue_properties props[] = { CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0 };
queue = clCreateCommandQueueWithProperties(context, device, props, &error);
```
We do not have to test for this capability, as it's mandatory for a device to provide profiling capabilities.

To record the execution time of every blur step, we save the associated events like this:
```c
cl_event pass[2];
clEnqueueNDRangeKernel(queue, blur1, 2, origin, image_size, NULL, 0, NULL, pass + 0);
clEnqueueNDRangeKernel(queue, blur2, 2, origin, image_size, NULL, 0, NULL, pass + 1);
clWaitForEvents(2, pass);
```
Calling the kernel returns the event associated with it. Instead calling finish on the queue, we may as well wait for all events in `passes` to signal completion. (For a complete list of event state, refer to the [Execution Model](https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#_execution_model) chapter of the OpenCL API spec).

Because most often we measure durations between event state transitions, we use the utility `cl_ulong cl_util_get_event_duration(cl_event event, cl_profiling_info start, cl_profiling_info end, cl_int *error)` to get duration in nanoseconds between the states `start` and `end`.
_(Note: the main reason why net kernel execution time doesn't amount to the time measured by the host are due to dispatching kernel binaries to the device which happen on the first execution, as the sample doesn't invoke a so called warm-up kernel. By doing so, one can reduce the difference to minimal runtime overhead.)_

## Kernel logic

### Vanilla single-pass 2D blur

### Dual-pass 1D blur

### Local memory exchange blur

### Sub-group exchange blur

### Used API surface

```c
cl_util_get_device(const cl_uint plat_id, const cl_uint dev_id, const cl_device_type type, cl_int * const error)
cl_util_get_device_info(const cl_device_id device, const cl_device_info info, cl_int * const error)
cl_util_print_device_info(const cl_device_id device)
cl_util_read_text_file(const char * const filename, size_t * const length, cl_int * const error)
cl_util_build_program(const cl_program pr, const cl_device_id dev, const char * const opt)
cl_sdk_read_image(const char * const file_name, cl_int * const error)
cl_sdk_write_image(const char * const file_name, const cl_sdk_image * const im)
cl_util_get_event_duration(const cl_event event, const cl_profiling_info start, const cl_profiling_info end, cl_int * const error)
cl_util_print_error(const cl_int error)
```
