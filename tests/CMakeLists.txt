#-------------------------------
# DOWNLOAD AND BUILD OF GOOGLETEST
#-------------------------------

find_package(GTest ${SDBUSCPP_GOOGLETEST_VERSION} CONFIG)
if (NOT TARGET GTest::gmock)
    # Try pkg-config if GTest was not found through CMake config
    find_package(PkgConfig)
    if (PkgConfig_FOUND)
        pkg_check_modules(GMock IMPORTED_TARGET GLOBAL gmock>=${SDBUSCPP_GOOGLETEST_VERSION})
        if(TARGET PkgConfig::GMock)
            add_library(GTest::gmock ALIAS PkgConfig::GMock)
        endif()
    endif()
    # GTest was not found in the system, build it on our own
    if (NOT TARGET GTest::gmock)
        include(FetchContent)

        if (SDBUSCPP_GOOGLETEST_VERSION VERSION_GREATER_EQUAL 1.13.0)
            set(GOOGLETEST_TAG "v${SDBUSCPP_GOOGLETEST_VERSION}")
        else()
            set(GOOGLETEST_TAG "release-${SDBUSCPP_GOOGLETEST_VERSION}")
        endif()

        message("Manually fetching & building googletest ${GOOGLETEST_TAG}...")
        FetchContent_Declare(googletest
                            GIT_REPOSITORY ${SDBUSCPP_GOOGLETEST_GIT_REPO}
                            GIT_TAG        ${GOOGLETEST_TAG}
                            GIT_SHALLOW    1
                            UPDATE_COMMAND "")

        set(gtest_force_shared_crt ON CACHE INTERNAL "" FORCE)
        set(INSTALL_GTEST OFF CACHE INTERNAL "" FORCE)
        set(BUILD_SHARED_LIBS_BAK ${BUILD_SHARED_LIBS})
        set(BUILD_SHARED_LIBS OFF)
        FetchContent_MakeAvailable(googletest)
        set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_BAK})
        add_library(GTest::gmock ALIAS gmock)
    endif()
endif()

#-------------------------------
# SOURCE FILES CONFIGURATION
#-------------------------------

set(UNITTESTS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/unittests)
set(UNITTESTS_SRCS
    ${UNITTESTS_SOURCE_DIR}/sdbus-c++-unit-tests.cpp
    ${UNITTESTS_SOURCE_DIR}/Message_test.cpp
    ${UNITTESTS_SOURCE_DIR}/PollData_test.cpp
    ${UNITTESTS_SOURCE_DIR}/Types_test.cpp
    ${UNITTESTS_SOURCE_DIR}/TypeTraits_test.cpp
    ${UNITTESTS_SOURCE_DIR}/Connection_test.cpp
    ${UNITTESTS_SOURCE_DIR}/mocks/SdBusMock.h)

set(INTEGRATIONTESTS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/integrationtests)
set(INTEGRATIONTESTS_SRCS
    ${INTEGRATIONTESTS_SOURCE_DIR}/DBusConnectionTests.cpp
    ${INTEGRATIONTESTS_SOURCE_DIR}/DBusGeneralTests.cpp
    ${INTEGRATIONTESTS_SOURCE_DIR}/DBusMethodsTests.cpp
    ${INTEGRATIONTESTS_SOURCE_DIR}/DBusAsyncMethodsTests.cpp
    ${INTEGRATIONTESTS_SOURCE_DIR}/DBusSignalsTests.cpp
    ${INTEGRATIONTESTS_SOURCE_DIR}/DBusPropertiesTests.cpp
    ${INTEGRATIONTESTS_SOURCE_DIR}/DBusStandardInterfacesTests.cpp
    ${INTEGRATIONTESTS_SOURCE_DIR}/Defs.h
    ${INTEGRATIONTESTS_SOURCE_DIR}/integrationtests-adaptor.h
    ${INTEGRATIONTESTS_SOURCE_DIR}/integrationtests-proxy.h
    ${INTEGRATIONTESTS_SOURCE_DIR}/TestFixture.h
    ${INTEGRATIONTESTS_SOURCE_DIR}/TestFixture.cpp
    ${INTEGRATIONTESTS_SOURCE_DIR}/TestAdaptor.h
    ${INTEGRATIONTESTS_SOURCE_DIR}/TestAdaptor.cpp
    ${INTEGRATIONTESTS_SOURCE_DIR}/TestProxy.h
    ${INTEGRATIONTESTS_SOURCE_DIR}/TestProxy.cpp
    ${INTEGRATIONTESTS_SOURCE_DIR}/sdbus-c++-integration-tests.cpp)

set(PERFTESTS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/perftests)
set(STRESSTESTS_CLIENT_SRCS
    ${PERFTESTS_SOURCE_DIR}/client.cpp
    ${PERFTESTS_SOURCE_DIR}/perftests-proxy.h)
set(STRESSTESTS_SERVER_SRCS
    ${PERFTESTS_SOURCE_DIR}/server.cpp
    ${PERFTESTS_SOURCE_DIR}/perftests-adaptor.h)

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

add_executable(sdbus-c++-unit-tests ${UNITTESTS_SRCS})
target_compile_definitions(sdbus-c++-unit-tests PRIVATE
    LIBSYSTEMD_VERSION=${SDBUSCPP_LIBSYSTEMD_VERSION}
    SDBUS_${SDBUS_IMPL}
    SDBUS_HEADER=<${SDBUS_IMPL}/sd-bus.h>)
target_link_libraries(sdbus-c++-unit-tests sdbus-c++-objlib GTest::gmock)

add_executable(sdbus-c++-integration-tests ${INTEGRATIONTESTS_SRCS})
target_compile_definitions(sdbus-c++-integration-tests PRIVATE
    LIBSYSTEMD_VERSION=${SDBUSCPP_LIBSYSTEMD_VERSION}
    SDBUS_${SDBUS_IMPL})
if(NOT SDBUS_IMPL STREQUAL "basu")
    # Systemd::Libsystemd is included because integration tests use sd-event. Otherwise sdbus-c++ encapsulates and hides libsystemd.
    target_link_libraries(sdbus-c++-integration-tests sdbus-c++ Systemd::Libsystemd GTest::gmock)
else()
    # sd-event implementation is not part of basu, so its integration tests will be skipped
    target_link_libraries(sdbus-c++-integration-tests sdbus-c++ GTest::gmock)
endif()

if(SDBUSCPP_BUILD_PERF_TESTS OR SDBUSCPP_BUILD_STRESS_TESTS)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)

    if(SDBUSCPP_BUILD_PERF_TESTS)
        message(STATUS "Building with performance tests")
        add_executable(sdbus-c++-perf-tests-client ${STRESSTESTS_CLIENT_SRCS})
        target_link_libraries(sdbus-c++-perf-tests-client sdbus-c++ Threads::Threads)
        add_executable(sdbus-c++-perf-tests-server ${STRESSTESTS_SERVER_SRCS})
        target_link_libraries(sdbus-c++-perf-tests-server sdbus-c++ Threads::Threads)
    endif()

    if(SDBUSCPP_BUILD_STRESS_TESTS)
        message(STATUS "Building with stress tests")
        add_executable(sdbus-c++-stress-tests ${STRESSTESTS_SRCS})
        target_link_libraries(sdbus-c++-stress-tests sdbus-c++ Threads::Threads)
    endif()
endif()

#----------------------------------
# INSTALLATION
#----------------------------------

if(SDBUSCPP_INSTALL)
    include(GNUInstallDirs)
    install(TARGETS sdbus-c++-unit-tests DESTINATION ${SDBUSCPP_TESTS_INSTALL_PATH} COMPONENT sdbus-c++-test)
    install(TARGETS sdbus-c++-integration-tests DESTINATION ${SDBUSCPP_TESTS_INSTALL_PATH} COMPONENT sdbus-c++-test)
    install(FILES ${INTEGRATIONTESTS_SOURCE_DIR}/files/org.sdbuscpp.integrationtests.conf
            DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/dbus-1/system.d
            COMPONENT sdbus-c++-test)
    if(SDBUSCPP_BUILD_PERF_TESTS)
        install(TARGETS sdbus-c++-perf-tests-client DESTINATION ${SDBUSCPP_TESTS_INSTALL_PATH} COMPONENT sdbus-c++-test)
        install(TARGETS sdbus-c++-perf-tests-server DESTINATION ${SDBUSCPP_TESTS_INSTALL_PATH} COMPONENT sdbus-c++-test)
        install(FILES ${PERFTESTS_SOURCE_DIR}/files/org.sdbuscpp.perftests.conf
                DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/dbus-1/system.d
                COMPONENT sdbus-c++-test)
    endif()
    if(SDBUSCPP_BUILD_STRESS_TESTS)
        install(TARGETS sdbus-c++-stress-tests DESTINATION ${SDBUSCPP_TESTS_INSTALL_PATH} COMPONENT sdbus-c++-test)
        install(FILES ${STRESSTESTS_SOURCE_DIR}/files/org.sdbuscpp.stresstests.conf
                DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/dbus-1/system.d
                COMPONENT sdbus-c++-test)
    endif()
endif()

#----------------------------------
# RUNNING THE TESTS UPON BUILD
#----------------------------------

if(NOT CMAKE_CROSSCOMPILING)
    add_test(NAME sdbus-c++-unit-tests COMMAND sdbus-c++-unit-tests)
    add_test(NAME sdbus-c++-integration-tests COMMAND sdbus-c++-integration-tests)
endif()
