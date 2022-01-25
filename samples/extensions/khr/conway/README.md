# Conway's Game of Life

## Sample Purpose

This sample intends to demonstrate how to share images (textures) between OpenCL and OpenGL. [How Does OpenCL-OpenGL Interop?](https://github.com/KhronosGroup/OpenCL-Guide/blob/main/chapters/how_does_opencl-opencl_interop.md) chapter of the OpenCL-Guide lays out the fundamentals of OpenCL-OpenGL interoperability.

## Key APIs and Concepts

The most important aspect of this sample is to understand how OpenCL-OpenGL interop contexts and shared resources are setup. The `cl::sdk::get_interop_context` function takes much of the tedium out of setting up such a context while the `cl::sdk::InteropWindow` adds "GLUT-like" features, dealing with typical windowed interop app control flow, making sure that API objects are created in the correct order.

### Application flow

Application flow is primarily dictated by `cl::sdk::InteropWindow`, more specifically by `cl::sdk::InteropWindow::run()`. For a deatiled overview of what the `InteropWindow` utility does, please refer to the [Utils documentation](../../../../lib/Utils.md).

### Double buffering

The kernel used to implement the stepping routine uses double-buffering of the shared textures. Without double buffering a data race arises between the pixels of the image.

### Implicit interop context synchronization

This sample uses basic and implicit interop context synchronization.For a detailed overview on the various ways OpenCL and OpenGL can be synchronized, refer to [Synchronizing the two APIs](https://github.com/KhronosGroup/OpenCL-Guide/blob/main/chapters/how_does_opencl-opencl_interop.md#Synchronizing-the-two-APIs) section of the OpenCL-OpenGL interop guide.

## Kernel logic

The kernel implements the classic [Game of Life rules](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life#Rules). Each pixel of the texture is 1 byte in size, which is stored in the red channel and is used to code 1 bit of data. (The current 1:7 useful:waste bit ratio can be improved in multiple ways, most trivially by bitcoding the data and storing muliple lattice sites in one pixel coordinate.)

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