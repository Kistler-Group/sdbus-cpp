/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2026 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file TestFixture.h
 *
 * Created on: Jan 2, 2017
 * Project: sdbus-c++
 * Description: High-level D-Bus IPC C++ library based on sd-bus
 *
 * This file is part of sdbus-c++.
 *
 * sdbus-c++ is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * sdbus-c++ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with sdbus-c++. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SDBUS_CPP_INTEGRATIONTESTS_TESTFIXTURE_H_
#define SDBUS_CPP_INTEGRATIONTESTS_TESTFIXTURE_H_

#include "TestAdaptor.h"
#include "TestProxy.h"
#include "Defs.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#ifndef SDBUS_basu // sd_event integration is not supported in basu-based sdbus-c++
#include <systemd/sd-event.h>
#endif // SDBUS_basu
#include <sys/eventfd.h>

#include <thread>
#include <chrono>
#include <atomic>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

namespace sdbus::test {

inline const uint32_t ANY_UNSIGNED_NUMBER{123};

class BaseTestFixture : public ::testing::Test
{
public:
    static void SetUpTestSuite()
    {
        s_adaptorConnection->requestName(SERVICE_NAME);
    }

    static void TearDownTestSuite()
    {
        s_adaptorConnection->releaseName(SERVICE_NAME);
    }

private:
    void SetUp() override
    {
        m_objectManagerProxy = std::make_unique<ObjectManagerTestProxy>(*s_proxyConnection, SERVICE_NAME, MANAGER_PATH);
        m_proxy = std::make_unique<TestProxy>(*s_proxyConnection, SERVICE_NAME, OBJECT_PATH);

        m_objectManagerAdaptor = std::make_unique<ObjectManagerTestAdaptor>(*s_adaptorConnection, MANAGER_PATH);
        m_adaptor = std::make_unique<TestAdaptor>(*s_adaptorConnection, OBJECT_PATH);
    }

    void TearDown() override
    {
        m_proxy.reset();
        m_adaptor.reset();
    }

public:
    static std::unique_ptr<sdbus::IConnection> s_adaptorConnection;
    static std::unique_ptr<sdbus::IConnection> s_proxyConnection;
    std::unique_ptr<ObjectManagerTestAdaptor> m_objectManagerAdaptor;
    std::unique_ptr<ObjectManagerTestProxy> m_objectManagerProxy;
    std::unique_ptr<TestAdaptor> m_adaptor;
    std::unique_ptr<TestProxy> m_proxy;
};

struct SdBusCppLoop{};
struct SdEventLoop{};

template <typename EventLoop>
class TestFixture : public BaseTestFixture{};

// Fixture working upon internal sdbus-c++ event loop
template <>
class TestFixture<SdBusCppLoop> : public BaseTestFixture
{
public:
    static void SetUpTestSuite()
    {
        BaseTestFixture::SetUpTestSuite();
        s_proxyConnection->enterEventLoopAsync();
        s_adaptorConnection->enterEventLoopAsync();
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Give time for the proxy connection to start listening to signals
    }

    static void TearDownTestSuite()
    {
        BaseTestFixture::TearDownTestSuite();
        s_adaptorConnection->leaveEventLoop();
        s_proxyConnection->leaveEventLoop();
    }
};

#ifndef SDBUS_basu // sd_event integration is not supported in basu-based sdbus-c++

// Fixture working upon attached external sd-event loop
template <>
class TestFixture<SdEventLoop> : public BaseTestFixture
{
public:
    static void SetUpTestSuite()
    {
        sd_event_new(&s_adaptorSdEvent);
        sd_event_new(&s_proxySdEvent);

        s_adaptorConnection->attachSdEventLoop(s_adaptorSdEvent);
        s_proxyConnection->attachSdEventLoop(s_proxySdEvent);

        s_eventExitFd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        auto exitHandler = [](sd_event_source *src, auto...){ return sd_event_exit(sd_event_source_get_event(src), 0); };
        sd_event_add_io(s_adaptorSdEvent, nullptr, s_eventExitFd, EPOLLIN, exitHandler, nullptr);
        sd_event_add_io(s_proxySdEvent, nullptr, s_eventExitFd, EPOLLIN, exitHandler, nullptr);

        s_adaptorEventLoopThread = std::thread([]()
        {
            sd_event_loop(s_adaptorSdEvent);
        });
        s_proxyEventLoopThread = std::thread([]()
        {
            sd_event_loop(s_proxySdEvent);
        });

        BaseTestFixture::SetUpTestSuite();
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Give time for the proxy connection to start listening to signals
    }

    static void TearDownTestSuite()
    {
        (void)eventfd_write(s_eventExitFd, 1);

        s_adaptorEventLoopThread.join();
        s_proxyEventLoopThread.join();

        sd_event_unref(s_adaptorSdEvent);
        sd_event_unref(s_proxySdEvent);
        close(s_eventExitFd);

        BaseTestFixture::TearDownTestSuite();
    }

private:
    static std::thread s_adaptorEventLoopThread;
    static std::thread s_proxyEventLoopThread;
    static sd_event *s_adaptorSdEvent;
    static sd_event *s_proxySdEvent;
    static int s_eventExitFd;
};

using EventLoopTags = ::testing::Types<SdBusCppLoop, SdEventLoop>;

#else // SDBUS_basu
using EventLoopTags = ::testing::Types<SdBusCppLoop>;
#endif // SDBUS_basu

TYPED_TEST_SUITE(TestFixture, EventLoopTags);

template <typename EventLoop>
using SdbusTestObject = TestFixture<EventLoop>;
TYPED_TEST_SUITE(SdbusTestObject, EventLoopTags);

template <typename EventLoop>
using AsyncSdbusTestObject = TestFixture<EventLoop>;
TYPED_TEST_SUITE(AsyncSdbusTestObject, EventLoopTags);

template <typename EventLoop>
using AConnection = TestFixture<EventLoop>;
TYPED_TEST_SUITE(AConnection, EventLoopTags);

class TestFixtureWithDirectConnection : public ::testing::Test
{
private:
    void SetUp() override
    {
        const int sock = openUnixSocket();
        createClientAndServerConnections(sock);
        createAdaptorAndProxyObjects();
    }

    void TearDown() override
    {
        m_proxy.reset();
        m_adaptor.reset();
        m_proxyConnection->leaveEventLoop();
        m_adaptorConnection->leaveEventLoop();
    }

    static int openUnixSocket()
    {
        const int sock = socket(AF_UNIX, SOCK_STREAM|SOCK_CLOEXEC, 0);
        assert(sock >= 0);

        sockaddr_un saddr{};
        memset(&saddr, 0, sizeof(saddr));
        saddr.sun_family = AF_UNIX;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
        (void)snprintf(saddr.sun_path, sizeof(saddr.sun_path), "%s", DIRECT_CONNECTION_SOCKET_PATH.c_str());

        unlink(DIRECT_CONNECTION_SOCKET_PATH.c_str());

        umask(0000);
        [[maybe_unused]] int r = bind(sock, reinterpret_cast<const sockaddr*>(&saddr), sizeof(saddr.sun_path));
        assert(r >= 0);

        r = listen(sock, 5);
        assert(r >= 0);

        return sock;
    }

    void createClientAndServerConnections(int sock)
    {
        std::thread t([&]()
        {
            auto fd = accept4(sock, nullptr, nullptr, /*SOCK_NONBLOCK|*/SOCK_CLOEXEC);
            m_adaptorConnection = sdbus::createServerBus(fd);
            // This is necessary so that createDirectBusConnection() below does not block
            m_adaptorConnection->enterEventLoopAsync();
        });

        m_proxyConnection = sdbus::createDirectBusConnection("unix:path=" + DIRECT_CONNECTION_SOCKET_PATH);
        m_proxyConnection->enterEventLoopAsync();

        t.join();
    }

    void createAdaptorAndProxyObjects()
    {
        assert(m_adaptorConnection != nullptr);
        assert(m_proxyConnection != nullptr);

        m_adaptor = std::make_unique<TestAdaptor>(*m_adaptorConnection, OBJECT_PATH);
        // Destination parameter can be empty in case of direct connections
        m_proxy = std::make_unique<TestProxy>(*m_proxyConnection, EMPTY_DESTINATION, OBJECT_PATH);
    }

public:
    std::unique_ptr<sdbus::IConnection> m_adaptorConnection;
    std::unique_ptr<sdbus::IConnection> m_proxyConnection;
    std::unique_ptr<TestAdaptor> m_adaptor;
    std::unique_ptr<TestProxy> m_proxy;
};

template <typename Fnc>
inline bool waitUntil(const Fnc& fnc, std::chrono::milliseconds timeout = std::chrono::seconds(5))
{
    using namespace std::chrono_literals;

    std::chrono::milliseconds elapsed{};
    const std::chrono::milliseconds step{5ms};
    do { // NOLINT(cppcoreguidelines-avoid-do-while)
        std::this_thread::sleep_for(step);
        elapsed += step;
        if (elapsed > timeout)
            return false;
    } while (!fnc());

    return true;
}

inline bool waitUntil(std::atomic<bool>& flag, std::chrono::milliseconds timeout = std::chrono::seconds(5))
{
    return waitUntil([&flag]() -> bool { return flag; }, timeout);
}

} // namespace sdbus::test

#endif /* SDBUS_CPP_INTEGRATIONTESTS_TESTFIXTURE_H_ */
