# enumopencl

## Sample Purpose

This is a very simple sample that demonstrates how to enumerate the OpenCL platforms that are installed on a machine, and the OpenCL devices that these platforms expose.

This is one of the few samples that uses the OpenCL C APIs, as described in the OpenCL specification.
Most of the other samples use the OpenCL C++ API bindings, since they make it a lot easier to write and understand OpenCL code!

This is a good first sample to run to verify that OpenCL is correctly installed on your machine, and that your build environment is correctly setup.

## Key APIs and Concepts

The most important concepts to understand from this sample are OpenCL platforms and OpenCL devices:

An OpenCL platform is a container describes a collection of OpenCL devices supported by an OpenCL implementation.
An OpenCL device will eventually be used to execute OpenCL code, but this sample only lists the devices available on the system.

```c
clGetPlatformIDs
clGetDeviceIDs
clGetPlatformInfo
clGetDeviceInfo
```
