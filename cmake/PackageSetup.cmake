set(CPACK_PACKAGE_VENDOR "khronos")

set(CPACK_DEBIAN_DESCRIPTION "Khronos OpenCL Software Development Kit")

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

if(NOT CPACK_PACKAGING_INSTALL_PREFIX)
  set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
endif()

# DEB packaging configuration
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_ALL binary)

set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${CPACK_PACKAGE_VENDOR})

set(CPACK_DEBIAN_PACKAGE_HOMEPAGE
    "https://github.com/KhronosGroup/OpenCL-SDK")

# Version number [epoch:]upstream_version[-debian_revision]
set(CPACK_DEBIAN_PACKAGE_VERSION "${PROJECT_VERSION}") # upstream_version
set(CPACK_DEBIAN_PACKAGE_RELEASE "1") # debian_revision (because this is a
                                      # non-native pkg)
set(PACKAGE_VERSION_REVISION "${CPACK_DEBIAN_PACKAGE_VERSION}-${CPACK_DEBIAN_PACKAGE_RELEASE}${DEBIAN_VERSION_SUFFIX}")

# Get architecture
execute_process(COMMAND dpkg "--print-architecture" OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE)
string(STRIP "${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}" CPACK_DEBIAN_PACKAGE_ARCHITECTURE)

## Package runtime component
set(CPACK_DEBIAN_PACKAGE_NAME "opencl-sdk")

set(CPACK_DEBIAN_BINARY_PACKAGE_NAME "${CPACK_DEBIAN_PACKAGE_NAME}")

# Package file name in deb format:
# <PackageName>_<VersionNumber>-<DebianRevisionNumber>_<DebianArchitecture>.deb
set(CPACK_DEBIAN_BINARY_FILE_NAME "${CPACK_DEBIAN_BINARY_PACKAGE_NAME}_${PACKAGE_VERSION_REVISION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb")

# Replacements
# ToDo
# set(CPACK_DEBIAN_BINARY_PACKAGE_DEPENDS "opencl-c-headers (>= 3.0~${PROJECT_VERSION}), opencl-clhpp-headers (>= 3.0~${PROJECT_VERSION}), khronos-opencl-loader-libopencl1 (>= 3.0~${PROJECT_VERSION}), khronos-opencl-loader-opencl-dev (>= 3.0~${PROJECT_VERSION})")
set(CPACK_DEBIAN_BINARY_PACKAGE_DEPENDS "opencl-c-headers, opencl-clhpp-headers, khronos-opencl-loader-libopencl1, khronos-opencl-loader-opencl-dev")
set(CPACK_DEBIAN_BINARY_PACKAGE_SECTION "libdevel")

# Package clinfo, if enabled
if(OPENCL_SDK_BUILD_CLINFO)
  list(APPEND CPACK_COMPONENTS_ALL "clinfo")
  set(CPACK_DEBIAN_CLINFO_PACKAGE_NAME "clinfo")
  set(CPACK_DEBIAN_CLINFO_FILE_NAME "clinfo_${PACKAGE_VERSION_REVISION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb")
  set(CPACK_DEBIAN_CLINFO_DESCRIPTION
"Query OpenCL system information
OpenCL (Open Computing Language) is a multivendor open standard for
general-purpose parallel programming of heterogeneous systems that include
CPUs, GPUs and other processors.
.
This package contains a tool that queries the capabilities of the available
OpenCL drivers.")
  set(CPACK_DEBIAN_CLINFO_PACKAGE_DEPENDS "libc6 (>= 2.14), khronos-opencl-loader-libopencl1 (>= 3.0~${CPACK_DEBIAN_PACKAGE_VERSION}) | libopencl1")
  set(CPACK_DEBIAN_CLINFO_PACKAGE_CONFLICTS "amd-clinfo, clinfo, fglrx-updates-core")
  set(CPACK_DEBIAN_CLINFO_PACKAGE_REPLACES "amd-clinfo, clinfo, fglrx-updates-core")
  set(CPACK_DEBIAN_CLINFO_PACKAGE_PROVIDES "clinfo")
  set(CPACK_DEBIAN_CLINFO_PACKAGE_SECTION "admin")
endif()
