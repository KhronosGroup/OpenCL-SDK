# copybufferkernel

## Sample Purpose

This example uses an OpenCL kernel to do work.
An OpenCL kernel is a short program defining what one OpenCL work item should do.
In this case, each OpenCL work item will copy one value from a source buffer to a destination buffer.
Since this sample launches one work item for every element in the source buffer, behaviorally this sample will do exactly the same thing as the copy buffer sample.

In this sample, the source code for the OpenCL kernel is embedded into the host code as a raw string.
At runtime, an OpenCL program is created from the raw string, and the OpenCL device compiler is invoked to compile the OpenCL program for the OpenCL device.
This isn't the only way to create OpenCL programs, but it is fairly common, especially while learning and developing an OpenCL application.

By default, this sample will run in the first enumerated OpenCL device on the first enumerated OpenCL platform.
To run on a different OpenCL device or platform, please use the provided command line options.

## Key APIs and Concepts

This example shows how to create an OpenCL program from a source string and enqueue an ND range for the kernel into an OpenCL command queue.


```c
clCreateProgramWithSource
clBuildProgram
clCreateKernel
clSetKernelArg
clEnqueueNDRangeKernel
```

## Command Line Options

| Option | Default Value | Description |
|:--|:-:|:--|
| `-d <index>` | 0 | Specify the index of the OpenCL device in the platform to execute on the sample on.
| `-p <index>` | 0 | Specify the index of the OpenCL platform to execute the sample on.
