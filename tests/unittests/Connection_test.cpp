/**
 * (C) 2016 - 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2019 Stanislav Angelovic <angelovic.s@gmail.com>
 *
 * @file Connection_test.cpp
 * @author Ardazishvili Roman (ardazishvili.roman@yandex.ru)
 *
 * Created on: Feb 4, 2019
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

#include "Connection.h"
#include "unittests/mocks/SdBusMock.h"

#include <gtest/gtest.h>

using ::testing::_;
using ::testing::DoAll;
using ::testing::SetArgPointee;
using ::testing::Return;
using ::testing::NiceMock;
using sdbus::internal::Connection;
constexpr sdbus::internal::Connection::default_bus_t default_bus;
constexpr sdbus::internal::Connection::system_bus_t system_bus;
constexpr sdbus::internal::Connection::session_bus_t session_bus;
constexpr sdbus::internal::Connection::remote_system_bus_t remote_system_bus;

class ConnectionCreationTest : public ::testing::Test
{
protected:
    ConnectionCreationTest() = default;

    std::unique_ptr<NiceMock<SdBusMock>> sdBusIntfMock_ = std::make_unique<NiceMock<SdBusMock>>();
    sd_bus* fakeBusPtr_ = reinterpret_cast<sd_bus*>(1);
};

using ADefaultBusConnection = ConnectionCreationTest;
using ASystemBusConnection = ConnectionCreationTest;
using ASessionBusConnection = ConnectionCreationTest;

TEST_F(ADefaultBusConnection, OpensAndFlushesBusWhenCreated)
{
    EXPECT_CALL(*sdBusIntfMock_, sd_bus_open(_)).WillOnce(DoAll(SetArgPointee<0>(fakeBusPtr_), Return(1)));
    EXPECT_CALL(*sdBusIntfMock_, sd_bus_flush(_)).Times(1);
    Connection(std::move(sdBusIntfMock_), default_bus);
}

TEST_F(ASystemBusConnection, OpensAndFlushesBusWhenCreated)
{
    EXPECT_CALL(*sdBusIntfMock_, sd_bus_open_system(_)).WillOnce(DoAll(SetArgPointee<0>(fakeBusPtr_), Return(1)));
    EXPECT_CALL(*sdBusIntfMock_, sd_bus_flush(_)).Times(1);
    Connection(std::move(sdBusIntfMock_), system_bus);
}

TEST_F(ASessionBusConnection, OpensAndFlushesBusWhenCreated)
{
    EXPECT_CALL(*sdBusIntfMock_, sd_bus_open_user(_)).WillOnce(DoAll(SetArgPointee<0>(fakeBusPtr_), Return(1)));
    EXPECT_CALL(*sdBusIntfMock_, sd_bus_flush(_)).Times(1);
    Connection(std::move(sdBusIntfMock_), session_bus);
}

TEST_F(ADefaultBusConnection, ClosesAndUnrefsBusWhenDestructed)
{
    ON_CALL(*sdBusIntfMock_, sd_bus_open(_)).WillByDefault(DoAll(SetArgPointee<0>(fakeBusPtr_), Return(1)));
    EXPECT_CALL(*sdBusIntfMock_, sd_bus_flush_close_unref(_)).Times(1);
    Connection(std::move(sdBusIntfMock_), default_bus);
}

TEST_F(ASystemBusConnection, ClosesAndUnrefsBusWhenDestructed)
{
    ON_CALL(*sdBusIntfMock_, sd_bus_open_system(_)).WillByDefault(DoAll(SetArgPointee<0>(fakeBusPtr_), Return(1)));
    EXPECT_CALL(*sdBusIntfMock_, sd_bus_flush_close_unref(_)).Times(1);
    Connection(std::move(sdBusIntfMock_), system_bus);
}

TEST_F(ASessionBusConnection, ClosesAndUnrefsBusWhenDestructed)
{
    ON_CALL(*sdBusIntfMock_, sd_bus_open_user(_)).WillByDefault(DoAll(SetArgPointee<0>(fakeBusPtr_), Return(1)));
    EXPECT_CALL(*sdBusIntfMock_, sd_bus_flush_close_unref(_)).Times(1);
    Connection(std::move(sdBusIntfMock_), session_bus);
}

TEST_F(ADefaultBusConnection, ThrowsErrorWhenOpeningTheBusFailsDuringConstruction)
{
    ON_CALL(*sdBusIntfMock_, sd_bus_open(_)).WillByDefault(DoAll(SetArgPointee<0>(fakeBusPtr_), Return(-1)));
    ASSERT_THROW(Connection(std::move(sdBusIntfMock_), default_bus), sdbus::Error);
}

TEST_F(ASystemBusConnection, ThrowsErrorWhenOpeningTheBusFailsDuringConstruction)
{
    ON_CALL(*sdBusIntfMock_, sd_bus_open_system(_)).WillByDefault(DoAll(SetArgPointee<0>(fakeBusPtr_), Return(-1)));
    ASSERT_THROW(Connection(std::move(sdBusIntfMock_), system_bus), sdbus::Error);
}

TEST_F(ASessionBusConnection, ThrowsErrorWhenOpeningTheBusFailsDuringConstruction)
{
    ON_CALL(*sdBusIntfMock_, sd_bus_open_user(_)).WillByDefault(DoAll(SetArgPointee<0>(fakeBusPtr_), Return(-1)));
    ASSERT_THROW(Connection(std::move(sdBusIntfMock_), session_bus), sdbus::Error);
}

TEST_F(ADefaultBusConnection, ThrowsErrorWhenFlushingTheBusFailsDuringConstruction)
{
    ON_CALL(*sdBusIntfMock_, sd_bus_open(_)).WillByDefault(DoAll(SetArgPointee<0>(fakeBusPtr_), Return(1)));
    ON_CALL(*sdBusIntfMock_, sd_bus_flush(_)).WillByDefault(Return(-1));
    ASSERT_THROW(Connection(std::move(sdBusIntfMock_), default_bus), sdbus::Error);
}

TEST_F(ASystemBusConnection, ThrowsErrorWhenFlushingTheBusFailsDuringConstruction)
{
    ON_CALL(*sdBusIntfMock_, sd_bus_open_system(_)).WillByDefault(DoAll(SetArgPointee<0>(fakeBusPtr_), Return(1)));
    ON_CALL(*sdBusIntfMock_, sd_bus_flush(_)).WillByDefault(Return(-1));
    ASSERT_THROW(Connection(std::move(sdBusIntfMock_), system_bus), sdbus::Error);
}

TEST_F(ASessionBusConnection, ThrowsErrorWhenFlushingTheBusFailsDuringConstruction)
{
    ON_CALL(*sdBusIntfMock_, sd_bus_open_user(_)).WillByDefault(DoAll(SetArgPointee<0>(fakeBusPtr_), Return(1)));
    ON_CALL(*sdBusIntfMock_, sd_bus_flush(_)).WillByDefault(Return(-1));
    ASSERT_THROW(Connection(std::move(sdBusIntfMock_), session_bus), sdbus::Error);
}

namespace
{
template <typename _BusTypeTag>
class AConnectionNameRequest : public ::testing::Test
{
protected:
    void setUpBusOpenExpectation();
    std::unique_ptr<Connection> makeConnection();

    void SetUp() override
    {
        setUpBusOpenExpectation();
        ON_CALL(*sdBusIntfMock_, sd_bus_flush(_)).WillByDefault(Return(1));
        ON_CALL(*sdBusIntfMock_, sd_bus_flush_close_unref(_)).WillByDefault(Return(fakeBusPtr_));
        con_ = makeConnection();
    }

    NiceMock<SdBusMock>* sdBusIntfMock_ = new NiceMock<SdBusMock>(); // con_ below will assume ownership
    sd_bus* fakeBusPtr_ = reinterpret_cast<sd_bus*>(1);
    std::unique_ptr<Connection> con_;
};

template<> void AConnectionNameRequest<Connection::default_bus_t>::setUpBusOpenExpectation()
{
    EXPECT_CALL(*sdBusIntfMock_, sd_bus_open(_)).WillOnce(DoAll(SetArgPointee<0>(fakeBusPtr_), Return(1)));
}
template<> void AConnectionNameRequest<Connection::system_bus_t>::setUpBusOpenExpectation()
{
    EXPECT_CALL(*sdBusIntfMock_, sd_bus_open_system(_)).WillOnce(DoAll(SetArgPointee<0>(fakeBusPtr_), Return(1)));
}
template<> void AConnectionNameRequest<Connection::session_bus_t>::setUpBusOpenExpectation()
{
    EXPECT_CALL(*sdBusIntfMock_, sd_bus_open_user(_)).WillOnce(DoAll(SetArgPointee<0>(fakeBusPtr_), Return(1)));
}
template<> void AConnectionNameRequest<Connection::remote_system_bus_t>::setUpBusOpenExpectation()
{
    EXPECT_CALL(*sdBusIntfMock_, sd_bus_open_system_remote(_, _)).WillOnce(DoAll(SetArgPointee<0>(fakeBusPtr_), Return(1)));
}
template <typename _BusTypeTag>
std::unique_ptr<Connection> AConnectionNameRequest<_BusTypeTag>::makeConnection()
{
    return std::make_unique<Connection>(std::unique_ptr<NiceMock<SdBusMock>>(sdBusIntfMock_), _BusTypeTag{});
}
template<> std::unique_ptr<Connection> AConnectionNameRequest<Connection::remote_system_bus_t>::makeConnection()
{
    return std::make_unique<Connection>(std::unique_ptr<NiceMock<SdBusMock>>(sdBusIntfMock_), remote_system_bus, "some host");
}

typedef ::testing::Types< Connection::default_bus_t
                        , Connection::system_bus_t
                        , Connection::session_bus_t
                        , Connection::remote_system_bus_t
                        > BusTypeTags;

TYPED_TEST_SUITE(AConnectionNameRequest, BusTypeTags);
}

TYPED_TEST(AConnectionNameRequest, DoesNotThrowOnSuccess)
{
    EXPECT_CALL(*this->sdBusIntfMock_, sd_bus_request_name(_, _, _)).WillOnce(Return(1));
    this->con_->requestName("org.sdbuscpp.somename");
}

TYPED_TEST(AConnectionNameRequest, ThrowsOnFail)
{
    EXPECT_CALL(*this->sdBusIntfMock_, sd_bus_request_name(_, _, _)).WillOnce(Return(-1));

    ASSERT_THROW(this->con_->requestName("org.sdbuscpp.somename"), sdbus::Error);
}
