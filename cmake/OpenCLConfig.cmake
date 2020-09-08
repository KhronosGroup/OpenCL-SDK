if(NOT OpenCL_FIND_COMPONENTS)
  set(OpenCL_FIND_COMPONENTS Headers ICDLoader)
endif()

if(OpenCL_FIND_REQUIRED)
  set(_OpenCL_FIND_PARTS_REQUIRED REQUIRED)
endif()
if(OpenCL_FIND_QUIETLY)
  set(_OpenCL_FIND_PARTS_QUIET QUIET)
endif()

get_filename_component(_opencl_install_prefix "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

set(_OpenCL_NOTFOUND_MESSAGE)

foreach(module ${OpenCL_FIND_COMPONENTS})
  find_package(OpenCL${module}
      ${_OpenCL_FIND_PARTS_QUIET}
      ${_OpenCL_FIND_PARTS_REQUIRED}
      PATHS ${_opencl_install_prefix} NO_DEFAULT_PATH
  )
  if(OpenCL${module}_FOUND)
    if(NOT OpenCL::${module})
      if(${module} MATCHES ICDLoader)
        add_library(OpenCL::${module} ALIAS OpenCL::OpenCL)
      else()
        add_library(OpenCL::${module} ALIAS OpenCL::OpenCL${module})
      endif()
    endif()
  else()
    if(OpenCL_FIND_REQUIRED_${module})
      set(_OpenCL_NOTFOUND_MESSAGE "${_OpenCL_NOTFOUND_MESSAGE}OpenCL component \"${module}\" config file could not be found using find_package(OpenCL${module} PATHS ${_opencl_install_prefix})\n")
    elseif(NOT OpenCL_FIND_QUIETLY)
      message(WARNING "${_OpenCL_NOTFOUND_MESSAGE}OpenCL component \"${module}\" config file could not be found using find_package(OpenCL${module} PATHS ${_opencl_install_prefix})\n")
    endif()
  endif()
endforeach()

if(_OpenCL_NOTFOUND_MESSAGE)
  set(OpenCL_NOT_FOUND_MESSAGE "${_OpenCL_NOTFOUND_MESSAGE}")
  set(OpenCL_FOUND FALSE)
endif()