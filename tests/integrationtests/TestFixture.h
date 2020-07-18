/**
 * (C) 2016 - 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2019 Stanislav Angelovic <angelovic.s@gmail.com>
 *
 * @file TestAdaptor.h
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

#include <thread>
#include <chrono>
#include <atomic>
#include <chrono>

namespace sdbus { namespace test {

class TestFixture : public ::testing::Test
{
public:
    static void SetUpTestCase()
    {
        s_connection->requestName(INTERFACE_NAME);
        s_connection->enterEventLoopAsync();
    }

    static void TearDownTestCase()
    {
        s_connection->leaveEventLoop();
        s_connection->releaseName(INTERFACE_NAME);
    }

    template <typename _Fnc>
    static bool waitUntil(_Fnc&& fnc, std::chrono::milliseconds timeout = std::chrono::seconds(5))
    {
        using namespace std::chrono_literals;

        std::chrono::milliseconds elapsed{};
        std::chrono::milliseconds step{5ms};
        do {
            std::this_thread::sleep_for(step);
            elapsed += step;
            if (elapsed > timeout)
                return false;
        } while (!fnc());

        return true;
    }

    static bool waitUntil(std::atomic<bool>& flag, std::chrono::milliseconds timeout = std::chrono::seconds(5))
    {
        return waitUntil([&flag]() -> bool { return flag; }, timeout);
    }

private:
    void SetUp() override
    {
        m_adaptor = std::make_unique<TestAdaptor>(*s_connection);
        m_proxy = std::make_unique<TestProxy>(INTERFACE_NAME, OBJECT_PATH);
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Give time for the proxy to start listening to signals
    }

    void TearDown() override
    {
        m_proxy.reset();
        m_adaptor.reset();
    }

public:
    static std::unique_ptr<sdbus::IConnection> s_connection;

    std::unique_ptr<TestAdaptor> m_adaptor;
    std::unique_ptr<TestProxy> m_proxy;
};

}}

#endif /* SDBUS_CPP_INTEGRATIONTESTS_TESTFIXTURE_H_ */
