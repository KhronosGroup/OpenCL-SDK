find_package(OpenGL)

if(NOT OpenGL_FOUND)
  if(WIN32)
    message(FATAL_ERROR "OpenGL development files not found. Please install some version of the Windows SDK.")
  elseif(Linux)
    message(FATAL_ERROR "OpenGL development files not found. Please install them using your system package manager, for eg. libgl-dev")
  else(Apple)
    message(FATAL_ERROR "OpenGL development files not found.")
  endif()
endif()

if(NOT TARGET OpenGL::GLU)
  # GLU is a dependency of GLEW but it's not advertized as an OpenGL COMPONENT
  message(FATAL_ERROR "GLU is a dependency of GLEW but was not found.")
endif()