#-------------------------------
# GENERAL COMPILER CONFIGURATION
#-------------------------------

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

#----------------------------------
# BUILD INFORMATION
#----------------------------------

# Turn off -isystem gcc option that CMake uses for imported
# targets even when INTERFACE_INCLUDE_DIRECTORIES is used.
#set(CMAKE_NO_SYSTEM_FROM_IMPORTED "1")

add_executable(perftestclient client.cpp perftest-proxy.h)
target_link_libraries(perftestclient sdbus-c++)

add_executable(perftestserver server.cpp perftest-adaptor.h)
target_link_libraries(perftestserver sdbus-c++)

#----------------------------------
# INSTALLATION
#----------------------------------

install(TARGETS perftestclient DESTINATION /opt/test/bin)
install(TARGETS perftestserver DESTINATION /opt/test/bin)
