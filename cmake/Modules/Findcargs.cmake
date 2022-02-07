# - Find cargs
# Find the cargs headers and library
#
# cargs - INTERFACE library to link against
# cargs_FOUND       - True if cargs is found

if (TARGET cargs)
  # already in cache, be silent
  set (cargs_FIND_QUIETLY TRUE)
endif ()

# find the headers
find_path(
  cargs_INCLUDE_PATH cargs.h
  PATHS ${cargs_DIR}
  PATH_SUFFIXES
    inc
    include
)
# find the library
find_library(
  cargs_LIBRARY cargs
  PATHS ${cargs_DIR}
  PATH_SUFFIXES
    lib
)

# handle the QUIETLY and REQUIRED arguments and set cargs_FOUND to
# TRUE if all listed variables are TRUE
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (cargs
  "cargs (https://github.com/likle/cargs) not found. To self-host, set cargs_INCLUDE_PATH and cargs_LIBRARY to point to the headers and library respectively adding '-D cargs_INCLUDE_PATH=/path/to/cargs/include/dir -D cargs_LIBRARY/path/to/cargs/libcargs' to the cmake command."
  cargs_INCLUDE_PATH
  cargs_LIBRARY
)

if (cargs_FOUND)
  add_library(cargs INTERFACE)
  target_include_directories(cargs INTERFACE "${cargs_INCLUDE_PATH}")
  target_link_libraries(cargs INTERFACE "${cargs_LIBRARY}")
endif (cargs_FOUND)

mark_as_advanced(
  cargs_INCLUDE_PATH
  cargs_LIBRARY
)
