/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2022 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file DBusMethodsTests.cpp
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
#include <thread>
#include <tuple>
#include <chrono>
#include <fstream>
#include <future>
#include <unistd.h>

using ::testing::Eq;
using ::testing::DoubleEq;
using ::testing::Gt;
using ::testing::Le;
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
    auto resObjectPath = m_proxy->getObjPath();
    ASSERT_THAT(resObjectPath, Eq(OBJECT_PATH_VALUE));
}

TEST_F(SdbusTestObject, CallsMethodWithUnixFdSuccesfully)
{
    auto resUnixFd = m_proxy->getUnixFd();
    ASSERT_THAT(resUnixFd.get(), Gt(UNIX_FD_VALUE));
}

TEST_F(SdbusTestObject, CallsMethodWithComplexTypeSuccesfully)
{
    auto resComplex = m_proxy->getComplex();
    ASSERT_THAT(resComplex.count(0), Eq(1));
}

TEST_F(SdbusTestObject, CallsMultiplyMethodWithNoReplyFlag)
{
    m_proxy->multiplyWithNoReply(INT64_VALUE, DOUBLE_VALUE);

    ASSERT_TRUE(waitUntil(m_adaptor->m_wasMultiplyCalled));
    ASSERT_THAT(m_adaptor->m_multiplyResult, Eq(INT64_VALUE * DOUBLE_VALUE));
}

TEST_F(SdbusTestObject, CallsMethodWithCustomTimeoutSuccessfully)
{
    auto res = m_proxy->doOperationWithTimeout(500ms, 20); // The operation will take 20ms, but the timeout is 500ms, so we are fine
    ASSERT_THAT(res, Eq(20));
}

TEST_F(SdbusTestObject, ThrowsTimeoutErrorWhenMethodTimesOut)
{
    auto start = std::chrono::steady_clock::now();
    try
    {
        m_proxy->doOperationWithTimeout(1us, 1000); // The operation will take 1s, but the timeout is 1us, so we should time out
        FAIL() << "Expected sdbus::Error exception";
    }
    catch (const sdbus::Error& e)
    {
        ASSERT_THAT(e.getName(), AnyOf("org.freedesktop.DBus.Error.Timeout", "org.freedesktop.DBus.Error.NoReply"));
        ASSERT_THAT(e.getMessage(), AnyOf("Connection timed out", "Method call timed out"));
        auto measuredTimeout = std::chrono::steady_clock::now() - start;
        ASSERT_THAT(measuredTimeout, Le(50ms));
    }
    catch(...)
    {
        FAIL() << "Expected sdbus::Error exception";
    }
}

TEST_F(SdbusTestObject, CallsMethodThatThrowsError)
{
    try
    {
        m_proxy->throwError();
        FAIL() << "Expected sdbus::Error exception";
    }
    catch (const sdbus::Error& e)
    {
        ASSERT_THAT(e.getName(), Eq("org.freedesktop.DBus.Error.AccessDenied"));
        ASSERT_THAT(e.getMessage(), Eq("A test error occurred (Operation not permitted)"));
    }
    catch(...)
    {
        FAIL() << "Expected sdbus::Error exception";
    }
}

TEST_F(SdbusTestObject, CallsErrorThrowingMethodWithDontExpectReplySet)
{
    ASSERT_NO_THROW(m_proxy->throwErrorWithNoReply());

    ASSERT_TRUE(waitUntil(m_adaptor->m_wasThrowErrorCalled));
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
    TestProxy proxy("sdbuscpp.destination.that.does.not.exist", OBJECT_PATH);
    ASSERT_THROW(proxy.getInt(), sdbus::Error);
}

TEST_F(SdbusTestObject, FailsCallingMethodOnNonexistentObject)
{
    TestProxy proxy(BUS_NAME, "/sdbuscpp/path/that/does/not/exist");
    ASSERT_THROW(proxy.getInt(), sdbus::Error);
}

TEST_F(SdbusTestObject, CanReceiveSignalWhileMakingMethodCall)
{
    m_proxy->emitTwoSimpleSignals();

    ASSERT_TRUE(waitUntil(m_proxy->m_gotSimpleSignal));
    ASSERT_TRUE(waitUntil(m_proxy->m_gotSignalWithMap));
}

TEST_F(SdbusTestObject, CanAccessAssociatedMethodCallMessageInMethodCallHandler)
{
    m_proxy->doOperation(10); // This will save pointer to method call message on server side

    ASSERT_THAT(m_adaptor->m_methodCallMsg, NotNull());
    ASSERT_THAT(m_adaptor->m_methodCallMemberName, Eq("doOperation"));
}

TEST_F(SdbusTestObject, CanAccessAssociatedMethodCallMessageInAsyncMethodCallHandler)
{
    m_proxy->doOperationAsync(10); // This will save pointer to method call message on server side

    ASSERT_THAT(m_adaptor->m_methodCallMsg, NotNull());
    ASSERT_THAT(m_adaptor->m_methodCallMemberName, Eq("doOperationAsync"));
}

#if LIBSYSTEMD_VERSION>=240
TEST_F(SdbusTestObject, CanSetGeneralMethodTimeoutWithLibsystemdVersionGreaterThan239)
{
    s_adaptorConnection->setMethodCallTimeout(5000000);
    ASSERT_THAT(s_adaptorConnection->getMethodCallTimeout(), Eq(5000000));
}
#else
TEST_F(SdbusTestObject, CannotSetGeneralMethodTimeoutWithLibsystemdVersionLessThan240)
{
    ASSERT_THROW(s_adaptorConnection->setMethodCallTimeout(5000000), sdbus::Error);
    ASSERT_THROW(s_adaptorConnection->getMethodCallTimeout(), sdbus::Error);
}
#endif

TEST_F(SdbusTestObject, CanCallMethodSynchronouslyWithoutAnEventLoopThread)
{
    auto proxy = std::make_unique<TestProxy>(BUS_NAME, OBJECT_PATH, sdbus::dont_run_event_loop_thread);

    auto multiplyRes = proxy->multiply(INT64_VALUE, DOUBLE_VALUE);

    ASSERT_THAT(multiplyRes, Eq(INT64_VALUE * DOUBLE_VALUE));
}
