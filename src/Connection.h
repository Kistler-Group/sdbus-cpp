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
#include "ScopeGuard.h"
#include <systemd/sd-bus.h>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <future>
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

        Connection(BusType type);
        ~Connection() override;

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

        MethodCall createMethodCall( const std::string& destination
                                   , const std::string& objectPath
                                   , const std::string& interfaceName
                                   , const std::string& methodName ) const override;
        Signal createSignal( const std::string& objectPath
                           , const std::string& interfaceName
                           , const std::string& signalName ) const override;

        void* registerSignalHandler( const std::string& objectPath
                                   , const std::string& interfaceName
                                   , const std::string& signalName
                                   , sd_bus_message_handler_t callback
                                   , void* userData ) override;
        void unregisterSignalHandler(void* handlerCookie) override;

        MethodReply callMethod(const MethodCall& message) override;
        void callMethod(const AsyncMethodCall& message, void* callback, void* userData) override;
        void sendMethodReply(const MethodReply& message) override;
        void emitSignal(const Signal& message) override;

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

        // TODO move down
        struct IUserRequest
        {
            virtual void process() = 0;
            virtual ~IUserRequest() = default;
        };

        struct MethodCallRequest : IUserRequest
        {
            MethodCall msg;
            std::promise<MethodReply> result;

            void process() override
            {
                SCOPE_EXIT_NAMED(onSdbusError){ result.set_exception(std::current_exception()); };

                auto reply = msg.send();
                result.set_value(std::move(reply));

                onSdbusError.dismiss();
            }
        };

        struct AsyncMethodCallRequest : IUserRequest
        {
            AsyncMethodCall msg;
            void* callback;
            void* userData;

            // TODO: Catch exception and store to promise?
            void process() override
            {
                msg.send(callback, userData);
            }
        };

        struct MethodReplyRequest : IUserRequest
        {
            MethodReply msg;

            // TODO: Catch exception and store to promise?
            void process() override
            {
                msg.send();
            }
        };

        struct SignalEmissionRequest : IUserRequest
        {
            Signal msg;

            // TODO: Catch exception and store to promise?
            void process() override
            {
                msg.send();
            }
        };

        struct SignalRegistrationRequest : IUserRequest
        {
            std::function<void*()> registerSignalHandler;
            std::promise<void*> result;

            void process() override
            {
                SCOPE_EXIT_NAMED(onSdbusError){ result.set_exception(std::current_exception()); };

                assert(registerSignalHandler);
                void* slot = registerSignalHandler();
                result.set_value(slot);

                onSdbusError.dismiss();
            }
        };

        struct SignalUnregistrationRequest : IUserRequest
        {
            std::function<void()> unregisterSignalHandler;
            std::promise<void> result;

            void process() override
            {
                SCOPE_EXIT_NAMED(onSdbusError){ result.set_exception(std::current_exception()); };

                assert(unregisterSignalHandler);
                unregisterSignalHandler();
                result.set_value();

                onSdbusError.dismiss();
            }
        };

        static sd_bus* openBus(Connection::BusType type);
        static void finishHandshake(sd_bus* bus);
        static int createLoopNotificationDescriptor();
        static void closeLoopNotificationDescriptor(int fd);
        bool processPendingRequest();
        void queueUserRequest(std::unique_ptr<IUserRequest>&& request);
        void processUserRequests();
        WaitResult waitForNextRequest();
        static std::string composeSignalMatchFilter( const std::string& objectPath
                                                   , const std::string& interfaceName
                                                   , const std::string& signalName );
        void notifyProcessingLoop();
        void notifyProcessingLoopToExit();
        void joinWithProcessingLoop();

    private:
        std::unique_ptr<sd_bus, decltype(&sd_bus_flush_close_unref)> bus_{nullptr, &sd_bus_flush_close_unref};
        std::thread asyncLoopThread_;
        std::atomic<std::thread::id> loopThreadId_;
        std::mutex loopMutex_;

        std::queue<std::unique_ptr<IUserRequest>> userRequests_;
        std::mutex userRequestsMutex_;

        std::atomic<bool> exitLoopThread_;
        int notificationFd_{-1};
        BusType busType_;

        static constexpr const uint64_t POLL_TIMEOUT_USEC = 500000;
    };

}}

#endif /* SDBUS_CXX_INTERNAL_CONNECTION_H_ */
