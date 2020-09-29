# copybuffer

## Sample Purpose

In this very simple sample, OpenCL APIs are used to copy the contents of one buffer to another buffer on the OpenCL device.
To do this, OpenCL APIs are used to create both buffers, to create the OpenCL command queue, and to initialize the source buffer and verify the contents of the destination buffer on the host.

By default, this sample will run in the first enumerated OpenCL device on the first enumerated OpenCL platform.
To run on a different OpenCL device or platform, please use the provided command line options.

## Key APIs and Concepts

This example shows how to create OpenCL buffers and command queues, and how to enqueue a command to copy between the two buffers in the command queue.
Additionally, the example shows one way to initialize the contents of the source buffer on the host, and one way to check the contents of the destination buffer on the device.

```c
clCreateCommandQueue / clCreateCommandQueueWithProperties
clCreateBuffer
clEnqueueMapBuffer
clEnqueueUnmapMemObject
clEnqueueCopyBuffer
```

## Command Line Options

| Option | Default Value | Description |
|:--|:-:|:--|
| `-d <index>` | 0 | Specify the index of the OpenCL device in the platform to execute on the sample on.
| `-p <index>` | 0 | Specify the index of the OpenCL platform to execute the sample on.
