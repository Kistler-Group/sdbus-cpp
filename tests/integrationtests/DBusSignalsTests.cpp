/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2022 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file AdaptorAndProxy_test.cpp
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

#include "TestFixture.h"
#include "TestAdaptor.h"
#include "TestProxy.h"
#include "sdbus-c++/sdbus-c++.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <chrono>

using ::testing::Eq;
using ::testing::DoubleEq;
using ::testing::Gt;
using ::testing::AnyOf;
using ::testing::ElementsAre;
using ::testing::SizeIs;
using ::testing::NotNull;
using namespace std::chrono_literals;
using namespace sdbus::test;

/*-------------------------------------*/
/* --          TEST CASES           -- */
/*-------------------------------------*/

TYPED_TEST(SdbusTestObject, EmitsSimpleSignalSuccesfully)
{
    this->m_adaptor->emitSimpleSignal();

    ASSERT_TRUE(waitUntil(this->m_proxy->m_gotSimpleSignal));
}

TYPED_TEST(SdbusTestObject, EmitsSimpleSignalToMultipleProxiesSuccesfully)
{
    auto proxy1 = std::make_unique<TestProxy>(*this->s_adaptorConnection, BUS_NAME, OBJECT_PATH);
    auto proxy2 = std::make_unique<TestProxy>(*this->s_adaptorConnection, BUS_NAME, OBJECT_PATH);

    this->m_adaptor->emitSimpleSignal();

    ASSERT_TRUE(waitUntil(this->m_proxy->m_gotSimpleSignal));
    ASSERT_TRUE(waitUntil(proxy1->m_gotSimpleSignal));
    ASSERT_TRUE(waitUntil(proxy2->m_gotSimpleSignal));
}

TYPED_TEST(SdbusTestObject, ProxyDoesNotReceiveSignalFromOtherBusName)
{
    auto otherBusName = BUS_NAME + "2";
    auto connection2 = sdbus::createConnection(otherBusName);
    auto adaptor2 = std::make_unique<TestAdaptor>(*connection2, OBJECT_PATH);

    adaptor2->emitSimpleSignal();

    ASSERT_FALSE(waitUntil(this->m_proxy->m_gotSimpleSignal, 1s));
}

TYPED_TEST(SdbusTestObject, EmitsSignalWithMapSuccesfully)
{
    this->m_adaptor->emitSignalWithMap({{0, "zero"}, {1, "one"}});

    ASSERT_TRUE(waitUntil(this->m_proxy->m_gotSignalWithMap));
    ASSERT_THAT(this->m_proxy->m_mapFromSignal[0], Eq("zero"));
    ASSERT_THAT(this->m_proxy->m_mapFromSignal[1], Eq("one"));
}

TYPED_TEST(SdbusTestObject, EmitsSignalWithLargeMapSuccesfully)
{
  std::map<int32_t, std::string> largeMap;
  for (int32_t i = 0; i < 20'000; ++i)
      largeMap.emplace(i, "This is string nr. " + std::to_string(i+1));
  this->m_adaptor->emitSignalWithMap(largeMap);

  ASSERT_TRUE(waitUntil(this->m_proxy->m_gotSignalWithMap));
  ASSERT_THAT(this->m_proxy->m_mapFromSignal[0], Eq("This is string nr. 1"));
  ASSERT_THAT(this->m_proxy->m_mapFromSignal[1], Eq("This is string nr. 2"));
}

TYPED_TEST(SdbusTestObject, EmitsSignalWithVariantSuccesfully)
{
    double d = 3.14;
    this->m_adaptor->emitSignalWithVariant(sdbus::Variant{d});

    ASSERT_TRUE(waitUntil(this->m_proxy->m_gotSignalWithVariant));
    ASSERT_THAT(this->m_proxy->m_variantFromSignal, DoubleEq(d));
}

TYPED_TEST(SdbusTestObject, EmitsSignalWithoutRegistrationSuccesfully)
{
    this->m_adaptor->emitSignalWithoutRegistration({"platform", {"av"}});

    ASSERT_TRUE(waitUntil(this->m_proxy->m_gotSignalWithSignature));
    ASSERT_THAT(this->m_proxy->m_signatureFromSignal["platform"], Eq("av"));
}

TYPED_TEST(SdbusTestObject, CanAccessAssociatedSignalMessageInSignalHandler)
{
    this->m_adaptor->emitSimpleSignal();

    waitUntil(this->m_proxy->m_gotSimpleSignal);

    ASSERT_THAT(this->m_proxy->m_signalMsg, NotNull());
    ASSERT_THAT(this->m_proxy->m_signalMemberName, Eq("simpleSignal"));
}

TYPED_TEST(SdbusTestObject, UnregistersSignalHandler)
{
    ASSERT_NO_THROW(this->m_proxy->unregisterSimpleSignalHandler());

    this->m_adaptor->emitSimpleSignal();

    ASSERT_FALSE(waitUntil(this->m_proxy->m_gotSimpleSignal, 1s));
}

TYPED_TEST(SdbusTestObject, UnregistersSignalHandlerForSomeProxies)
{
    auto proxy1 = std::make_unique<TestProxy>(*this->s_adaptorConnection, BUS_NAME, OBJECT_PATH);
    auto proxy2 = std::make_unique<TestProxy>(*this->s_adaptorConnection, BUS_NAME, OBJECT_PATH);

    ASSERT_NO_THROW(this->m_proxy->unregisterSimpleSignalHandler());

    this->m_adaptor->emitSimpleSignal();

    ASSERT_TRUE(waitUntil(proxy1->m_gotSimpleSignal));
    ASSERT_TRUE(waitUntil(proxy2->m_gotSimpleSignal));
    ASSERT_FALSE(waitUntil(this->m_proxy->m_gotSimpleSignal, 1s));
}

TYPED_TEST(SdbusTestObject, ReRegistersSignalHandler)
{
    // unregister simple-signal handler
    ASSERT_NO_THROW(this->m_proxy->unregisterSimpleSignalHandler());

    this->m_adaptor->emitSimpleSignal();

    ASSERT_FALSE(waitUntil(this->m_proxy->m_gotSimpleSignal, 1s));

    // re-register simple-signal handler
    ASSERT_NO_THROW(this->m_proxy->reRegisterSimpleSignalHandler());

    this->m_adaptor->emitSimpleSignal();

    ASSERT_TRUE(waitUntil(this->m_proxy->m_gotSimpleSignal));
}
