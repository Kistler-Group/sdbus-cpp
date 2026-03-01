/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2026 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 * (C) 2026 - Alex Cani <alexcani109@gmail.com>
 *
 * @file DBusAwaitableMethodsTests.cpp
 *
 * Created on: Mar 1, 2026
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

#include <coroutine>
#include <cstdint>
#include <exception>
#include <future>
#include <map>
#include <string>
#include <type_traits>
#include <utility>

#include <sdbus-c++/sdbus-c++.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "TestFixture.h"
#include "TestProxy.h"
#include "Defs.h"

using ::testing::Eq;
using namespace std::chrono_literals;
using namespace sdbus::test;

// Simple coroutine task type for testing purposes
// Uses a promise/future pair to communicate results and exceptions
// between the coroutine and the test code. Do not mistake this for
// the std::future-based async API of sdbus-c++.
template<typename T>
struct Task {
    struct promise_type {
        T value;
        std::exception_ptr exception;
        std::promise<T> completion;
        std::future<T> future;

        promise_type() : future(completion.get_future()) {}

        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        // Lazy coroutine
        std::suspend_always initial_suspend() noexcept { return {}; }

        // final_suspend suspends so that the test code can retrieve the result
        // or exception from the promise before the coroutine is destroyed
        std::suspend_always final_suspend() noexcept
        {
            if (exception)
            {
                completion.set_exception(exception);
            }
            else
            {
                completion.set_value(std::move(value));
            }
            return {};
        }

        void return_value(T v) { value = std::move(v); }
        void unhandled_exception() { exception = std::current_exception(); }
    };

    std::coroutine_handle<promise_type> handle;

    // Ctor and rule of 5 for proper handle management
    explicit Task(std::coroutine_handle<promise_type> h) : handle(h) {}
    Task(Task&& other) noexcept : handle(std::exchange(other.handle, {})) {}
    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (handle) handle.destroy();
            handle = std::exchange(other.handle, {});
        }
        return *this;
    }
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    ~Task() { if (handle) handle.destroy(); }

    // "User API"  for the test code, allows starting the task and retrieving the result or exception
    void resume() { if (handle && !handle.done()) handle.resume(); }
    T get() { return handle.promise().future.get(); }
};

// Specialization for void
template<>
struct Task<void> {
    struct promise_type {
        std::exception_ptr exception;
        std::promise<void> completion;
        std::future<void> future;

        promise_type() : future(completion.get_future()) {}

        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() noexcept { return {}; }

        std::suspend_always final_suspend() noexcept
        {
            if (exception)
            {
                completion.set_exception(exception);
            }
            else
            {
                completion.set_value();
            }
            return {};
        }

        void return_void() {}
        void unhandled_exception() { exception = std::current_exception(); }
    };

    std::coroutine_handle<promise_type> handle;

    explicit Task(std::coroutine_handle<promise_type> h) : handle(h) {}
    Task(Task&& other) noexcept : handle(std::exchange(other.handle, {})) {}
    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (handle) handle.destroy();
            handle = std::exchange(other.handle, {});
        }
        return *this;
    }
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    ~Task() { if (handle) handle.destroy(); }

    void resume() { if (handle && !handle.done()) handle.resume(); }
    void get() { handle.promise().future.get(); }
};

/*-------------------------------------*/
/* --          TEST CASES           -- */
/*-------------------------------------*/

TYPED_TEST(AsyncSdbusTestObject, InvokesMethodAsynchronouslyOnClientSideWithAwaitable)
{
    auto task = [](TestProxy* proxy) -> Task<uint32_t> {
        co_return co_await proxy->doOperationClientSideAsync(100, sdbus::with_awaitable);
    }(this->m_proxy.get());

    task.resume();

    ASSERT_THAT(task.get(), Eq(100));
}

TYPED_TEST(AsyncSdbusTestObject, InvokesMethodAsynchronouslyOnClientSideWithAwaitableOnBasicAPILevel)
{
    auto task = [](TestProxy* proxy) -> Task<uint32_t> {
        auto methodReply = co_await proxy->doOperationClientSideAsyncOnBasicAPILevel(100, sdbus::with_awaitable);
        uint32_t returnValue{};
        methodReply >> returnValue;
        co_return returnValue;
    }(this->m_proxy.get());

    task.resume();

    ASSERT_THAT(task.get(), Eq(100));
}

TYPED_TEST(AsyncSdbusTestObject, InvokesMethodWithLargeDataAsynchronouslyOnClientSideWithAwaitable)
{
    std::map<int32_t, std::string> largeMap;
    for (int32_t i = 0; i < 40'000; ++i)
        largeMap.emplace(i, "This is string nr. " + std::to_string(i+1));

    auto task = [&largeMap, this]() -> Task<std::map<int32_t, std::string>> {
        co_return co_await this->m_proxy->doOperationWithLargeDataClientSideAsync(largeMap, sdbus::with_awaitable);
    }();

    task.resume();

    ASSERT_THAT(task.get(), Eq(largeMap));
}

TYPED_TEST(AsyncSdbusTestObject, ThrowsErrorWhenClientSideAsynchronousMethodCallWithAwaitableFails)
{
    auto task = [](TestProxy* proxy) -> Task<void> {
        co_await proxy->doErroneousOperationClientSideAsync(sdbus::with_awaitable);
    }(this->m_proxy.get());

    task.resume();

    ASSERT_THROW(task.get(), sdbus::Error);
}

TYPED_TEST(AsyncSdbusTestObject, AwaitableSupportsMultipleSequentialCalls)
{
    auto task = [](TestProxy* proxy) -> Task<uint32_t> {
        auto result1 = co_await proxy->doOperationClientSideAsync(10, sdbus::with_awaitable);
        auto result2 = co_await proxy->doOperationClientSideAsync(20, sdbus::with_awaitable);
        auto result3 = co_await proxy->doOperationClientSideAsync(30, sdbus::with_awaitable);
        co_return result1 + result2 + result3;
    }(this->m_proxy.get());

    task.resume();

    ASSERT_THAT(task.get(), Eq(60));
}

TYPED_TEST(AsyncSdbusTestObject, AwaitablePropagatesExceptionsCorrectly)
{
    auto task = [](TestProxy* proxy) -> Task<std::string> {
        try {
            co_await proxy->doErroneousOperationClientSideAsync(sdbus::with_awaitable);
            co_return "FAILED";
        } catch (const sdbus::Error& e) {
            // Verify we can inspect the exception
            co_return std::string(e.getName());
        }
    }(this->m_proxy.get());

    task.resume();

    ASSERT_THAT(task.get(), ::testing::HasSubstr("Error"));
}
