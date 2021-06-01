# Using OpenCL from Python

The examples in this directory illustrate how to use OpenCL from
[Python](https://www.python.org/) using [PyOpenCL](https://documen.tician.de/pyopencl/).

If you would like to try them out on your own computer (Linux/mac/Windows are all supported),
follow the [installation instructions](https://documen.tician.de/pyopencl/misc.html#installation)
and then simply run the example file:
```sh
python3 example-file.py
```
There is no build process and no need to compile anything.

## Examples

- `demo.py` demonstrates memory allocation and running kernels.
- `demo-array.py` demonstrates memory allocation and running kernels,
  but using the minimal array package that comes with PyOpenCL.
- `dump-properties.py` demonstrates access to properties with
  a sort-of reimplementation of the `clinfo` utility.
- `transpose.py` demonstrates a matrix transposition kernel.

You can find many more examples in [PyOpenCL's Github repository](https://github.com/inducer/pyopencl/tree/main/examples).
