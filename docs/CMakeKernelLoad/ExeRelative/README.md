# Locate kernel file relative to executable path

One way to locate and load kernel files at runtime is searching relative to the path of the executable. Doing so requires one to copy kernel files to the build and install trees alike. (That is if one does not wish the executable to rely on the source tree, which is not recommended.) When building with CMake specifically, one can instruct the build system to copy this file both for development builds and installs.

We'll be using these functions, as per the docs.

```cmake
file(<COPY|INSTALL> <files>... DESTINATION <dir>
     [FILE_PERMISSIONS <permissions>...]
     [DIRECTORY_PERMISSIONS <permissions>...]
     [NO_SOURCE_PERMISSIONS] [USE_SOURCE_PERMISSIONS]
     [FOLLOW_SYMLINK_CHAIN]
     [FILES_MATCHING]
     [[PATTERN <pattern> | REGEX <regex>]
      [EXCLUDE] [PERMISSIONS <permissions>...]] [...])

install(<FILES|PROGRAMS> files...
        TYPE <type> | DESTINATION <dir>
        [PERMISSIONS permissions...]
        [CONFIGURATIONS [Debug|Release|...]]
        [COMPONENT <component>]
        [RENAME <name>] [OPTIONAL] [EXCLUDE_FROM_ALL])
```

We'll instruct CMake to copy the kernel for development builds to `${CMAKE_CURRENT_BINARY_DIR}` and to install the executable and the kernel into the same directory, wherever the default `${CMAKE_INSTALL_BINDIR}` points to for install steps.

```cmake
file(
  COPY
    ${Files_KRNS}
  DESTINATION
    ${CMAKE_CURRENT_BINARY_DIR}
)
install(
  TARGETS
    ${PROJECT_NAME}
  DESTINATION
    bin
)
install(
  FILES
    ${Files_KRNS}
  DESTINATION
    bin
)
```

Unfortunately we can't obtain the path to the currently running executable without resorting to platform-specific APIs. Such functionality is provided for eg. by the OpenCL SDK utility library by the function `std::filesystem::path cl::util::get_exe_path()` for all supported operating systems.

```c++
auto kernel_path = cl::util::get_exe_path().parent_path().append("saxpy.cl");
std::ifstream source_file{ kernel_path };
if (!source_file.is_open())
    throw std::runtime_error{ std::string{ "Cannot open kernel source: " } + kernel_path.generic_string() };

cl::Program program{ context,
                     std::string{ std::istreambuf_iterator<char>{ source_file },
                                  std::istreambuf_iterator<char>{}
                     }
};
```

When locating the kernel file for reading in this fashion, one is free to relocate the entire build or install tree as needed.

When building libraries with kernels as components, without surfacing a root path for kernel search, there is an implicit requirement imposed on the build / install trees of downstreams linking to the library. (For eg. the relative path of the kernel files and the final executable consuming the library have to be the same as the kernels and the unit tests of said library.) Depending on your workflow or use-case, this may or may not be acceptable.

> The API is exposed through C++17 std::filesystem::path, because character encoding of paths is platform specific. To maintain a platform agnostic interface, we rely on the abstraction introduced by `std::filesystem::path`, so the user need not be exposed to implementation details such as path encodings (`char` vs. `wchar_t`).

_(Note: the author is aware of the performance implication of using the filestream iterator adapters in constructors as opposed to raw seeking and pre-allocating in file handles, however kernel files more often than not are small enough to consider chosing simplicity over performance.)_