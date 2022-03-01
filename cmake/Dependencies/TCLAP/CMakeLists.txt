cmake_minimum_required(VERSION 3.7)

project(TCLAP VERSION 1.2.2)

option(TCLAP_BUILD_EXAMPLES "Build TCLAP examples" ON)

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(TCLAP_BUILD_EXAMPLES)
add_subdirectory(examples)
endif()