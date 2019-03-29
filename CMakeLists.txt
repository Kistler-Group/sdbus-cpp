#-------------------------------
# PROJECT INFORMATION
#-------------------------------

cmake_minimum_required(VERSION 3.8)

project(sdbus-c++ VERSION 0.5.0 LANGUAGES C CXX)

include(GNUInstallDirs) # Installation directories for `install` command and pkgconfig file

#-------------------------------
# PERFORMING CHECKS
#-------------------------------

find_package(PkgConfig REQUIRED)
pkg_check_modules(SYSTEMD REQUIRED libsystemd>=236)

#-------------------------------
# SOURCE FILES CONFIGURATION
#-------------------------------

set(SDBUSCPP_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)
set(SDBUSCPP_INCLUDE_SUBDIR sdbus-c++)
set(SDBUSCPP_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include/${SDBUSCPP_INCLUDE_SUBDIR})

set(SDBUSCPP_CPP_SRCS
    ${SDBUSCPP_SOURCE_DIR}/Connection.cpp
    ${SDBUSCPP_SOURCE_DIR}/ConvenienceClasses.cpp
    ${SDBUSCPP_SOURCE_DIR}/Error.cpp
    ${SDBUSCPP_SOURCE_DIR}/Message.cpp
    ${SDBUSCPP_SOURCE_DIR}/Object.cpp
    ${SDBUSCPP_SOURCE_DIR}/ObjectProxy.cpp
    ${SDBUSCPP_SOURCE_DIR}/Types.cpp
    ${SDBUSCPP_SOURCE_DIR}/Flags.cpp
    ${SDBUSCPP_SOURCE_DIR}/VTableUtils.c
    ${SDBUSCPP_SOURCE_DIR}/SdBus.cpp)

set(SDBUSCPP_HDR_SRCS
    ${SDBUSCPP_SOURCE_DIR}/Connection.h
    ${SDBUSCPP_SOURCE_DIR}/IConnection.h
    ${SDBUSCPP_SOURCE_DIR}/MessageUtils.h
    ${SDBUSCPP_SOURCE_DIR}/Object.h
    ${SDBUSCPP_SOURCE_DIR}/ObjectProxy.h
    ${SDBUSCPP_SOURCE_DIR}/ScopeGuard.h
    ${SDBUSCPP_SOURCE_DIR}/VTableUtils.h
    ${SDBUSCPP_SOURCE_DIR}/SdBus.h
    ${SDBUSCPP_SOURCE_DIR}/ISdBus.h)

set(SDBUSCPP_PUBLIC_HDRS
    ${SDBUSCPP_INCLUDE_DIR}/ConvenienceClasses.h
    ${SDBUSCPP_INCLUDE_DIR}/ConvenienceClasses.inl
    ${SDBUSCPP_INCLUDE_DIR}/Error.h
    ${SDBUSCPP_INCLUDE_DIR}/IConnection.h
    ${SDBUSCPP_INCLUDE_DIR}/Interfaces.h
    ${SDBUSCPP_INCLUDE_DIR}/Introspection.h
    ${SDBUSCPP_INCLUDE_DIR}/IObject.h
    ${SDBUSCPP_INCLUDE_DIR}/IObjectProxy.h
    ${SDBUSCPP_INCLUDE_DIR}/Message.h
    ${SDBUSCPP_INCLUDE_DIR}/MethodResult.h
    ${SDBUSCPP_INCLUDE_DIR}/MethodResult.inl
    ${SDBUSCPP_INCLUDE_DIR}/Types.h
    ${SDBUSCPP_INCLUDE_DIR}/TypeTraits.h
    ${SDBUSCPP_INCLUDE_DIR}/Flags.h
    ${SDBUSCPP_INCLUDE_DIR}/sdbus-c++.h)

set(SDBUSCPP_SRCS ${SDBUSCPP_CPP_SRCS} ${SDBUSCPP_HDR_SRCS} ${SDBUSCPP_PUBLIC_HDRS})

#-------------------------------
# GENERAL COMPILER CONFIGURATION
#-------------------------------

set(CMAKE_CXX_STANDARD 17)
include_directories("${CMAKE_SOURCE_DIR}/include")
include_directories("${CMAKE_SOURCE_DIR}/src")

#----------------------------------
# LIBRARY BUILD INFORMATION
#----------------------------------

set(SDBUSCPP_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(SDBUSCPP_VERSION "${PROJECT_VERSION}")

option(BUILD_SHARED_LIBS "Build shared libraries (.so) instead of static ones (.a)" ON)

add_library(sdbus-cpp OBJECT ${SDBUSCPP_SRCS})
target_include_directories(sdbus-cpp PUBLIC ${SYSTEMD_INCLUDE_DIRS})
target_compile_definitions(sdbus-cpp PRIVATE BUILDLIB=1)
if(BUILD_SHARED_LIBS)
    set_target_properties(sdbus-cpp PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

add_library(sdbus-c++ $<TARGET_OBJECTS:sdbus-cpp>)
set_target_properties(sdbus-c++
    PROPERTIES
    PUBLIC_HEADER "${SDBUSCPP_PUBLIC_HDRS}"
    VERSION "${SDBUSCPP_VERSION}"
    SOVERSION "${SDBUSCPP_VERSION_MAJOR}"
    OUTPUT_NAME "sdbus-c++")
target_link_libraries(sdbus-c++ ${SYSTEMD_LIBRARIES})

#----------------------------------
# INSTALLATION
#----------------------------------

install(TARGETS sdbus-c++
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
    add_subdirectory("${CMAKE_SOURCE_DIR}/test")
endif()

#----------------------------------
# UTILS
#----------------------------------

option(BUILD_CODE_GEN "Build and install interface stub code generator (default OFF)" OFF)

if(BUILD_CODE_GEN)
    add_subdirectory("${CMAKE_SOURCE_DIR}/stub-generator")
endif()

#----------------------------------
# DOCUMENTATION
#----------------------------------

option(BUILD_DOC "Build documentation" ON)

if(BUILD_DOC)
    add_subdirectory("${CMAKE_SOURCE_DIR}/doc")
endif()

#----------------------------------
# CMAKE CONFIG & PACKAGE CONFIG
#----------------------------------

include(CMakePackageConfigHelpers)
set(SDBUSCPP_CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/sdbus-c++)

configure_file(sdbus-c++-config.cmake.in sdbus-c++-config.cmake @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/sdbus-c++-config.cmake DESTINATION ${SDBUSCPP_CONFIG_INSTALL_DIR} COMPONENT dev)

write_basic_package_version_file(sdbus-c++-config-version.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion)
install(FILES ${CMAKE_BINARY_DIR}/sdbus-c++-config-version.cmake DESTINATION ${SDBUSCPP_CONFIG_INSTALL_DIR} COMPONENT dev)

configure_file(sdbus-c++.pc.in sdbus-c++.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/sdbus-c++.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig COMPONENT dev)
