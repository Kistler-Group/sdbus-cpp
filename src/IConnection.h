/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
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

#include <systemd/sd-bus.h>
#include <string>
#include <memory>
#include <functional>
#include <vector>

// Forward declaration
namespace sdbus {
    class MethodCall;
    class AsyncMethodCall;
    class MethodReply;
    class Signal;
    namespace internal {
        class ISdBus;
    }
}

namespace sdbus {
namespace internal {

    using SlotPtr = std::unique_ptr<void, std::function<void(void*)>>;

    class IConnection
    {
    public:
        IConnection() = default;
        virtual ~IConnection() = default;
        IConnection(const IConnection& other) = delete;
        IConnection(IConnection&& other) = delete;
        IConnection& operator=(const IConnection& rhs) = delete;
        IConnection& operator=(IConnection&& rhs) = delete;

        virtual const ISdBus& getSdBusInterface() const = 0;
        virtual ISdBus& getSdBusInterface() = 0;

        virtual SlotPtr addObjectVTable( const std::string& objectPath
                                       , const std::string& interfaceName
                                       , const sd_bus_vtable* vtable
                                       , void* userData ) = 0;

        virtual MethodCall createMethodCall( const std::string& destination
                                           , const std::string& objectPath
                                           , const std::string& interfaceName
                                           , const std::string& methodName ) const = 0;

        virtual Signal createSignal( const std::string& objectPath
                                   , const std::string& interfaceName
                                   , const std::string& signalName ) const = 0;

        virtual void emitPropertiesChangedSignal( const std::string& objectPath
                                                , const std::string& interfaceName
                                                , const std::vector<std::string>& propNames ) = 0;
        virtual void emitInterfacesAddedSignal(const std::string& objectPath) = 0;
        virtual void emitInterfacesAddedSignal( const std::string& objectPath
                                              , const std::vector<std::string>& interfaces ) = 0;
        virtual void emitInterfacesRemovedSignal(const std::string& objectPath) = 0;
        virtual void emitInterfacesRemovedSignal( const std::string& objectPath
                                                , const std::vector<std::string>& interfaces ) = 0;

        virtual SlotPtr addObjectManager(const std::string& objectPath, void* /*dummy*/ = nullptr) = 0;

        virtual SlotPtr registerSignalHandler( const std::string& objectPath
                                             , const std::string& interfaceName
                                             , const std::string& signalName
                                             , sd_bus_message_handler_t callback
                                             , void* userData ) = 0;

        virtual void enterProcessingLoopAsync() = 0;
        virtual void leaveProcessingLoop() = 0;
    };

}
}

#endif /* SDBUS_CXX_INTERNAL_ICONNECTION_H_ */
