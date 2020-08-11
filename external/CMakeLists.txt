if(SDK_EXTERNAL_UPDATE) # Tedious inversion of logic
  set(DISCONNECT OFF)
else()
  set(DISCONNECT ON)
endif()

include(ExternalProject)

ExternalProject_Add(OpenCL-Headers
  GIT_REPOSITORY https://github.com/MathiasMagnus/OpenCL-Headers.git
  GIT_TAG origin/pkgconfig
  GIT_SUBMODULES ""
  PREFIX ${PROJECT_BINARY_DIR}
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/OpenCL-Headers
  UPDATE_DISCONNECTED ${DISCONNECT}
  #UPDATE_COMMAND ""
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
  GIT_TAG origin/pkgconfig
  GIT_SUBMODULES ""
  PREFIX ${PROJECT_BINARY_DIR}
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/OpenCL-CLHPP
  UPDATE_DISCONNECTED ${DISCONNECT}
  #UPDATE_COMMAND ""
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
  GIT_TAG origin/pkgconfig
  GIT_SUBMODULES ""
  PREFIX ${PROJECT_BINARY_DIR}
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/OpenCL-ICD-Loader
  UPDATE_DISCONNECTED ${DISCONNECT}
  #UPDATE_COMMAND ""
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

add_library(OpenCL::ICD
  INTERFACE
  IMPORTED
  GLOBAL
)

target_link_libraries(OpenCL::ICD
  INTERFACE
    OpenCL::Headers
    OpenCL
)

target_link_directories(OpenCL::ICD
  INTERFACE
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/lib>
    $<INSTALL_INTERFACE:lib>
)

add_dependencies(OpenCL::ICD
  OpenCL-ICD-Loader
)