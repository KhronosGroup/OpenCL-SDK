# stb
file(GLOB_RECURSE _HDRS "*.h")

add_library(stb INTERFACE)

target_include_directories(stb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
