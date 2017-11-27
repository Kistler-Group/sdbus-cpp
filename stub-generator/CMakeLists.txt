cmake_minimum_required (VERSION 3.3)

add_definitions(-std=c++14)

project (sdbuscpp-xml2cpp)

add_executable(${PROJECT_NAME} xml2cpp.cpp xml.cpp generator_utils.cpp BaseGenerator.cpp AdaptorGenerator.cpp ProxyGenerator.cpp)

find_package (EXPAT REQUIRED)

target_link_libraries (${PROJECT_NAME} ${EXPAT_LIBRARIES})

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME} DESTINATION bin)
