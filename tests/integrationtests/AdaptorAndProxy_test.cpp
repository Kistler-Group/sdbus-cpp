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

// Own
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
#include <chrono>
#include <fstream>
#include <future>

#include <unistd.h>

using ::testing::Eq;
using ::testing::DoubleEq;
using ::testing::Gt;
using ::testing::AnyOf;
using ::testing::ElementsAre;
using ::testing::SizeIs;
using namespace std::chrono_literals;

namespace
{

class AdaptorAndProxyFixture : public ::testing::Test
{
public:
    static void SetUpTestCase()
    {
        s_connection->requestName(INTERFACE_NAME);
        s_connection->enterEventLoopAsync();
    }

    static void TearDownTestCase()
    {
        s_connection->leaveEventLoop();
        s_connection->releaseName(INTERFACE_NAME);
    }

    static bool waitUntil(std::atomic<bool>& flag, std::chrono::milliseconds timeout = 5s)
    {
        std::chrono::milliseconds elapsed{};
        std::chrono::milliseconds step{5ms};
        do {
            std::this_thread::sleep_for(step);
            elapsed += step;
            if (elapsed > timeout)
                return false;
        } while (!flag);

        return true;
    }

private:
    void SetUp() override
    {
        m_adaptor = std::make_unique<TestingAdaptor>(*s_connection);
        m_proxy = std::make_unique<TestingProxy>(INTERFACE_NAME, OBJECT_PATH);
        std::this_thread::sleep_for(50ms); // Give time for the proxy to start listening to signals
    }

    void TearDown() override
    {
        m_proxy.reset();
        m_adaptor.reset();
    }

public:
    static std::unique_ptr<sdbus::IConnection> s_connection;

    std::unique_ptr<TestingAdaptor> m_adaptor;
    std::unique_ptr<TestingProxy> m_proxy;
};

std::unique_ptr<sdbus::IConnection> AdaptorAndProxyFixture::s_connection = sdbus::createSystemBusConnection();

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
    auto res = m_proxy->doOperationWith500msTimeout(20); // The operation will take 20ms, but the timeout is 500ms, so we are fine
    ASSERT_THAT(res, Eq(20));
}

TEST_F(SdbusTestObject, ThrowsTimeoutErrorWhenMethodTimesOut)
{
    try
    {
        m_proxy->doOperationWith500msTimeout(1000); // The operation will take 1s, but the timeout is 500ms, so we should time out
        FAIL() << "Expected sdbus::Error exception";
    }
    catch (const sdbus::Error& e)
    {
        ASSERT_THAT(e.getName(), AnyOf("org.freedesktop.DBus.Error.Timeout", "org.freedesktop.DBus.Error.NoReply"));
        ASSERT_THAT(e.getMessage(), AnyOf("Connection timed out", "Method call timed out"));
    }
    catch(...)
    {
        FAIL() << "Expected sdbus::Error exception";
    }
}

TEST_F(SdbusTestObject, ThrowsTimeoutErrorWhenClientSideAsyncMethodTimesOut)
{
    try
    {
        std::promise<uint32_t> promise;
        auto future = promise.get_future();
        m_proxy->installDoOperationClientSideAsyncReplyHandler([&](uint32_t res, const sdbus::Error* err)
        {
            if (err == nullptr)
                promise.set_value(res);
            else
                promise.set_exception(std::make_exception_ptr(*err));
        });

        m_proxy->doOperationClientSideAsyncWith500msTimeout(1000); // The operation will take 1s, but the timeout is 500ms, so we should time out
        future.get(), Eq(100);

        FAIL() << "Expected sdbus::Error exception";
    }
    catch (const sdbus::Error& e)
    {
        ASSERT_THAT(e.getName(), AnyOf("org.freedesktop.DBus.Error.Timeout", "org.freedesktop.DBus.Error.NoReply"));
        ASSERT_THAT(e.getMessage(), AnyOf("Connection timed out", "Method call timed out"));
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

TEST_F(SdbusTestObject, RunsServerSideAsynchoronousMethodAsynchronously)
{
    // Yeah, this is kinda timing-dependent test, but times should be safe...
    std::mutex mtx;
    std::vector<uint32_t> results;
    std::atomic<bool> invoke{};
    std::atomic<int> startedCount{};
    auto call = [&](uint32_t param)
    {
        TestingProxy proxy{INTERFACE_NAME, OBJECT_PATH};
        ++startedCount;
        while (!invoke) ;
        auto result = proxy.doOperationAsync(param);
        std::lock_guard<std::mutex> guard(mtx);
        results.push_back(result);
    };

    std::thread invocations[]{std::thread{call, 1500}, std::thread{call, 1000}, std::thread{call, 500}};
    while (startedCount != 3) ;
    invoke = true;
    std::for_each(std::begin(invocations), std::end(invocations), [](auto& t){ t.join(); });

    ASSERT_THAT(results, ElementsAre(500, 1000, 1500));
}

TEST_F(SdbusTestObject, HandlesCorrectlyABulkOfParallelServerSideAsyncMethods)
{
    std::atomic<size_t> resultCount{};
    std::atomic<bool> invoke{};
    std::atomic<int> startedCount{};
    auto call = [&]()
    {
        TestingProxy proxy{INTERFACE_NAME, OBJECT_PATH};
        ++startedCount;
        while (!invoke) ;

        size_t localResultCount{};
        for (size_t i = 0; i < 500; ++i)
        {
            auto result = proxy.doOperationAsync(i % 2);
            if (result == (i % 2)) // Correct return value?
                localResultCount++;
        }

        resultCount += localResultCount;
    };

    std::thread invocations[]{std::thread{call}, std::thread{call}, std::thread{call}};
    while (startedCount != 3) ;
    invoke = true;
    std::for_each(std::begin(invocations), std::end(invocations), [](auto& t){ t.join(); });

    ASSERT_THAT(resultCount, Eq(1500));
}

TEST_F(SdbusTestObject, InvokesMethodAsynchronouslyOnClientSide)
{
    std::promise<uint32_t> promise;
    auto future = promise.get_future();
    m_proxy->installDoOperationClientSideAsyncReplyHandler([&](uint32_t res, const sdbus::Error* err)
    {
        if (err == nullptr)
            promise.set_value(res);
        else
            promise.set_exception(std::make_exception_ptr(*err));
    });

    m_proxy->doOperationClientSideAsync(100);

    ASSERT_THAT(future.get(), Eq(100));
}

TEST_F(SdbusTestObject, InvokesErroneousMethodAsynchronouslyOnClientSide)
{
    std::promise<uint32_t> promise;
    auto future = promise.get_future();
    m_proxy->installDoOperationClientSideAsyncReplyHandler([&](uint32_t res, const sdbus::Error* err)
    {
        if (err == nullptr)
            promise.set_value(res);
        else
            promise.set_exception(std::make_exception_ptr(*err));
    });

    m_proxy->doErroneousOperationClientSideAsync();

    ASSERT_THROW(future.get(), sdbus::Error);
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
    TestingProxy proxy("sdbuscpp.destination.that.does.not.exist", OBJECT_PATH);
    ASSERT_THROW(proxy.getInt(), sdbus::Error);
}

TEST_F(SdbusTestObject, FailsCallingMethodOnNonexistentObject)
{
    TestingProxy proxy(INTERFACE_NAME, "/sdbuscpp/path/that/does/not/exist");
    ASSERT_THROW(proxy.getInt(), sdbus::Error);
}

TEST_F(SdbusTestObject, ReceivesTwoSignalsWhileMakingMethodCall)
{
    m_proxy->emitTwoSimpleSignals();

    ASSERT_TRUE(waitUntil(m_proxy->m_gotSimpleSignal));
    ASSERT_TRUE(waitUntil(m_proxy->m_gotSignalWithMap));
}

#if LIBSYSTEMD_VERSION>=240
TEST_F(SdbusTestObject, CanSetGeneralMethodTimeoutWithLibsystemdVersionGreaterThan239)
{
    s_connection->setMethodCallTimeout(5000000);
    ASSERT_THAT(s_connection->getMethodCallTimeout(), Eq(5000000));
}
#else
TEST_F(SdbusTestObject, CannotSetGeneralMethodTimeoutWithLibsystemdVersionLessThan240)
{
    ASSERT_THROW(s_connection->setMethodCallTimeout(5000000), sdbus::Error);
    ASSERT_THROW(s_connection->getMethodCallTimeout(), sdbus::Error);
}
#endif

// Signals

TEST_F(SdbusTestObject, EmitsSimpleSignalSuccesfully)
{
    m_adaptor->emitSimpleSignal();

    ASSERT_TRUE(waitUntil(m_proxy->m_gotSimpleSignal));
}

TEST_F(SdbusTestObject, EmitsSimpleSignalToMultipleProxiesSuccesfully)
{
    auto proxy1 = std::make_unique<TestingProxy>(*s_connection, INTERFACE_NAME, OBJECT_PATH);
    auto proxy2 = std::make_unique<TestingProxy>(*s_connection, INTERFACE_NAME, OBJECT_PATH);

    m_adaptor->emitSimpleSignal();

    ASSERT_TRUE(waitUntil(m_proxy->m_gotSimpleSignal));
    ASSERT_TRUE(waitUntil(proxy1->m_gotSimpleSignal));
    ASSERT_TRUE(waitUntil(proxy2->m_gotSimpleSignal));
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

// Properties

TEST_F(SdbusTestObject, ReadsReadOnlyPropertySuccesfully)
{
    ASSERT_THAT(m_proxy->state(), Eq(DEFAULT_STATE_VALUE));
}

TEST_F(SdbusTestObject, FailsWritingToReadOnlyProperty)
{
    ASSERT_THROW(m_proxy->state("new_value"), sdbus::Error);
}

TEST_F(SdbusTestObject, WritesAndReadsReadWritePropertySuccesfully)
{
    uint32_t newActionValue = 5678;

    m_proxy->action(newActionValue);

    ASSERT_THAT(m_proxy->action(), Eq(newActionValue));
}

// Standard D-Bus interfaces

TEST_F(SdbusTestObject, PingsViaPeerInterface)
{
    ASSERT_NO_THROW(m_proxy->Ping());
}

TEST_F(SdbusTestObject, AnswersMachineUuidViaPeerInterface)
{
    // If /etc/machine-id does not exist in your system (which is very likely because you have
    // a non-systemd Linux), org.freedesktop.DBus.Peer.GetMachineId() will not work. To solve
    // this, you can create /etc/machine-id yourself as symlink to /var/lib/dbus/machine-id,
    // and then org.freedesktop.DBus.Peer.GetMachineId() will start to work.
    if (::access("/etc/machine-id", F_OK) == -1)
        GTEST_SKIP() << "/etc/machine-id file does not exist, GetMachineId() will not work";

    ASSERT_NO_THROW(m_proxy->GetMachineId());
}

TEST_F(SdbusTestObject, AnswersXmlApiDescriptionViaIntrospectableInterface)
{
    ASSERT_THAT(m_proxy->Introspect(), Eq(m_adaptor->getExpectedXmlApiDescription()));
}

TEST_F(SdbusTestObject, GetsPropertyViaPropertiesInterface)
{
    ASSERT_THAT(m_proxy->Get(INTERFACE_NAME, "state").get<std::string>(), Eq(DEFAULT_STATE_VALUE));
}

TEST_F(SdbusTestObject, SetsPropertyViaPropertiesInterface)
{
    uint32_t newActionValue = 2345;

    m_proxy->Set(INTERFACE_NAME, "action", newActionValue);

    ASSERT_THAT(m_proxy->action(), Eq(newActionValue));
}

TEST_F(SdbusTestObject, GetsAllPropertiesViaPropertiesInterface)
{
    const auto properties = m_proxy->GetAll(INTERFACE_NAME);

    ASSERT_THAT(properties, SizeIs(3));
    EXPECT_THAT(properties.at("state").get<std::string>(), Eq(DEFAULT_STATE_VALUE));
    EXPECT_THAT(properties.at("action").get<uint32_t>(), Eq(DEFAULT_ACTION_VALUE));
    EXPECT_THAT(properties.at("blocking").get<bool>(), Eq(DEFAULT_BLOCKING_VALUE));
}

TEST_F(SdbusTestObject, EmitsPropertyChangedSignalForSelectedProperties)
{
    std::atomic<bool> signalReceived{false};
    m_proxy->m_onPropertiesChangedHandler = [&signalReceived]( const std::string& interfaceName
                                                             , const std::map<std::string, sdbus::Variant>& changedProperties
                                                             , const std::vector<std::string>& /*invalidatedProperties*/ )
    {
        EXPECT_THAT(interfaceName, Eq(INTERFACE_NAME));
        EXPECT_THAT(changedProperties, SizeIs(1));
        EXPECT_THAT(changedProperties.at("blocking").get<bool>(), Eq(!DEFAULT_BLOCKING_VALUE));
        signalReceived = true;
    };

    m_proxy->blocking(!DEFAULT_BLOCKING_VALUE);
    m_proxy->action(DEFAULT_ACTION_VALUE*2);
    m_adaptor->emitPropertiesChangedSignal(INTERFACE_NAME, {"blocking"});

    ASSERT_TRUE(waitUntil(signalReceived));
}

TEST_F(SdbusTestObject, EmitsPropertyChangedSignalForAllProperties)
{
    std::atomic<bool> signalReceived{false};
    m_proxy->m_onPropertiesChangedHandler = [&signalReceived]( const std::string& interfaceName
                                                             , const std::map<std::string, sdbus::Variant>& changedProperties
                                                             , const std::vector<std::string>& invalidatedProperties )
    {
        EXPECT_THAT(interfaceName, Eq(INTERFACE_NAME));
        EXPECT_THAT(changedProperties, SizeIs(1));
        EXPECT_THAT(changedProperties.at("blocking").get<bool>(), Eq(DEFAULT_BLOCKING_VALUE));
        ASSERT_THAT(invalidatedProperties, SizeIs(1));
        EXPECT_THAT(invalidatedProperties[0], Eq("action"));
        signalReceived = true;
    };

    m_adaptor->emitPropertiesChangedSignal(INTERFACE_NAME);

    ASSERT_TRUE(waitUntil(signalReceived));
}

TEST_F(SdbusTestObject, GetsZeroManagedObjectsIfHasNoSubPathObjects)
{
    const auto objectsInterfacesAndProperties = m_proxy->GetManagedObjects();

    ASSERT_THAT(objectsInterfacesAndProperties, SizeIs(0));
}

TEST_F(SdbusTestObject, GetsManagedObjectsSuccessfully)
{
    auto subObject1 = sdbus::createObject(*s_connection, "/sub/path1");
    subObject1->registerProperty("aProperty1").onInterface("org.sdbuscpp.integrationtests.iface1").withGetter([]{return uint8_t{123};});
    subObject1->finishRegistration();
    auto subObject2 = sdbus::createObject(*s_connection, "/sub/path2");
    subObject2->registerProperty("aProperty2").onInterface("org.sdbuscpp.integrationtests.iface2").withGetter([]{return "hi";});
    subObject2->finishRegistration();

    const auto objectsInterfacesAndProperties = m_proxy->GetManagedObjects();

    ASSERT_THAT(objectsInterfacesAndProperties, SizeIs(2));
    EXPECT_THAT(objectsInterfacesAndProperties.at("/sub/path1").at("org.sdbuscpp.integrationtests.iface1").at("aProperty1").get<uint8_t>(), Eq(123));
    EXPECT_THAT(objectsInterfacesAndProperties.at("/sub/path2").at("org.sdbuscpp.integrationtests.iface2").at("aProperty2").get<std::string>(), Eq("hi"));
}

TEST_F(SdbusTestObject, EmitsInterfacesAddedSignalForSelectedObjectInterfaces)
{
    std::atomic<bool> signalReceived{false};
    m_proxy->m_onInterfacesAddedHandler = [&signalReceived]( const sdbus::ObjectPath& objectPath
                                                           , const std::map<std::string, std::map<std::string, sdbus::Variant>>& interfacesAndProperties )
    {
        EXPECT_THAT(objectPath, Eq(OBJECT_PATH));
        EXPECT_THAT(interfacesAndProperties, SizeIs(1));
        EXPECT_THAT(interfacesAndProperties.count(INTERFACE_NAME), Eq(1));
        EXPECT_THAT(interfacesAndProperties.at(INTERFACE_NAME), SizeIs(3));
        signalReceived = true;
    };

    m_adaptor->emitInterfacesAddedSignal({INTERFACE_NAME});

    ASSERT_TRUE(waitUntil(signalReceived));
}

TEST_F(SdbusTestObject, EmitsInterfacesAddedSignalForAllObjectInterfaces)
{
    std::atomic<bool> signalReceived{false};
    m_proxy->m_onInterfacesAddedHandler = [&signalReceived]( const sdbus::ObjectPath& objectPath
                                                           , const std::map<std::string, std::map<std::string, sdbus::Variant>>& interfacesAndProperties )
    {
        EXPECT_THAT(objectPath, Eq(OBJECT_PATH));
        EXPECT_THAT(interfacesAndProperties, SizeIs(5)); // INTERFACE_NAME + 4 standard interfaces
        EXPECT_THAT(interfacesAndProperties.at(INTERFACE_NAME), SizeIs(3)); // 3 properties under INTERFACE_NAME
        signalReceived = true;
    };

    m_adaptor->emitInterfacesAddedSignal();

    ASSERT_TRUE(waitUntil(signalReceived));
}

TEST_F(SdbusTestObject, EmitsInterfacesRemovedSignalForSelectedObjectInterfaces)
{
    std::atomic<bool> signalReceived{false};
    m_proxy->m_onInterfacesRemovedHandler = [&signalReceived]( const sdbus::ObjectPath& objectPath
                                                             , const std::vector<std::string>& interfaces )
    {
        EXPECT_THAT(objectPath, Eq(OBJECT_PATH));
        ASSERT_THAT(interfaces, SizeIs(1));
        EXPECT_THAT(interfaces[0], Eq(INTERFACE_NAME));
        signalReceived = true;
    };

    m_adaptor->emitInterfacesRemovedSignal({INTERFACE_NAME});

    ASSERT_TRUE(waitUntil(signalReceived));
}

TEST_F(SdbusTestObject, EmitsInterfacesRemovedSignalForAllObjectInterfaces)
{
    std::atomic<bool> signalReceived{false};
    m_proxy->m_onInterfacesRemovedHandler = [&signalReceived]( const sdbus::ObjectPath& objectPath
                                                             , const std::vector<std::string>& interfaces )
    {
        EXPECT_THAT(objectPath, Eq(OBJECT_PATH));
        ASSERT_THAT(interfaces, SizeIs(5)); // INTERFACE_NAME + 4 standard interfaces
        signalReceived = true;
    };

    m_adaptor->emitInterfacesRemovedSignal();

    ASSERT_TRUE(waitUntil(signalReceived));
}
