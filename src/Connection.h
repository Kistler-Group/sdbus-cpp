/**
 * (C) 2016 - 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2019 Stanislav Angelovic <angelovic.s@gmail.com>
 *
 * @file Connection.h
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

#ifndef SDBUS_CXX_INTERNAL_CONNECTION_H_
#define SDBUS_CXX_INTERNAL_CONNECTION_H_

#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/Message.h>
#include "IConnection.h"
#include "ScopeGuard.h"
#include "ISdBus.h"
#include <systemd/sd-bus.h>
#include <memory>
#include <thread>
#include <string>
#include <vector>

namespace sdbus { namespace internal {

    class Connection
        : public sdbus::IConnection           // External, public interface
        , public sdbus::internal::IConnection // Internal, private interface
    {
    public:
        // Bus type tags
        struct system_bus_t{};
        struct session_bus_t{};
        struct remote_system_bus_t{};

        Connection(std::unique_ptr<ISdBus>&& interface, system_bus_t);
        Connection(std::unique_ptr<ISdBus>&& interface, session_bus_t);
        Connection(std::unique_ptr<ISdBus>&& interface, remote_system_bus_t, const std::string& host);
        ~Connection() override;

        void requestName(const std::string& name) override;
        void releaseName(const std::string& name) override;
        void enterProcessingLoop() override;
        void enterProcessingLoopAsync() override;
        void leaveProcessingLoop() override;
        sdbus::IConnection::PollData getProcessLoopPollData() override;
        bool processPendingRequest() override;

        void addObjectManager(const std::string& objectPath) override;
        SlotPtr addObjectManager(const std::string& objectPath, void* /*dummy*/) override;

        void setMethodCallTimeout(uint64_t timeout) override;
        uint64_t getMethodCallTimeout() const override;

        const ISdBus& getSdBusInterface() const override;
        ISdBus& getSdBusInterface() override;

        SlotPtr addObjectVTable( const std::string& objectPath
                               , const std::string& interfaceName
                               , const sd_bus_vtable* vtable
                               , void* userData ) override;

        MethodCall createMethodCall( const std::string& destination
                                   , const std::string& objectPath
                                   , const std::string& interfaceName
                                   , const std::string& methodName ) const override;

        Signal createSignal( const std::string& objectPath
                           , const std::string& interfaceName
                           , const std::string& signalName ) const override;

        void emitPropertiesChangedSignal( const std::string& objectPath
                                        , const std::string& interfaceName
                                        , const std::vector<std::string>& propNames ) override;
        void emitInterfacesAddedSignal(const std::string& objectPath) override;
        void emitInterfacesAddedSignal( const std::string& objectPath
                                      , const std::vector<std::string>& interfaces ) override;
        void emitInterfacesRemovedSignal(const std::string& objectPath) override;
        void emitInterfacesRemovedSignal( const std::string& objectPath
                                        , const std::vector<std::string>& interfaces ) override;

        SlotPtr registerSignalHandler( const std::string& objectPath
                                     , const std::string& interfaceName
                                     , const std::string& signalName
                                     , sd_bus_message_handler_t callback
                                     , void* userData ) override;

    private:
        using BusFactory = std::function<int(sd_bus**)>;
        using BusPtr = std::unique_ptr<sd_bus, std::function<sd_bus*(sd_bus*)>>;
        Connection(std::unique_ptr<ISdBus>&& interface, const BusFactory& busFactory);

        BusPtr openBus(const std::function<int(sd_bus**)>& busFactory);
        void finishHandshake(sd_bus* bus);
        bool waitForNextRequest();
        static std::string composeSignalMatchFilter( const std::string& objectPath
                                                   , const std::string& interfaceName
                                                   , const std::string& signalName );
        void notifyProcessingLoopToExit();
        void clearExitNotification();
        void joinWithProcessingLoop();
        static std::vector</*const */char*> to_strv(const std::vector<std::string>& strings);

        struct LoopExitEventFd
        {
            LoopExitEventFd();
            ~LoopExitEventFd();
            int fd;
        };

    private:
        std::unique_ptr<ISdBus> iface_;
        BusPtr bus_;
        std::thread asyncLoopThread_;
        LoopExitEventFd loopExitFd_;
    };

}}

#endif /* SDBUS_CXX_INTERNAL_CONNECTION_H_ */
