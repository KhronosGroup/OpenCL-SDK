# OpenCL<sup>TM</sup> SDK (in development)

This is the Khronos OpenCL SDK. It brings together all the components needed to
develop OpenCL applications:

- OpenCL Headers (`include/api`)
- OpenCL C++ bindings (`include/cpp`)
- OpenCL Loader
- OpenCL utility library (`include/utils`)

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

> The example build guide uses [Vcpkg](https://vcpkg.io/en/index.html) to fetch all dependencies. Note that Vcpkg is _not_ a requirement and is only used for convenience. One may provide dependencies through any other CMake mechanism. For details on how to install Vcpkg, refer to its [Getting Started Guide](https://vcpkg.io/en/getting-started.html). The example build assumes targeting 64-bit Windows.

1. Clone this repo with the rest of the OpenCL SDK components:

       git clone https://github.com/KhronosGroup/OpenCL-SDK.git
       git submodule init
       git submodule update

1. Install dependencies:

       vcpkg --triplet x64-windows install sfml tclap glm

1. Build and install SDK with samples and no downstream unit tests:

       cmake -A x64 `
             -D BUILD_TESTING=OFF `
             -D BUILD_DOCS=OFF `
             -D BUILD_EXAMPLES=OFF `
             -D BUILD_TESTS=OFF `
             -D OPENCL_SDK_BUILD_SAMPLES=ON `
             -D OPENCL_SDK_TEST_SAMPLES=OFF `
             -D CMAKE_TOOLCHAIN_FILE=/vcpkg/install/root/scripts/buildsystems/vcpkg.cmake `
             -D VCPKG_TARGET_TRIPLET=x64-windows `
             -B ./OpenCL-SDK/build -S ./OpenCL-SDK
       cmake --build ./OpenCL-SDK/build --target install

_(Note: on Linux, paths to dependent libraries are automatically handled by RPATH in both the build and install tree. On Windows, all DLLs have to be on the `PATH`. Vcpkg copies dependent DLLs to the build tree, but in order to do the same in the install tree, a sufficiently new CMake version is required. CMake 3.21 introduces `install(IMPORTED_RUNTIME_ARTIFACTS)`.)_
