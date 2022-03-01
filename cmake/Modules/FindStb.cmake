# - Find Stb
# Find the Stb headers
#
# Stb_INCLUDE_DIR - where to find the TCLAP headers
# Stb_FOUND       - True if TCLAP is found

if (Stb_INCLUDE_DIR)
  # already in cache, be silent
  set (Stb_FIND_QUIETLY TRUE)
endif (Stb_INCLUDE_DIR)

# find the headers
find_path (Stb_INCLUDE_PATH stb_image.h
  PATHS
    ${Stb_DIR}
  PATH_SUFFIXES
    include
    include/stb
  )

# handle the QUIETLY and REQUIRED arguments and set Stb_FOUND to
# TRUE if all listed variables are TRUE
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (Stb
  "Stb (https://github.com/nothings/stb) not found. To self-host, set Stb_INCLUDE_PATH to point to the headers adding '-D Stb_INCLUDE_PATH=/path/to/stb' to the cmake command."
  Stb_INCLUDE_PATH
)

if (Stb_FOUND)
  set (Stb_INCLUDE_DIR ${Stb_INCLUDE_PATH})
endif (Stb_FOUND)

mark_as_advanced(Stb_INCLUDE_PATH)
