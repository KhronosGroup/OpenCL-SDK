# Ocean surface simulation with Opencl and Vulkan interoperability

[Ocean Simulation With OpenCL and Vulkan](ocean.png)

## Sample Purpose

This sample demonstrates how to share compute/render resources between OpenCL and Vulkan to simulate an ocean surface. If the cl_khr_external_memory extension is available and requested (through CLI options), some OpenCL images will be created through a file descriptor handle received with vkGetMemoryFdKHR. These images will then be used for ocean rendering. If cl_khr_external_memory is not available, additional copying from OpenCL buffers to Vulkan images will be performed.

## Key APIs and Concepts

The primary focus of this sample is to understand how to set up shared resources between OpenCL and Vulkan interoperability. Additionally, this sample demonstrates how to approach physical, real-time simulations in OpenCL and the API objects involved in executing an OpenCL application such as ocean surface simulation.


### Application flow

The application performs an initial setup during which:

    -An OpenCL platform and Vulkan physical device are selected based on CLI options.
    -OpenCL and Vulkan devices are prepared.
    -A GLFW window, camera, and related keyboard event callbacks are created.
    -Both shared and private resources for OpenCL and Vulkan are set up.

After the setup, the simulation starts with initial ocean parameters that can be modified with keyboard events in real-time:

    - a/z - Increase/decrease wind magnitude.
    - s/x - Change wind heading.
    - d/c - Increase/decrease waving amplitude.
    - f/v - Increase/decrease wave choppiness.
    - g/b - Increase/decrease additional altitude scale.

Additionally, the simulation and rendering can be paused with the Space key. Rendering can toggle between wireframe and filled modes using the 'w' key. While the simulation is in progress, each frame of the application performs the following general steps:

    -Necessary Vulkan/OpenCL semaphores are signaled/waited.
    -Uniform buffers are updated to handle camera and ocean parameters.
    -OpenCL kernels are enqueued.
    -The ocean grid is rendered using the previous OpenCL computation outcome.


### Kernel logic

Multiple kernels follow the general steps (with multiple optimizations) described in the publication:  [Realtime GPGPU FFT ocean water simulation](https://tore.tuhh.de/bitstream/11420/1439/1/GPGPU_FFT_Ocean_Simulation.pdf)

### Used API surface

```c++
cl::util::supports_extension(cl::Device, cl::string)
cl::Context(cl::Device)
cl::CommandQueue(cl::Context, cl::Device)
cl::Platform::get(vector<Platform>)
cl::Platform::getDevices(Type, vector<Device>)
cl::Program::build()
cl::Image2D(cl::Context, cl_mem_flags, ImageFormat, size_type, size_type)
cl::Error::what()
cl::Error::err()
cl::NDRange(size_type, size_type)
cl::Buffer::Buffer(cl::Context, cl_mem_flags, size_type)
```


