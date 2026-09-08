cmake_minimum_required(VERSION 3.16)

execute_process(COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" "${CMAKE_CURRENT_BINARY_DIR}/CMakeLists.txt" RESULT_VARIABLE RESULT_VAR)
if (NOT "${RESULT_VAR}" EQUAL "0")
    message(FATAL_ERROR "Could not copy file with CMake")
endif()
