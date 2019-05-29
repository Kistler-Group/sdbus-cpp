find_program(MESON meson)
find_program(NINJA ninja)

if((NOT MESON) OR (NOT NINJA))
    message(FATAL_ERROR "Meson and Ninja are required to build libsystemd")
endif()

find_library(GLIBC_RT_LIBRARY rt)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MOUNT mount)
pkg_check_modules(CAP REQUIRED libcap)
if (NOT CAP_FOUND)
    find_library(CAP_LIBRARIES cap) # Compat with Ubuntu 14.04 which ships libcap w/o .pc file
endif()

set(LIBSYSTEMD_VERSION "242" CACHE STRING "libsystemd version (>=239) to build and incorporate into libsdbus-c++")

if(NOT CMAKE_BUILD_TYPE)
    set(LIBSYSTEMD_BUILD_TYPE "plain")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(LIBSYSTEMD_BUILD_TYPE "debug")
else()
    set(LIBSYSTEMD_BUILD_TYPE "release")
endif()

if(LIBSYSTEMD_VERSION LESS "239")
    message(FATAL_ERROR "Only libsystemd version >=239 can be built as static part of sdbus-c++")
endif()
if(LIBSYSTEMD_VERSION GREATER "240")
    set(BUILD_VERSION_H ${NINJA} -C <BINARY_DIR> version.h)
endif()

include(ExternalProject)
ExternalProject_Add(LibsystemdBuildProject
                    PREFIX libsystemd-v${LIBSYSTEMD_VERSION}
                    GIT_REPOSITORY    https://github.com/systemd/systemd.git
                    GIT_TAG           v${LIBSYSTEMD_VERSION}
                    GIT_SHALLOW       1
                    UPDATE_COMMAND    ""
                    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E remove <BINARY_DIR>/*
                              COMMAND ${MESON} --buildtype=${LIBSYSTEMD_BUILD_TYPE} -Dstatic-libsystemd=pic <SOURCE_DIR> <BINARY_DIR>
                    BUILD_COMMAND     ${BUILD_VERSION_H}
                          COMMAND     ${NINJA} -C <BINARY_DIR> libsystemd.a
                    BUILD_ALWAYS 1
                    INSTALL_COMMAND   ""
                    LOG_DOWNLOAD 1 LOG_UPDATE 1 LOG_CONFIGURE 1 LOG_BUILD 1)

ExternalProject_Get_property(LibsystemdBuildProject SOURCE_DIR)
set(SYSTEMD_INCLUDE_DIRS ${SOURCE_DIR}/src)
ExternalProject_Get_property(LibsystemdBuildProject BINARY_DIR)
set(SYSTEMD_LIBRARY_DIRS ${BINARY_DIR})

add_library(Systemd::Libsystemd STATIC IMPORTED)
set_target_properties(Systemd::Libsystemd PROPERTIES IMPORTED_LOCATION ${SYSTEMD_LIBRARY_DIRS}/libsystemd.a)
set(SYSTEMD_LIBRARIES Systemd::Libsystemd ${CAP_LIBRARIES} ${GLIBC_RT_LIBRARY} ${MOUNT_LIBRARIES})
