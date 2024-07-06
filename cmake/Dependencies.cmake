# Save global flags and strip diagnostics locally
set(USER_C_FLAGS ${CMAKE_C_FLAGS})
set(USER_CXX_FLAGS ${CMAKE_CXX_FLAGS})
if(DEFINED BUILD_SHARED_LIBS)
  set(USER_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
endif()
set(USER_ROCM_WARN_TOOLCHAIN_VAR ${ROCM_WARN_TOOLCHAIN_VAR})

set(ROCM_WARN_TOOLCHAIN_VAR OFF CACHE BOOL "")
# Turn off C warnings and errors for all warnings in dependencies
separate_arguments(C_FLAGS_LIST NATIVE_COMMAND ${CMAKE_C_FLAGS})
list(REMOVE_ITEM C_FLAGS_LIST /WX -Werror -Werror=pendantic -pedantic-errors)
if(MSVC)
  list(FILTER C_FLAGS_LIST EXCLUDE REGEX "/[Ww]([0-4]?)(all)?") # Remove MSVC warning flags
  list(APPEND C_FLAGS_LIST /w)
else()
  list(FILTER C_FLAGS_LIST EXCLUDE REGEX "-W(all|extra|everything)") # Remove GCC/LLVM flags
  list(APPEND C_FLAGS_LIST -w)
endif()
list(JOIN C_FLAGS_LIST " " CMAKE_C_FLAGS)
# Turn off C++ warnings and errors for all warnings in dependencies
separate_arguments(CXX_FLAGS_LIST NATIVE_COMMAND ${CMAKE_CXX_FLAGS})
list(REMOVE_ITEM CXX_FLAGS_LIST /WX -Werror -Werror=pendantic -pedantic-errors)
if(MSVC)
  list(FILTER CXX_FLAGS_LIST EXCLUDE REGEX "/[Ww]([0-4]?)(all)?") # Remove MSVC warning flags
  list(APPEND CXX_FLAGS_LIST /w)
else()
  list(FILTER CXX_FLAGS_LIST EXCLUDE REGEX "-W(all|extra|everything)") # Remove GCC/LLVM flags
  list(APPEND CXX_FLAGS_LIST -w)
endif()
list(JOIN CXX_FLAGS_LIST " " CMAKE_CXX_FLAGS)
# Don't build client dependencies as shared
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Global flag to cause add_library() to create shared libraries if on." FORCE)

# Fetch dependencies
if(OPENCL_SDK_BUILD_SAMPLES)
  foreach(DEP IN ITEMS cargs TCLAP Stb)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/Dependencies/${DEP}")
    include(${DEP})
  endforeach()

  if(OPENCL_SDK_BUILD_OPENGL_SAMPLES)
    foreach(DEP IN ITEMS X11 glm OpenGL GLEW Freetype SFML)
      list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/Dependencies/${DEP}")
      include(${DEP})
    endforeach()
  endif(OPENCL_SDK_BUILD_OPENGL_SAMPLES)
endif(OPENCL_SDK_BUILD_SAMPLES)

if(OPENCL_SDK_BUILD_CLINFO)
  include("${CMAKE_CURRENT_LIST_DIR}/Dependencies/clinfo/clinfo.cmake")
endif()

# Restore user global state
set(CMAKE_C_FLAGS ${USER_C_FLAGS})
set(CMAKE_CXX_FLAGS ${USER_CXX_FLAGS})
if(DEFINED USER_BUILD_SHARED_LIBS)
  set(BUILD_SHARED_LIBS ${USER_BUILD_SHARED_LIBS})
else()
  unset(BUILD_SHARED_LIBS CACHE )
endif()
