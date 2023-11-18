#-------------------------------
# DOWNLOAD AND BUILD OF GOOGLETEST
#-------------------------------

set(GOOGLETEST_VERSION 1.10.0 CACHE STRING "Version of gmock to use")
set(GOOGLETEST_GIT_REPO "https://github.com/google/googletest.git" CACHE STRING "A git repo to clone and build googletest from if gmock is not found in the system")

find_package(GTest ${GOOGLETEST_VERSION} CONFIG)
if (NOT TARGET GTest::gmock)
    # Try pkg-config if GTest was not found through CMake config
    find_package(PkgConfig)
    if (PkgConfig_FOUND)
        pkg_check_modules(GMock IMPORTED_TARGET GLOBAL gmock>=${GOOGLETEST_VERSION})
        if(TARGET PkgConfig::GMock)
            add_library(GTest::gmock ALIAS PkgConfig::GMock)
        endif()
    endif()
    # GTest was not found in the system, build it on our own
    if (NOT TARGET GTest::gmock)
        include(FetchContent)

        message("Fetching googletest v${GOOGLETEST_VERSION}...")
        FetchContent_Declare(googletest
                            GIT_REPOSITORY ${GOOGLETEST_GIT_REPO}
                            GIT_TAG        release-${GOOGLETEST_VERSION}
                            GIT_SHALLOW    1
                            UPDATE_COMMAND "")

        #FetchContent_MakeAvailable(googletest) # Not available in CMake 3.13 :-( Let's do it manually:
        FetchContent_GetProperties(googletest)
        if(NOT googletest_POPULATED)
            FetchContent_Populate(googletest)
            set(gtest_force_shared_crt ON CACHE INTERNAL "" FORCE)
            set(BUILD_GMOCK ON CACHE INTERNAL "" FORCE)
            set(INSTALL_GTEST OFF CACHE INTERNAL "" FORCE)
            set(BUILD_SHARED_LIBS_BAK ${BUILD_SHARED_LIBS})
            set(BUILD_SHARED_LIBS OFF)
            add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
            set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_BAK})
            add_library(GTest::gmock ALIAS gmock)
        endif()
    endif()
endif()

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
    LIBSYSTEMD_VERSION=${LIBSYSTEMD_VERSION}
    SDBUS_HEADER=<${LIBSYSTEMD}/sd-bus.h>)
target_link_libraries(sdbus-c++-unit-tests sdbus-c++-objlib GTest::gmock)

add_executable(sdbus-c++-integration-tests ${INTEGRATIONTESTS_SRCS})
target_compile_definitions(sdbus-c++-integration-tests PRIVATE LIBSYSTEMD_VERSION=${LIBSYSTEMD_VERSION})
target_link_libraries(sdbus-c++-integration-tests sdbus-c++ GTest::gmock)

# Manual performance and stress tests
option(ENABLE_PERF_TESTS "Build and install manual performance tests (default OFF)" OFF)
option(ENABLE_STRESS_TESTS "Build and install manual stress tests (default OFF)" OFF)

if(ENABLE_PERF_TESTS OR ENABLE_STRESS_TESTS)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)

    if(ENABLE_PERF_TESTS)
        message(STATUS "Building with performance tests")
        add_executable(sdbus-c++-perf-tests-client ${STRESSTESTS_CLIENT_SRCS})
        target_link_libraries(sdbus-c++-perf-tests-client sdbus-c++ Threads::Threads)
        add_executable(sdbus-c++-perf-tests-server ${STRESSTESTS_SERVER_SRCS})
        target_link_libraries(sdbus-c++-perf-tests-server sdbus-c++ Threads::Threads)
    endif()

    if(ENABLE_STRESS_TESTS)
        message(STATUS "Building with stress tests")
        add_executable(sdbus-c++-stress-tests ${STRESSTESTS_SRCS})
        target_link_libraries(sdbus-c++-stress-tests sdbus-c++ Threads::Threads)
    endif()
endif()

#----------------------------------
# INSTALLATION
#----------------------------------

option(INSTALL_TESTS "Install tests (default OFF)" OFF)

if(INSTALL_TESTS)
    include(GNUInstallDirs)
    set(TESTS_INSTALL_PATH "tests/${PROJECT_NAME}" CACHE STRING "Specifies where the test binaries will be installed")

    install(TARGETS sdbus-c++-unit-tests DESTINATION ${TESTS_INSTALL_PATH} COMPONENT test)
    install(TARGETS sdbus-c++-integration-tests DESTINATION ${TESTS_INSTALL_PATH} COMPONENT test)
    install(FILES ${INTEGRATIONTESTS_SOURCE_DIR}/files/org.sdbuscpp.integrationtests.conf DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/dbus-1/system.d COMPONENT test)

    if(ENABLE_PERF_TESTS)
        install(TARGETS sdbus-c++-perf-tests-client DESTINATION ${TESTS_INSTALL_PATH} COMPONENT test)
        install(TARGETS sdbus-c++-perf-tests-server DESTINATION ${TESTS_INSTALL_PATH} COMPONENT test)
        install(FILES ${PERFTESTS_SOURCE_DIR}/files/org.sdbuscpp.perftests.conf DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/dbus-1/system.d COMPONENT test)
    endif()

    if(ENABLE_STRESS_TESTS)
        install(TARGETS sdbus-c++-stress-tests DESTINATION ${TESTS_INSTALL_PATH} COMPONENT test)
        install(FILES ${STRESSTESTS_SOURCE_DIR}/files/org.sdbuscpp.stresstests.conf DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/dbus-1/system.d COMPONENT test)
    endif()
endif()

#----------------------------------
# RUNNING THE TESTS UPON BUILD
#----------------------------------

if(NOT CMAKE_CROSSCOMPILING)
    add_test(NAME sdbus-c++-unit-tests COMMAND sdbus-c++-unit-tests)
    add_test(NAME sdbus-c++-integration-tests COMMAND sdbus-c++-integration-tests)
endif()
