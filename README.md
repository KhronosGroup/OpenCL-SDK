# OpenCL-SDK

This is the Khronos OpenCL SDK. It brings together all the components needed to
develop OpenCL applications:

- OpenCL Headers (`include/api`)
- OpenCL C++ bindings (`include/cpp`)
- OpenCL Loader
- OpenCL utility library (`include/utils`)

It also contains resources useful to OpenCL developers:

- Code samples (`samples/`)
- Documentation (`docs/`)

## Setting Up the SDK

This repository uses sub-modules for the OpenCL Headers, OpenCL C++ bindings, and OpenCL Loader.

To clone a new repository with sub-modules included, use the `--recursive` option:

```sh
$ git clone --recursive https://github.com/KhronosGroup/OpenCL-SDK.git
```

To setup sub-modules from an existing repository, use:

```sh
$ git submodule init
$ git submodule update
```

## Building the Samples

This repository uses CMake as its build system.
The suggested build directory is `build`.

To generate build files, use for example:

```sh
$ mkdir build && cd build
$ cmake ..
```

Then build with the generated build files.
