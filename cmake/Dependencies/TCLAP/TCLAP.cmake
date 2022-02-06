if(NOT DEPENDENCIES_FORCE_DOWNLOAD)
  find_package(TCLAP QUIET)
endif()

if(NOT TCLAP_FOUND OR NOT DEPENDENCIES_FORCE_DOWNLOAD)
  message(STATUS
    "TCLAP not found or DEPENDENCIES_FORCE_DOWNLOAD is ON. Fetching TCLAP."
  )
  include(FetchContent)
  FetchContent_Declare(
    tclap-external
    GIT_REPOSITORY      https://github.com/mirror/tclap.git
    GIT_TAG             v1.2.5 # 58c5c8ef24111072fc21fb723f8ab45d23395809
    UPDATE_COMMAND      ""
    PATCH_COMMAND       ""
    CONFIGURE_COMMAND   ""
    BUILD_COMMAND       ""
    INSTALL_COMMAND     ""
    TEST_COMMAND        ""
  )
  FetchContent_MakeAvailable(tclap-external)
  list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_BINARY_DIR}/_deps/tclap-external-src/include")
  find_package(TCLAP REQUIRED)
endif()