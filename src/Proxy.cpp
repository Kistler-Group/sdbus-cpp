/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2024 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file Proxy.cpp
 *
 * Created on: Nov 8, 2016
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

#include "Proxy.h"

#include "sdbus-c++/Error.h"
#include "sdbus-c++/IConnection.h"
#include "sdbus-c++/IProxy.h"
#include "sdbus-c++/Message.h"
#include "sdbus-c++/TypeTraits.h"

#include "IConnection.h"
#include "MessageUtils.h"
#include "ScopeGuard.h"
#include "Utils.h"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <exception>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include SDBUS_HEADER
#include <utility>

namespace sdbus::internal {

Proxy::Proxy(sdbus::internal::IConnection& connection, ServiceName destination, ObjectPath objectPath)
    : connection_(&connection, [](sdbus::internal::IConnection *){ /* Intentionally left empty */ })
    , destination_(std::move(destination))
    , objectPath_(std::move(objectPath))
{
    SDBUS_CHECK_SERVICE_NAME(destination_.c_str());
    SDBUS_CHECK_OBJECT_PATH(objectPath_.c_str());

    // The connection is not ours only, it is owned and managed by the user and we just reference
    // it here, so we expect the client to manage the event loop upon this connection themselves.
}

Proxy::Proxy( std::unique_ptr<sdbus::internal::IConnection>&& connection
            , ServiceName destination
            , ObjectPath objectPath )
    : connection_(std::move(connection))
    , destination_(std::move(destination))
    , objectPath_(std::move(objectPath))
{
    SDBUS_CHECK_SERVICE_NAME(destination_.c_str());
    SDBUS_CHECK_OBJECT_PATH(objectPath_.c_str());

    // The connection is ours only, i.e. it's us who has to manage the event loop upon this connection,
    // in order that we get and process signals, async call replies, and other messages from D-Bus.
    connection_->enterEventLoopAsync();
}

Proxy::Proxy( std::unique_ptr<sdbus::internal::IConnection>&& connection
            , ServiceName destination
            , ObjectPath objectPath
            , dont_run_event_loop_thread_t )
    : connection_(std::move(connection))
    , destination_(std::move(destination))
    , objectPath_(std::move(objectPath))
{
    SDBUS_CHECK_SERVICE_NAME(destination_.c_str());
    SDBUS_CHECK_OBJECT_PATH(objectPath_.c_str());

    // Even though the connection is ours only, we don't start an event loop thread.
    // This proxy is meant to be created, used for simple synchronous D-Bus call(s) and then dismissed.
}

MethodCall Proxy::createMethodCall(const InterfaceName& interfaceName, const MethodName& methodName) const
{
    return connection_->createMethodCall(destination_, objectPath_, interfaceName, methodName);
}

MethodCall Proxy::createMethodCall(const char* interfaceName, const char* methodName) const
{
    return connection_->createMethodCall(destination_.c_str(), objectPath_.c_str(), interfaceName, methodName);
}

MethodReply Proxy::callMethod(const MethodCall& message)
{
    return Proxy::callMethod(message, /*timeout*/ 0);
}

MethodReply Proxy::callMethod(const MethodCall& message, uint64_t timeout)
{
    SDBUS_THROW_ERROR_IF(!message.isValid(), "Invalid method call message provided", EINVAL);

    return message.send(timeout);
}

PendingAsyncCall Proxy::callMethodAsync(const MethodCall& message, async_reply_handler asyncReplyCallback)
{
    return Proxy::callMethodAsync(message, std::move(asyncReplyCallback), /*timeout*/ 0);
}

Slot Proxy::callMethodAsync(const MethodCall& message, async_reply_handler asyncReplyCallback, return_slot_t)
{
    return Proxy::callMethodAsync(message, std::move(asyncReplyCallback), /*timeout*/ 0, return_slot);
}

PendingAsyncCall Proxy::callMethodAsync(const MethodCall& message, async_reply_handler asyncReplyCallback, uint64_t timeout)
{
    SDBUS_THROW_ERROR_IF(!message.isValid(), "Invalid async method call message provided", EINVAL);

    auto asyncCallInfo = std::make_shared<AsyncCallInfo>(AsyncCallInfo{ .callback = std::move(asyncReplyCallback)
                                                                      , .proxy = *this
                                                                      , .floating = false });

    asyncCallInfo->slot = message.send(reinterpret_cast<void*>(&Proxy::sdbus_async_reply_handler), asyncCallInfo.get(), timeout, return_slot);

    auto asyncCallInfoWeakPtr = std::weak_ptr{asyncCallInfo};

    floatingAsyncCallSlots_.push_back(std::move(asyncCallInfo));

    return {asyncCallInfoWeakPtr};
}

Slot Proxy::callMethodAsync(const MethodCall& message, async_reply_handler asyncReplyCallback, uint64_t timeout, return_slot_t)
{
    SDBUS_THROW_ERROR_IF(!message.isValid(), "Invalid async method call message provided", EINVAL);

    auto asyncCallInfo = std::make_unique<AsyncCallInfo>(AsyncCallInfo{ .callback = std::move(asyncReplyCallback)
                                                                      , .proxy = *this
                                                                      , .floating = true });

    asyncCallInfo->slot = message.send(reinterpret_cast<void*>(&Proxy::sdbus_async_reply_handler), asyncCallInfo.get(), timeout, return_slot);

    return {asyncCallInfo.release(), [](void *ptr){ delete static_cast<AsyncCallInfo*>(ptr); }}; // NOLINT(cppcoreguidelines-owning-memory)
}

std::future<MethodReply> Proxy::callMethodAsync(const MethodCall& message, with_future_t)
{
    return Proxy::callMethodAsync(message, {}, with_future);
}

std::future<MethodReply> Proxy::callMethodAsync(const MethodCall& message, uint64_t timeout, with_future_t)
{
    auto promise = std::make_shared<std::promise<MethodReply>>();
    auto future = promise->get_future();

    async_reply_handler asyncReplyCallback = [promise = std::move(promise)](MethodReply reply, std::optional<Error> error) noexcept
    {
        if (!error)
            promise->set_value(std::move(reply));
        else
            promise->set_exception(std::make_exception_ptr(*std::move(error)));
    };

    (void)Proxy::callMethodAsync(message, std::move(asyncReplyCallback), timeout);

    return future;
}

void Proxy::registerSignalHandler( const InterfaceName& interfaceName
                                 , const SignalName& signalName
                                 , signal_handler signalHandler )
{
    Proxy::registerSignalHandler(interfaceName.c_str(), signalName.c_str(), std::move(signalHandler));
}

void Proxy::registerSignalHandler( const char* interfaceName
                                 , const char* signalName
                                 , signal_handler signalHandler )
{
    auto slot = Proxy::registerSignalHandler(interfaceName, signalName, std::move(signalHandler), return_slot);

    floatingSignalSlots_.push_back(std::move(slot));
}

Slot Proxy::registerSignalHandler( const InterfaceName& interfaceName
                                 , const SignalName& signalName
                                 , signal_handler signalHandler
                                 , return_slot_t )
{
    return Proxy::registerSignalHandler(interfaceName.c_str(), signalName.c_str(), std::move(signalHandler), return_slot);
}

Slot Proxy::registerSignalHandler( const char* interfaceName
                                 , const char* signalName
                                 , signal_handler signalHandler
                                 , return_slot_t )
{
    SDBUS_CHECK_INTERFACE_NAME(interfaceName);
    SDBUS_CHECK_MEMBER_NAME(signalName);
    SDBUS_THROW_ERROR_IF(!signalHandler, "Invalid signal handler provided", EINVAL);

    auto signalInfo = std::make_unique<SignalInfo>(SignalInfo{std::move(signalHandler), *this, {}});

    signalInfo->slot = connection_->registerSignalHandler( destination_.c_str()
                                                         , objectPath_.c_str()
                                                         , interfaceName
                                                         , signalName
                                                         , &Proxy::sdbus_signal_handler
                                                         , signalInfo.get()
                                                         , return_slot );

    return {signalInfo.release(), [](void *ptr){ delete static_cast<SignalInfo*>(ptr); }}; // NOLINT(cppcoreguidelines-owning-memory)
}

void Proxy::unregister()
{
    floatingAsyncCallSlots_.clear();
    floatingSignalSlots_.clear();
}

sdbus::IConnection& Proxy::getConnection() const
{
    return *connection_;
}

const ObjectPath& Proxy::getObjectPath() const
{
    return objectPath_;
}

Message Proxy::getCurrentlyProcessedMessage() const
{
    return connection_->getCurrentlyProcessedMessage();
}

int Proxy::sdbus_async_reply_handler(sd_bus_message *sdbusMessage, void *userData, sd_bus_error *retError)
{
    auto* asyncCallInfo = static_cast<AsyncCallInfo*>(userData);
    assert(asyncCallInfo != nullptr);
    assert(asyncCallInfo->callback);
    auto& proxy = asyncCallInfo->proxy;

    // We are removing the CallData item at the complete scope exit, after the callback has been invoked.
    // We can't do it earlier (before callback invocation for example), because CallBack data (slot release)
    // is the synchronization point between callback invocation and Proxy::unregister.
    SCOPE_EXIT
    {
        proxy.floatingAsyncCallSlots_.erase(asyncCallInfo);
    };

    auto message = Message::Factory::create<MethodReply>(sdbusMessage, proxy.connection_.get());

    auto ok = invokeHandlerAndCatchErrors([&]
    {
        const auto* error = sd_bus_message_get_error(sdbusMessage);
        if (error == nullptr)
        {
            asyncCallInfo->callback(std::move(message), {});
        }
        else
        {
            Error exception(Error::Name{error->name}, error->message);
            asyncCallInfo->callback(std::move(message), std::move(exception));
        }
    }, retError);

    return ok ? 0 : -1;
}

int Proxy::sdbus_signal_handler(sd_bus_message *sdbusMessage, void *userData, sd_bus_error *retError)
{
    auto* signalInfo = static_cast<SignalInfo*>(userData);
    assert(signalInfo != nullptr);
    assert(signalInfo->callback);

    auto message = Message::Factory::create<Signal>(sdbusMessage, signalInfo->proxy.connection_.get());

    auto ok = invokeHandlerAndCatchErrors([&](){ signalInfo->callback(std::move(message)); }, retError);

    return ok ? 0 : -1;
}

Proxy::FloatingAsyncCallSlots::~FloatingAsyncCallSlots()
{
    clear();
}

void Proxy::FloatingAsyncCallSlots::push_back(std::shared_ptr<AsyncCallInfo> asyncCallInfo)
{
    const std::lock_guard lock(mutex_);
    if (!asyncCallInfo->finished) // The call may have finished in the meantime
        slots_.emplace_back(std::move(asyncCallInfo));
}

void Proxy::FloatingAsyncCallSlots::erase(AsyncCallInfo* info)
{
    std::unique_lock lock(mutex_);
    info->finished = true;
    auto it = std::find_if(slots_.begin(), slots_.end(), [info](auto const& entry){ return entry.get() == info; });
    if (it != slots_.end())
    {
        auto callInfo = std::move(*it);
        slots_.erase(it);
        lock.unlock();

        // Releasing call slot pointer acquires global sd-bus mutex. We have to perform the release
        // out of the `mutex_' critical section here, because if the `removeCall` is called by some
        // thread and at the same time Proxy's async reply handler (which already holds global sd-bus
        // mutex) is in progress in a different thread, we get double-mutex deadlock.
    }
}

void Proxy::FloatingAsyncCallSlots::clear()
{
    std::unique_lock lock(mutex_);
    auto asyncCallSlots = std::move(slots_);
    slots_ = {};
    lock.unlock();

    // Releasing call slot pointer acquires global sd-bus mutex. We have to perform the release
    // out of the `mutex_' critical section here, because if the `clear` is called by some thread
    // and at the same time Proxy's async reply handler (which already holds global sd-bus
    // mutex) is in progress in a different thread, we get double-mutex deadlock.
}

} // namespace sdbus::internal

namespace sdbus {

PendingAsyncCall::PendingAsyncCall(std::weak_ptr<void> callInfo)
    : callInfo_(std::move(callInfo))
{
}

void PendingAsyncCall::cancel()
{
    if (auto ptr = callInfo_.lock(); ptr != nullptr)
    {
        auto* asyncCallInfo = static_cast<internal::Proxy::AsyncCallInfo*>(ptr.get());
        asyncCallInfo->proxy.floatingAsyncCallSlots_.erase(asyncCallInfo);

        // At this point, the callData item is being deleted, leading to the release of the
        // sd-bus slot pointer. This release locks the global sd-bus mutex. If the async
        // callback is currently being processed, the sd-bus mutex is locked by the event
        // loop thread, thus access to the callData item is synchronized and thread-safe.
    }
}

bool PendingAsyncCall::isPending() const
{
    return !callInfo_.expired();
}

} // namespace sdbus

namespace sdbus {

std::unique_ptr<sdbus::IProxy> createProxy( IConnection& connection
                                          , ServiceName destination
                                          , ObjectPath objectPath )
{
    auto* sdbusConnection = dynamic_cast<sdbus::internal::IConnection*>(&connection);
    SDBUS_THROW_ERROR_IF(!sdbusConnection, "Connection is not a real sdbus-c++ connection", EINVAL);

    return std::make_unique<sdbus::internal::Proxy>( *sdbusConnection
                                                   , std::move(destination)
                                                   , std::move(objectPath) );
}

// NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved): connection is moved but cast to an internal type
std::unique_ptr<sdbus::IProxy> createProxy( std::unique_ptr<IConnection>&& connection
                                          , ServiceName destination
                                          , ObjectPath objectPath )
{
    auto* sdbusConnection = dynamic_cast<sdbus::internal::IConnection*>(connection.release());
    SDBUS_THROW_ERROR_IF(!sdbusConnection, "Connection is not a real sdbus-c++ connection", EINVAL);

    return std::make_unique<sdbus::internal::Proxy>( std::unique_ptr<sdbus::internal::IConnection>(sdbusConnection)
                                                   , std::move(destination)
                                                   , std::move(objectPath) );
}

// NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved): connection is moved cast to an internal type
std::unique_ptr<sdbus::IProxy> createProxy( std::unique_ptr<IConnection>&& connection
                                          , ServiceName destination
                                          , ObjectPath objectPath
                                          , dont_run_event_loop_thread_t )
{
    auto* sdbusConnection = dynamic_cast<sdbus::internal::IConnection*>(connection.release());
    SDBUS_THROW_ERROR_IF(!sdbusConnection, "Connection is not a real sdbus-c++ connection", EINVAL);

    return std::make_unique<sdbus::internal::Proxy>( std::unique_ptr<sdbus::internal::IConnection>(sdbusConnection)
                                                   , std::move(destination)
                                                   , std::move(objectPath)
                                                   , dont_run_event_loop_thread );
}

std::unique_ptr<sdbus::IProxy> createLightWeightProxy( std::unique_ptr<IConnection>&& connection
                                                     , ServiceName destination
                                                     , ObjectPath objectPath )
{
    return createProxy(std::move(connection), std::move(destination), std::move(objectPath), dont_run_event_loop_thread);
}

std::unique_ptr<sdbus::IProxy> createProxy( ServiceName destination
                                          , ObjectPath objectPath )
{
    auto connection = sdbus::createBusConnection();

    auto sdbusConnection = std::unique_ptr<sdbus::internal::IConnection>(dynamic_cast<sdbus::internal::IConnection*>(connection.release()));
    assert(sdbusConnection != nullptr);

    return std::make_unique<sdbus::internal::Proxy>( std::move(sdbusConnection)
                                                   , std::move(destination)
                                                   , std::move(objectPath) );
}

std::unique_ptr<sdbus::IProxy> createProxy( ServiceName destination
                                          , ObjectPath objectPath
                                          , dont_run_event_loop_thread_t )
{
    auto connection = sdbus::createBusConnection();

    auto sdbusConnection = std::unique_ptr<sdbus::internal::IConnection>(dynamic_cast<sdbus::internal::IConnection*>(connection.release()));
    assert(sdbusConnection != nullptr);

    return std::make_unique<sdbus::internal::Proxy>( std::move(sdbusConnection)
                                                   , std::move(destination)
                                                   , std::move(objectPath)
                                                   , dont_run_event_loop_thread );
}

std::unique_ptr<sdbus::IProxy> createLightWeightProxy(ServiceName destination, ObjectPath objectPath)
{
    return createProxy(std::move(destination), std::move(objectPath), dont_run_event_loop_thread);
}

} // namespace sdbus
