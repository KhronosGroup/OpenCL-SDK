# Ocean surface simulation with Opencl and OpenGL interoperability

## Sample Purpose

This sample demonstrates how to share compute/render resources between OpenCL and OpenGL to simulate an ocean surface and compare it with the Vulkan equivalent sample. If the cl_khr_gl_sharing extension is available and requested through CLI option, some OpenCL images will be created through a clCreateFromGLTexture procesure and updated every frame through clEnqueueAcquireGLObjects and clEnqueueReleaseGLObjects procedures. These images will then be used for ocean rendering. If cl_khr_gl_sharing is not available, additional copying from OpenCL buffers to OpenGL images will be performed.

## Key APIs and Concepts

The primary focus of this sample is to understand how to set up shared resources between OpenCL and OpenGL interoperability and compare both performance and necessary workload between this and equivalent vulkan sample. Additionally, this sample demonstrates how to approach physical, real-time simulations in OpenCL and the API objects involved in executing an OpenCL application such as ocean surface simulation.


### Application flow

The application performs an initial setup during which:

    -OpenCL and OpenGL resources such as device and context are prepared.
    -A SFML window, camera, and related keyboard event callbacks are created.
    -Both shared and private resources for OpenCL and OpenGL are set up.

Available CLI options are as follows:

    --useGLSharing, requests use of cl_khr_gl_sharing

After the setup, the simulation starts with initial ocean parameters that can be modified with keyboard events in real-time:

    - a/z - Increase/decrease wind magnitude.
    - s/x - Change wind heading.
    - d/c - Increase/decrease waving amplitude.
    - f/v - Increase/decrease wave choppiness.
    - g/b - Increase/decrease additional altitude scale.

Additionally, the simulation and rendering can be paused with the Space key. Rendering can toggle between wireframe and filled modes using the 'w' key. Application tracks its performance in the title bar of the window, it could be toggled by pressing 'e' key.

While the simulation is in progress, each frame of the application performs the following general steps:

    -Uniform buffers are updated to handle camera and ocean parameters.
    -OpenCL kernels are enqueued.
    -The ocean grid is rendered using the previous OpenCL computation outcome.


### Kernel logic

Multiple kernels follow the general steps (with multiple optimizations) described in the publication:  [Realtime GPGPU FFT ocean water simulation](https://tore.tuhh.de/bitstream/11420/1439/1/GPGPU_FFT_Ocean_Simulation.pdf)

### Used API surface

```c++
cl::util::supports_extension(cl::Device, cl::string)
cl::util::read_exe_relative_text_file(const char*, cl_int* const)
cl::util::read_exe_relative_binary_file(const char*, cl_int* const)
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


