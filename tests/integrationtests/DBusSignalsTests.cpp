/**
 * (C) 2016 - 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2019 Stanislav Angelovic <angelovic.s@gmail.com>
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

using ::testing::Eq;
using ::testing::DoubleEq;
using ::testing::Gt;
using ::testing::AnyOf;
using ::testing::ElementsAre;
using ::testing::SizeIs;
using ::testing::NotNull;
using namespace std::chrono_literals;
using namespace sdbus::test;

using SdbusTestObject = TestFixture;

/*-------------------------------------*/
/* --          TEST CASES           -- */
/*-------------------------------------*/

TEST_F(SdbusTestObject, EmitsSimpleSignalSuccesfully)
{
    m_adaptor->emitSimpleSignal();

    ASSERT_TRUE(waitUntil(m_proxy->m_gotSimpleSignal));
}

TEST_F(SdbusTestObject, EmitsSimpleSignalToMultipleProxiesSuccesfully)
{
    auto proxy1 = std::make_unique<TestProxy>(*s_adaptorConnection, INTERFACE_NAME, OBJECT_PATH);
    auto proxy2 = std::make_unique<TestProxy>(*s_adaptorConnection, INTERFACE_NAME, OBJECT_PATH);

    m_adaptor->emitSimpleSignal();

    ASSERT_TRUE(waitUntil(m_proxy->m_gotSimpleSignal));
    ASSERT_TRUE(waitUntil(proxy1->m_gotSimpleSignal));
    ASSERT_TRUE(waitUntil(proxy2->m_gotSimpleSignal));
}

TEST_F(SdbusTestObject, ProxyDoesNotReceiveSignalFromOtherBusName)
{
    auto otherBusName = INTERFACE_NAME + "2";
    auto connection2 = sdbus::createConnection(otherBusName);
    auto adaptor2 = std::make_unique<TestAdaptor>(*connection2, OBJECT_PATH);

    adaptor2->emitSimpleSignal();

    ASSERT_FALSE(waitUntil(m_proxy->m_gotSimpleSignal, std::chrono::seconds(1)));
}

TEST_F(SdbusTestObject, EmitsSignalWithMapSuccesfully)
{
    m_adaptor->emitSignalWithMap({{0, "zero"}, {1, "one"}});

    ASSERT_TRUE(waitUntil(m_proxy->m_gotSignalWithMap));
    ASSERT_THAT(m_proxy->m_mapFromSignal[0], Eq("zero"));
    ASSERT_THAT(m_proxy->m_mapFromSignal[1], Eq("one"));
}

TEST_F(SdbusTestObject, EmitsSignalWithVariantSuccesfully)
{
    double d = 3.14;
    m_adaptor->emitSignalWithVariant(d);

    ASSERT_TRUE(waitUntil(m_proxy->m_gotSignalWithVariant));
    ASSERT_THAT(m_proxy->m_variantFromSignal, DoubleEq(d));
}

TEST_F(SdbusTestObject, EmitsSignalWithoutRegistrationSuccesfully)
{
    m_adaptor->emitSignalWithoutRegistration({"platform", {"av"}});

    ASSERT_TRUE(waitUntil(m_proxy->m_gotSignalWithSignature));
    ASSERT_THAT(m_proxy->m_signatureFromSignal["platform"], Eq("av"));
}

TEST_F(SdbusTestObject, CanAccessAssociatedSignalMessageInSignalHandler)
{
    m_adaptor->emitSimpleSignal();

    waitUntil(m_proxy->m_gotSimpleSignal);

    ASSERT_THAT(m_proxy->m_signalMsg, NotNull());
    ASSERT_THAT(m_proxy->m_signalMemberName, Eq("simpleSignal"));
}
