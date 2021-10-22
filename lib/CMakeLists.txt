include(GenerateExportHeader)
include(GNUInstallDirs)

foreach(UTIL_LIB_NAME IN ITEMS Utils UtilsCpp)
  if(UTIL_LIB_NAME STREQUAL Utils)
    set(UTIL_LIB_SOURCES src/Utils/Utils.c)
    set(UTIL_LIB_DEPS
      OpenCL::Headers
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
    message(FATAL_ERROR "Unkown SDK flavor")
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
  )

  set_target_properties(${UTIL_LIB_TARGET}
    PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}
      FOLDER "Libraries/${UTIL_LIB_NAME}"
  )

  install(
    TARGETS ${UTIL_LIB_TARGET}
    EXPORT OpenCL${UTIL_LIB_NAME}Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
  install(
    DIRECTORY include/CL
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
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