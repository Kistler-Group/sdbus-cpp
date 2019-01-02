#-------------------------------
# PROJECT INFORMATION
#-------------------------------

cmake_minimum_required(VERSION 3.5)

project(sdbuscpp-xml2cpp)

include(GNUInstallDirs)

#-------------------------------
# PERFORMING CHECKS
#-------------------------------

find_package(EXPAT REQUIRED)

#-------------------------------
# SOURCE FILES CONFIGURATION
#-------------------------------

set(SDBUSCPP_XML2CPP_SRCS
    xml2cpp.cpp
    xml.h
    xml.cpp
    generator_utils.h
    generator_utils.cpp
    BaseGenerator.h
    BaseGenerator.cpp
    AdaptorGenerator.h
    AdaptorGenerator.cpp
    ProxyGenerator.h
    ProxyGenerator.cpp)

#-------------------------------
# GENERAL COMPILER CONFIGURATION
#-------------------------------

set(CMAKE_CXX_STANDARD 17)

#----------------------------------
# EXECUTABLE BUILD INFORMATION
#----------------------------------

add_executable(${PROJECT_NAME} ${SDBUSCPP_XML2CPP_SRCS})

target_link_libraries (${PROJECT_NAME} ${EXPAT_LIBRARIES})

#----------------------------------
# INSTALLATION
#----------------------------------

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
