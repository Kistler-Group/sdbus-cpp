/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2024 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file DBusAsyncMethodsTests.cpp
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
using namespace std::chrono_literals;
using namespace sdbus::test;

/*-------------------------------------*/
/* --          TEST CASES           -- */
/*-------------------------------------*/

TYPED_TEST(AsyncSdbusTestObject, ThrowsTimeoutErrorWhenClientSideAsyncMethodTimesOut)
{
    std::chrono::time_point<std::chrono::steady_clock> start;
    try
    {
        std::promise<uint32_t> promise;
        auto future = promise.get_future();
        this->m_proxy->installDoOperationClientSideAsyncReplyHandler([&](uint32_t res, std::optional<sdbus::Error> err)
        {
            if (!err)
                promise.set_value(res);
            else
                promise.set_exception(std::make_exception_ptr(*std::move(err)));
        });

        start = std::chrono::steady_clock::now();
        this->m_proxy->doOperationClientSideAsyncWithTimeout(1us, (1s).count()); // The operation will take 1s, but the timeout is 1us, so we should time out
        future.get();

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

TYPED_TEST(AsyncSdbusTestObject, RunsServerSideAsynchoronousMethodAsynchronously)
{
    // Yeah, this is kinda timing-dependent test, but times should be safe...
    std::mutex mtx;
    std::vector<uint32_t> results;
    std::atomic<bool> invoke{};
    std::atomic<int> startedCount{};
    auto call = [&](uint32_t param)
    {
        TestProxy proxy{BUS_NAME, OBJECT_PATH};
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

TYPED_TEST(AsyncSdbusTestObject, HandlesCorrectlyABulkOfParallelServerSideAsyncMethods)
{
    std::atomic<size_t> resultCount{};
    std::atomic<bool> invoke{};
    std::atomic<int> startedCount{};
    auto call = [&]()
    {
        TestProxy proxy{BUS_NAME, OBJECT_PATH};
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

TYPED_TEST(AsyncSdbusTestObject, InvokesMethodAsynchronouslyOnClientSide)
{
    std::promise<uint32_t> promise;
    auto future = promise.get_future();
    this->m_proxy->installDoOperationClientSideAsyncReplyHandler([&](uint32_t res, std::optional<sdbus::Error> err)
    {
        if (!err)
            promise.set_value(res);
        else
            promise.set_exception(std::make_exception_ptr(std::move(err)));
    });

    this->m_proxy->doOperationClientSideAsync(100);

    ASSERT_THAT(future.get(), Eq(100));
}

TYPED_TEST(AsyncSdbusTestObject, InvokesMethodAsynchronouslyOnClientSideWithFuture)
{
    auto future = this->m_proxy->doOperationClientSideAsync(100, sdbus::with_future);

    ASSERT_THAT(future.get(), Eq(100));
}

TYPED_TEST(AsyncSdbusTestObject, InvokesMethodAsynchronouslyOnClientSideWithFutureOnBasicAPILevel)
{
    auto future = this->m_proxy->doOperationClientSideAsyncOnBasicAPILevel(100);

    auto methodReply = future.get();
    uint32_t returnValue{};
    methodReply >> returnValue;

    ASSERT_THAT(returnValue, Eq(100));
}

TYPED_TEST(AsyncSdbusTestObject, AnswersThatAsyncCallIsPendingIfItIsInProgress)
{
    this->m_proxy->installDoOperationClientSideAsyncReplyHandler([&](uint32_t /*res*/, std::optional<sdbus::Error> /*err*/){});

    auto call = this->m_proxy->doOperationClientSideAsync(100);

    ASSERT_TRUE(call.isPending());
}

TYPED_TEST(AsyncSdbusTestObject, CancelsPendingAsyncCallOnClientSide)
{
    std::promise<uint32_t> promise;
    auto future = promise.get_future();
    this->m_proxy->installDoOperationClientSideAsyncReplyHandler([&](uint32_t /*res*/, std::optional<sdbus::Error> /*err*/){ promise.set_value(1); });
    auto call = this->m_proxy->doOperationClientSideAsync(100);

    call.cancel();

    ASSERT_THAT(future.wait_for(300ms), Eq(std::future_status::timeout));
}

TYPED_TEST(AsyncSdbusTestObject, AnswersThatAsyncCallIsNotPendingAfterItHasBeenCancelled)
{
    std::promise<uint32_t> promise;
    auto future = promise.get_future();
    this->m_proxy->installDoOperationClientSideAsyncReplyHandler([&](uint32_t /*res*/, std::optional<sdbus::Error> /*err*/){ promise.set_value(1); });
    auto call = this->m_proxy->doOperationClientSideAsync(100);

    call.cancel();

    ASSERT_FALSE(call.isPending());
}

TYPED_TEST(AsyncSdbusTestObject, AnswersThatAsyncCallIsNotPendingAfterItHasBeenCompleted)
{
    std::promise<uint32_t> promise;
    auto future = promise.get_future();
    this->m_proxy->installDoOperationClientSideAsyncReplyHandler([&](uint32_t /*res*/, std::optional<sdbus::Error> /*err*/){ promise.set_value(1); });

    auto call = this->m_proxy->doOperationClientSideAsync(0);
    (void) future.get(); // Wait for the call to finish

    ASSERT_TRUE(waitUntil([&call](){ return !call.isPending(); }));
}

TYPED_TEST(AsyncSdbusTestObject, AnswersThatDefaultConstructedAsyncCallIsNotPending)
{
    sdbus::PendingAsyncCall call;

    ASSERT_FALSE(call.isPending());
}

TYPED_TEST(AsyncSdbusTestObject, SupportsAsyncCallCopyAssignment)
{
    sdbus::PendingAsyncCall call;

    call = this->m_proxy->doOperationClientSideAsync(100);

    ASSERT_TRUE(call.isPending());
}

TYPED_TEST(AsyncSdbusTestObject, ReturnsNonnullErrorWhenAsynchronousMethodCallFails)
{
    std::promise<uint32_t> promise;
    auto future = promise.get_future();
    this->m_proxy->installDoOperationClientSideAsyncReplyHandler([&](uint32_t res, std::optional<sdbus::Error> err)
    {
        if (!err)
            promise.set_value(res);
        else
            promise.set_exception(std::make_exception_ptr(*std::move(err)));
    });

    this->m_proxy->doErroneousOperationClientSideAsync();

    ASSERT_THROW(future.get(), sdbus::Error);
}

TYPED_TEST(AsyncSdbusTestObject, ThrowsErrorWhenClientSideAsynchronousMethodCallWithFutureFails)
{
    auto future = this->m_proxy->doErroneousOperationClientSideAsync(sdbus::with_future);

    ASSERT_THROW(future.get(), sdbus::Error);
}
