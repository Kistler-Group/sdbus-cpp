#-------------------------------
# PROJECT INFORMATION
#-------------------------------

cmake_minimum_required(VERSION 3.5)

project(sdbus-c++-xml2cpp)

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

add_executable(${PROJECT_NAME} ${SDBUSCPP_XML2CPP_SRCS})

target_link_libraries (${PROJECT_NAME} ${EXPAT_LIBRARIES})

#----------------------------------
# INSTALLATION
#----------------------------------

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
