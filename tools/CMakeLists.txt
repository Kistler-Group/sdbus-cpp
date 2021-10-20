#-------------------------------
# PROJECT INFORMATION
#-------------------------------

cmake_minimum_required(VERSION 3.5)

project(sdbus-c++-tools VERSION 1.0.0)

include(GNUInstallDirs)

#-------------------------------
# PERFORMING CHECKS
#-------------------------------

find_package(EXPAT REQUIRED)

#-------------------------------
# SOURCE FILES CONFIGURATION
#-------------------------------

set(SDBUSCPP_XML2CPP_SRCS
    xml2cpp-codegen/xml2cpp.cpp
    xml2cpp-codegen/xml.h
    xml2cpp-codegen/xml.cpp
    xml2cpp-codegen/generator_utils.h
    xml2cpp-codegen/generator_utils.cpp
    xml2cpp-codegen/BaseGenerator.h
    xml2cpp-codegen/BaseGenerator.cpp
    xml2cpp-codegen/AdaptorGenerator.h
    xml2cpp-codegen/AdaptorGenerator.cpp
    xml2cpp-codegen/ProxyGenerator.h
    xml2cpp-codegen/ProxyGenerator.cpp)

#-------------------------------
# GENERAL COMPILER CONFIGURATION
#-------------------------------

set(CMAKE_CXX_STANDARD 14)

#----------------------------------
# EXECUTABLE BUILD INFORMATION
#----------------------------------

add_executable(sdbus-c++-xml2cpp ${SDBUSCPP_XML2CPP_SRCS})
target_link_libraries (sdbus-c++-xml2cpp ${EXPAT_LIBRARIES})
target_include_directories(sdbus-c++-xml2cpp PRIVATE ${EXPAT_INCLUDE_DIRS})

#----------------------------------
# INSTALLATION
#----------------------------------

install(TARGETS sdbus-c++-xml2cpp EXPORT sdbus-c++-tools-targets DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT dev)

#----------------------------------
# CMAKE CONFIG & PACKAGE CONFIG
#----------------------------------

include(CMakePackageConfigHelpers)

install(EXPORT sdbus-c++-tools-targets
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sdbus-c++-tools
        NAMESPACE SDBusCpp::
        COMPONENT dev)

configure_package_config_file(cmake/sdbus-c++-tools-config.cmake.in cmake/sdbus-c++-tools-config.cmake
                              INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sdbus-c++)
write_basic_package_version_file(cmake/sdbus-c++-tools-config-version.cmake COMPATIBILITY SameMajorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cmake/sdbus-c++-tools-config.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/cmake/sdbus-c++-tools-config-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sdbus-c++-tools
        COMPONENT dev)

configure_file(pkgconfig/sdbus-c++-tools.pc.in pkgconfig/sdbus-c++-tools.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/sdbus-c++-tools.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig COMPONENT dev)
