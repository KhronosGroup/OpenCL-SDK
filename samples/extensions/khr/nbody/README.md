# Gravitational NBody

## Sample Purpose

This sample intends to demonstrate how to share (vertex) buffers between OpenCL and OpenGL. [How Does OpenCL-OpenGL Interop?](https://github.com/KhronosGroup/OpenCL-Guide/blob/main/chapters/how_does_opencl-opencl_interop.md) chapter of the OpenCL-Guide lays out the fundamentals of OpenCL-OpenGL interoperability.

## Key APIs and Concepts

The most important aspect of this sample is to understand how OpenCL-OpenGL interop contexts and shared resources are setup. The `cl::util::get_interop_context` function takes much of the tedium out of setting up such a context while the `cl::util::InteropWindow` adds "GLUT-like" features, dealing with typical windowed interop app control flow, making sure that API objects are created in the correct order.

### Application flow

Application flow is primarily dictated by `cl::sdk::InteropWindow`, more specifically by `cl::ask::InteropWindow::run()`. For a detailed overview of what the `InteropWindow` utility does, please refer to the [SDK library documentation](../../../../lib/SDK.md).

### Double buffering

The kernel used to implement the gravitational interaction and time-stepping in a fused manner uses double-buffering of some of the data. It is not possible to calculate forces between the particles and carry out the forward-Euler in the same kernel without global syncing. (Some praticles may have already updated their position while others are still summing up forces, using the now out-of-sync positions.)

### Implicit interop context synchronization

This sample uses basic and implicit interop context synchronization. For a detailed overview on the various ways OpenCL and OpenGL can be synchronized, refer to [Synchronizing the two APIs](https://github.com/KhronosGroup/OpenCL-Guide/blob/main/chapters/how_does_opencl-opencl_interop.md#Synchronizing-the-two-APIs) section of the OpenCL-OpenGL interop guide.

### Used API surface

```c++
cl::sdk::InteropWindow
cl::Context::getInfo<CL_CONTEXT_DEVICES>()
cl::CommandQueue(cl::Context, cl::Device)
cl::util::get_program(cl::Context, cl::string)
cl::Program::build(cl::Device)
cl::KernelFunctor<...>(cl::Program, const char*)
cl::Buffer(cl::CommandQueue, Iter, Iter, bool)
cl::BufferGL::BufferGL(cl::Context, cl_mem_flags, cl_GLuint)
cl::copy(cl::CommandQueue, cl::Buffer, Iter, Iter)
cl::CommandQueue::enqueueAcquireGLObjects(const cl::vector<cl::Memory>*, const cl::vector<cl::Event>*, cl::Event*)
cl::CommandQueue::enqueueReleaseGLObjects(const cl::vector<cl::Memory>*, const cl::vector<cl::Event>*, cl::Event*)
cl::finish()
cl::Event::wait()
```
