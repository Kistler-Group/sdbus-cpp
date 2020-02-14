#-------------------------------
# PROJECT INFORMATION
#-------------------------------

cmake_minimum_required(VERSION 3.13)

project(sdbus-c++ VERSION 0.8.1 LANGUAGES C CXX)

include(GNUInstallDirs) # Installation directories for `install` command and pkgconfig file

#-------------------------------
# PERFORMING CHECKS & PREPARING THE DEPENDENCIES
#-------------------------------

option(BUILD_LIBSYSTEMD "Build libsystemd static library and incorporate it into libsdbus-c++" OFF)

if(NOT BUILD_LIBSYSTEMD)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(Systemd IMPORTED_TARGET GLOBAL libsystemd>=236)
    if(NOT TARGET PkgConfig::Systemd)
        message(FATAL_ERROR "libsystemd of version at least 236 is required, but was not found "
                            "(you may turn BUILD_LIBSYSTEMD on for sdbus-c++ to try downloading "
                            "and building libsystemd in as part of sdbus-c++ during configuration)")
    endif()
    add_library(Systemd::Libsystemd ALIAS PkgConfig::Systemd)
    set(LIBSYSTEMD_VERSION ${Systemd_VERSION})
else()
    # Build static libsystemd library as an external project
    include(cmake/LibsystemdExternalProject.cmake)
endif()

#-------------------------------
# SOURCE FILES CONFIGURATION
#-------------------------------

set(SDBUSCPP_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SDBUSCPP_INCLUDE_SUBDIR sdbus-c++)
set(SDBUSCPP_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/${SDBUSCPP_INCLUDE_SUBDIR})

set(SDBUSCPP_CPP_SRCS
    ${SDBUSCPP_SOURCE_DIR}/Connection.cpp
    ${SDBUSCPP_SOURCE_DIR}/Error.cpp
    ${SDBUSCPP_SOURCE_DIR}/Message.cpp
    ${SDBUSCPP_SOURCE_DIR}/Object.cpp
    ${SDBUSCPP_SOURCE_DIR}/Proxy.cpp
    ${SDBUSCPP_SOURCE_DIR}/Types.cpp
    ${SDBUSCPP_SOURCE_DIR}/Flags.cpp
    ${SDBUSCPP_SOURCE_DIR}/VTableUtils.c
    ${SDBUSCPP_SOURCE_DIR}/SdBus.cpp)

set(SDBUSCPP_HDR_SRCS
    ${SDBUSCPP_SOURCE_DIR}/Connection.h
    ${SDBUSCPP_SOURCE_DIR}/IConnection.h
    ${SDBUSCPP_SOURCE_DIR}/MessageUtils.h
    ${SDBUSCPP_SOURCE_DIR}/Object.h
    ${SDBUSCPP_SOURCE_DIR}/Proxy.h
    ${SDBUSCPP_SOURCE_DIR}/ScopeGuard.h
    ${SDBUSCPP_SOURCE_DIR}/VTableUtils.h
    ${SDBUSCPP_SOURCE_DIR}/SdBus.h
    ${SDBUSCPP_SOURCE_DIR}/ISdBus.h)

set(SDBUSCPP_PUBLIC_HDRS
    ${SDBUSCPP_INCLUDE_DIR}/ConvenienceApiClasses.h
    ${SDBUSCPP_INCLUDE_DIR}/ConvenienceApiClasses.inl
    ${SDBUSCPP_INCLUDE_DIR}/Error.h
    ${SDBUSCPP_INCLUDE_DIR}/IConnection.h
    ${SDBUSCPP_INCLUDE_DIR}/AdaptorInterfaces.h
    ${SDBUSCPP_INCLUDE_DIR}/ProxyInterfaces.h
    ${SDBUSCPP_INCLUDE_DIR}/StandardInterfaces.h
    ${SDBUSCPP_INCLUDE_DIR}/IObject.h
    ${SDBUSCPP_INCLUDE_DIR}/IProxy.h
    ${SDBUSCPP_INCLUDE_DIR}/Message.h
    ${SDBUSCPP_INCLUDE_DIR}/MethodResult.h
    ${SDBUSCPP_INCLUDE_DIR}/Types.h
    ${SDBUSCPP_INCLUDE_DIR}/TypeTraits.h
    ${SDBUSCPP_INCLUDE_DIR}/Flags.h
    ${SDBUSCPP_INCLUDE_DIR}/sdbus-c++.h)

set(SDBUSCPP_SRCS ${SDBUSCPP_CPP_SRCS} ${SDBUSCPP_HDR_SRCS} ${SDBUSCPP_PUBLIC_HDRS})

#-------------------------------
# GENERAL COMPILER CONFIGURATION
#-------------------------------

set(CMAKE_CXX_STANDARD 17)

#----------------------------------
# LIBRARY BUILD INFORMATION
#----------------------------------

set(SDBUSCPP_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(SDBUSCPP_VERSION "${PROJECT_VERSION}")

# We promote BUILD_SHARED_LIBS flags to (global) option only if we are the main project
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    option(BUILD_SHARED_LIBS "Build shared libraries (.so) instead of static ones (.a)" ON)
endif()

# Having an object target allows unit tests to reuse already built sources without re-building
add_library(sdbus-c++-objlib OBJECT ${SDBUSCPP_SRCS})
target_compile_definitions(sdbus-c++-objlib PRIVATE BUILD_LIB=1 LIBSYSTEMD_VERSION=${LIBSYSTEMD_VERSION})
target_include_directories(sdbus-c++-objlib PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                                                   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)
if(DEFINED BUILD_SHARED_LIBS)
    set_target_properties(sdbus-c++-objlib PROPERTIES POSITION_INDEPENDENT_CODE ${BUILD_SHARED_LIBS})
endif()
if(BUILD_LIBSYSTEMD)
    add_dependencies(sdbus-c++-objlib LibsystemdBuildProject)
endif()
target_link_libraries(sdbus-c++-objlib PUBLIC Systemd::Libsystemd)

add_library(sdbus-c++)
target_include_directories(sdbus-c++ PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                                            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
set_target_properties(sdbus-c++
                      PROPERTIES PUBLIC_HEADER "${SDBUSCPP_PUBLIC_HDRS}"
                                 VERSION "${SDBUSCPP_VERSION}"
                                 SOVERSION "${SDBUSCPP_VERSION_MAJOR}"
                                 OUTPUT_NAME "sdbus-c++")
target_link_libraries(sdbus-c++ PRIVATE sdbus-c++-objlib)

#----------------------------------
# INSTALLATION
#----------------------------------
set(EXPORT_SET sdbus-c++)
if(NOT BUILD_SHARED_LIBS)
    list(APPEND EXPORT_SET "sdbus-c++-objlib")
endif()

install(TARGETS ${EXPORT_SET}
        EXPORT sdbus-c++-targets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT libraries
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT libraries
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT static_libraries
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${SDBUSCPP_INCLUDE_SUBDIR} COMPONENT dev)

#----------------------------------
# TESTS
#----------------------------------

option(BUILD_TESTS "Build and install tests (default OFF)" OFF)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/tests")
endif()

#----------------------------------
# UTILS
#----------------------------------

option(BUILD_CODE_GEN "Build and install interface stub code generator (default OFF)" OFF)

if(BUILD_CODE_GEN)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/tools")
endif()

#----------------------------------
# DOCUMENTATION
#----------------------------------

option(BUILD_DOC "Build documentation for sdbus-c++" ON)

if(BUILD_DOC)
    option(BUILD_DOXYGEN_DOC "Build doxygen documentation for sdbus-c++ API" OFF)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/docs")
    install(FILES README README.md NEWS COPYING ChangeLog AUTHORS DESTINATION ${CMAKE_INSTALL_DOCDIR})
endif()

#----------------------------------
# CMAKE CONFIG & PACKAGE CONFIG
#----------------------------------

include(CMakePackageConfigHelpers)

install(EXPORT sdbus-c++-targets
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sdbus-c++
        NAMESPACE SDBusCpp::
        COMPONENT dev)

configure_package_config_file(cmake/sdbus-c++-config.cmake.in cmake/sdbus-c++-config.cmake
                              INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sdbus-c++)
write_basic_package_version_file(cmake/sdbus-c++-config-version.cmake COMPATIBILITY SameMajorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cmake/sdbus-c++-config.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/cmake/sdbus-c++-config-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sdbus-c++
        COMPONENT dev)

configure_file(pkgconfig/sdbus-c++.pc.in pkgconfig/sdbus-c++.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/sdbus-c++.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig COMPONENT dev)
