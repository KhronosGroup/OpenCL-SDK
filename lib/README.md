# OpenCL Utility and OpenCL SDK Library Documentation

There are two libraries in the OpenCL SDK which all samples utilize to different extents. One such library is the OpenCL Utility Library which is an exported library and is meant to ease the use of OpenCL, while the OpenCL SDK Library builds on top of it but is not exported when installing the SDK. The OpenCL SDK lib extends the Utility library in ways which likely don't make sense outside the context of the SDK samples. 

## OpenCL Utility Library

One may think of this library as analogous to GLU and GLUT in the domain of OpenGL. A set of utilities which condense common tasks into singular functions or add missing functionality of the API which otherwise couldn't be added as a non-API-breaking change.

For a complete list utilities provided by this library, refer to the [OpenCL Utility Library docs](./Utils.md).

## OpenCL SDK Library

The SDK library extends the Utility library by deduplicating common tasks like command-line argument parsing, selecting devices, logging, and other contentious tasks which your application likely does differently, hence the value in shipping it for external use, moreover promise forward and/or backward compatibility is low.

For a complete list functionality provided by this library, refer to the [OpenCL SDK Library docs](./SDK.md).