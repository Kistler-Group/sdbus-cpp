/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2024 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file DBusConnectionTests.cpp
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

// Own
#include "Defs.h"

// sdbus
#include <sdbus-c++/Error.h>
#include <sdbus-c++/IConnection.h>

// gmock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// STL
#include <thread>
#include <chrono>

using ::testing::Eq;
using namespace sdbus::test;
using namespace std::chrono_literals;

/*-------------------------------------*/
/* --          TEST CASES           -- */
/*-------------------------------------*/

TEST(Connection, CanBeDefaultConstructed)
{
    ASSERT_NO_THROW(auto con = sdbus::createConnection());
}

TEST(Connection, CanRequestRegisteredDbusName)
{
    auto connection = sdbus::createConnection();

    ASSERT_NO_THROW(connection->requestName(BUS_NAME))
        << "Perhaps you've forgotten to copy `org.sdbuscpp.integrationtests.conf` file to `/etc/dbus-1/system.d` directory before running the tests?";
}

TEST(Connection, CannotRequestNonregisteredDbusName)
{
    auto connection = sdbus::createConnection();
    ASSERT_THROW(connection->requestName("some.random.not.supported.dbus.name"), sdbus::Error);
}

TEST(Connection, CanReleasedRequestedName)
{
    auto connection = sdbus::createConnection();

    connection->requestName(BUS_NAME);
    ASSERT_NO_THROW(connection->releaseName(BUS_NAME));
}

TEST(Connection, CannotReleaseNonrequestedName)
{
    auto connection = sdbus::createConnection();
    ASSERT_THROW(connection->releaseName("some.random.nonrequested.name"), sdbus::Error);
}

TEST(Connection, CanEnterAndLeaveEventLoop)
{
    auto connection = sdbus::createConnection();
    connection->requestName(BUS_NAME);

    std::thread t([&](){ connection->enterEventLoop(); });
    connection->leaveEventLoop();

    t.join();
}

TEST(Connection, PollDataGetZeroTimeout)
{
    sdbus::IConnection::PollData pd{};
    pd.timeout_usec = 0;
    ASSERT_TRUE(pd.getRelativeTimeout().has_value());
    EXPECT_THAT(pd.getRelativeTimeout().value(), Eq(std::chrono::microseconds::zero()));
    EXPECT_THAT(pd.getPollTimeout(), Eq(0));
}

TEST(Connection, PollDataGetInfiniteTimeout)
{
    sdbus::IConnection::PollData pd{};
    pd.timeout_usec = UINT64_MAX;
    ASSERT_FALSE(pd.getRelativeTimeout().has_value());
    EXPECT_THAT(pd.getPollTimeout(), Eq(-1));
}

TEST(Connection, PollDataGetZeroRelativeTimeoutForPast)
{
    sdbus::IConnection::PollData pd{};
    auto past = std::chrono::steady_clock::now() - 10s;
    pd.timeout_usec = std::chrono::duration_cast<std::chrono::microseconds>(past.time_since_epoch()).count();
    ASSERT_TRUE(pd.getRelativeTimeout().has_value());
    EXPECT_THAT(pd.getRelativeTimeout().value(), Eq(0us));
    EXPECT_THAT(pd.getPollTimeout(), Eq(0));
}

TEST(Connection, PollDataGetRelativeTimeoutInTolerance)
{
    sdbus::IConnection::PollData pd{};
    constexpr auto TIMEOUT = 1s;
    constexpr auto TOLERANCE = 100ms;
    auto future = std::chrono::steady_clock::now() + TIMEOUT;
    pd.timeout_usec = std::chrono::duration_cast<std::chrono::microseconds>(future.time_since_epoch()).count();
    ASSERT_TRUE(pd.getRelativeTimeout().has_value());
    EXPECT_GE(pd.getRelativeTimeout().value(), TIMEOUT - TOLERANCE);
    EXPECT_LE(pd.getRelativeTimeout().value(), TIMEOUT + TOLERANCE);
    EXPECT_GE(pd.getPollTimeout(), 900);
    EXPECT_LE(pd.getPollTimeout(), 1100);
}
