include(GenerateExportHeader)
include(GNUInstallDirs)

foreach(UTIL_LIB_NAME IN ITEMS Utils UtilsCpp)
  if(UTIL_LIB_NAME STREQUAL Utils)
    set(UTIL_LIB_SOURCES src/Utils/Utils.c)
    set(UTIL_LIB_DEPS
      OpenCL::Headers
      $<$<BOOL:${HAVE_LIB_M}>:m>
    )
    set(UTIL_CL_VERSION_MACRO_NAME CL_TARGET_OPENCL_VERSION)
  elseif(UTIL_LIB_NAME STREQUAL UtilsCpp)
    set(UTIL_LIB_SOURCES src/Utils/Utils.cpp)
    set(UTIL_LIB_DEPS
      OpenCL::HeadersCpp
      OpenCL::Utils
    )
    set(UTIL_CL_VERSION_MACRO_NAME CL_HPP_TARGET_OPENCL_VERSION)
  else()
    message(FATAL_ERROR "Unkown Util flavor")
  endif()
  set(UTIL_LIB_TARGET OpenCL${UTIL_LIB_NAME})
  add_library(${UTIL_LIB_TARGET} ${UTIL_LIB_SOURCES})
  add_library(OpenCL::${UTIL_LIB_NAME} ALIAS ${UTIL_LIB_TARGET})

  string(TOUPPER ${UTIL_LIB_NAME} UPPER_UTIL_LIB_NAME)
  generate_export_header(${UTIL_LIB_TARGET}
    EXPORT_MACRO_NAME ${UPPER_UTIL_LIB_NAME}_EXPORT
    EXPORT_FILE_NAME OpenCL${UTIL_LIB_NAME}_Export.h
  )

  target_include_directories(${UTIL_LIB_TARGET}
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  )
  target_link_libraries(${UTIL_LIB_TARGET}
    PUBLIC
      ${UTIL_LIB_DEPS}
      OpenCL::OpenCL
  )
  target_compile_definitions(${UTIL_LIB_TARGET}
    PRIVATE
      ${UTIL_CL_VERSION_MACRO_NAME}=300
    PUBLIC
      CL_HPP_ENABLE_EXCEPTIONS
  )

  set_target_properties(${UTIL_LIB_TARGET}
    PROPERTIES
      ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}
      LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}
      RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}
      INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
      FOLDER "Libraries/${UTIL_LIB_NAME}"
      EXPORT_NAME ${UTIL_LIB_NAME}
      DEBUG_POSTFIX d
  )

  install(
    TARGETS ${UTIL_LIB_TARGET}
    EXPORT OpenCL${UTIL_LIB_NAME}Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
  install(
    DIRECTORY include/CL/Utils
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CL
  )
  install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/OpenCL${UTIL_LIB_NAME}_Export.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CL/Utils
  )
  export(
    EXPORT OpenCL${UTIL_LIB_NAME}Targets
    FILE ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/cmake/OpenCL${UTIL_LIB_NAME}/OpenCL${UTIL_LIB_NAME}Targets.cmake
    NAMESPACE OpenCL::
  )
  file(
    WRITE ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/cmake/OpenCL${UTIL_LIB_NAME}/OpenCL${UTIL_LIB_NAME}Config.cmake
    "include(\"\${CMAKE_CURRENT_LIST_DIR}/OpenCL${UTIL_LIB_NAME}Targets.cmake\")"
  )

  set(config_package_location ${CMAKE_INSTALL_DATADIR}/cmake/OpenCL${UTIL_LIB_NAME})
  install(
    EXPORT OpenCL${UTIL_LIB_NAME}Targets
    FILE OpenCL${UTIL_LIB_NAME}Targets.cmake
    NAMESPACE OpenCL::
    DESTINATION ${config_package_location}
  )
  install(
    FILES ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/cmake/OpenCL${UTIL_LIB_NAME}/OpenCL${UTIL_LIB_NAME}Config.cmake
    DESTINATION ${config_package_location}
  )

  unset(CMAKE_SIZEOF_VOID_P)
  include(CMakePackageConfigHelpers)
  write_basic_package_version_file(
    ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/cmake/OpenCL${UTIL_LIB_NAME}/OpenCL${UTIL_LIB_NAME}ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
  )
  install(
    FILES ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/cmake/OpenCL${UTIL_LIB_NAME}/OpenCL${UTIL_LIB_NAME}ConfigVersion.cmake
    DESTINATION ${config_package_location}
  )
endforeach()

if(OPENCL_SDK_BUILD_SAMPLES)
  foreach(SDK_LIB_NAME IN ITEMS SDK SDKCpp)
    if(SDK_LIB_NAME STREQUAL SDK)
      set(SDK_LIB_SOURCES
        src/SDK/CLI.c
        src/SDK/Context.c
        src/SDK/Image.c
        src/SDK/Random.c
      )
      set(SDK_LIB_DEPS
        OpenCL::Headers
        OpenCL::Utils
        cargs
      )
      set(SDK_LIB_INCLUDES
        ${Stb_INCLUDE_DIR}
      )
      set(SDK_CL_VERSION_MACRO_NAME CL_TARGET_OPENCL_VERSION)
    elseif(SDK_LIB_NAME STREQUAL SDKCpp)
      set(SDK_LIB_SOURCES
        src/SDK/CLI.cpp
        src/SDK/Image.cpp
        $<$<BOOL:${OPENCL_SDK_BUILD_OPENGL_SAMPLES}>:src/SDK/InteropContext.cpp>
        $<$<BOOL:${OPENCL_SDK_BUILD_OPENGL_SAMPLES}>:src/SDK/InteropWindow.cpp>
      )
      set(SDK_LIB_DEPS
        OpenCL::HeadersCpp
        OpenCL::UtilsCpp
        $<$<BOOL:${OPENCL_SDK_BUILD_OPENGL_SAMPLES}>:OpenGL::GL>
        $<$<BOOL:${OPENCL_SDK_BUILD_OPENGL_SAMPLES}>:GLEW::GLEW>
        $<$<AND:$<BOOL:${OPENCL_SDK_BUILD_OPENGL_SAMPLES}>,$<PLATFORM_ID:Linux>>:OpenGL::GLU>
        $<$<BOOL:${OPENCL_SDK_BUILD_OPENGL_SAMPLES}>:sfml-system>
        $<$<BOOL:${OPENCL_SDK_BUILD_OPENGL_SAMPLES}>:sfml-window>
        $<$<BOOL:${OPENCL_SDK_BUILD_OPENGL_SAMPLES}>:sfml-graphics>
      )
      set(SDK_LIB_INCLUDES
        ${TCLAP_INCLUDE_DIR}
        ${Stb_INCLUDE_DIR}
      )
      set(SDK_CL_VERSION_MACRO_NAME CL_HPP_TARGET_OPENCL_VERSION)
    else()
      message(FATAL_ERROR "Unkown SDK flavor")
    endif()

    set(SDK_LIB_TARGET OpenCL${SDK_LIB_NAME})
    add_library(${SDK_LIB_TARGET} ${SDK_LIB_SOURCES})
    add_library(OpenCL::${SDK_LIB_NAME} ALIAS ${SDK_LIB_TARGET})

    string(TOUPPER ${SDK_LIB_NAME} UPPER_SDK_LIB_NAME)
    generate_export_header(${SDK_LIB_TARGET}
      EXPORT_MACRO_NAME ${UPPER_SDK_LIB_NAME}_EXPORT
      EXPORT_FILE_NAME OpenCL${SDK_LIB_NAME}_Export.h
    )
    configure_file(
      include/CL/SDK/OpenCLSDK_Config.in.h
      ${CMAKE_CURRENT_BINARY_DIR}/OpenCLSDK_Config.h
    )

    target_include_directories(${SDK_LIB_TARGET}
      PUBLIC
        ${SDK_LIB_INCLUDES}
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    )
    target_link_libraries(${SDK_LIB_TARGET}
      PUBLIC
        ${SDK_LIB_DEPS}
        OpenCL::OpenCL
    )
    target_compile_definitions(${SDK_LIB_TARGET}
      PRIVATE
        ${SDK_CL_VERSION_MACRO_NAME}=300
      PUBLIC
        CL_HPP_ENABLE_EXCEPTIONS
    )

    set_target_properties(${SDK_LIB_TARGET}
      PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}
        LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
        FOLDER "Libraries/${SDK_LIB_TARGET}"
    )
    install(
      TARGETS ${SDK_LIB_TARGET}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    # Copying DLLs the samples depend on only makes sense on Wnidows. On *nix
    # OSes we rely on RPATH. We can only do this with sufficiently new CMake.
    if(
      OPENCL_SDK_BUILD_OPENGL_SAMPLES AND
      CMAKE_VERSION VERSION_GREATER_EQUAL 3.21 AND
      CMAKE_SYSTEM_NAME MATCHES Windows
    )
      foreach(DEP IN ITEMS
        GLEW::GLEW
        sfml-window
        sfml-graphics
        sfml-system
      )
        if(TARGET ${DEP})
          get_target_property(DEP_TYPE ${DEP} TYPE)
          if(${DEP_TYPE} STREQUAL SHARED_LIBRARY)
            install(
              IMPORTED_RUNTIME_ARTIFACTS
                ${DEP}
              RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            )
          endif()
        endif()
      endforeach()
    endif()
  endforeach()
endif(OPENCL_SDK_BUILD_SAMPLES)

# Override the default install behavior for the extension loader.  We always want to
# build the install target for the OpenCL SDK.
option (OPENCL_EXTENSION_LOADER_INSTALL         "Generate Installation Target" ON)
add_subdirectory(src/Extensions)
