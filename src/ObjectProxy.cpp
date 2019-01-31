/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 *
 * @file ObjectProxy.cpp
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

#include "ObjectProxy.h"
#include <sdbus-c++/Message.h>
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/Error.h>
#include "IConnection.h"
#include <systemd/sd-bus.h>
#include <cassert>

namespace sdbus { namespace internal {

ObjectProxy::ObjectProxy(sdbus::internal::IConnection& connection, std::string destination, std::string objectPath)
    : connection_(&connection, [](sdbus::internal::IConnection *){ /* Intentionally left empty */ })
    , destination_(std::move(destination))
    , objectPath_(std::move(objectPath))
{
}

ObjectProxy::ObjectProxy( std::unique_ptr<sdbus::internal::IConnection>&& connection
                        , std::string destination
                        , std::string objectPath )
    : connection_(std::move(connection))
    , destination_(std::move(destination))
    , objectPath_(std::move(objectPath))
{
    // The connection is ours only, so we have to manage event loop upon this connection,
    // so we get signals, async replies, and other messages from D-Bus.
    // TODO uncomment connection_->enterProcessingLoopAsync();
}

MethodCall ObjectProxy::createMethodCall(const std::string& interfaceName, const std::string& methodName)
{
    return connection_->createMethodCall(destination_, objectPath_, interfaceName, methodName);
}

AsyncMethodCall ObjectProxy::createAsyncMethodCall(const std::string& interfaceName, const std::string& methodName)
{
    return AsyncMethodCall{ObjectProxy::createMethodCall(interfaceName, methodName)};
}

MethodReply ObjectProxy::callMethod(const MethodCall& message)
{
    return connection_->callMethod(message);
}

void ObjectProxy::callMethod(const AsyncMethodCall& message, async_reply_handler asyncReplyCallback)
{
    auto callback = (void*)&ObjectProxy::sdbus_async_reply_handler;
    // Allocated userData gets deleted in the sdbus_async_reply_handler
    auto userData = new async_reply_handler(std::move(asyncReplyCallback));

    connection_->callMethod(message, callback, userData);
}

void ObjectProxy::registerSignalHandler( const std::string& interfaceName
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

void ObjectProxy::finishRegistration()
{
    registerSignalHandlers(*connection_);
}

void ObjectProxy::registerSignalHandlers(sdbus::internal::IConnection& connection)
{
    for (auto& interfaceItem : interfaces_)
    {
        const auto& interfaceName = interfaceItem.first;
        auto& signalsOnInterface = interfaceItem.second.signals_;

        for (auto& signalItem : signalsOnInterface)
        {
            const auto& signalName = signalItem.first;
            auto& slot = signalItem.second.slot_;
            auto* rawSlotPtr = connection.registerSignalHandler( objectPath_
                                                               , interfaceName
                                                               , signalName
                                                               , &ObjectProxy::sdbus_signal_callback
                                                               , this );
            slot.reset(rawSlotPtr);
            slot.get_deleter() = [&connection](void *slot){ connection.unregisterSignalHandler(slot); };
        }
    }
}

int ObjectProxy::sdbus_async_reply_handler(sd_bus_message *sdbusMessage, void *userData, sd_bus_error *retError)
{
    MethodReply message(sdbusMessage);

    std::unique_ptr<async_reply_handler> asyncReplyCallback{static_cast<async_reply_handler*>(userData)};
    assert(asyncReplyCallback != nullptr);

    if (!sd_bus_error_is_set(retError))
    {
        (*asyncReplyCallback)(message, nullptr);
    }
    else
    {
        sdbus::Error error(retError->name, retError->message);
        (*asyncReplyCallback)(message, &error);
    }
}

int ObjectProxy::sdbus_signal_callback(sd_bus_message *sdbusMessage, void *userData, sd_bus_error */*retError*/)
{
    Signal message(sdbusMessage);

    auto* object = static_cast<ObjectProxy*>(userData);
    assert(object != nullptr);
    // Note: The lookup can be optimized by using sorted vectors instead of associative containers
    auto& callback = object->interfaces_[message.getInterfaceName()].signals_[message.getMemberName()].callback_;
    assert(callback);

    callback(message);

    return 1;
}

}}

namespace sdbus {

std::unique_ptr<sdbus::IObjectProxy> createObjectProxy( IConnection& connection
                                                      , std::string destination
                                                      , std::string objectPath )
{
    auto* sdbusConnection = dynamic_cast<sdbus::internal::IConnection*>(&connection);
    SDBUS_THROW_ERROR_IF(!sdbusConnection, "Connection is not a real sdbus-c++ connection", EINVAL);

    return std::make_unique<sdbus::internal::ObjectProxy>( *sdbusConnection
                                                         , std::move(destination)
                                                         , std::move(objectPath) );
}

std::unique_ptr<sdbus::IObjectProxy> createObjectProxy( std::unique_ptr<IConnection>&& connection
                                                      , std::string destination
                                                      , std::string objectPath )
{
    auto* sdbusConnection = dynamic_cast<sdbus::internal::IConnection*>(connection.get());
    SDBUS_THROW_ERROR_IF(!sdbusConnection, "Connection is not a real sdbus-c++ connection", EINVAL);

    connection.release();

    return std::make_unique<sdbus::internal::ObjectProxy>( std::unique_ptr<sdbus::internal::IConnection>(sdbusConnection)
                                                         , std::move(destination)
                                                         , std::move(objectPath) );
}

std::unique_ptr<sdbus::IObjectProxy> createObjectProxy( std::string destination
                                                      , std::string objectPath )
{
    auto connection = sdbus::createConnection();

    auto sdbusConnection = std::unique_ptr<sdbus::internal::IConnection>(dynamic_cast<sdbus::internal::IConnection*>(connection.release()));
    assert(sdbusConnection != nullptr);

    return std::make_unique<sdbus::internal::ObjectProxy>( std::move(sdbusConnection)
                                                         , std::move(destination)
                                                         , std::move(objectPath) );
}

}
