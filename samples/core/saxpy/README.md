# saxpy

## Sample Purpose

This sample intends to be a minimal end-to-end OpenCL application doing actual device-side computation. The structure of the sample rhymes well with the [How Does OpenCL Work?](https://github.com/KhronosGroup/OpenCL-Guide/blob/main/chapters/how_does_opencl_work.md) chapter of the OpenCL-Guide, particularly the [Executing an OpenCL Program](https://github.com/KhronosGroup/OpenCL-Guide/blob/main/chapters/how_does_opencl_work.md#executing-an-opencl-program) part.

This sample is implemented using both C and C++ languages to demonstrate the difference in verbosity when using the naked C bindings compared to the C++ wrapper.

## Key APIs and Concepts

The most important aspect of this sample is to understand what entities / API objects are involved in the execution of a typical OpenCL application.

### Application flow

You may note that the actual order in creating the relevant API objects in this sample differs slightly from the list in [Executing an OpenCL Program](https://github.com/KhronosGroup/OpenCL-Guide/blob/main/chapters/how_does_opencl_work.md#executing-an-opencl-program). That is because the actual data dependence allows for some flexibility. The most important is obtaining a `cl_context`/`cl::Context`, because that is _the alpha and the omega_ of all other API objects in the runtime layer. For a distinction on where the platform layer ends and where the runtime layer begings, refer to **REF**.

If we aim for absolute fastest execution, compiling our kernel may have been concurrent to initializing our input data.

### Lazy buffer movement

OpenCL buffers when created are bound to a context, not any specific device. Device-local physical memory is conceptually used as a cache to access the contents of the buffer. (See [OpenCL API - Memory Model: Fundamental Memory Regions](https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#_fundamental_memory_regions)) To paraphrase, one may think of buffers migrating from device-to-device as needed, while the OpenCL Runtime keeps track of where the most current (last written) contents reside.

This sample creates buffers, specifies their initial content with an iterator range spanning host containers and launches a kernel using said buffer. The OpenCL Runtime will take care of dispatching data to device global memory.

Reading buffer contents on host cannot happen lazily, as we have to obtain a pointer to the buffer's contents. This happens internally inside `cl::copy( cl::CommandQueue, cl::Buffer, Iter, Iter )`.

#### Explicit host-device-host buffer movement

Note that this sample uses the more minimal `cl::Buffer{ cl::Context, Iter, Iter, bool }` buffer constructor (henceforth CTOR) overload, but there exists a `cl::Buffer{ cl::CommandQueue, Iter, Iter, bool }` overload, which still inside the CTOR invokes obtains the context of the queue, creates the buffer with it, followed by a `cl::copy( cl::CommandQueue, Iter, Iter, cl::Buffer )` which copies to the device associated with the queue using `cl::CommandQueue::enqueueMapBuffer`.

### In-order queue usage

While our kernel launch operation is asynchronous (and the host validation set is calculated concurrently, even if we use `CL_DEVICE_TYPE_CPU`), one may think that if the host is fast enough it's possible to fetch buffer contents before the device finishes running the kernels. This doesn't happen, because the queue we created had no properties specified (no `cl::QueueProperties::OutOfOrder`) and therefore commands enqueued are not allowed to overtake each other, so `cl::copy` may only start once the previous kernel has finished executing (and it's memory operations are visible to subsequent commands).

### Used API surface

```c++
cl::util::get_context(cl::util::Triplet)
cl::Context::getInfo<CL_CONTEXT_DEVICES>()
cl::CommandQueue(cl::Context, cl::Device)
cl::Device::getInfo<CL_DEVICE_PLATFORM>()
cl::util::get_program(cl::Context, cl::string)
cl::KernelFunctor<...>(cl::Program, const char*)
cl::sdk::fill_with_random(...)
cl::Buffer(cl::CommandQueue, Iter, Iter, bool)
cl::copy(cl::CommandQueue, cl::Buffer, Iter, Iter)
```
