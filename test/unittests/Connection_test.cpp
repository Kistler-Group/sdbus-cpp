/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
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

using BusType = sdbus::internal::Connection::BusType;


class ConnectionCreationTest : public ::testing::Test
{
protected:
    ConnectionCreationTest() = default;

    std::unique_ptr<NiceMock<SdBusMock>> mock_ { std::make_unique<NiceMock<SdBusMock>>() };
    sd_bus* STUB_ { reinterpret_cast<sd_bus*>(1) };
};

using ASystemBusConnection = ConnectionCreationTest;
using ASessionBusConnection = ConnectionCreationTest;

TEST_F(ASystemBusConnection, OpensAndFlushesBusWhenCreated)
{
    EXPECT_CALL(*mock_, sd_bus_open_system(_)).WillOnce(DoAll(SetArgPointee<0>(STUB_), Return(1)));
    EXPECT_CALL(*mock_, sd_bus_flush(_)).Times(1);
    sdbus::internal::Connection(BusType::eSystem, std::move(mock_));
}

TEST_F(ASessionBusConnection, OpensAndFlushesBusWhenCreated)
{
    EXPECT_CALL(*mock_, sd_bus_open_user(_)).WillOnce(DoAll(SetArgPointee<0>(STUB_), Return(1)));
    EXPECT_CALL(*mock_, sd_bus_flush(_)).Times(1);
    sdbus::internal::Connection(BusType::eSession, std::move(mock_));
}

TEST_F(ASystemBusConnection, ClosesAndUnrefsBusWhenDestructed)
{
    ON_CALL(*mock_, sd_bus_open_user(_)).WillByDefault(DoAll(SetArgPointee<0>(STUB_), Return(1)));
    EXPECT_CALL(*mock_, sd_bus_flush_close_unref(_)).Times(1);
    sdbus::internal::Connection(BusType::eSession, std::move(mock_));
}

TEST_F(ASessionBusConnection, ClosesAndUnrefsBusWhenDestructed)
{
    ON_CALL(*mock_, sd_bus_open_user(_)).WillByDefault(DoAll(SetArgPointee<0>(STUB_), Return(1)));
    EXPECT_CALL(*mock_, sd_bus_flush_close_unref(_)).Times(1);
    sdbus::internal::Connection(BusType::eSession, std::move(mock_));
}

TEST_F(ASystemBusConnection, ThrowsErrorWhenOpeningTheBusFailsDuringConstruction)
{
    ON_CALL(*mock_, sd_bus_open_system(_)).WillByDefault(DoAll(SetArgPointee<0>(STUB_), Return(-1)));
    ASSERT_THROW(sdbus::internal::Connection(BusType::eSystem, std::move(mock_)), sdbus::Error);
}

TEST_F(ASessionBusConnection, ThrowsErrorWhenOpeningTheBusFailsDuringConstruction)
{
    ON_CALL(*mock_, sd_bus_open_user(_)).WillByDefault(DoAll(SetArgPointee<0>(STUB_), Return(-1)));
    ASSERT_THROW(sdbus::internal::Connection(BusType::eSession, std::move(mock_)), sdbus::Error);
}

TEST_F(ASystemBusConnection, ThrowsErrorWhenFlushingTheBusFailsDuringConstruction)
{
    ON_CALL(*mock_, sd_bus_open_system(_)).WillByDefault(DoAll(SetArgPointee<0>(STUB_), Return(1)));
    ON_CALL(*mock_, sd_bus_flush(_)).WillByDefault(Return(-1));
    ASSERT_THROW(sdbus::internal::Connection(BusType::eSystem, std::move(mock_)), sdbus::Error);
}

TEST_F(ASessionBusConnection, ThrowsErrorWhenFlushingTheBusFailsDuringConstruction)
{
    ON_CALL(*mock_, sd_bus_open_user(_)).WillByDefault(DoAll(SetArgPointee<0>(STUB_), Return(1)));
    ON_CALL(*mock_, sd_bus_flush(_)).WillByDefault(Return(-1));
    ASSERT_THROW(sdbus::internal::Connection(BusType::eSession, std::move(mock_)), sdbus::Error);
}

class ConnectionRequestTest : public ::testing::TestWithParam<BusType>
{
protected:
    ConnectionRequestTest() = default;
    void SetUp() override
    {
        switch (GetParam())
        {
            case BusType::eSystem:
                EXPECT_CALL(*mock_, sd_bus_open_system(_)).WillOnce(DoAll(SetArgPointee<0>(STUB_), Return(1)));
                break;
            case BusType::eSession:
                EXPECT_CALL(*mock_, sd_bus_open_user(_)).WillOnce(DoAll(SetArgPointee<0>(STUB_), Return(1)));
                break;
            default:
                break;
        }
        ON_CALL(*mock_, sd_bus_flush(_)).WillByDefault(Return(1));
        ON_CALL(*mock_, sd_bus_flush_close_unref(_)).WillByDefault(Return(STUB_));
    }

    std::unique_ptr<NiceMock<SdBusMock>> mock_ { std::make_unique<NiceMock<SdBusMock>>() };
    sd_bus* STUB_ { reinterpret_cast<sd_bus*>(1) };
};

using AConnectionNameRequest = ConnectionRequestTest;

TEST_P(AConnectionNameRequest, DoesNotThrowOnSuccess)
{
    EXPECT_CALL(*mock_, sd_bus_request_name(_, _, _)).WillOnce(Return(1));
    sdbus::internal::Connection(GetParam(), std::move(mock_)).requestName("");
}

TEST_P(AConnectionNameRequest, ThrowsOnFail)
{
    EXPECT_CALL(*mock_, sd_bus_request_name(_, _, _)).WillOnce(Return(-1));

    auto conn_ = sdbus::internal::Connection(GetParam(), std::move(mock_)) ;
    ASSERT_THROW(conn_.requestName(""), sdbus::Error);
}

INSTANTIATE_TEST_SUITE_P(Request, AConnectionNameRequest, ::testing::Values(BusType::eSystem, BusType::eSession));
