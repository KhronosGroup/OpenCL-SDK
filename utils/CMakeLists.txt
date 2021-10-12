foreach(UTIL_LIB_NAME IN ITEMS SDKUtils SDKUtilsCpp)
  if(UTIL_LIB_NAME STREQUAL SDKUtils)
    set(UTIL_LIB_SOURCES src/SDKUtils.c)
    set(UTIL_LIB_DEPS OpenCL::Headers)
  elseif(UTIL_LIB_NAME STREQUAL SDKUtilsCpp)
    set(UTIL_LIB_SOURCES src/SDKUtils.cpp)
    set(UTIL_LIB_DEPS OpenCL::HeadersCpp)
  else()
    message(FATAL_ERROR "Unkown SDKUtil flavor")
  endif()
  #add_library(${UTIL_LIB_NAME} ${UTIL_LIB_SOURCES})
  add_library(${UTIL_LIB_NAME} INTERFACE)
  add_library(OpenCL::${UTIL_LIB_NAME} ALIAS ${UTIL_LIB_NAME})

  target_include_directories(${UTIL_LIB_NAME}
    INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
  )
  target_link_libraries(${UTIL_LIB_NAME}
    INTERFACE
      ${UTIL_LIB_DEPS}
      OpenCL::OpenCL
  )
  #target_compile_definitions(${UTIL_LIB_NAME}
  #  INTERFACE
  #    CL_HPP_TARGET_OPENCL_VERSION=300
  #)

  install(
    TARGETS ${UTIL_LIB_NAME}
    EXPORT OpenCL${UTIL_LIB_NAME}Targets
  )

  include(GNUInstallDirs)

  install(
    DIRECTORY include/CL
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
  export(
    EXPORT OpenCL${UTIL_LIB_NAME}Targets
    FILE ${PROJECT_BINARY_DIR}/OpenCL${UTIL_LIB_NAME}/OpenCL${UTIL_LIB_NAME}Targets.cmake
    NAMESPACE OpenCL::
  )
  file(
    WRITE ${PROJECT_BINARY_DIR}/OpenCL${UTIL_LIB_NAME}/OpenCL${UTIL_LIB_NAME}Config.cmake
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
    FILES ${CMAKE_CURRENT_BINARY_DIR}/OpenCL${UTIL_LIB_NAME}/OpenCL${UTIL_LIB_NAME}Config.cmake
    DESTINATION ${config_package_location}
  )

  unset(CMAKE_SIZEOF_VOID_P)
  include(CMakePackageConfigHelpers)
  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/OpenCL${UTIL_LIB_NAME}/OpenCL${UTIL_LIB_NAME}ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
  )
  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/OpenCL${UTIL_LIB_NAME}/OpenCL${UTIL_LIB_NAME}ConfigVersion.cmake
    DESTINATION ${config_package_location}
  )
endforeach()