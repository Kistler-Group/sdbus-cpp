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
    ${UNITTESTS_SOURCE_DIR}/sdbus-c++-unit-tests.cpp
    ${UNITTESTS_SOURCE_DIR}/Message_test.cpp
    ${UNITTESTS_SOURCE_DIR}/Types_test.cpp
    ${UNITTESTS_SOURCE_DIR}/TypeTraits_test.cpp
    ${UNITTESTS_SOURCE_DIR}/Connection_test.cpp
    ${UNITTESTS_SOURCE_DIR}/mocks/SdBusMock.h)

set(INTEGRATIONTESTS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/integrationtests)
set(INTEGRATIONTESTS_SRCS
    ${INTEGRATIONTESTS_SOURCE_DIR}/AdaptorAndProxy_test.cpp
    ${INTEGRATIONTESTS_SOURCE_DIR}/Connection_test.cpp
    ${INTEGRATIONTESTS_SOURCE_DIR}/sdbus-c++-integration-tests.cpp
    ${INTEGRATIONTESTS_SOURCE_DIR}/adaptor-glue.h
    ${INTEGRATIONTESTS_SOURCE_DIR}/defs.h
    ${INTEGRATIONTESTS_SOURCE_DIR}/proxy-glue.h
    ${INTEGRATIONTESTS_SOURCE_DIR}/TestingAdaptor.h
    ${INTEGRATIONTESTS_SOURCE_DIR}/TestingProxy.h)

set(PERFTESTS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/perftests)
set(STRESSTESTS_CLIENT_SRCS
    ${PERFTESTS_SOURCE_DIR}/client.cpp
    ${PERFTESTS_SOURCE_DIR}/perftest-proxy.h)
set(STRESSTESTS_SERVER_SRCS
    ${PERFTESTS_SOURCE_DIR}/server.cpp
    ${PERFTESTS_SOURCE_DIR}/perftest-adaptor.h)

set(STRESSTESTS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/stresstests)
set(STRESSTESTS_SRCS
    ${STRESSTESTS_SOURCE_DIR}/sdbus-c++-stress-tests.cpp
    ${STRESSTESTS_SOURCE_DIR}/fahrenheit-thermometer-adaptor.h
    ${STRESSTESTS_SOURCE_DIR}/fahrenheit-thermometer-proxy.h
    ${STRESSTESTS_SOURCE_DIR}/concatenator-adaptor.h
    ${STRESSTESTS_SOURCE_DIR}/concatenator-proxy.h)

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

add_executable(sdbus-c++-unit-tests ${UNITTESTS_SRCS} $<TARGET_OBJECTS:sdbuscppobjects>)
target_link_libraries(sdbus-c++-unit-tests ${SYSTEMD_LIBRARIES} gmock gmock_main)

add_executable(sdbus-c++-integration-tests ${INTEGRATIONTESTS_SRCS})
target_link_libraries(sdbus-c++-integration-tests sdbus-c++ gmock gmock_main)

# Manual performance tests
option(ENABLE_PERF_TESTS "Build and install manual performance tests (default OFF)" OFF)
if(ENABLE_PERF_TESTS)
    add_executable(sdbus-c++-perf-tests-client ${STRESSTESTS_CLIENT_SRCS})
    target_link_libraries(sdbus-c++-perf-tests-client sdbus-c++)
    add_executable(sdbus-c++-perf-tests-server ${STRESSTESTS_SERVER_SRCS})
    target_link_libraries(sdbus-c++-perf-tests-server sdbus-c++)
endif()

# Manual stress tests
option(ENABLE_STRESS_TESTS "Build and install manual stress tests (default OFF)" OFF)
if(ENABLE_STRESS_TESTS)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)

    add_executable(sdbus-c++-stress-tests ${STRESSTESTS_SRCS})
    target_link_libraries(sdbus-c++-stress-tests sdbus-c++ Threads::Threads)
endif()

#----------------------------------
# INSTALLATION
#----------------------------------

install(TARGETS sdbus-c++-unit-tests DESTINATION /opt/test/bin)
install(TARGETS sdbus-c++-integration-tests DESTINATION /opt/test/bin)
install(FILES ${INTEGRATIONTESTS_SOURCE_DIR}/files/org.sdbuscpp.integrationtests.conf DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/dbus-1/system.d)

if(ENABLE_PERF_TESTS)
    install(TARGETS sdbus-c++-perf-tests-client DESTINATION /opt/test/bin)
    install(TARGETS sdbus-c++-perf-tests-server DESTINATION /opt/test/bin)
    install(FILES ${PERFTESTS_SOURCE_DIR}/files/org.sdbuscpp.perftests.conf DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/dbus-1/system.d)
endif()

if(ENABLE_STRESS_TESTS)
    install(TARGETS sdbus-c++-stress-tests DESTINATION /opt/test/bin)
    install(FILES ${STRESSTESTS_SOURCE_DIR}/files/org.sdbuscpp.stresstests.conf DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/dbus-1/system.d)
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
    add_test(NAME sdbus-c++-unit-tests COMMAND ${UNIT_TESTS_RUNNER} --deviceip=${TEST_DEVICE_IP} --testbin=sdbus-c++-unit-tests)
    add_test(NAME sdbus-c++-integration-tests COMMAND ${UNIT_TESTS_RUNNER} --deviceip=${TEST_DEVICE_IP} --testbin=sdbus-c++-integration-tests)
else()
    add_test(NAME sdbus-c++_unit-tests COMMAND sdbus-c++-unit-tests)
    add_test(NAME sdbus-c++_integration-tests COMMAND sdbus-c++-integration-tests)
endif() 
