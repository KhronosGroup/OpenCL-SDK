if(CMAKE_SYSTEM_NAME MATCHES Linux) # TODO: Add EGL support
  # OpenGL doesn't explicitly depend on X11 (as of CMake v3.2) but we'll need it
  find_package(X11)

  if(NOT X11_FOUND)
    message(FATAL_ERROR "X11 development files not found. Please install them using your system package manager, for eg. libx11-dev")
  endif()
endif()