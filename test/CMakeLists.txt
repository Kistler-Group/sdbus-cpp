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
    ${STRESSTESTS_SOURCE_DIR}/celsius-thermometer-adaptor.h
    ${STRESSTESTS_SOURCE_DIR}/celsius-thermometer-proxy.h
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

add_executable(sdbus-c++-unit-tests ${UNITTESTS_SRCS} $<TARGET_OBJECTS:sdbus-cpp>)
target_link_libraries(sdbus-c++-unit-tests ${SYSTEMD_LIBRARIES} gmock gmock_main)

add_executable(sdbus-c++-integration-tests ${INTEGRATIONTESTS_SRCS})
target_link_libraries(sdbus-c++-integration-tests sdbus-c++ gmock gmock_main)

# Manual performance and stress tests
option(ENABLE_PERF_TESTS "Build and install manual performance tests (default OFF)" OFF)
option(ENABLE_STRESS_TESTS "Build and install manual stress tests (default OFF)" OFF)

if(ENABLE_PERF_TESTS OR ENABLE_STRESS_TESTS)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)

    if(ENABLE_PERF_TESTS)
        add_executable(sdbus-c++-perf-tests-client ${STRESSTESTS_CLIENT_SRCS})
        target_link_libraries(sdbus-c++-perf-tests-client sdbus-c++ Threads::Threads)
        add_executable(sdbus-c++-perf-tests-server ${STRESSTESTS_SERVER_SRCS})
        target_link_libraries(sdbus-c++-perf-tests-server sdbus-c++ Threads::Threads)
    endif()

    if(ENABLE_STRESS_TESTS)
        add_executable(sdbus-c++-stress-tests ${STRESSTESTS_SRCS})
        target_link_libraries(sdbus-c++-stress-tests sdbus-c++ Threads::Threads)
    endif()
endif()

#----------------------------------
# INSTALLATION
#----------------------------------

set(TESTS_INSTALL_PATH "/opt/test/bin" CACHE STRING "Specifies where the test binaries will be installed")

install(TARGETS sdbus-c++-unit-tests DESTINATION ${TESTS_INSTALL_PATH})
install(TARGETS sdbus-c++-integration-tests DESTINATION ${TESTS_INSTALL_PATH})
install(FILES ${INTEGRATIONTESTS_SOURCE_DIR}/files/org.sdbuscpp.integrationtests.conf DESTINATION /etc/dbus-1/system.d)

if(ENABLE_PERF_TESTS)
    install(TARGETS sdbus-c++-perf-tests-client DESTINATION ${TESTS_INSTALL_PATH})
    install(TARGETS sdbus-c++-perf-tests-server DESTINATION ${TESTS_INSTALL_PATH})
    install(FILES ${PERFTESTS_SOURCE_DIR}/files/org.sdbuscpp.perftests.conf DESTINATION /etc/dbus-1/system.d)
endif()

if(ENABLE_STRESS_TESTS)
    install(TARGETS sdbus-c++-stress-tests DESTINATION ${TESTS_INSTALL_PATH})
    install(FILES ${STRESSTESTS_SOURCE_DIR}/files/org.sdbuscpp.stresstests.conf DESTINATION /etc/dbus-1/system.d)
endif()

#----------------------------------
# RUNNING THE TESTS UPON BUILD
#----------------------------------

if(NOT CMAKE_CROSSCOMPILING)
    add_test(NAME sdbus-c++-unit-tests COMMAND sdbus-c++-unit-tests)
    add_test(NAME sdbus-c++-integration-tests COMMAND sdbus-c++-integration-tests)
endif() 
