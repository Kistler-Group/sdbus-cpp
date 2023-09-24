/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2022 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file DBusGeneralTests.cpp
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

#include "TestAdaptor.h"
#include "TestProxy.h"
#include "TestFixture.h"
#include "sdbus-c++/sdbus-c++.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <thread>
#include <tuple>
#include <chrono>
#include <fstream>
#include <future>
#include <unistd.h>
#include <variant>

using ::testing::ElementsAre;
using ::testing::Eq;
using namespace std::chrono_literals;
using namespace sdbus::test;

using AConnection = TestFixture;
using ADirectConnection = TestFixtureWithDirectConnection;

/*-------------------------------------*/
/* --          TEST CASES           -- */
/*-------------------------------------*/

TEST(AdaptorAndProxy, CanBeConstructedSuccesfully)
{
    auto connection = sdbus::createConnection();
    connection->requestName(BUS_NAME);

    ASSERT_NO_THROW(TestAdaptor adaptor(*connection, OBJECT_PATH));
    ASSERT_NO_THROW(TestProxy proxy(BUS_NAME, OBJECT_PATH));
}

TEST(AProxy, SupportsMoveSemantics)
{
    static_assert(std::is_move_constructible_v<DummyTestProxy>);
    static_assert(std::is_move_assignable_v<DummyTestProxy>);
}

TEST(AnAdaptor, SupportsMoveSemantics)
{
    static_assert(std::is_move_constructible_v<DummyTestAdaptor>);
    static_assert(std::is_move_assignable_v<DummyTestAdaptor>);
}

TEST_F(AConnection, WillCallCallbackHandlerForIncomingMessageMatchingMatchRule)
{
    auto matchRule = "sender='" + BUS_NAME + "',path='" + OBJECT_PATH + "'";
    std::atomic<bool> matchingMessageReceived{false};
    auto slot = s_proxyConnection->addMatch(matchRule, [&](sdbus::Message& msg)
    {
        if(msg.getPath() == OBJECT_PATH)
            matchingMessageReceived = true;
    });

    m_adaptor->emitSimpleSignal();

    ASSERT_TRUE(waitUntil(matchingMessageReceived));
}

TEST_F(AConnection, WillUnsubscribeMatchRuleWhenClientDestroysTheAssociatedSlot)
{
    auto matchRule = "sender='" + BUS_NAME + "',path='" + OBJECT_PATH + "'";
    std::atomic<bool> matchingMessageReceived{false};
    auto slot = s_proxyConnection->addMatch(matchRule, [&](sdbus::Message& msg)
    {
        if(msg.getPath() == OBJECT_PATH)
            matchingMessageReceived = true;
    });
    slot.reset();

    m_adaptor->emitSimpleSignal();

    ASSERT_FALSE(waitUntil(matchingMessageReceived, 2s));
}

TEST_F(AConnection, CanAddFloatingMatchRule)
{
    auto matchRule = "sender='" + BUS_NAME + "',path='" + OBJECT_PATH + "'";
    std::atomic<bool> matchingMessageReceived{false};
    auto con = sdbus::createSystemBusConnection();
    con->enterEventLoopAsync();
    auto callback = [&](sdbus::Message& msg)
    {
        if(msg.getPath() == OBJECT_PATH)
            matchingMessageReceived = true;
    };
    con->addMatch(matchRule, std::move(callback), sdbus::floating_slot);
    m_adaptor->emitSimpleSignal();
    [[maybe_unused]] auto gotMessage = waitUntil(matchingMessageReceived, 2s);
    assert(gotMessage);
    matchingMessageReceived = false;

    con.reset();
    m_adaptor->emitSimpleSignal();

    ASSERT_FALSE(waitUntil(matchingMessageReceived, 2s));
}

TEST_F(AConnection, WillNotPassToMatchCallbackMessagesThatDoNotMatchTheRule)
{
    auto matchRule = "type='signal',interface='" + INTERFACE_NAME + "',member='simpleSignal'";
    std::atomic<size_t> numberOfMatchingMessages{};
    auto slot = s_proxyConnection->addMatch(matchRule, [&](sdbus::Message& msg)
    {
        if(msg.getMemberName() == "simpleSignal")
            numberOfMatchingMessages++;
    });
    auto adaptor2 = std::make_unique<TestAdaptor>(*s_adaptorConnection, OBJECT_PATH_2);

    m_adaptor->emitSignalWithMap({});
    adaptor2->emitSimpleSignal();
    m_adaptor->emitSimpleSignal();

    ASSERT_TRUE(waitUntil([&](){ return numberOfMatchingMessages == 2; }));
    ASSERT_FALSE(waitUntil([&](){ return numberOfMatchingMessages > 2; }, 1s));
}

// A simple direct connection test similar in nature to https://github.com/systemd/systemd/blob/main/src/libsystemd/sd-bus/test-bus-server.c
TEST_F(ADirectConnection, CanBeUsedBetweenClientAndServer)
{
    auto val = m_proxy->sumArrayItems({1, 7}, {2, 3, 4});
    m_adaptor->emitSimpleSignal();

    // Make sure method call passes and emitted signal is received
    ASSERT_THAT(val, Eq(1 + 7 + 2 + 3 + 4));
    ASSERT_TRUE(waitUntil(m_proxy->m_gotSimpleSignal));
}
