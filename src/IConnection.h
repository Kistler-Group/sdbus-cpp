/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2026 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file IConnection.h
 *
 * Created on: Nov 9, 2016
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

#ifndef SDBUS_CXX_INTERNAL_ICONNECTION_H_
#define SDBUS_CXX_INTERNAL_ICONNECTION_H_

#include "sdbus-c++/IConnection.h"

#include "sdbus-c++/TypeTraits.h"

#include <memory>
#include SDBUS_HEADER
#include <vector>

// Forward declarations
namespace sdbus {
    class MethodCall;
    class MethodReply;
    class Signal;
    class PlainMessage;
    class ObjectPath;
    class InterfaceName;
    class BusName;
    using ServiceName = BusName;
    class MemberName;
    using MethodName = MemberName;
    using SignalName = MemberName;
    using PropertyName = MemberName;
    class Error;
    namespace internal {
        class ISdBus;
    } // namespace internal
} // namespace sdbus

namespace sdbus::internal {

    class IConnection
        : public ::sdbus::IConnection
    {
    public:
        ~IConnection() override = default;

        [[nodiscard]] virtual Slot addObjectVTable( const ObjectPath& objectPath
                                                  , const InterfaceName& interfaceName
                                                  , const sd_bus_vtable* vtable
                                                  , void* userData
                                                  , return_slot_t ) = 0;

        [[nodiscard]] virtual PlainMessage createPlainMessage() const = 0;
        [[nodiscard]] virtual MethodCall createMethodCall( const ServiceName& destination
                                                         , const ObjectPath& objectPath
                                                         , const InterfaceName& interfaceName
                                                         , const MethodName& methodName ) const = 0;
        [[nodiscard]] virtual MethodCall createMethodCall( const char* destination
                                                         , const char* objectPath
                                                         , const char* interfaceName
                                                         , const char* methodName ) const = 0;
        [[nodiscard]] virtual Signal createSignal( const ObjectPath& objectPath
                                                 , const InterfaceName& interfaceName
                                                 , const SignalName& signalName ) const = 0;
        [[nodiscard]] virtual Signal createSignal( const char* objectPath
                                                 , const char* interfaceName
                                                 , const char* signalName ) const = 0;

        virtual void emitPropertiesChangedSignal( const ObjectPath& objectPath
                                                , const InterfaceName& interfaceName
                                                , const std::vector<PropertyName>& propNames ) = 0;
        virtual void emitPropertiesChangedSignal( const char* objectPath
                                                , const char* interfaceName
                                                , const std::vector<PropertyName>& propNames ) = 0;
        virtual void emitInterfacesAddedSignal(const ObjectPath& objectPath) = 0;
        virtual void emitInterfacesAddedSignal( const ObjectPath& objectPath
                                              , const std::vector<InterfaceName>& interfaces ) = 0;
        virtual void emitInterfacesRemovedSignal(const ObjectPath& objectPath) = 0;
        virtual void emitInterfacesRemovedSignal( const ObjectPath& objectPath
                                                , const std::vector<InterfaceName>& interfaces ) = 0;

        [[nodiscard]] virtual Slot registerSignalHandler( const char* sender
                                                        , const char* objectPath
                                                        , const char* interfaceName
                                                        , const char* signalName
                                                        , sd_bus_message_handler_t callback
                                                        , void* userData
                                                        , return_slot_t ) = 0;

        virtual sd_bus_message* incrementMessageRefCount(sd_bus_message* sdbusMsg) = 0;
        virtual sd_bus_message* decrementMessageRefCount(sd_bus_message* sdbusMsg) = 0;

        // TODO: Refactor to higher level (Creds class will ownership handling and getters)
        virtual int querySenderCredentials(sd_bus_message* sdbusMsg, uint64_t mask, sd_bus_creds **creds) = 0;
        virtual sd_bus_creds* incrementCredsRefCount(sd_bus_creds* creds) = 0;
        virtual sd_bus_creds* decrementCredsRefCount(sd_bus_creds* creds) = 0;

        virtual sd_bus_message* callMethod(sd_bus_message* sdbusMsg, uint64_t timeout) = 0;
        [[nodiscard]] virtual Slot callMethodAsync( sd_bus_message* sdbusMsg
                                                  , sd_bus_message_handler_t callback
                                                  , void* userData
                                                  , uint64_t timeout
                                                  , return_slot_t ) = 0;
        virtual void sendMessage(sd_bus_message* sdbusMsg) = 0;

        virtual sd_bus_message* createMethodReply(sd_bus_message* sdbusMsg) = 0;
        virtual sd_bus_message* createErrorReplyMessage(sd_bus_message* sdbusMsg, const Error& error) = 0;
    };

    [[nodiscard]] std::unique_ptr<IConnection> createPseudoConnection();

} // namespace sdbus::internal

#endif /* SDBUS_CXX_INTERNAL_ICONNECTION_H_ */
