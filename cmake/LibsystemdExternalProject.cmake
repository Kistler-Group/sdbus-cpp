find_program(MESON meson)
find_program(NINJA ninja)

if((NOT MESON) OR (NOT NINJA))
    message(FATAL_ERROR "Meson and Ninja are required to build libsystemd")
endif()

find_library(GLIBC_RT_LIBRARY rt)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MOUNT REQUIRED mount)
pkg_check_modules(CAP REQUIRED libcap)
if (NOT CAP_FOUND)
    find_library(CAP_LIBRARIES cap) # Compat with Ubuntu 14.04 which ships libcap w/o .pc file
endif()

set(LIBSYSTEMD_VERSION v239 CACHE STRING "libsystemd version (>=v239) to build and incorporate into libsdbus-c++")

if(NOT CMAKE_BUILD_TYPE)
    set(LIBSYSTEMD_BUILD_TYPE "plain")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(LIBSYSTEMD_BUILD_TYPE "debug")
else()
    set(LIBSYSTEMD_BUILD_TYPE "release")
endif()

include(ExternalProject)
ExternalProject_Add(LibsystemdBuildProject
                    PREFIX libsystemd
                    GIT_REPOSITORY https://github.com/systemd/systemd.git
                    GIT_TAG        ${LIBSYSTEMD_VERSION}
                    UPDATE_COMMAND ""
                    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E remove <BINARY_DIR>/*
                              COMMAND ${MESON} --buildtype=${LIBSYSTEMD_BUILD_TYPE} -Dstatic-libsystemd=pic <SOURCE_DIR> <BINARY_DIR>
                    BUILD_COMMAND ${NINJA} -C <BINARY_DIR> libsystemd.a
                    INSTALL_COMMAND ""
                    LOG_DOWNLOAD 1 LOG_UPDATE 1 LOG_CONFIGURE 1 LOG_BUILD 1)

ExternalProject_Get_property(LibsystemdBuildProject SOURCE_DIR)
set(SYSTEMD_INCLUDE_DIRS ${SOURCE_DIR}/src)
ExternalProject_Get_property(LibsystemdBuildProject BINARY_DIR)
set(SYSTEMD_LIBRARY_DIRS ${BINARY_DIR})

#add_library(libsystemd-static STATIC IMPORTED)
#set_target_properties(libsystemd-static PROPERTIES IMPORTED_LOCATION ${SYSTEMD_LIBRARY_DIRS}/libsystemd.a)
#target_link_libraries(libsystemd-static ${MOUNT_LIBRARIES} ${CAP_LIBRARIES} ${GLIBC_RT_LIBRARY})
set(SYSTEMD_LIBRARIES ${SYSTEMD_LIBRARY_DIRS}/libsystemd.a ${MOUNT_LIBRARIES} ${CAP_LIBRARIES} ${GLIBC_RT_LIBRARY})
