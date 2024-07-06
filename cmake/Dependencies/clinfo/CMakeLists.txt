include(GNUInstallDirs)

add_executable(clinfo src/clinfo.c)
target_include_directories(clinfo PRIVATE src)
target_link_libraries(clinfo PRIVATE OpenCL::OpenCL)
target_compile_definitions(clinfo PRIVATE
    CL_TARGET_OPENCL_VERSION=300
    CL_NO_NON_ICD_DISPATCH_EXTENSION_PROTOTYPES
)
add_test(NAME clinfo COMMAND $<TARGET_FILE:clinfo>)

# if (APPLE)
#     set(RBASE "@loader_path")
# else ()
#     set(RBASE "$ORIGIN")
# endif ()
# file(RELATIVE_PATH REL_LIB_DIR
#     "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}"
#     "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}"
# )
# set_target_properties(clinfo PROPERTIES INSTALL_RPATH "${RBASE}/${REL_LIB_DIR}")
install(
    TARGETS clinfo
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    COMPONENT clinfo
)

# Generating and installing the documentation
find_program(gzip_program gzip)
if (EXISTS "${gzip_program}")
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/clinfo.1.gz"
        COMMAND gzip -c "${CMAKE_CURRENT_SOURCE_DIR}/man1/clinfo.1" > "${CMAKE_CURRENT_BINARY_DIR}/clinfo.1.gz"
        MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/man1/clinfo.1"
    )
    add_custom_target(
        clinfo_manpage
        ALL
        DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/clinfo.1.gz"
    )
    install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/clinfo.1.gz"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/man/man1"
        COMPONENT clinfo
    )
else()
    message(WARNING "Could not find gzip. Skipping the generation of documentation for clinfo")
endif()
