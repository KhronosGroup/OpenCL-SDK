# stb
file(GLOB_RECURSE _HDRS "*.h")

add_library(stb INTERFACE)
set_target_properties(stb PROPERTIES LINKER_LANGUAGE C)

target_include_directories(stb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
