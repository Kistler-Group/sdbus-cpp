/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2026 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 * (C) 2026 - Alex Cani <alexcani109@gmail.com>
 *
 * @file Awaitable.h
 *
 * Created on: Feb 28, 2026
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

#ifndef SDBUS_CXX_AWAITABLE_H_
#define SDBUS_CXX_AWAITABLE_H_

#include <atomic>
#if __has_include(<coroutine>)
#include <coroutine>
#endif
#include <cstdint>
#include <exception>
#include <memory>
#include <type_traits>
#include <variant>

namespace sdbus
{

/********************************************//**
 * @enum AwaitableState
 *
 * Represents the lifecycle state of an asynchronous
 * operation in the coroutine awaitable protocol.
 * Used for atomic coordination between the coroutine
 * and the D-Bus callback thread.
 *
 ***********************************************/
enum class AwaitableState : uint8_t
{
    NotReady, // Initial state: callback hasn't fired yet
    Waiting,  // Coroutine is suspended and waiting for callback
    Completed // Callback completed, result is ready
};

// Shared data
template <typename T>
struct AwaitableData
{
    using result_type = std::conditional_t<std::is_void_v<T>, std::monostate, T>;
    std::variant<result_type, std::exception_ptr> result;
#ifdef __cpp_lib_coroutine
    std::coroutine_handle<> handle;
#endif // __cpp_lib_coroutine
    std::atomic<AwaitableState> status{AwaitableState::NotReady};

    void resumeCoroutine()
    {
#ifdef __cpp_lib_coroutine
        handle.resume();
#endif // __cpp_lib_coroutine
    }
};

/********************************************//**
 * @class Awaitable
 *
 * A C++20 coroutine awaitable that represents an asynchronous
 * operation. Allows suspending a coroutine until a D-Bus method
 * call completes, then resuming with the result or exception.
 *
 * This is not a full-fledged coroutine type, but a simple awaitable
 * that can be used with `co_await` to retrieve results of async D-Bus calls.
 * This is independent of any specific coroutine framework or scheduler,
 * as it relies on the D-Bus callback mechanism to resume the coroutine.
 *
 * You most likely don't need to use this class directly. Instead, use the
 * respective low-level or high-level API functions that return an Awaitable
 * instance, such as IProxy::callMethodAsync with with_awaitable_t tag,
 * or the .getResultAsAwaitable() methods of the high-level API.
 *
 * The class represents nothing, i.e. is a simple placeholder class, if the API
 * is used as C++17 or with a standard library not supporting coroutines.
 *
 ***********************************************/
template <typename T>
class Awaitable
{
public:
    explicit Awaitable(std::shared_ptr<AwaitableData<T>> data)
        : data_(std::move(data))
    {
    }

#ifdef __cpp_lib_coroutine

    // Called when the coroutine is co_await'ed. Returns true if the coroutine should be suspended.
    [[nodiscard]] bool await_ready() const noexcept
    {
        return data_->status.load(std::memory_order_acquire) == AwaitableState::Completed;
    }

    // Called when the coroutine is suspended, returning false here will immediately
    // resume the coroutine.
    bool await_suspend(std::coroutine_handle<> handle) noexcept
    {
        data_->handle = handle;

        // Attempt transition from NotReady to Waiting.
        AwaitableState expected = AwaitableState::NotReady;
        return data_->status.compare_exchange_strong(expected, AwaitableState::Waiting, std::memory_order_acq_rel);
    }

    // Called when the coroutine is resumed. Returns the result or throws the exception.
    T await_resume() const
    {
        if (auto* exception = std::get_if<std::exception_ptr>(&data_->result); exception != nullptr)
            std::rethrow_exception(*exception);

        if constexpr (std::is_void_v<T>)
            return;
        else
            return std::get<T>(std::move(data_->result));
    }

#endif // __cpp_lib_coroutine

private:
    std::shared_ptr<AwaitableData<T>> data_;
};

} // namespace sdbus

#endif // SDBUS_CXX_AWAITABLE_H_
