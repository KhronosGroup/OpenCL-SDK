# Locate kernel file by embedding into executable

This method slightly contrary to the name omits kernel location. By embedding into the executable, one omits loading the kernel file at runtime altogether. Proper execution of this method involves extensive build system support. Inlining the kernel source as a string literal is trivial, but one loses all syntax highlight capabilities in semantically aware editors. Instead, this document is aiming for maintaining a separate source file with canonical `.cl` extension to aid tooling, but actually embed the result into source code.

When building with CMake specifically, We cannot deal with this issue with a single invocation of `configure_file` to embed kernel code as a literal, because that command runs only once during configuration and preserves the contents as a snapshot and not update during development. Instead we want to involve the build system to properly track time stamps and perform only the minimally required steps in incremental builds too. For this, we need to define build steps, and this particular example will use CMake itself as a "compiler" for our custom build step. (We'll be using CMake as a glorified regex-replace engine, calling it a compiler may be exaggerating.)

_(Note: `#include`-ing kernel source into C/C++ source code as a string literal is not possible, because of how the preprocessor is required to work on a per-token basis. There is no way around this issue, hence ISO proposals for `std::embed`.)_

We'll be using these functions, as per the docs.

```cmake
file(READ <filename> <variable>
     [OFFSET <offset>] [LIMIT <max-in>] [HEX])

configure_file(<input> <output>
                [COPYONLY] [ESCAPE_QUOTES] [@ONLY]
                [NEWLINE_STYLE [UNIX|DOS|WIN32|LF|CRLF] ])
```

The logic of our custom build step will be a CMake script as follows:

```cmake
file(
  READ
    ${INPUT_FILE}
  INPUT_STRING
)
set(${VAR_NAME}
  ${INPUT_STRING}
)
configure_file(
  ${CONFIG_FILE}
  ${OUTPUT_FILE}
)
```

This script will read the file pointed to by `INPUT_FILE`, and substitute it's contents into `CONFIG_FILE` which contains a placeholder provided by `VAR_NAME` and saves result to `OUTPUT_FILE`.

With our `CONFIG_FILE` being `Kernel.in.hpp` and containing:

```c++
static std::string kernel_string{ R"(${Kernel_STRING})" };
```

and our kernel file being `SAXPY.cl` while also maintaining a clean source tree and generating files into our build tree, our build script woudl contain:

```cmake
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Kernel.hpp
  COMMAND ${CMAKE_COMMAND}
  ARGS -D INPUT_FILE=SAXPY.cl
       -D VAR_NAME=Kernel_STRING
       -D CONFIG_FILE=Kernel.in.hpp
       -D OUTPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/Kernel.hpp
       -P ${PROJECT_SOURCE_DIR}/cmake/Embed.cmake
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  DEPENDS SAXPY.cl
  COMMENT "Embedding SAXPY.cl into Kernel.hpp"
)
add_custom_target(${PROJECT_NAME}-kernels
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Kernel.hpp
)
add_dependencies(${PROJECT_NAME}
  ${PROJECT_NAME}-kernels
)
```

Breaking it down:

1. We define a custom command which forwards the proper variables to our "compiler" script.
2. We explicitly state using `DEPENDS` that this command is out-of-date if the timestamp of `SAXPY.cl` is newer than the `OUTPUT`.
3. We create a custom target that depends on the command's `OUTPUT`, because build systems cannot invoke commands, they have to be bound to a build target.
4. Our executable does not depend on this machinery yet in any way and may rush to compiling without the generated header in place, hence we declare proper ordering via one target depending on the other.

We can then omit kernel loading and jump straight to creating our `cl::Program`:

```c++
// Create program and kernel
cl::Program program{ context, kernel_string };
program.build({ device });
```

When loading kernel code in this fashion, one is free to move the executable anywhere on disk, and is free of any working directory requirements. The downside is that as kernel sources grow in size, the executable may grow to unhealthy proportions. Embedding data into executables puts unnecessary strain on system memory (RAM). Depending on your workflow or use-case, this may or may not be acceptable.

_(Note: the method detailed above is provided as a utility CMake macro as part of the OpenCL SDK with slightly different semantics covering more use cases. This section shows the essence of how embedding the contents of a file into a raw string literal looks like.)_