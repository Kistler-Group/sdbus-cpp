/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
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
#include "ISdBus.h"

#include <systemd/sd-bus.h>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>

namespace sdbus { namespace internal {

    class Connection
        : public sdbus::IConnection           // External, public interface
        , public sdbus::internal::IConnection // Internal, private interface
    {
    public:
        enum class BusType
        {
            eSystem,
            eSession
        };

        Connection(BusType type, std::unique_ptr<ISdBus>&& interface);
        ~Connection();

        void requestName(const std::string& name) override;
        void releaseName(const std::string& name) override;
        void enterProcessingLoop() override;
        void enterProcessingLoopAsync() override;
        void leaveProcessingLoop() override;

        void* addObjectVTable( const std::string& objectPath
                             , const std::string& interfaceName
                             , const void* vtable
                             , void* userData ) override;
        void removeObjectVTable(void* vtableHandle) override;

        sdbus::MethodCall createMethodCall( const std::string& destination
                                          , const std::string& objectPath
                                          , const std::string& interfaceName
                                          , const std::string& methodName ) const override;
        sdbus::Signal createSignal( const std::string& objectPath
                                  , const std::string& interfaceName
                                  , const std::string& signalName ) const override;

        void* registerSignalHandler( const std::string& objectPath
                                   , const std::string& interfaceName
                                   , const std::string& signalName
                                   , sd_bus_message_handler_t callback
                                   , void* userData ) override;
        void unregisterSignalHandler(void* handlerCookie) override;

        void sendReplyAsynchronously(const sdbus::MethodReply& reply) override;

        std::unique_ptr<sdbus::internal::IConnection> clone() const override;

    private:
        struct WaitResult
        {
            bool msgsToProcess;
            bool asyncMsgsToProcess;
            operator bool()
            {
                return msgsToProcess || asyncMsgsToProcess;
            }
        };
        sd_bus* openBus(Connection::BusType type);
        void finishHandshake(sd_bus* bus);
        static int createLoopNotificationDescriptor();
        static void closeLoopNotificationDescriptor(int fd);
        bool processPendingRequest();
        void processAsynchronousMessages();
        WaitResult waitForNextRequest();
        static std::string composeSignalMatchFilter( const std::string& objectPath
                                                   , const std::string& interfaceName
                                                   , const std::string& signalName );
        void notifyProcessingLoop();
        void notifyProcessingLoopToExit();
        void joinWithProcessingLoop();

    private:
        std::unique_ptr<ISdBus> iface_;
        std::unique_ptr<sd_bus, std::function<sd_bus*(sd_bus*)>> bus_ {nullptr, [this](sd_bus* bus)
                                                                                {
                                                                                    return iface_->sd_bus_flush_close_unref(bus);
                                                                                }};
        std::thread asyncLoopThread_;
        std::mutex mutex_;
        std::queue<MethodReply> asyncReplies_;
        std::atomic<bool> exitLoopThread_;
        int notificationFd_{-1};
        BusType busType_;

        static constexpr const uint64_t POLL_TIMEOUT_USEC = 500000;
    };

}}

#endif /* SDBUS_CXX_INTERNAL_CONNECTION_H_ */
