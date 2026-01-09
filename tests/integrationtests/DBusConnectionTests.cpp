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

// STL
#include <thread>

using namespace sdbus::test;

/*-------------------------------------*/
/* --          TEST CASES           -- */
/*-------------------------------------*/

TEST(Connection, CanBeDefaultConstructed)
{
    ASSERT_NO_THROW(auto con = sdbus::createBusConnection());
}

TEST(Connection, CanRequestName)
{
    auto connection = sdbus::createBusConnection();

    // In case of system bus connection, requesting may throw as we need to allow that first through a config file in /etc/dbus-1/system.d
    ASSERT_NO_THROW(connection->requestName(SERVICE_NAME))
        << "Perhaps you've forgotten to copy `org.sdbuscpp.integrationtests.conf` file to `/etc/dbus-1/system.d` directory before running the tests?";
}

TEST(SystemBusConnection, CannotRequestNonregisteredDbusName)
{
    auto connection = sdbus::createSystemBusConnection();
    sdbus::ServiceName const notSupportedBusName{"some.random.not.supported.dbus.name"};

    ASSERT_THROW(connection->requestName(notSupportedBusName), sdbus::Error);
}

TEST(Connection, CanReleaseRequestedName)
{
    auto connection = sdbus::createBusConnection();
    connection->requestName(SERVICE_NAME);

    ASSERT_NO_THROW(connection->releaseName(SERVICE_NAME));
}

TEST(Connection, CannotReleaseNonrequestedName)
{
    auto connection = sdbus::createBusConnection();
    sdbus::ServiceName const notAcquiredBusName{"some.random.unacquired.name"};

    ASSERT_THROW(connection->releaseName(notAcquiredBusName), sdbus::Error);
}

TEST(Connection, CanEnterAndLeaveInternalEventLoop)
{
    auto connection = sdbus::createBusConnection();
    connection->requestName(SERVICE_NAME);

    std::thread t([&](){ connection->enterEventLoop(); });
    connection->leaveEventLoop();

    t.join();
}
