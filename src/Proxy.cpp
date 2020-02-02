/**
 * (C) 2016 - 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2019 Stanislav Angelovic <angelovic.s@gmail.com>
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
#include "IConnection.h"
#include "MessageUtils.h"
#include "sdbus-c++/Message.h"
#include "sdbus-c++/IConnection.h"
#include "sdbus-c++/Error.h"
#include "ScopeGuard.h"
#include <systemd/sd-bus.h>
#include <cassert>
#include <chrono>
#include <thread>

namespace sdbus::internal {

Proxy::Proxy(sdbus::internal::IConnection& connection, std::string destination, std::string objectPath)
    : connection_(&connection, [](sdbus::internal::IConnection *){ /* Intentionally left empty */ })
    , destination_(std::move(destination))
    , objectPath_(std::move(objectPath))
{
    // The connection is not ours only, it is owned and managed by the user and we just reference
    // it here, so we expect the client to manage the event loop upon this connection themselves.
}

Proxy::Proxy( std::unique_ptr<sdbus::internal::IConnection>&& connection
            , std::string destination
            , std::string objectPath )
    : connection_(std::move(connection))
    , destination_(std::move(destination))
    , objectPath_(std::move(objectPath))
{
    // The connection is ours only, i.e. it's us who has to manage the event loop upon this connection,
    // in order that we get and process signals, async call replies, and other messages from D-Bus.
    connection_->enterEventLoopAsync();
}

MethodCall Proxy::createMethodCall(const std::string& interfaceName, const std::string& methodName)
{
    return connection_->createMethodCall(destination_, objectPath_, interfaceName, methodName);
}

MethodReply Proxy::callMethod(const MethodCall& message, uint64_t timeout)
{
    // Sending method call synchronously is the only operation that blocks, waiting for the method
    // reply message among the incoming message on the sd-bus connection socket. But typically there
    // already is somebody that generally handles incoming D-Bus messages -- the connection event loop
    // running typically in its own thread. We have to avoid polling on socket from several threads.
    // So we have to branch here: either we are within the context of the event loop thread, then we
    // can send the message simply via sd_bus_call, which blocks. Or we are in another thread, then
    // we can perform the send operation of the method call message from here (because that is thread-
    // safe like other sd-bus API accesses), but the incoming reply we have to get through the event
    // loop thread, because this is be the only rightful listener on the sd-bus connection socket.
    // So, technically, we use async means to wait here for reply received by the event loop thread.

    SDBUS_THROW_ERROR_IF(!message.isValid(), "Invalid method call message provided", EINVAL);

    // If we don't need to wait for any reply, we can send the message now irrespective of the context
    if (message.doesntExpectReply())
        return message.send(timeout);

    // If we are in the context of event loop thread, we can send the D-Bus call synchronously
    // and wait blockingly for the reply, because we are the exclusive listeners on the socket
    auto reply = connection_->tryCallMethodSynchronously(message, timeout);
    if (reply.isValid())
        return reply;

    // Otherwise we send the call asynchronously and do blocking wait for the reply from the event loop thread
    return sendMethodCallMessageAndWaitForReply(message, timeout);
}

void Proxy::callMethod(const MethodCall& message, async_reply_handler asyncReplyCallback, uint64_t timeout)
{
    SDBUS_THROW_ERROR_IF(!message.isValid(), "Invalid async method call message provided", EINVAL);

    auto callback = (void*)&Proxy::sdbus_async_reply_handler;
    auto callData = std::make_unique<AsyncCalls::CallData>(AsyncCalls::CallData{*this, std::move(asyncReplyCallback), {}});

    callData->slot = message.send(callback, callData.get(), timeout);

    pendingAsyncCalls_.addCall(callData->slot.get(), std::move(callData));
}

MethodReply Proxy::sendMethodCallMessageAndWaitForReply(const MethodCall& message, uint64_t timeout)
{
    /*thread_local*/ SyncCallReplyData syncCallReplyData;

    async_reply_handler asyncReplyCallback = [&syncCallReplyData](MethodReply& reply, const Error* error)
    {
        syncCallReplyData.sendMethodReplyToWaitingThread(reply, error);
    };
    auto callback = (void*)&Proxy::sdbus_async_reply_handler;
    AsyncCalls::CallData callData{*this, std::move(asyncReplyCallback), {}};

    message.send(callback, &callData, timeout, dont_request_slot);

    return syncCallReplyData.waitForMethodReply();
}

void Proxy::SyncCallReplyData::sendMethodReplyToWaitingThread(MethodReply& reply, const Error* error)
{
    SCOPE_EXIT{ cond_.notify_one(); };
    std::unique_lock<std::mutex> lock{mutex_};
    SCOPE_EXIT{ arrived_ = true; };

    //error_ = nullptr; // Necessary if SyncCallReplyData instance is thread_local

    if (error == nullptr)
        reply_ = std::move(reply);
    else
        error_ = std::make_unique<Error>(*error);
}

MethodReply Proxy::SyncCallReplyData::waitForMethodReply()
{
    std::unique_lock<std::mutex> lock{mutex_};
    cond_.wait(lock, [this](){ return arrived_; });

    //arrived_ = false; // Necessary if SyncCallReplyData instance is thread_local

    if (error_)
        throw *error_;

    return std::move(reply_);
}

void Proxy::registerSignalHandler( const std::string& interfaceName
                                 , const std::string& signalName
                                 , signal_handler signalHandler )
{
    SDBUS_THROW_ERROR_IF(!signalHandler, "Invalid signal handler provided", EINVAL);

    auto& interface = interfaces_[interfaceName];

    InterfaceData::SignalData signalData{std::move(signalHandler), nullptr};
    auto insertionResult = interface.signals_.emplace(signalName, std::move(signalData));

    auto inserted = insertionResult.second;
    SDBUS_THROW_ERROR_IF(!inserted, "Failed to register signal handler: handler already exists", EINVAL);
}

void Proxy::finishRegistration()
{
    registerSignalHandlers(*connection_);
}

void Proxy::registerSignalHandlers(sdbus::internal::IConnection& connection)
{
    for (auto& interfaceItem : interfaces_)
    {
        const auto& interfaceName = interfaceItem.first;
        auto& signalsOnInterface = interfaceItem.second.signals_;

        for (auto& signalItem : signalsOnInterface)
        {
            const auto& signalName = signalItem.first;
            auto& slot = signalItem.second.slot_;
            slot = connection.registerSignalHandler( objectPath_
                                                   , interfaceName
                                                   , signalName
                                                   , &Proxy::sdbus_signal_handler
                                                   , this );
        }
    }
}

void Proxy::unregister()
{
    pendingAsyncCalls_.clear();
    interfaces_.clear();
}

int Proxy::sdbus_async_reply_handler(sd_bus_message *sdbusMessage, void *userData, sd_bus_error */*retError*/)
{
    auto* asyncCallData = static_cast<AsyncCalls::CallData*>(userData);
    assert(asyncCallData != nullptr);
    assert(asyncCallData->callback);
    auto& proxy = asyncCallData->proxy;

    SCOPE_EXIT
    {
        // Slot may be null if we're doing blocking synchronous call implemented by means of asynchronous call,
        // because in that case the call data is still alive on the stack, we don't need to manage it separately.
        if (asyncCallData->slot)
            proxy.pendingAsyncCalls_.removeCall(asyncCallData->slot.get());
    };

    auto message = Message::Factory::create<MethodReply>(sdbusMessage, &proxy.connection_->getSdBusInterface());

    const auto* error = sd_bus_message_get_error(sdbusMessage);
    if (error == nullptr)
    {
        asyncCallData->callback(message, nullptr);
    }
    else
    {
        sdbus::Error exception(error->name, error->message);
        asyncCallData->callback(message, &exception);
    }

    return 1;
}

int Proxy::sdbus_signal_handler(sd_bus_message *sdbusMessage, void *userData, sd_bus_error */*retError*/)
{
    auto* proxy = static_cast<Proxy*>(userData);
    assert(proxy != nullptr);

    auto message = Message::Factory::create<Signal>(sdbusMessage, &proxy->connection_->getSdBusInterface());

    // Note: The lookup can be optimized by using sorted vectors instead of associative containers
    auto& callback = proxy->interfaces_[message.getInterfaceName()].signals_[message.getMemberName()].callback_;
    assert(callback);

    callback(message);

    return 1;
}

}

namespace sdbus {

std::unique_ptr<sdbus::IProxy> createProxy( IConnection& connection
                                          , std::string destination
                                          , std::string objectPath )
{
    auto* sdbusConnection = dynamic_cast<sdbus::internal::IConnection*>(&connection);
    SDBUS_THROW_ERROR_IF(!sdbusConnection, "Connection is not a real sdbus-c++ connection", EINVAL);

    return std::make_unique<sdbus::internal::Proxy>( *sdbusConnection
                                                   , std::move(destination)
                                                   , std::move(objectPath) );
}

std::unique_ptr<sdbus::IProxy> createProxy( std::unique_ptr<IConnection>&& connection
                                          , std::string destination
                                          , std::string objectPath )
{
    auto* sdbusConnection = dynamic_cast<sdbus::internal::IConnection*>(connection.get());
    SDBUS_THROW_ERROR_IF(!sdbusConnection, "Connection is not a real sdbus-c++ connection", EINVAL);

    connection.release();

    return std::make_unique<sdbus::internal::Proxy>( std::unique_ptr<sdbus::internal::IConnection>(sdbusConnection)
                                                   , std::move(destination)
                                                   , std::move(objectPath) );
}

std::unique_ptr<sdbus::IProxy> createProxy( std::string destination
                                          , std::string objectPath )
{
    auto connection = sdbus::createConnection();

    auto sdbusConnection = std::unique_ptr<sdbus::internal::IConnection>(dynamic_cast<sdbus::internal::IConnection*>(connection.release()));
    assert(sdbusConnection != nullptr);

    return std::make_unique<sdbus::internal::Proxy>( std::move(sdbusConnection)
                                                   , std::move(destination)
                                                   , std::move(objectPath) );
}

}
