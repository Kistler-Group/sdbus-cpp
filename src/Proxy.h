/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2024 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file Proxy.h
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

#ifndef SDBUS_CXX_INTERNAL_PROXY_H_
#define SDBUS_CXX_INTERNAL_PROXY_H_

#include "sdbus-c++/IProxy.h"

#include "IConnection.h"
#include "sdbus-c++/Types.h"

#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include SDBUS_HEADER
#include <vector>

namespace sdbus::internal {

    class Proxy
        : public IProxy
    {
    public:
        Proxy( sdbus::internal::IConnection& connection
             , ServiceName destination
             , ObjectPath objectPath );
        Proxy( std::unique_ptr<sdbus::internal::IConnection>&& connection
             , ServiceName destination
             , ObjectPath objectPath );
        Proxy( std::unique_ptr<sdbus::internal::IConnection>&& connection
             , ServiceName destination
             , ObjectPath objectPath
             , dont_run_event_loop_thread_t );

        MethodCall createMethodCall(const InterfaceName& interfaceName, const MethodName& methodName) override;
        MethodCall createMethodCall(const char* interfaceName, const char* methodName) override;
        MethodReply callMethod(const MethodCall& message) override;
        MethodReply callMethod(const MethodCall& message, uint64_t timeout) override;
        PendingAsyncCall callMethodAsync(const MethodCall& message, async_reply_handler asyncReplyCallback) override;
        Slot callMethodAsync( const MethodCall& message
                            , async_reply_handler asyncReplyCallback
                            , return_slot_t ) override;
        PendingAsyncCall callMethodAsync( const MethodCall& message
                                        , async_reply_handler asyncReplyCallback
                                        , uint64_t timeout ) override;
        Slot callMethodAsync( const MethodCall& message
                            , async_reply_handler asyncReplyCallback
                            , uint64_t timeout
                            , return_slot_t ) override;
        std::future<MethodReply> callMethodAsync(const MethodCall& message, with_future_t) override;
        std::future<MethodReply> callMethodAsync(const MethodCall& message, uint64_t timeout, with_future_t) override;

        void registerSignalHandler( const InterfaceName& interfaceName
                                  , const SignalName& signalName
                                  , signal_handler signalHandler ) override;
        void registerSignalHandler( const char* interfaceName
                                  , const char* signalName
                                  , signal_handler signalHandler ) override;
        Slot registerSignalHandler( const InterfaceName& interfaceName
                                  , const SignalName& signalName
                                  , signal_handler signalHandler
                                  , return_slot_t ) override;
        Slot registerSignalHandler( const char* interfaceName
                                  , const char* signalName
                                  , signal_handler signalHandler
                                  , return_slot_t ) override;
        void unregister() override;

        [[nodiscard]] sdbus::IConnection& getConnection() const override;
        [[nodiscard]] const ObjectPath& getObjectPath() const override;
        [[nodiscard]] Message getCurrentlyProcessedMessage() const override;

    private:
        static int sdbus_signal_handler(sd_bus_message *sdbusMessage, void *userData, sd_bus_error *retError);
        static int sdbus_async_reply_handler(sd_bus_message *sdbusMessage, void *userData, sd_bus_error *retError);

    private:
        friend PendingAsyncCall;

        std::unique_ptr< sdbus::internal::IConnection
                       , std::function<void(sdbus::internal::IConnection*)>
                       > connection_;
        ServiceName destination_;
        ObjectPath objectPath_;

        std::vector<Slot> floatingSignalSlots_;

        struct SignalInfo
        {
            signal_handler callback;
            Proxy& proxy;
            Slot slot;
        };

        struct AsyncCallInfo
        {
            async_reply_handler callback;
            Proxy& proxy;
            Slot slot{};
            bool finished{false};
            bool floating;
        };

        // Container keeping track of pending async calls
        class FloatingAsyncCallSlots
        {
        public:
            ~FloatingAsyncCallSlots();
            void push_back(std::shared_ptr<AsyncCallInfo> asyncCallInfo);
            void erase(AsyncCallInfo* info);
            void clear();

        private:
            std::mutex mutex_;
            std::deque<std::shared_ptr<AsyncCallInfo>> slots_;
        };

        FloatingAsyncCallSlots floatingAsyncCallSlots_;
    };

}

#endif /* SDBUS_CXX_INTERNAL_PROXY_H_ */
