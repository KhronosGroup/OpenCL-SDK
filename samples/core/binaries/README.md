# Binaries

## Sample Purpose

This sample intends to demonstrate how to use separate compilation and consumption of OpenCL program by saving and loading program binaries.

## Key APIs and Concepts

The binary of a compiled program can be extracted from the program by invoking `clGetProgramInfo` with `CL_PROGRAM_BINARIES` parameter. Each device is provided with separate binary, and this binary can be saved into a file, and then loaded into the same runtime for a specified context to use on the same device. Loaded binaries could be used for `clCreateProgramWithBinary` invocation that constructs OpenCL program out of them. As a result offline compilation of the program becomes possible.

### Application flow

The application once a platform and a device is selected tries to read corresponding binary file from the disk. In the case of success the program is built from the binary and executed. If the binary file is not present in the folder of the program then kernel `.cl` file is loaded, built for the specified device, saved as a binary file, loaded and executed.

#### Event profiling

While events are multi-purpose in OpenCL, they are the handles through which synchronization and profiling can be done, here we only demonstrate their profiling capabilities. Event profiling is turned on by creating the queue as such:
```c
cl_command_queue_properties props[] = { CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0 };
queue = clCreateCommandQueueWithProperties(context, device, props, &error);
```
We do not have to test for this capability, as it's mandatory for a device to provide profiling capabilities.

To record the execution time of every blur step, we save the associated events like this:
```c
cl_event pass;
clEnqueueNDRangeKernel(queue, Collatz, 1, NULL, &length, NULL, 0, NULL, &pass);
clWaitForEvents(1, &pass);
```
Calling the kernel returns the event associated with it. Instead calling finish on the queue, we may as well wait for all events in `passes` to signal completion. (For a complete list of event state, refer to the [Execution Model](https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#_execution_model) chapter of the OpenCL API spec).

Because most often we measure durations between event state transitions, we use the utility `cl_ulong cl_util_get_event_duration(cl_event event, cl_profiling_info start, cl_profiling_info end, cl_int *error)` to get duration in nanoseconds between the states `start` and `end`.
_(Note: the main reason why net kernel execution time doesn't amount to the time measured by the host are due to dispatching kernel binaries to the device which happen on the first execution, as the sample doesn't invoke a so called warm-up kernel. By doing so, one can reduce the difference to minimal runtime overhead.)_

### Kernel logic

Kernel is testing Collatz conjecture that states that the sequential application of rules

1) if `n` is odd, `n` -> `3*n+1`
2) if `n` is even, `n` -> `n/2`

to any of positive integer numbers results ultimately in a loop of `1`->`4`->`2`->`1` etc. (See [Wikipedia article](https://en.wikipedia.org/wiki/Collatz_conjecture) for a brief introduction.) It writes down the number of iterations needed to get into the circle for each of the unique global work-item ID values (plus 1).

## Used API surface

```c
cl_util_get_device(const cl_uint plat_id, const cl_uint dev_id, const cl_device_type type, cl_int * const error)
cl_util_print_device_info(const cl_device_id device)
cl_util_read_binaries(const cl_context context, const cl_device_id * const devices, const cl_uint num_devices, const char * const program_file_name, cl_int * const error)
cl_util_build_program(const cl_program pr, const cl_device_id dev, const char * const opt)
cl_util_read_text_file(const char * const filename, size_t * const length, cl_int * const error)
cl_util_write_binaries(const cl_program program, const char * const program_file_name)
cl_util_print_error(const cl_int error)
```
