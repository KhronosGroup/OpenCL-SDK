# cargs
add_library(cargs STATIC ${CMAKE_CURRENT_SOURCE_DIR}/cargs.c)

target_include_directories(cargs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(cargs
  PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/library
    POSITION_INDEPENDENT_CODE ON
    C_STANDARD 99
)
