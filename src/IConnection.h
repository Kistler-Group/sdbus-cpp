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

// Forward declaration
namespace sdbus {
    class Message;
}

namespace sdbus {
namespace internal {

    class IConnection
    {
    public:
        virtual void* addObjectVTable( const std::string& objectPath
                                     , const std::string& interfaceName
                                     , const void* vtable
                                     , void* userData ) = 0;
        virtual void removeObjectVTable(void* vtableHandle) = 0;

        virtual sdbus::Message createMethodCall( const std::string& destination
                                               , const std::string& objectPath
                                               , const std::string& interfaceName
                                               , const std::string& methodName ) const = 0;

        virtual sdbus::Message createSignal( const std::string& objectPath
                                           , const std::string& interfaceName
                                           , const std::string& signalName ) const = 0;

        virtual void* registerSignalHandler( const std::string& objectPath
                                           , const std::string& interfaceName
                                           , const std::string& signalName
                                           , sd_bus_message_handler_t callback
                                           , void* userData ) = 0;
        virtual void unregisterSignalHandler(void* handlerCookie) = 0;

        virtual void enterProcessingLoopAsync() = 0;
        virtual void leaveProcessingLoop() = 0;

        virtual std::unique_ptr<sdbus::internal::IConnection> clone() const = 0;

        virtual ~IConnection() = default;
    };

}
}

#endif /* SDBUS_CXX_INTERNAL_ICONNECTION_H_ */
