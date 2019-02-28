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
#include <mutex>

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

    private:
        static sd_bus* openBus(Connection::BusType type);
        static void finishHandshake(sd_bus* bus);
        static int createProcessingLoopExitDescriptor();
        static void closeProcessingLoopExitDescriptor(int fd);
        bool processPendingRequest();
        bool waitForNextRequest();
        static std::string composeSignalMatchFilter( const std::string& objectPath
                                                   , const std::string& interfaceName
                                                   , const std::string& signalName );
        void notifyProcessingLoopToExit();
        void clearExitNotification();
        void joinWithProcessingLoop();

        // TODO move this and threading logic and method around it to separate class?
        /*
        template <typename _Callable, typename... _Args, std::enable_if_t<std::is_void<function_result_t<_Callable>>::value, int> = 0>
        inline auto tryExecuteSync(_Callable&& fnc, const _Args&... args);
        template <typename _Callable, typename... _Args, std::enable_if_t<!std::is_void<function_result_t<_Callable>>::value, int> = 0>
        inline auto tryExecuteSync(_Callable&& fnc, const _Args&... args);
        template <typename _Callable, typename... _Args, std::enable_if_t<std::is_void<function_result_t<_Callable>>::value, int> = 0>
        inline void executeAsync(_Callable&& fnc, const _Args&... args);
        template <typename _Callable, typename... _Args, std::enable_if_t<!std::is_void<function_result_t<_Callable>>::value, int> = 0>
        inline auto executeAsync(_Callable&& fnc, const _Args&... args);
        template <typename _Callable, typename... _Args>
        inline void executeAsyncAndDontWaitForResult(_Callable&& fnc, const _Args&... args);
        */

    private:
        std::unique_ptr<sd_bus, decltype(&sd_bus_flush_close_unref)> bus_{nullptr, &sd_bus_flush_close_unref};
        BusType busType_;
        std::recursive_mutex busMutex_;

        std::thread asyncLoopThread_;
        int loopExitFd_{-1};
    };

}}

#endif /* SDBUS_CXX_INTERNAL_CONNECTION_H_ */
