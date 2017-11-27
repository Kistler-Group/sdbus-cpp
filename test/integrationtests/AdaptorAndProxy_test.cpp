/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
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

// Own
#include "Connection.h"

#include "TestingAdaptor.h"
#include "TestingProxy.h"

// sdbus
#include "sdbus-c++/sdbus-c++.h"

// gmock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// STL
#include <string>
#include <thread>
#include <tuple>

using ::testing::Eq;
using ::testing::Gt;

namespace
{

class AdaptorAndProxyFixture : public ::testing::Test
{
public:
    static void SetUpTestCase()
    {
        m_connection.requestName(INTERFACE_NAME);
        m_connection.enterProcessingLoopAsync();
    }

    static void TearDownTestCase()
    {
        m_connection.leaveProcessingLoop();
        m_connection.releaseName(INTERFACE_NAME);
    }

private:
    void SetUp() override
    {
        m_adaptor = std::make_unique<TestingAdaptor>(m_connection);
        m_proxy = std::make_unique<TestingProxy>(INTERFACE_NAME, OBJECT_PATH);
        usleep(50000); // Give time for the proxy to start listening to signals
    }

    void TearDown() override
    {
        m_proxy.reset();
        m_adaptor.reset();
    }

public:
    static sdbus::internal::Connection m_connection;

    std::unique_ptr<TestingAdaptor> m_adaptor;
    std::unique_ptr<TestingProxy> m_proxy;
};

sdbus::internal::Connection AdaptorAndProxyFixture::m_connection{sdbus::internal::Connection::BusType::eSystem};

}

/*-------------------------------------*/
/* --          TEST CASES           -- */
/*-------------------------------------*/

TEST(AdaptorAndProxy, CanBeConstructedSuccesfully)
{
    auto connection = sdbus::createConnection();
    connection->requestName(INTERFACE_NAME);

    ASSERT_NO_THROW(TestingAdaptor adaptor(*connection));
    ASSERT_NO_THROW(TestingProxy proxy(INTERFACE_NAME, OBJECT_PATH));

    connection->releaseName(INTERFACE_NAME);
}

// Methods

using SdbusTestObject = AdaptorAndProxyFixture;

TEST_F(SdbusTestObject, CallsEmptyMethodSuccesfully)
{
    ASSERT_NO_THROW(m_proxy->noArgNoReturn());
}

TEST_F(SdbusTestObject, CallsMethodsWithBaseTypesSuccesfully)
{
    auto resInt = m_proxy->getInt();
    ASSERT_THAT(resInt, Eq(INT32_VALUE));

    auto multiplyRes = m_proxy->multiply(INT64_VALUE, DOUBLE_VALUE);
    ASSERT_THAT(multiplyRes, Eq(INT64_VALUE * DOUBLE_VALUE));
}

TEST_F(SdbusTestObject, CallsMethodsWithTuplesSuccesfully)
{
    auto resTuple = m_proxy->getTuple();
    ASSERT_THAT(std::get<0>(resTuple), Eq(UINT32_VALUE));
    ASSERT_THAT(std::get<1>(resTuple), Eq(STRING_VALUE));
}

TEST_F(SdbusTestObject, CallsMethodsWithStructSuccesfully)
{
    sdbus::Struct<uint8_t, int16_t, double, std::string, std::vector<int16_t>> a{};
    auto vectorRes = m_proxy->getInts16FromStruct(a);
    ASSERT_THAT(vectorRes, Eq(std::vector<int16_t>{0})); // because second item is by default initialized to 0


    sdbus::Struct<uint8_t, int16_t, double, std::string, std::vector<int16_t>> b{
        UINT8_VALUE, INT16_VALUE, DOUBLE_VALUE, STRING_VALUE, {INT16_VALUE, -INT16_VALUE}
    };
    vectorRes = m_proxy->getInts16FromStruct(b);
    ASSERT_THAT(vectorRes, Eq(std::vector<int16_t>{INT16_VALUE, INT16_VALUE, -INT16_VALUE}));
}

TEST_F(SdbusTestObject, CallsMethodWithVariantSuccesfully)
{
    sdbus::Variant v{DOUBLE_VALUE};
    auto variantRes = m_proxy->processVariant(v);
    ASSERT_THAT(variantRes.get<int32_t>(), Eq(static_cast<int32_t>(DOUBLE_VALUE)));
}

TEST_F(SdbusTestObject, CallsMethodWithStructVariantsAndGetMapSuccesfully)
{
    std::vector<int32_t> x{-2, 0, 2};
    sdbus::Struct<sdbus::Variant, sdbus::Variant> y{false, true};
    auto mapOfVariants = m_proxy->getMapOfVariants(x, y);
    decltype(mapOfVariants) res{{-2, false}, {0, false}, {2, true}};

    ASSERT_THAT(mapOfVariants[-2].get<bool>(), Eq(res[-2].get<bool>()));
    ASSERT_THAT(mapOfVariants[0].get<bool>(), Eq(res[0].get<bool>()));
    ASSERT_THAT(mapOfVariants[2].get<bool>(), Eq(res[2].get<bool>()));
}

TEST_F(SdbusTestObject, CallsMethodWithStructInStructSuccesfully)
{
    auto val = m_proxy->getStructInStruct();
    ASSERT_THAT(val.get<0>(), Eq(STRING_VALUE));
    ASSERT_THAT(std::get<0>(std::get<1>(val))[INT32_VALUE], Eq(INT32_VALUE));
}

TEST_F(SdbusTestObject, CallsMethodWithTwoStructsSuccesfully)
{
    auto val = m_proxy->sumStructItems({1, 2}, {3, 4});
    ASSERT_THAT(val, Eq(1 + 2 + 3 + 4));
}

TEST_F(SdbusTestObject, CallsMethodWithTwoVectorsSuccesfully)
{
    auto val = m_proxy->sumVectorItems({1, 7}, {2, 3});
    ASSERT_THAT(val, Eq(1 + 7 + 2 + 3));
}

TEST_F(SdbusTestObject, CallsMethodWithSignatureSuccesfully)
{
    auto resSignature = m_proxy->getSignature();
    ASSERT_THAT(resSignature, Eq(SIGNATURE_VALUE));
}

TEST_F(SdbusTestObject, CallsMethodWithObjectPathSuccesfully)
{
    auto resObjectPath = m_proxy->getObjectPath();
    ASSERT_THAT(resObjectPath, Eq(OBJECT_PATH_VALUE));
}

TEST_F(SdbusTestObject, CallsMethodWithComplexTypeSuccesfully)
{
    auto resComplex = m_proxy->getComplex();
    ASSERT_THAT(resComplex.count(0), Eq(1));
}

TEST_F(SdbusTestObject, FailsCallingNonexistentMethod)
{
    ASSERT_THROW(m_proxy->callNonexistentMethod(), sdbus::Error);
}

TEST_F(SdbusTestObject, FailsCallingMethodOnNonexistentInterface)
{
    ASSERT_THROW(m_proxy->callMethodOnNonexistentInterface(), sdbus::Error);
}

TEST_F(SdbusTestObject, FailsCallingMethodOnNonexistentDestination)
{
    TestingProxy proxy("wrongDestination", OBJECT_PATH);
    ASSERT_THROW(proxy.getInt(), sdbus::Error);
}

TEST_F(SdbusTestObject, FailsCallingMethodOnNonexistentObject)
{
    TestingProxy proxy(INTERFACE_NAME, "/wrong/path");
    ASSERT_THROW(proxy.getInt(), sdbus::Error);
}

// Signals

TEST_F(SdbusTestObject, EmitsSimpleSignalSuccesfully)
{
    auto count = m_proxy->getSimpleCallCount();
    m_adaptor->simpleSignal();
    usleep(10000);

    ASSERT_THAT(m_proxy->getSimpleCallCount(), Eq(count + 1));
}

TEST_F(SdbusTestObject, EmitsSignalWithMapSuccesfully)
{
    m_adaptor->signalWithMap({{0, "zero"}, {1, "one"}});
    usleep(10000);

    auto map = m_proxy->getMap();
    ASSERT_THAT(map[0], Eq("zero"));
    ASSERT_THAT(map[1], Eq("one"));
}

TEST_F(SdbusTestObject, EmitsSignalWithVariantSuccesfully)
{
    double d = 3.14;
    m_adaptor->signalWithVariant(3.14);
    usleep(10000);

    ASSERT_THAT(m_proxy->getVariantValue(), d);
}

TEST_F(SdbusTestObject, EmitsSignalWithoutRegistrationSuccesfully)
{
    m_adaptor->signalWithoutRegistration({"platform", {"av"}});
    usleep(10000);

    auto signature = m_proxy->getSignatureFromSignal();
    ASSERT_THAT(signature["platform"], Eq("av"));
}

TEST_F(SdbusTestObject, failsEmitingSignalOnNonexistentInterface)
{
    ASSERT_THROW(m_adaptor->emitSignalOnNonexistentInterface(), sdbus::Error);
}

// Properties

TEST_F(SdbusTestObject, ReadsReadPropertySuccesfully)
{
    ASSERT_THAT(m_proxy->state(), Eq(STRING_VALUE));
}

TEST_F(SdbusTestObject, WritesAndReadsReadWritePropertySuccesfully)
{
    auto x = 42;
    ASSERT_NO_THROW(m_proxy->action(x));
    ASSERT_THAT(m_proxy->action(), Eq(x));
}

TEST_F(SdbusTestObject, WritesToWritePropertySuccesfully)
{
    auto x = true;
    ASSERT_NO_THROW(m_proxy->blocking(x));
}

TEST_F(SdbusTestObject, CannotReadFromWriteProperty)
{
    ASSERT_THROW(m_proxy->blocking(), sdbus::Error);
}
