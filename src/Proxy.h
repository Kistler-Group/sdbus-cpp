/**
 * (C) 2016 - 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2019 Stanislav Angelovic <angelovic.s@gmail.com>
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

#include <sdbus-c++/IProxy.h>
#include "IConnection.h"
#include <systemd/sd-bus.h>
#include <string>
#include <memory>
#include <map>
#include <unordered_map>
#include <mutex>

namespace sdbus {
namespace internal {

    class Proxy
        : public IProxy
    {
    public:
        Proxy( sdbus::internal::IConnection& connection
             , std::string destination
             , std::string objectPath );
        Proxy( std::unique_ptr<sdbus::internal::IConnection>&& connection
             , std::string destination
             , std::string objectPath );

        MethodCall createMethodCall(const std::string& interfaceName, const std::string& methodName) override;
        AsyncMethodCall createAsyncMethodCall(const std::string& interfaceName, const std::string& methodName, uint64_t timeout = 0) override;
        MethodReply callMethod(const MethodCall& message) override;
        void callMethod(const AsyncMethodCall& message, async_reply_handler asyncReplyCallback) override;

        void registerSignalHandler( const std::string& interfaceName
                                  , const std::string& signalName
                                  , signal_handler signalHandler ) override;
        void finishRegistration() override;
        void unregister() override;

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
                SlotPtr slot_;
            };
            std::map<SignalName, SignalData> signals_;
        };
        std::map<InterfaceName, InterfaceData> interfaces_;

        // We need to keep track of pending async calls. When the proxy is being destructed, we must
        // remove all slots of these pending calls, otherwise in case when the connection outlives
        // the proxy, we might get async reply handlers invoked for pending async calls after the proxy
        // has been destroyed, which is a free ticket into the realm of undefined behavior.
        class AsyncCalls
        {
        public:
            struct CallData
            {
                Proxy& proxy;
                async_reply_handler callback;
                AsyncMethodCall::Slot slot;
            };

            ~AsyncCalls()
            {
                clear();
            }

            bool addCall(void* slot, std::unique_ptr<CallData>&& asyncCallData)
            {
                std::lock_guard<std::mutex> lock(mutex_);
                return calls_.emplace(slot, std::move(asyncCallData)).second;
            }

            bool removeCall(void* slot)
            {
                std::lock_guard<std::mutex> lock(mutex_);
                return calls_.erase(slot) > 0;
            }

            void clear()
            {
                std::unique_lock<std::mutex> lock(mutex_);
                auto asyncCallSlots = std::move(calls_);
                // Perform releasing of sd-bus slots outside of the calls_ critical section which avoids
                // double mutex dead lock when the async reply handler is invoked at the same time.
                lock.unlock();
            }

        private:
            std::unordered_map<void*, std::unique_ptr<CallData>> calls_;
            std::mutex mutex_;
        } pendingAsyncCalls_;
    };

}}

#endif /* SDBUS_CXX_INTERNAL_PROXY_H_ */
