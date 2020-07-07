# Locate kernel file relative to current working directory

One way to locate and load kernel files at runtime is searching relative to the current working directory. Doing so requires one to copy kernel files to the build and install trees alike. (That is if one does not wish the executable to rely on the source tree, which is not recommended.) When building with CMake specifically, one can instruct the build system to copy this file both for development builds and installs.

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

We can then load the kernel file simply by:

```c++
std::ifstream source_file{ "./saxpy" };
if (!source_file.is_open())
    throw std::runtime_error{ std::string{ "Cannot open kernel source: " } + "./saxpy" };

cl::Program program{ context,
                     std::string{ std::istreambuf_iterator<char>{ source_file },
                                  std::istreambuf_iterator<char>{}
                     }
};
```

When locating the kernel file for reading in this fashion, one is restricted to being able to run the code only from the same directory. Depending on your workflow or use-case, this may or may not be acceptable.

_(Note: the author is aware of the performance implication of using the filestream iterator adapters in constructors as opposed to raw seeking and pre-allocating in file handles, however kernel files more often than not are small enough to consider chosing simplicity over performance.)_