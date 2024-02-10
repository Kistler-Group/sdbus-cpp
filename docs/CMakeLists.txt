# Building doxygen documentation

find_package(Doxygen)

if(BUILD_DOXYGEN_DOC)
    if(DOXYGEN_FOUND)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

        add_custom_target(doc
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM)

        # workaround bug https://github.com/doxygen/doxygen/pull/6787
        add_custom_command(TARGET doc POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/sdbus-c++-class-diagram.png ${CMAKE_CURRENT_BINARY_DIR}/html/.)
    else()
        message(WARNING "Documentation enabled, but Doxygen cannot be found")
    endif()
endif()

if(SDBUSCPP_INSTALL)
    install(FILES sdbus-c++-class-diagram.png
                  sdbus-c++-class-diagram.uml
                  systemd-dbus-config.md
                  using-sdbus-c++.md
            DESTINATION ${CMAKE_INSTALL_DOCDIR} COMPONENT sdbus-c++-doc)

    if (BUILD_DOXYGEN_DOC AND DOXYGEN_FOUND)
        install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION ${CMAKE_INSTALL_DOCDIR} OPTIONAL COMPONENT sdbus-c++-doc)
    endif()
endif()
