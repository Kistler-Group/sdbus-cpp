/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2026 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
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
#include "Defs.h"
#include <sdbus-c++/sdbus-c++.h>

#include <atomic>
#include <cassert>
#include <cstddef>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string_view>
#include <chrono>
#include <type_traits>

using ::testing::Eq;
using namespace std::chrono_literals;
using namespace sdbus::test;
using namespace std::string_view_literals;

using ADirectConnection = TestFixtureWithDirectConnection;

/*-------------------------------------*/
/* --          TEST CASES           -- */
/*-------------------------------------*/

TEST(AdaptorAndProxy, CanBeConstructedSuccessfully)
{
    auto connection = sdbus::createBusConnection();
    connection->requestName(SERVICE_NAME);

    ASSERT_NO_THROW(const TestAdaptor adaptor(*connection, OBJECT_PATH));
    ASSERT_NO_THROW(const TestProxy proxy(SERVICE_NAME, OBJECT_PATH));

    connection->releaseName(SERVICE_NAME);
}

TEST(AProxy, DoesNotSupportMoveSemantics)
{
    static_assert(!std::is_move_constructible_v<DummyTestProxy>);
    static_assert(!std::is_move_assignable_v<DummyTestProxy>);
}

TEST(AnAdaptor, DoesNotSupportMoveSemantics)
{
    static_assert(!std::is_move_constructible_v<DummyTestAdaptor>);
    static_assert(!std::is_move_assignable_v<DummyTestAdaptor>);
}

TYPED_TEST(AConnection, WillCallCallbackHandlerForIncomingMessageMatchingMatchRule)
{
    auto matchRule = "sender='" + SERVICE_NAME + "',path='" + OBJECT_PATH + "'";
    std::atomic<bool> matchingMessageReceived{false};
    auto slot = this->s_proxyConnection->addMatch(matchRule, [&](const sdbus::Message& msg)
    {
        if(msg.getPath() == OBJECT_PATH)
            matchingMessageReceived = true;
    }, sdbus::return_slot);

    this->m_adaptor->emitSimpleSignal();

    ASSERT_TRUE(waitUntil(matchingMessageReceived));
}

TYPED_TEST(AConnection, CanInstallMatchRuleAsynchronously)
{
    auto matchRule = "sender='" + SERVICE_NAME + "',path='" + OBJECT_PATH + "'";
    std::atomic<bool> matchingMessageReceived{false};
    std::atomic<bool> matchRuleInstalled{false};
    auto slot = this->s_proxyConnection->addMatchAsync( matchRule
                                                      , [&](const sdbus::Message& msg)
                                                        {
                                                            if(msg.getPath() == OBJECT_PATH)
                                                                matchingMessageReceived = true;
                                                        }
                                                      , [&](const sdbus::Message& /*msg*/)
                                                        {
                                                            matchRuleInstalled = true;
                                                        }
                                                      , sdbus::return_slot );

    EXPECT_TRUE(waitUntil(matchRuleInstalled));

    this->m_adaptor->emitSimpleSignal();

    ASSERT_TRUE(waitUntil(matchingMessageReceived));
}

TYPED_TEST(AConnection, WillUnsubscribeMatchRuleWhenClientDestroysTheAssociatedSlot)
{
    auto matchRule = "sender='" + SERVICE_NAME + "',path='" + OBJECT_PATH + "'";
    std::atomic<bool> matchingMessageReceived{false};
    auto slot = this->s_proxyConnection->addMatch(matchRule, [&](const sdbus::Message& msg)
    {
        if(msg.getPath() == OBJECT_PATH)
            matchingMessageReceived = true;
    }, sdbus::return_slot);
    slot.reset();

    this->m_adaptor->emitSimpleSignal();

    ASSERT_FALSE(waitUntil(matchingMessageReceived, 1s));
}

TYPED_TEST(AConnection, CanAddFloatingMatchRule)
{
    auto matchRule = "sender='" + SERVICE_NAME + "',path='" + OBJECT_PATH + "'";
    std::atomic<bool> matchingMessageReceived{false};
    auto con = sdbus::createBusConnection();
    con->enterEventLoopAsync();
    auto callback = [&](const sdbus::Message& msg)
    {
        if(msg.getPath() == OBJECT_PATH)
            matchingMessageReceived = true;
    };
    con->addMatch(matchRule, std::move(callback));
    this->m_adaptor->emitSimpleSignal();
    [[maybe_unused]] auto gotMessage = waitUntil(matchingMessageReceived, 2s);
    assert(gotMessage);
    matchingMessageReceived = false;

    con.reset();
    this->m_adaptor->emitSimpleSignal();

    ASSERT_FALSE(waitUntil(matchingMessageReceived, 1s));
}

TYPED_TEST(AConnection, WillNotPassToMatchCallbackMessagesThatDoNotMatchTheRule)
{
    auto matchRule = "type='signal',interface='" + INTERFACE_NAME + "',member='simpleSignal'";
    std::atomic<size_t> numberOfMatchingMessages{};
    auto slot = this->s_proxyConnection->addMatch(matchRule, [&](const sdbus::Message& msg)
    {
        if(msg.getMemberName() == "simpleSignal"sv)
            numberOfMatchingMessages++;
    }, sdbus::return_slot);
    auto adaptor2 = std::make_unique<TestAdaptor>(*this->s_adaptorConnection, OBJECT_PATH_2);

    this->m_adaptor->emitSignalWithMap({});
    adaptor2->emitSimpleSignal();
    this->m_adaptor->emitSimpleSignal();

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
