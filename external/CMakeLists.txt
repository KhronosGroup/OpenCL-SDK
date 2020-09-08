if(SDK_EXTERNAL_UPDATE) # Tedious inversion of logic
  set(DISCONNECT OFF)
else()
  set(DISCONNECT ON)
  if(${CMAKE_VERSION} VERSION_LESS "3.8.0") # Bug: https://gitlab.kitware.com/cmake/cmake/issues/16428
    set(UPDATE_COMMAND_ARG [[UPDATE_COMMAND ""]])
  endif()
endif()

include(ExternalProject)

ExternalProject_Add(OpenCL-Headers
  GIT_REPOSITORY https://github.com/MathiasMagnus/OpenCL-Headers.git
  GIT_TAG pkgconfig
  GIT_SUBMODULES ""
  PREFIX ${PROJECT_BINARY_DIR}
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/OpenCL-Headers
  UPDATE_DISCONNECTED ${DISCONNECT}
  ${UPDATE_COMMAND_ARG}
  LIST_SEPARATOR |
  CMAKE_ARGS
    "-Wno-dev"
    "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
    "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"
  TEST_EXCLUDE_FROM_MAIN ON
)

add_library(OpenCL::Headers
  INTERFACE
  IMPORTED
  GLOBAL
)

target_include_directories(OpenCL::Headers
  INTERFACE
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_dependencies(OpenCL::Headers
  OpenCL-Headers
)

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/include)

ExternalProject_Add(OpenCL-CLHPP
  GIT_REPOSITORY https://github.com/MathiasMagnus/OpenCL-CLHPP.git
  GIT_TAG pkgconfig
  GIT_SUBMODULES ""
  PREFIX ${PROJECT_BINARY_DIR}
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/OpenCL-CLHPP
  UPDATE_DISCONNECTED ${DISCONNECT}
  ${UPDATE_COMMAND_ARG}
  LIST_SEPARATOR |
  CMAKE_ARGS
    "-Wno-dev"
    "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
    "-DCMAKE_PREFIX_PATH=${PROJECT_BINARY_DIR}|${CMAKE_PREFIX_PATH_ALT_SEP}"
    "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"
    "-DBUILD_DOCS=OFF"
    "-DBUILD_EXAMPLES=OFF"
    "-DBUILD_TESTS=OFF"
  TEST_EXCLUDE_FROM_MAIN ON
)

add_library(OpenCL::HeadersCpp
  INTERFACE
  IMPORTED
  GLOBAL
)

target_link_libraries(OpenCL::HeadersCpp
  INTERFACE
    OpenCL::Headers
)

add_dependencies(OpenCL::HeadersCpp
  OpenCL-CLHPP
)

ExternalProject_Add(OpenCL-ICD-Loader
  GIT_REPOSITORY https://github.com/MathiasMagnus/OpenCL-ICD-Loader.git
  GIT_TAG pkgconfig
  GIT_SUBMODULES ""
  PREFIX ${PROJECT_BINARY_DIR}
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/OpenCL-ICD-Loader
  UPDATE_DISCONNECTED ${DISCONNECT}
  ${UPDATE_COMMAND_ARG}
  LIST_SEPARATOR |
  CMAKE_ARGS
    "-Wno-dev"
    "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
    "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
    "-DCMAKE_PREFIX_PATH=${PROJECT_BINARY_DIR}|${CMAKE_PREFIX_PATH_ALT_SEP}"
    "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"
    "-DBUILD_TESTING=OFF"
  TEST_EXCLUDE_FROM_MAIN ON
  DEPENDS OpenCL-Headers
)

add_library(OpenCL::ICDLoader
  INTERFACE
  IMPORTED
  GLOBAL
)

target_link_libraries(OpenCL::ICDLoader
  INTERFACE
    OpenCL::Headers
    OpenCL
)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.13")
  target_link_directories(OpenCL::ICDLoader
    INTERFACE
      $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/lib>
      $<INSTALL_INTERFACE:lib>
  )
else()
  set_target_properties(OpenCL::ICDLoader
    PROPERTIES
      INTERFACE_LINK_DIRECTORIES ${PROJECT_BINARY_DIR}/lib
  )
endif()

add_dependencies(OpenCL::ICDLoader
  OpenCL-ICD-Loader
)