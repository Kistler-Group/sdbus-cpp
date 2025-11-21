#-------------------------------
# PROJECT INFORMATION
#-------------------------------

cmake_minimum_required(VERSION 3.14)

project(sdbus-c++ VERSION 2.2.1 LANGUAGES CXX C)

include(GNUInstallDirs) # Installation directories for `install` command and pkgconfig file

# -------------------------------
# CONFIGURATION OPTIONS
# -------------------------------

option(SDBUSCPP_BUILD_LIBSYSTEMD "Fetch & build libsystemd static library and make it part of libsdbus-c++, instead of searching for it in the system" OFF)
if(NOT SDBUSCPP_BUILD_LIBSYSTEMD)
    set(SDBUSCPP_SDBUS_LIB "default" CACHE STRING "sd-bus implementation library to search for and use (default, systemd, elogind, or basu)")
    set_property(CACHE SDBUSCPP_SDBUS_LIB PROPERTY STRINGS default systemd elogind basu)
else()
    set(SDBUSCPP_LIBSYSTEMD_VERSION "252" CACHE STRING "libsystemd version (>=239) to build and incorporate into libsdbus-c++")
    set(SDBUSCPP_LIBSYSTEMD_EXTRA_CONFIG_OPTS "" CACHE STRING "Additional configuration options to be passed as-is to libsystemd build system")
endif()
option(SDBUSCPP_INSTALL "Enable installation of sdbus-c++ (downstream projects embedding sdbus-c++ may want to turn this OFF)" ON)
option(SDBUSCPP_BUILD_TESTS "Build tests" OFF)
if (SDBUSCPP_BUILD_TESTS)
    option(SDBUSCPP_BUILD_PERF_TESTS "Build also sdbus-c++ performance tests" OFF)
    option(SDBUSCPP_BUILD_STRESS_TESTS "Build also sdbus-c++ stress tests" OFF)
    set(SDBUSCPP_TESTS_INSTALL_PATH "tests/${PROJECT_NAME}" CACHE STRING "Specifies where the test binaries will be installed")
    set(SDBUSCPP_GOOGLETEST_VERSION 1.14.0 CACHE STRING "Version of gmock library to use")
    set(SDBUSCPP_GOOGLETEST_GIT_REPO "https://github.com/google/googletest.git" CACHE STRING "A git repo to clone and build googletest from if gmock is not found in the system")
endif()
option(SDBUSCPP_BUILD_CODEGEN "Build generator tool for C++ native bindings" OFF)
option(SDBUSCPP_BUILD_EXAMPLES "Build example programs" OFF)
option(SDBUSCPP_BUILD_DOCS "Build documentation for sdbus-c++" ON)
if(SDBUSCPP_BUILD_DOCS)
    option(SDBUSCPP_BUILD_DOXYGEN_DOCS "Build doxygen documentation for sdbus-c++ API" OFF)
endif()
#option(SDBUSCPP_CLANG_TIDY "Co-compile with clang-tidy static analyzer" OFF)
#option(SDBUSCPP_COVERAGE "Build sdbus-c++ with code coverage instrumentation" OFF)
# We promote the BUILD_SHARED_LIBS flag to a (global) option only if we are the main project
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    option(BUILD_SHARED_LIBS "Build shared libraries (.so) instead of static ones (.a)" ON)
endif()

message(STATUS "")
message(STATUS "  *******************************************")
message(STATUS "  *         SDBUS-C++ CONFIGURATION         *")
message(STATUS "  *******************************************")
message(STATUS "")
message(STATUS "  SDBUSCPP_BUILD_LIBSYSTEMD: ${SDBUSCPP_BUILD_LIBSYSTEMD}")
if(SDBUSCPP_BUILD_LIBSYSTEMD)
    message(STATUS "    SDBUSCPP_LIBSYSTEMD_VERSION: ${SDBUSCPP_LIBSYSTEMD_VERSION}")
    message(STATUS "    SDBUSCPP_LIBSYSTEMD_EXTRA_CONFIG_OPTS: ${SDBUSCPP_LIBSYSTEMD_EXTRA_CONFIG_OPTS}")
endif()
message(STATUS "  SDBUSCPP_INSTALL: ${SDBUSCPP_INSTALL}")
message(STATUS "  SDBUSCPP_BUILD_TESTS: ${SDBUSCPP_BUILD_TESTS}")
if(SDBUSCPP_BUILD_TESTS)
    message(STATUS "    SDBUSCPP_BUILD_PERF_TESTS: ${SDBUSCPP_BUILD_PERF_TESTS}")
    message(STATUS "    SDBUSCPP_BUILD_STRESS_TESTS: ${SDBUSCPP_BUILD_STRESS_TESTS}")
    message(STATUS "    SDBUSCPP_TESTS_INSTALL_PATH: ${SDBUSCPP_TESTS_INSTALL_PATH}")
    message(STATUS "    SDBUSCPP_GOOGLETEST_VERSION: ${SDBUSCPP_GOOGLETEST_VERSION}")
    message(STATUS "    SDBUSCPP_GOOGLETEST_GIT_REPO: ${SDBUSCPP_GOOGLETEST_GIT_REPO}")
endif()
message(STATUS "  SDBUSCPP_BUILD_CODEGEN: ${SDBUSCPP_BUILD_CODEGEN}")
message(STATUS "  SDBUSCPP_BUILD_EXAMPLES: ${SDBUSCPP_BUILD_EXAMPLES}")
message(STATUS "  SDBUSCPP_BUILD_DOCS: ${SDBUSCPP_BUILD_DOCS}")
if(SDBUSCPP_BUILD_DOCS)
    message(STATUS "    SDBUSCPP_BUILD_DOXYGEN_DOCS: ${SDBUSCPP_BUILD_DOXYGEN_DOCS}")
endif()
message(STATUS "  BUILD_SHARED_LIBS: ${BUILD_SHARED_LIBS}")
message(STATUS "")

#-------------------------------
# PERFORMING CHECKS & PREPARING THE DEPENDENCIES
#-------------------------------

if(SDBUSCPP_BUILD_LIBSYSTEMD)
    # Build static libsystemd library as an external project
    include(cmake/LibsystemdExternalProject.cmake)
    set(SDBUS_IMPL "systemd")
    set(SDBUS_LIB "libsystemd")
else()
    # Search for sd-bus implementations in the system as per user's configuration
    set(SDBUS_LIBS ${SDBUSCPP_SDBUS_LIB})
    if(SDBUSCPP_SDBUS_LIB STREQUAL "default")
        set(SDBUS_LIBS systemd elogind basu) # This is the default search order
    endif()

    set(MINIMUM_SDBUS_VERSION 238)

    find_package(PkgConfig REQUIRED)
    foreach(LIB ${SDBUS_LIBS})
        if(LIB STREQUAL "systemd")
            pkg_check_modules(Systemd IMPORTED_TARGET GLOBAL libsystemd>=${MINIMUM_SDBUS_VERSION})
            if(TARGET PkgConfig::Systemd)
                set(SDBUS_IMPL "systemd")
                set(SDBUS_LIB "libsystemd")
                break()
            endif()
        elseif(LIB STREQUAL "elogind")
            pkg_check_modules(Systemd IMPORTED_TARGET GLOBAL libelogind>=${MINIMUM_SDBUS_VERSION})
            if(TARGET PkgConfig::Systemd)
                set(SDBUS_IMPL "elogind")
                set(SDBUS_LIB "libelogind")
                string(REPLACE "." ";" VERSION_LIST ${Systemd_VERSION})
                list(GET VERSION_LIST 0 Systemd_VERSION)
                break()
            endif()
        elseif(LIB STREQUAL "basu")
            pkg_check_modules(Systemd IMPORTED_TARGET GLOBAL basu)
            if(TARGET PkgConfig::Systemd)
                set(SDBUS_IMPL "basu")
                set(SDBUS_LIB "basu")
                set(Systemd_VERSION "240") # https://git.sr.ht/~emersion/basu/commit/d4d185d29a26
                break()
            endif()
        else()
            message(FATAL_ERROR "Unrecognized sd-bus implementation library ${LIB}")
        endif()
    endforeach()

    if(NOT TARGET PkgConfig::Systemd)
        message(FATAL_ERROR "None of ${SDBUS_LIBS} libraries implementing sd-bus was found (Are their dev packages installed?)")
    endif()
    add_library(Systemd::Libsystemd ALIAS PkgConfig::Systemd)
    string(REGEX MATCHALL "([0-9]+)" SYSTEMD_VERSION_LIST "${Systemd_VERSION}")
    list(GET SYSTEMD_VERSION_LIST 0 SDBUSCPP_LIBSYSTEMD_VERSION)
    message(STATUS "Building with libsystemd v${SDBUSCPP_LIBSYSTEMD_VERSION}")
endif()

find_package(Threads REQUIRED)

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
    ${SDBUSCPP_SOURCE_DIR}/Utils.h
    ${SDBUSCPP_SOURCE_DIR}/Object.h
    ${SDBUSCPP_SOURCE_DIR}/Proxy.h
    ${SDBUSCPP_SOURCE_DIR}/ScopeGuard.h
    ${SDBUSCPP_SOURCE_DIR}/VTableUtils.h
    ${SDBUSCPP_SOURCE_DIR}/SdBus.h
    ${SDBUSCPP_SOURCE_DIR}/ISdBus.h)

set(SDBUSCPP_PUBLIC_HDRS
    ${SDBUSCPP_INCLUDE_DIR}/ConvenienceApiClasses.h
    ${SDBUSCPP_INCLUDE_DIR}/ConvenienceApiClasses.inl
    ${SDBUSCPP_INCLUDE_DIR}/VTableItems.h
    ${SDBUSCPP_INCLUDE_DIR}/VTableItems.inl
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

set(CMAKE_CXX_STANDARD 20)

#----------------------------------
# LIBRARY BUILD INFORMATION
#----------------------------------

set(SDBUSCPP_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(SDBUSCPP_VERSION "${PROJECT_VERSION}")

# Having an object target allows unit tests to reuse already built sources without re-building
add_library(sdbus-c++-objlib OBJECT ${SDBUSCPP_SRCS})
target_compile_definitions(sdbus-c++-objlib PRIVATE
    BUILD_LIB=1
    LIBSYSTEMD_VERSION=${SDBUSCPP_LIBSYSTEMD_VERSION}
    SDBUS_${SDBUS_IMPL}
    SDBUS_HEADER=<${SDBUS_IMPL}/sd-bus.h>)
target_include_directories(sdbus-c++-objlib PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                                                   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)
if(BUILD_SHARED_LIBS)
    set_target_properties(sdbus-c++-objlib PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
if(SDBUSCPP_BUILD_LIBSYSTEMD)
    add_dependencies(sdbus-c++-objlib LibsystemdBuildProject)
endif()
target_link_libraries(sdbus-c++-objlib
        PUBLIC
            Systemd::Libsystemd
            Threads::Threads)

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
# TESTS
#----------------------------------

if(SDBUSCPP_BUILD_TESTS)
    message(STATUS "Building with tests")
    enable_testing()
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/tests")
endif()

#----------------------------------
# UTILS
#----------------------------------

if(SDBUSCPP_BUILD_CODEGEN)
    message(STATUS "Building with code generator tool")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/tools")
endif()

#----------------------------------
# EXAMPLES
#----------------------------------

if(SDBUSCPP_BUILD_EXAMPLES)
    message(STATUS "Building with examples")
    add_subdirectory(examples)
endif()

#----------------------------------
# DOCUMENTATION
#----------------------------------

if(SDBUSCPP_BUILD_DOCS)
    message(STATUS "Building with documentation")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/docs")
endif()

#----------------------------------
# CMAKE CONFIG & PACKAGE CONFIG
#----------------------------------

include(CMakePackageConfigHelpers)

configure_package_config_file(cmake/sdbus-c++-config.cmake.in cmake/sdbus-c++-config.cmake
                              INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sdbus-c++)
write_basic_package_version_file(cmake/sdbus-c++-config-version.cmake COMPATIBILITY SameMajorVersion)

if(BUILD_SHARED_LIBS AND (SDBUSCPP_BUILD_LIBSYSTEMD OR Systemd_LINK_LIBRARIES MATCHES "/libsystemd\.a(;|$)"))
    set(PKGCONFIG_REQS ".private")
else()
    set(PKGCONFIG_REQS "")
endif()
set(PKGCONFIG_DEPS ${SDBUS_LIB})
configure_file(pkgconfig/sdbus-c++.pc.in pkgconfig/sdbus-c++.pc @ONLY)

#----------------------------------
# INSTALLATION
#----------------------------------

if(SDBUSCPP_INSTALL)
    set(EXPORT_SET sdbus-c++)
    if(NOT BUILD_SHARED_LIBS)
        list(APPEND EXPORT_SET "sdbus-c++-objlib")
    endif()
    install(TARGETS ${EXPORT_SET}
            EXPORT sdbus-c++-targets
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT sdbus-c++-runtime
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT sdbus-c++-runtime NAMELINK_COMPONENT sdbus-c++-dev
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT sdbus-c++-dev
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${SDBUSCPP_INCLUDE_SUBDIR} COMPONENT sdbus-c++-dev)
    install(EXPORT sdbus-c++-targets
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sdbus-c++
            NAMESPACE SDBusCpp::
            COMPONENT sdbus-c++-dev)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cmake/sdbus-c++-config.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/cmake/sdbus-c++-config-version.cmake
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sdbus-c++
            COMPONENT sdbus-c++-dev)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/sdbus-c++.pc
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig COMPONENT sdbus-c++-dev)
    if(SDBUSCPP_BUILD_DOCS)
        install(FILES README README.md NEWS COPYING ChangeLog AUTHORS DESTINATION ${CMAKE_INSTALL_DOCDIR} COMPONENT sdbus-c++-doc)
    endif()
endif()

#----------------------------------
# CPACK
#----------------------------------

set(CPACK_PACKAGE_VENDOR "Kistler")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High-level C++ D-Bus library")
set(CPACK_PACKAGE_CONTACT "info@kistler.com")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_COMPONENTS_ALL sdbus-c++-runtime sdbus-c++-dev sdbus-c++-doc)
set(CPACK_COMPONENT_DEV_DEPENDS "sdbus-c++-runtime")
# specific for DEB generator
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_RUNTIME_DEBUGINFO_PACKAGE ON)
set(CPACK_DEBIAN_SDBUS-C++-RUNTIME_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_DEBIAN_SDBUS-C++-DEV_PACKAGE_NAME "sdbus-c++-dev")
set(CPACK_DEBIAN_SDBUS-C++-DOC_PACKAGE_NAME "sdbus-c++-doc")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_SECTION "libs")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_ENABLE_COMPONENT_DEPENDS ON)
set(CPACK_DEBIAN_DEV_PACKAGE_DEPENDS "libsystemd-dev (>=${MINIMUM_SDBUS_VERSION})")

include(CPack)
