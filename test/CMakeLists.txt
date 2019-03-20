#-------------------------------
# DOWNLOAD AND BUILD OF GOOGLETEST
# https://github.com/google/googletest/blob/master/googletest/README.md#incorporating-into-an-existing-cmake-project
#-------------------------------

configure_file(googletest-download/CMakeLists.txt.in googletest-download/CMakeLists.txt)

execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download)

if(result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} --build .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download)

if(result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
    ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
    EXCLUDE_FROM_ALL)

#-------------------------------
# SOURCE FILES CONFIGURATION
#-------------------------------

set(UNITTESTS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/unittests)
set(UNITTESTS_SRCS
    ${UNITTESTS_SOURCE_DIR}/libsdbus-c++_unittests.cpp
    ${UNITTESTS_SOURCE_DIR}/Message_test.cpp
    ${UNITTESTS_SOURCE_DIR}/Types_test.cpp
    ${UNITTESTS_SOURCE_DIR}/TypeTraits_test.cpp
    ${UNITTESTS_SOURCE_DIR}/Connection_test.cpp
    ${UNITTESTS_SOURCE_DIR}/mocks/SdBusMock.h)

set(INTEGRATIONTESTS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/integrationtests)
set(INTEGRATIONTESTS_SRCS
    ${INTEGRATIONTESTS_SOURCE_DIR}/AdaptorAndProxy_test.cpp
    ${INTEGRATIONTESTS_SOURCE_DIR}/Connection_test.cpp
    ${INTEGRATIONTESTS_SOURCE_DIR}/libsdbus-c++_integrationtests.cpp
    ${INTEGRATIONTESTS_SOURCE_DIR}/adaptor-glue.h
    ${INTEGRATIONTESTS_SOURCE_DIR}/defs.h
    ${INTEGRATIONTESTS_SOURCE_DIR}/proxy-glue.h
    ${INTEGRATIONTESTS_SOURCE_DIR}/TestingAdaptor.h
    ${INTEGRATIONTESTS_SOURCE_DIR}/TestingProxy.h)

#-------------------------------
# GENERAL COMPILER CONFIGURATION
#-------------------------------

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

#----------------------------------
# BUILD INFORMATION
#----------------------------------

# Turn off -isystem gcc option that CMake uses for imported
# targets even when INTERFACE_INCLUDE_DIRECTORIES is used.
set(CMAKE_NO_SYSTEM_FROM_IMPORTED "1")

add_executable(libsdbus-c++_unittests ${UNITTESTS_SRCS} $<TARGET_OBJECTS:sdbuscppobjects>)
#target_link_libraries(libsdbus-c++_unittests ${SYSTEMD_LIBRARIES} gmock gmock_main)
target_link_libraries(libsdbus-c++_unittests -L/home/aeywalee/data/repos/systemd/.libs/ -lsystemd gmock gmock_main)

add_executable(libsdbus-c++_integrationtests ${INTEGRATIONTESTS_SRCS})
target_link_libraries(libsdbus-c++_integrationtests sdbus-c++ gmock gmock_main)

# Manual performance tests
option(ENABLE_PERFTESTS "Build and install manual performance tests (default OFF)" OFF)
if(ENABLE_PERFTESTS)
    add_executable(libsdbus-c++_perftests_client perftests/client.cpp perftests/perftest-proxy.h)
    target_link_libraries(libsdbus-c++_perftests_client sdbus-c++)
    add_executable(libsdbus-c++_perftests_server perftests/server.cpp perftests/perftest-adaptor.h)
    target_link_libraries(libsdbus-c++_perftests_server sdbus-c++)
endif()

#----------------------------------
# INSTALLATION
#----------------------------------

install(TARGETS libsdbus-c++_unittests DESTINATION /opt/test/bin)
install(TARGETS libsdbus-c++_integrationtests DESTINATION /opt/test/bin)
install(FILES ${INTEGRATIONTESTS_SOURCE_DIR}/files/libsdbus-cpp-test.conf DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/dbus-1/system.d)

if(ENABLE_PERFTESTS)
    install(TARGETS libsdbus-c++_perftests_client DESTINATION /opt/test/bin)
    install(TARGETS libsdbus-c++_perftests_server DESTINATION /opt/test/bin)
    install(FILES perftests/files/org.sdbuscpp.perftest.conf DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/dbus-1/system.d)
endif()

#----------------------------------
# RUNNING THE TESTS UPON BUILD
#----------------------------------

if(CMAKE_CROSSCOMPILING)
    if(NOT DEFINED UNIT_TESTS_RUNNER)
        set(UNIT_TESTS_RUNNER $ENV{UNIT_TESTS_RUNNER})
    endif()
    if(NOT DEFINED TEST_DEVICE_IP)
        set(TEST_DEVICE_IP $ENV{TEST_DEVICE_IP})
    endif()

    if(NOT (UNIT_TESTS_RUNNER AND TEST_DEVICE_IP))
        message(WARNING "UNIT_TESTS_RUNNER and TEST_DEVICE_IP variables must be defined to run tests remotely")
    endif()
    add_test(NAME libsdbus-c++_unittests COMMAND ${UNIT_TESTS_RUNNER} --deviceip=${TEST_DEVICE_IP} --testbin=libsdbus-c++_unittests)
    add_test(NAME libsdbus-c++_integrationtests COMMAND ${UNIT_TESTS_RUNNER} --deviceip=${TEST_DEVICE_IP} --testbin=libsdbus-c++_integrationtests)
else()
    add_test(NAME libsdbus-c++_unittests COMMAND libsdbus-c++_unittests)
    add_test(NAME libsdbus-c++_integrationtests COMMAND libsdbus-c++_integrationtests)
endif() 
