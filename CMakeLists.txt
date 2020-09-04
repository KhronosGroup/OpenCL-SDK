cmake_minimum_required(VERSION 3.11) # specify include directories for imported target
cmake_policy(VERSION 3.11...3.18)

project(OpenCL-SDK)

option(SDK_SUBMODULE_DOWNLOAD "Download SDK Submodules as part of the build" ON)
option(SDK_EXTERNAL_UPDATE "Update step for external dependencies" ON)

add_subdirectory(external)

add_subdirectory(samples)
