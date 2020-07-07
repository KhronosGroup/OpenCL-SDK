# Locate kernel file via configuration header

One way to locate and load kernel files at runtime is using configuration headers. When building with CMake specifically, one can instruct the build system to generate this file when configuring the build.

We'll be using this function, as per the docs.

```cmake
configure_file(<input> <output>
                [COPYONLY] [ESCAPE_QUOTES] [@ONLY]
                [NEWLINE_STYLE [UNIX|DOS|WIN32|LF|CRLF] ])
```

This CMake function takes an input file, changes all sequences of special chars to their CMake variable values and writes the result to disk. We'll invoke it like this:

```cmake
configure_file(
  KernelLocation.in.hpp
  ${CMAKE_CURRENT_BINARY_DIR}/KernelLocation.hpp
)
```

We do not write generated files to the source tree to avoid having to `.gitignore` it, or otherwise accidentally subject it to version control. The file we configure, `KernelLocation.in.hpp` looks like this:

```c++
#pragma once

// ISO C++14
#include <string>

static std::string kernel_location{ "${Files_KRNS}" };
```

Including this file in our build and adding `${CMAKE_CURRENT_BINARY_DIR}` to the include search path for our executable allows us to use `kernel_location` as a C++ variable when loading the kernel file. In our build script we add:

```cmake
add_executable(${PROJECT_NAME}
  ${CMAKE_CURRENT_BINARY_DIR}/KernelLocation.hpp
  Main.cpp
)
```

and to load the kernel file we use idiomatic STL constructs:

```c++
std::ifstream source_file{ kernel_location };
if (!source_file.is_open())
    throw std::runtime_error{ std::string{ "Cannot open kernel source: " } + kernel_location };

cl::Program program{ context,
                     std::string{ std::istreambuf_iterator<char>{ source_file },
                                  std::istreambuf_iterator<char>{}
                     }
};
```

When locating the kernel file for reading in this fashion, one is restricted to relying on the kernel files inside the source tree. Defining different kernel locations for development builds and installations is not possible. Depending on your workflow or use-case, this may or may not be acceptable.

_(Note: the author is aware of the performance implication of using the filestream iterator adapters in constructors as opposed to raw seeking and pre-allocating in file handles, however kernel files more often than not are small enough to consider chosing simplicity over performance.)_