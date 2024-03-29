/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2024 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
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

/*-------------------------------------*/
/* --          TEST CASES           -- */
/*-------------------------------------*/

TYPED_TEST(SdbusTestObject, CallsEmptyMethodSuccesfully)
{
    ASSERT_NO_THROW(this->m_proxy->noArgNoReturn());
}

TYPED_TEST(SdbusTestObject, CallsMethodsWithBaseTypesSuccesfully)
{
    auto resInt = this->m_proxy->getInt();
    ASSERT_THAT(resInt, Eq(INT32_VALUE));

    auto multiplyRes = this->m_proxy->multiply(INT64_VALUE, DOUBLE_VALUE);
    ASSERT_THAT(multiplyRes, Eq(INT64_VALUE * DOUBLE_VALUE));
}

TYPED_TEST(SdbusTestObject, CallsMethodsWithTuplesSuccesfully)
{
    auto resTuple = this->m_proxy->getTuple();
    ASSERT_THAT(std::get<0>(resTuple), Eq(UINT32_VALUE));
    ASSERT_THAT(std::get<1>(resTuple), Eq(STRING_VALUE));
}

TYPED_TEST(SdbusTestObject, CallsMethodsWithStructSuccesfully)
{
    sdbus::Struct<uint8_t, int16_t, double, std::string, std::vector<int16_t>> a{};
    auto vectorRes = this->m_proxy->getInts16FromStruct(a);
    ASSERT_THAT(vectorRes, Eq(std::vector<int16_t>{0})); // because second item is by default initialized to 0

    sdbus::Struct<uint8_t, int16_t, double, std::string, std::vector<int16_t>> b{
        UINT8_VALUE, INT16_VALUE, DOUBLE_VALUE, STRING_VALUE, {INT16_VALUE, -INT16_VALUE}
    };
    vectorRes = this->m_proxy->getInts16FromStruct(b);
    ASSERT_THAT(vectorRes, Eq(std::vector<int16_t>{INT16_VALUE, INT16_VALUE, -INT16_VALUE}));
}

TYPED_TEST(SdbusTestObject, CallsMethodWithVariantSuccesfully)
{
    sdbus::Variant v{DOUBLE_VALUE};
    sdbus::Variant variantRes = this->m_proxy->processVariant(v);
    ASSERT_THAT(variantRes.get<int32_t>(), Eq(static_cast<int32_t>(DOUBLE_VALUE)));
}

TYPED_TEST(SdbusTestObject, CallsMethodWithStdVariantSuccesfully)
{
    std::variant<int32_t, double, std::string> v{DOUBLE_VALUE};
    auto variantRes = this->m_proxy->processVariant(v);
    ASSERT_THAT(std::get<int32_t>(variantRes), Eq(static_cast<int32_t>(DOUBLE_VALUE)));
}

TYPED_TEST(SdbusTestObject, CallsMethodWithStructVariantsAndGetMapSuccesfully)
{
    std::vector<int32_t> x{-2, 0, 2};
    sdbus::Struct<sdbus::Variant, sdbus::Variant> y{false, true};
    std::map<int32_t, sdbus::Variant> mapOfVariants = this->m_proxy->getMapOfVariants(x, y);
    decltype(mapOfVariants) res{ {sdbus::Variant{-2}, sdbus::Variant{false}}
                               , {sdbus::Variant{0}, sdbus::Variant{false}}
                               , {sdbus::Variant{2}, sdbus::Variant{true}}};

    ASSERT_THAT(mapOfVariants[-2].get<bool>(), Eq(res[-2].get<bool>()));
    ASSERT_THAT(mapOfVariants[0].get<bool>(), Eq(res[0].get<bool>()));
    ASSERT_THAT(mapOfVariants[2].get<bool>(), Eq(res[2].get<bool>()));
}

TYPED_TEST(SdbusTestObject, CallsMethodWithStructInStructSuccesfully)
{
    auto val = this->m_proxy->getStructInStruct();
    ASSERT_THAT(val.template get<0>(), Eq(STRING_VALUE));
    ASSERT_THAT(std::get<0>(std::get<1>(val))[INT32_VALUE], Eq(INT32_VALUE));
}

TYPED_TEST(SdbusTestObject, CallsMethodWithTwoStructsSuccesfully)
{
    auto val = this->m_proxy->sumStructItems({1, 2}, {3, 4});
    ASSERT_THAT(val, Eq(1 + 2 + 3 + 4));
}

TYPED_TEST(SdbusTestObject, CallsMethodWithTwoVectorsSuccesfully)
{
    auto val = this->m_proxy->sumArrayItems({1, 7}, {2, 3, 4});
    ASSERT_THAT(val, Eq(1 + 7 + 2 + 3 + 4));
}

TYPED_TEST(SdbusTestObject, CallsMethodWithSignatureSuccesfully)
{
    auto resSignature = this->m_proxy->getSignature();
    ASSERT_THAT(resSignature, Eq(SIGNATURE_VALUE));
}

TYPED_TEST(SdbusTestObject, CallsMethodWithObjectPathSuccesfully)
{
    auto resObjectPath = this->m_proxy->getObjPath();
    ASSERT_THAT(resObjectPath, Eq(OBJECT_PATH_VALUE));
}

TYPED_TEST(SdbusTestObject, CallsMethodWithUnixFdSuccesfully)
{
    auto resUnixFd = this->m_proxy->getUnixFd();
    ASSERT_THAT(resUnixFd.get(), Gt(UNIX_FD_VALUE));
}

TYPED_TEST(SdbusTestObject, CallsMethodWithComplexTypeSuccesfully)
{
    auto resComplex = this->m_proxy->getComplex();
    ASSERT_THAT(resComplex.count(0), Eq(1));
}

TYPED_TEST(SdbusTestObject, CallsMultiplyMethodWithNoReplyFlag)
{
    this->m_proxy->multiplyWithNoReply(INT64_VALUE, DOUBLE_VALUE);

    ASSERT_TRUE(waitUntil(this->m_adaptor->m_wasMultiplyCalled));
    ASSERT_THAT(this->m_adaptor->m_multiplyResult, Eq(INT64_VALUE * DOUBLE_VALUE));
}

TYPED_TEST(SdbusTestObject, CallsMethodWithCustomTimeoutSuccessfully)
{
    auto res = this->m_proxy->doOperationWithTimeout(500ms, (20ms).count()); // The operation will take 20ms, but the timeout is 500ms, so we are fine
    ASSERT_THAT(res, Eq(20));
}

TYPED_TEST(SdbusTestObject, ThrowsTimeoutErrorWhenMethodTimesOut)
{
    auto start = std::chrono::steady_clock::now();
    try
    {
        this->m_proxy->doOperationWithTimeout(1us, (1s).count()); // The operation will take 1s, but the timeout is 1us, so we should time out
        FAIL() << "Expected sdbus::Error exception";
    }
    catch (const sdbus::Error& e)
    {
        ASSERT_THAT(e.getName(), AnyOf("org.freedesktop.DBus.Error.Timeout", "org.freedesktop.DBus.Error.NoReply"));
        ASSERT_THAT(e.getMessage(), AnyOf("Connection timed out", "Operation timed out", "Method call timed out"));
        auto measuredTimeout = std::chrono::steady_clock::now() - start;
        ASSERT_THAT(measuredTimeout, Le(50ms));
    }
    catch(...)
    {
        FAIL() << "Expected sdbus::Error exception";
    }
}

TYPED_TEST(SdbusTestObject, CallsMethodThatThrowsError)
{
    try
    {
        this->m_proxy->throwError();
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

TYPED_TEST(SdbusTestObject, CallsErrorThrowingMethodWithDontExpectReplySet)
{
    ASSERT_NO_THROW(this->m_proxy->throwErrorWithNoReply());

    ASSERT_TRUE(waitUntil(this->m_adaptor->m_wasThrowErrorCalled));
}

TYPED_TEST(SdbusTestObject, FailsCallingNonexistentMethod)
{
    ASSERT_THROW(this->m_proxy->callNonexistentMethod(), sdbus::Error);
}

TYPED_TEST(SdbusTestObject, FailsCallingMethodOnNonexistentInterface)
{
    ASSERT_THROW(this->m_proxy->callMethodOnNonexistentInterface(), sdbus::Error);
}

TYPED_TEST(SdbusTestObject, FailsCallingMethodOnNonexistentDestination)
{
    TestProxy proxy(sdbus::ServiceName{"sdbuscpp.destination.that.does.not.exist"}, OBJECT_PATH);
    ASSERT_THROW(proxy.getInt(), sdbus::Error);
}

TYPED_TEST(SdbusTestObject, FailsCallingMethodOnNonexistentObject)
{
    TestProxy proxy(SERVICE_NAME, sdbus::ObjectPath{"/sdbuscpp/path/that/does/not/exist"});
    ASSERT_THROW(proxy.getInt(), sdbus::Error);
}

TYPED_TEST(SdbusTestObject, CanReceiveSignalWhileMakingMethodCall)
{
    this->m_proxy->emitTwoSimpleSignals();

    EXPECT_TRUE(waitUntil(this->m_proxy->m_gotSimpleSignal));
    EXPECT_TRUE(waitUntil(this->m_proxy->m_gotSignalWithMap));
}

TYPED_TEST(SdbusTestObject, CanAccessAssociatedMethodCallMessageInMethodCallHandler)
{
    this->m_proxy->doOperation(10); // This will save pointer to method call message on server side

    ASSERT_THAT(this->m_adaptor->m_methodCallMsg, NotNull());
    ASSERT_THAT(this->m_adaptor->m_methodName, Eq("doOperation"));
}

TYPED_TEST(SdbusTestObject, CanAccessAssociatedMethodCallMessageInAsyncMethodCallHandler)
{
    this->m_proxy->doOperationAsync(10); // This will save pointer to method call message on server side

    ASSERT_THAT(this->m_adaptor->m_methodCallMsg, NotNull());
    ASSERT_THAT(this->m_adaptor->m_methodName, Eq("doOperationAsync"));
}

#if LIBSYSTEMD_VERSION>=240
TYPED_TEST(SdbusTestObject, CanSetGeneralMethodTimeoutWithLibsystemdVersionGreaterThan239)
{
    this->s_adaptorConnection->setMethodCallTimeout(5000000);
    ASSERT_THAT(this->s_adaptorConnection->getMethodCallTimeout(), Eq(5000000));
}
#else
TYPED_TEST(SdbusTestObject, CannotSetGeneralMethodTimeoutWithLibsystemdVersionLessThan240)
{
    ASSERT_THROW(this->s_adaptorConnection->setMethodCallTimeout(5000000), sdbus::Error);
    ASSERT_THROW(this->s_adaptorConnection->getMethodCallTimeout(), sdbus::Error);
}
#endif

TYPED_TEST(SdbusTestObject, CanCallMethodSynchronouslyWithoutAnEventLoopThread)
{
    auto proxy = std::make_unique<TestProxy>(SERVICE_NAME, OBJECT_PATH, sdbus::dont_run_event_loop_thread);

    auto multiplyRes = proxy->multiply(INT64_VALUE, DOUBLE_VALUE);

    ASSERT_THAT(multiplyRes, Eq(INT64_VALUE * DOUBLE_VALUE));
}

TYPED_TEST(SdbusTestObject, CanRegisterAdditionalVTableDynamicallyAtAnyTime)
{
    auto& object = this->m_adaptor->getObject();
    sdbus::InterfaceName interfaceName{"org.sdbuscpp.integrationtests2"};
    auto vtableSlot = object.addVTable( interfaceName
                                      , { sdbus::registerMethod("add").implementedAs([](const int64_t& a, const double& b){ return a + b; })
                                        , sdbus::registerMethod("subtract").implementedAs([](const int& a, const int& b){ return a - b; }) }
                                      , sdbus::return_slot );

    // The new remote vtable is registered as long as we keep vtableSlot, so remote method calls now should pass
    auto proxy = sdbus::createProxy(SERVICE_NAME, OBJECT_PATH, sdbus::dont_run_event_loop_thread);
    int result{};
    proxy->callMethod("subtract").onInterface(interfaceName).withArguments(10, 2).storeResultsTo(result);

    ASSERT_THAT(result, Eq(8));
}

TYPED_TEST(SdbusTestObject, CanUnregisterAdditionallyRegisteredVTableAtAnyTime)
{
    auto& object = this->m_adaptor->getObject();
    sdbus::InterfaceName interfaceName{"org.sdbuscpp.integrationtests2"};

    auto vtableSlot = object.addVTable( interfaceName
                                      , { sdbus::registerMethod("add").implementedAs([](const int64_t& a, const double& b){ return a + b; })
                                        , sdbus::registerMethod("subtract").implementedAs([](const int& a, const int& b){ return a - b; }) }
                                      , sdbus::return_slot );
    vtableSlot.reset(); // Letting the slot go means letting go the associated vtable registration

    // No such remote D-Bus method under given interface exists anymore...
    auto proxy = sdbus::createProxy(SERVICE_NAME, OBJECT_PATH, sdbus::dont_run_event_loop_thread);
    ASSERT_THROW(proxy->callMethod("subtract").onInterface(interfaceName).withArguments(10, 2), sdbus::Error);
}
