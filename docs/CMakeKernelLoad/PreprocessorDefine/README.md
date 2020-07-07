# Locate kernel file from preprocessor `#define`

One way to locate and load kernel files at runtime is obtaining the path from the build system via compiler command switches, a preprocessor define to be precise. It is very similar in nature to a configuration header in the sense changing the path requires a rebuild of any source file relying on the header / define. Kernel paths will be hardcoded, and one cannot differentiate between development and installation builds, at least in the context of CMake.

We'll be using these functions, as per the docs.

```cmake
target_compile_definitions(<target>
  <INTERFACE|PUBLIC|PRIVATE> [items1...]
  [<INTERFACE|PUBLIC|PRIVATE> [items2...] ...])

file(<COPY|INSTALL> <files>... DESTINATION <dir>
     [FILE_PERMISSIONS <permissions>...]
     [DIRECTORY_PERMISSIONS <permissions>...]
     [NO_SOURCE_PERMISSIONS] [USE_SOURCE_PERMISSIONS]
     [FOLLOW_SYMLINK_CHAIN]
     [FILES_MATCHING]
     [[PATTERN <pattern> | REGEX <regex>]
      [EXCLUDE] [PERMISSIONS <permissions>...]] [...])
```

We'll instruct CMake to copy the kernel for development builds to `${CMAKE_CURRENT_BINARY_DIR}`. There is no point in defining an install target, as we cannot have different defines based on `cmake --build . --target all` and `cmake --build . --target install`.

```cmake
file(
  COPY
    ${Files_KRNS}
  DESTINATION
    ${CMAKE_CURRENT_BINARY_DIR}
)
file(TO_CMAKE_PATH
  "${CMAKE_CURRENT_BINARY_DIR}/${Files_KRNS}"
  Files_KRNS_PATH
)
```

Finall, we have to get our `#define`s right.

```cmake
target_compile_definitions(${PROJECT_NAME}
  PRIVATE
    KERNEL_PATH="${Files_KRNS_PATH}"
)
```

We can then load the kernel file simply by:

```c++
std::ifstream source_file{ KERNEL_PATH };
if (!source_file.is_open())
    throw std::runtime_error{ std::string{ "Cannot open kernel source: " } + "./saxpy" };

cl::Program program{ context,
                     std::string{ std::istreambuf_iterator<char>{ source_file },
                                  std::istreambuf_iterator<char>{}
                     }
};
```

When locating the kernel file for reading in this fashion, one is restricted to development builds. Invoking the `install` target will still result in `KERNEL_PATH` pointing to the development tree, hence installing such code isn't possible. (Installation trees depending on external files is bad practice.) Depending on your workflow or use-case, this may or may not be acceptable.

_(Note: the author is aware of the performance implication of using the filestream iterator adapters in constructors as opposed to raw seeking and pre-allocating in file handles, however kernel files more often than not are small enough to consider chosing simplicity over performance.)_