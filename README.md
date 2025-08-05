# OpenCL<sup>TM</sup> SDK (in development)

This is the Khronos OpenCL SDK. It brings together all the components needed to
develop OpenCL applications:

- OpenCL Headers (`external/OpenCL-Headers/`)
- OpenCL C++ bindings (`external/OpenCL-CLHPP/include`)
- OpenCL Loader (`external/OpenCL-ICD-Loader`)
- OpenCL utility library (`lib/include`)

It also contains resources useful to OpenCL developers:

- Code samples (`samples/`)
- Documentation (`docs/`)

## Build Instructions

### Dependencies

- This repository uses sub-modules for the OpenCL Headers, OpenCL C++ bindings, and OpenCL ICD Loader and some of their transitive dependencies.

  - To clone a new repository with all sub-modules included, use the `--recursive` option. Note that this option clones all sub-modules and their dependencies, which are not strictly required for the OpenCL SDK:

        git clone --recursive https://github.com/KhronosGroup/OpenCL-SDK.git

  - Alternatively, to clone only the sub-modules for the OpenCL SDK, first clone this repository without sub-modules included then setup submodules non-recursively:

        git clone https://github.com/KhronosGroup/OpenCL-SDK.git
        git submodule init
        git submodule update

- The SDK uses CMake for its build system.
If CMake is not provided by your build system or OS package manager, please consult the [CMake website](https://cmake.org).

- The SDK samples depend on

  - [Templatized C++ Command Line Parser Library](http://tclap.sourceforge.net/) (aka. TCLAP)
  - [Simple and Fast Multimedia Library](https://www.sfml-dev.org/) (aka. SFML)
  - [OpenGL Mathematics](https://glm.g-truc.net/0.9.9/index.html) (aka. GLM)

### Example Build

For most builds, the following steps may be used to build the OpenCL SDK.
These build steps work on both Windows and Linux:

1. Clone this repo and update submodules:

        git clone https://github.com/KhronosGroup/OpenCL-SDK.git
        git submodule init
        git submodule update

2. Create a "build" directory:

        mkdir build
        cd build

3. Generate build files inside of the "build" directory:

        cmake .. -DCMAKE_BUILD_TYPE=Release

4. Build the OpenCL SDK and copy files to an "install" directory:

        cmake --build . --target install --config Release

   Or, build the OpenCL SDK using the generated build files directly.

To customize the build, the following CMake variables are supported.
To specify one of these variables via the command line generator, use the CMake syntax `-D<variable>=<value>`.
See your CMake documentation for more details.

| Variable | Type | Description |
|:---------|:-----|:------------|
| CMAKE_BUILD_TYPE | STRING | Specifies the build type.  Does not affect multi-configuration generators, such as Visual Studio solution files.
| CMAKE_INSTALL_PREFIX | PATH | Install directory prefix.
| OPENCL_SDK_BUILD_UTILITY_LIBRARIES | BOOL | Enables building OpenCL SDK utility libraries.  Default: `TRUE`
| OPENCL_SDK_BUILD_SAMPLES | BOOL | Enables building OpenCL SDK samples.  Default: `TRUE`
| OPENCL_SDK_BUILD_OPENGL_SAMPLES | BOOL | Enables building OpenCL SDK samples that interoperate with OpenGL.  Default: `FALSE`
| OPENCL_SDK_BUILD_VULKAN_SAMPLES | BOOL | Enables building OpenCL SDK samples that interoperate with Vulkan.  Default: `TRUE`
| OPENCL_SDK_TEST_SAMPLES | BOOL | Enables a target to test the OpenCL SDK samples.  Default: `TRUE`
