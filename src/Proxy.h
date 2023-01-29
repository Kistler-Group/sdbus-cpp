/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2022 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
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
#include <atomic>
#include <condition_variable>

namespace sdbus::internal {

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
        Proxy( std::unique_ptr<sdbus::internal::IConnection>&& connection
             , std::string destination
             , std::string objectPath
             , dont_run_event_loop_thread_t );

        MethodCall createMethodCall(const std::string& interfaceName, const std::string& methodName) override;
        MethodReply callMethod(const MethodCall& message, uint64_t timeout) override;
        PendingAsyncCall callMethod(const MethodCall& message, async_reply_handler asyncReplyCallback, uint64_t timeout) override;
        std::future<MethodReply> callMethod(const MethodCall& message, with_future_t) override;
        std::future<MethodReply> callMethod(const MethodCall& message, uint64_t timeout, with_future_t) override;

        void registerSignalHandler( const std::string& interfaceName
                                  , const std::string& signalName
                                  , signal_handler signalHandler ) override;
        void unregisterSignalHandler( const std::string& interfaceName
                                    , const std::string& signalName ) override;

        void finishRegistration() override;
        void unregister() override;

        sdbus::IConnection& getConnection() const override;
        const std::string& getObjectPath() const override;
        const Message* getCurrentlyProcessedMessage() const override;

    private:
        class SyncCallReplyData
        {
        public:
            void sendMethodReplyToWaitingThread(MethodReply& reply, const Error* error);
            MethodReply waitForMethodReply();

        private:
            std::mutex mutex_;
            std::condition_variable cond_;
            bool arrived_{};
            MethodReply reply_;
            std::unique_ptr<Error> error_;
        };

        MethodReply sendMethodCallMessageAndWaitForReply(const MethodCall& message, uint64_t timeout);
        void registerSignalHandlers(sdbus::internal::IConnection& connection);
        static int sdbus_async_reply_handler(sd_bus_message *sdbusMessage, void *userData, sd_bus_error *retError);
        static int sdbus_signal_handler(sd_bus_message *sdbusMessage, void *userData, sd_bus_error *retError);

    private:
        friend PendingAsyncCall;

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
                SignalData(Proxy& proxy, signal_handler callback, Slot slot)
                    : proxy(proxy)
                    , callback(std::move(callback))
                    , slot(std::move(slot))
                {}
                Proxy& proxy;
                signal_handler callback;
                // slot must be listed after callback to ensure that slot is destructed first.
                // Destructing the slot will sd_bus_slot_unref() the callback.
                // Only after sd_bus_slot_unref(), we can safely delete the callback. The bus mutex (SdBus::sdbusMutex_)
                // ensures that sd_bus_slot_unref() and the callback execute sequentially.
                Slot slot;
            };
            std::map<SignalName, std::unique_ptr<SignalData>> signals_;
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
                Slot slot;
            };

            ~AsyncCalls()
            {
                clear();
            }

            bool addCall(void* slot, std::shared_ptr<CallData> asyncCallData)
            {
                std::lock_guard lock(mutex_);
                return calls_.emplace(slot, std::move(asyncCallData)).second;
            }

            void removeCall(void* slot)
            {
                std::unique_lock lock(mutex_);
                if (auto it = calls_.find(slot); it != calls_.end())
                {
                    auto callData = std::move(it->second);
                    calls_.erase(it);
                    lock.unlock();

                    // Releasing call slot pointer acquires global sd-bus mutex. We have to perform the release
                    // out of the `mutex_' critical section here, because if the `removeCall` is called by some
                    // thread and at the same time Proxy's async reply handler (which already holds global sd-bus
                    // mutex) is in progress in a different thread, we get double-mutex deadlock.
                }
            }

            void clear()
            {
                std::unique_lock lock(mutex_);
                auto asyncCallSlots = std::move(calls_);
                calls_ = {};
                lock.unlock();

                // Releasing call slot pointer acquires global sd-bus mutex. We have to perform the release
                // out of the `mutex_' critical section here, because if the `clear` is called by some thread
                // and at the same time Proxy's async reply handler (which already holds global sd-bus
                // mutex) is in progress in a different thread, we get double-mutex deadlock.
            }

        private:
            std::mutex mutex_;
            std::unordered_map<void*, std::shared_ptr<CallData>> calls_;
        } pendingAsyncCalls_;

        std::atomic<const Message*> m_CurrentlyProcessedMessage{nullptr};
    };

}

#endif /* SDBUS_CXX_INTERNAL_PROXY_H_ */
