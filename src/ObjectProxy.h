/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 *
 * @file ObjectProxy.h
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

#ifndef SDBUS_CXX_INTERNAL_OBJECTPROXY_H_
#define SDBUS_CXX_INTERNAL_OBJECTPROXY_H_

#include <sdbus-c++/IObjectProxy.h>
#include <systemd/sd-bus.h>
#include <string>
#include <memory>
#include <map>

// Forward declarations
namespace sdbus { namespace internal {
    class IConnection;
}}

namespace sdbus {
namespace internal {

    class ObjectProxy
        : public IObjectProxy
    {
    public:
        ObjectProxy( sdbus::internal::IConnection& connection
                   , std::string destination
                   , std::string objectPath );
        ObjectProxy( std::unique_ptr<sdbus::internal::IConnection>&& connection
                   , std::string destination
                   , std::string objectPath );

        MethodCall createMethodCall(const std::string& interfaceName, const std::string& methodName) override;
        AsyncMethodCall createAsyncMethodCall(const std::string& interfaceName, const std::string& methodName) override;
        MethodReply callMethod(const MethodCall& message) override;
        void callMethod(const AsyncMethodCall& message, async_reply_handler asyncReplyCallback) override;

        void registerSignalHandler( const std::string& interfaceName
                                  , const std::string& signalName
                                  , signal_handler signalHandler ) override;
        void finishRegistration() override;

    private:
        void registerSignalHandlers(sdbus::internal::IConnection& connection);
        static int sdbus_async_reply_handler(sd_bus_message *sdbusMessage, void *userData, sd_bus_error *retError);
        static int sdbus_signal_callback(sd_bus_message *sdbusMessage, void *userData, sd_bus_error *retError);

    private:
        std::unique_ptr< sdbus::internal::IConnection
                       , std::function<void(sdbus::internal::IConnection*)>
                       > connection_;
        std::string destination_;
        std::string objectPath_;

        using InterfaceName = std::string;
        struct InterfaceData
        {
            using SignalName = std::string;
            struct SignalData
            {
                signal_handler callback_;
                std::unique_ptr<sd_bus_slot, std::function<void(sd_bus_slot*)>> slot_;
            };
            std::map<SignalName, SignalData> signals_;
        };
        std::map<InterfaceName, InterfaceData> interfaces_;
    };

}}

#endif /* SDBUS_CXX_INTERNAL_OBJECTPROXY_H_ */
