/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2022 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
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

#include <functional>
#include <memory>
#include <string>
#include SDBUS_HEADER
#include <vector>

// Forward declaration
namespace sdbus {
    class MethodCall;
    class MethodReply;
    class Signal;
    class PlainMessage;
    namespace internal {
        class ISdBus;
    }
}

namespace sdbus::internal {

    class IConnection
        : public ::sdbus::IConnection
    {
    public:
        ~IConnection() override = default;

        [[nodiscard]] virtual const ISdBus& getSdBusInterface() const = 0;
        [[nodiscard]] virtual ISdBus& getSdBusInterface() = 0;

        [[nodiscard]] virtual Slot addObjectVTable( const std::string& objectPath
                                                  , const std::string& interfaceName
                                                  , const sd_bus_vtable* vtable
                                                  , void* userData ) = 0;

        [[nodiscard]] virtual PlainMessage createPlainMessage() const = 0;
        [[nodiscard]] virtual MethodCall createMethodCall( const std::string& destination
                                                         , const std::string& objectPath
                                                         , const std::string& interfaceName
                                                         , const std::string& methodName ) const = 0;
        [[nodiscard]] virtual Signal createSignal( const std::string& objectPath
                                                 , const std::string& interfaceName
                                                 , const std::string& signalName ) const = 0;

        virtual MethodReply callMethod(const MethodCall& message, uint64_t timeout) = 0;
        virtual void callMethod(const MethodCall& message, void* callback, void* userData, uint64_t timeout, floating_slot_t) = 0;
        [[nodiscard]] virtual Slot callMethod(const MethodCall& message, void* callback, void* userData, uint64_t timeout) = 0;

        virtual void emitPropertiesChangedSignal( const std::string& objectPath
                                                , const std::string& interfaceName
                                                , const std::vector<std::string>& propNames ) = 0;
        virtual void emitInterfacesAddedSignal(const std::string& objectPath) = 0;
        virtual void emitInterfacesAddedSignal( const std::string& objectPath
                                              , const std::vector<std::string>& interfaces ) = 0;
        virtual void emitInterfacesRemovedSignal(const std::string& objectPath) = 0;
        virtual void emitInterfacesRemovedSignal( const std::string& objectPath
                                                , const std::vector<std::string>& interfaces ) = 0;

        using sdbus::IConnection::addObjectManager;
        [[nodiscard]] virtual Slot addObjectManager(const std::string& objectPath, return_slot_t) = 0;

        [[nodiscard]] virtual Slot registerSignalHandler( const std::string& sender
                                                        , const std::string& objectPath
                                                        , const std::string& interfaceName
                                                        , const std::string& signalName
                                                        , sd_bus_message_handler_t callback
                                                        , void* userData ) = 0;
    };

    [[nodiscard]] std::unique_ptr<sdbus::internal::IConnection> createPseudoConnection();

}

#endif /* SDBUS_CXX_INTERNAL_ICONNECTION_H_ */
