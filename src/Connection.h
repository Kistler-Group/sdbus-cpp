/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2022 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
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
#include SDBUS_HEADER
#include <memory>
#include <thread>
#include <string>
#include <vector>

struct sd_event_source;

namespace sdbus::internal {

    class Connection final
        : public sdbus::internal::IConnection
    {
    public:
        // Bus type tags
        struct default_bus_t{};
        inline static constexpr default_bus_t default_bus{};
        struct system_bus_t{};
        inline static constexpr system_bus_t system_bus{};
        struct session_bus_t{};
        inline static constexpr session_bus_t session_bus{};
        struct custom_session_bus_t{};
        inline static constexpr custom_session_bus_t custom_session_bus{};
        struct remote_system_bus_t{};
        inline static constexpr remote_system_bus_t remote_system_bus{};
        struct private_bus_t{};
        inline static constexpr private_bus_t private_bus{};
        struct server_bus_t{};
        inline static constexpr server_bus_t server_bus{};
        struct sdbus_bus_t{}; // A bus connection created directly from existing sd_bus instance
        inline static constexpr sdbus_bus_t sdbus_bus{};
        struct pseudo_bus_t{}; // A bus connection that is not really established with D-Bus daemon
        inline static constexpr pseudo_bus_t pseudo_bus{};

        Connection(std::unique_ptr<ISdBus>&& interface, default_bus_t);
        Connection(std::unique_ptr<ISdBus>&& interface, system_bus_t);
        Connection(std::unique_ptr<ISdBus>&& interface, session_bus_t);
        Connection(std::unique_ptr<ISdBus>&& interface, custom_session_bus_t, const std::string& address);
        Connection(std::unique_ptr<ISdBus>&& interface, remote_system_bus_t, const std::string& host);
        Connection(std::unique_ptr<ISdBus>&& interface, private_bus_t, const std::string& address);
        Connection(std::unique_ptr<ISdBus>&& interface, private_bus_t, int fd);
        Connection(std::unique_ptr<ISdBus>&& interface, server_bus_t, int fd);
        Connection(std::unique_ptr<ISdBus>&& interface, sdbus_bus_t, sd_bus *bus);
        Connection(std::unique_ptr<ISdBus>&& interface, pseudo_bus_t);
        ~Connection() override;

        void requestName(const std::string& name) override;
        void releaseName(const std::string& name) override;
        std::string getUniqueName() const override;
        void enterEventLoop() override;
        void enterEventLoopAsync() override;
        void leaveEventLoop() override;
        PollData getEventLoopPollData() const override;
        bool processPendingEvent() override;
        Message getCurrentlyProcessedMessage() const override;

        void addObjectManager(const std::string& objectPath) override;
        void addObjectManager(const std::string& objectPath, floating_slot_t) override;
        Slot addObjectManager(const std::string& objectPath, return_slot_t) override;

        void setMethodCallTimeout(uint64_t timeout) override;
        uint64_t getMethodCallTimeout() const override;

        [[nodiscard]] Slot addMatch(const std::string& match, message_handler callback) override;
        void addMatch(const std::string& match, message_handler callback, floating_slot_t) override;
        [[nodiscard]] Slot addMatchAsync(const std::string& match, message_handler callback, message_handler installCallback) override;
        void addMatchAsync(const std::string& match, message_handler callback, message_handler installCallback, floating_slot_t) override;

        void attachSdEventLoop(sd_event *event, int priority) override;
        void detachSdEventLoop() override;
        sd_event *getSdEventLoop() override;

        const ISdBus& getSdBusInterface() const override;
        ISdBus& getSdBusInterface() override;

        Slot addObjectVTable( const std::string& objectPath
                            , const std::string& interfaceName
                            , const sd_bus_vtable* vtable
                            , void* userData ) override;

        PlainMessage createPlainMessage() const override;
        MethodCall createMethodCall( const std::string& destination
                                   , const std::string& objectPath
                                   , const std::string& interfaceName
                                   , const std::string& methodName ) const override;
        Signal createSignal( const std::string& objectPath
                           , const std::string& interfaceName
                           , const std::string& signalName ) const override;

        MethodReply callMethod(const MethodCall& message, uint64_t timeout) override;
        void callMethod(const MethodCall& message, void* callback, void* userData, uint64_t timeout, floating_slot_t) override;
        Slot callMethod(const MethodCall& message, void* callback, void* userData, uint64_t timeout) override;

        void emitPropertiesChangedSignal( const std::string& objectPath
                                        , const std::string& interfaceName
                                        , const std::vector<std::string>& propNames ) override;
        void emitInterfacesAddedSignal(const std::string& objectPath) override;
        void emitInterfacesAddedSignal( const std::string& objectPath
                                      , const std::vector<std::string>& interfaces ) override;
        void emitInterfacesRemovedSignal(const std::string& objectPath) override;
        void emitInterfacesRemovedSignal( const std::string& objectPath
                                        , const std::vector<std::string>& interfaces ) override;

        Slot registerSignalHandler( const std::string& sender
                                  , const std::string& objectPath
                                  , const std::string& interfaceName
                                  , const std::string& signalName
                                  , sd_bus_message_handler_t callback
                                  , void* userData ) override;

    private:
        using BusFactory = std::function<int(sd_bus**)>;
        using BusPtr = std::unique_ptr<sd_bus, std::function<sd_bus*(sd_bus*)>>;
        Connection(std::unique_ptr<ISdBus>&& interface, const BusFactory& busFactory);

        BusPtr openBus(const std::function<int(sd_bus**)>& busFactory);
        BusPtr openPseudoBus();
        void finishHandshake(sd_bus* bus);
        bool waitForNextEvent();

        bool arePendingMessagesInReadQueue() const;

        void notifyEventLoopToExit();
        void notifyEventLoopToWakeUpFromPoll();
        void wakeUpEventLoopIfMessagesInQueue();
        void joinWithEventLoop();

        static std::vector</*const */char*> to_strv(const std::vector<std::string>& strings);

        static int sdbus_match_callback(sd_bus_message *sdbusMessage, void *userData, sd_bus_error *retError);
        static int sdbus_match_install_callback(sd_bus_message *sdbusMessage, void *userData, sd_bus_error *retError);

    private:
#ifndef SDBUS_basu // sd_event integration is not supported if instead of libsystemd we are based on basu
        Slot createSdEventSlot(sd_event *event);
        Slot createSdTimeEventSourceSlot(sd_event *event, int priority);
        Slot createSdIoEventSourceSlot(sd_event *event, int fd, int priority);
        Slot createSdInternalEventSourceSlot(sd_event *event, int fd, int priority);
        static void deleteSdEventSource(sd_event_source *s);

        static int onSdTimerEvent(sd_event_source *s, uint64_t usec, void *userdata);
        static int onSdIoEvent(sd_event_source *s, int fd, uint32_t revents, void *userdata);
        static int onSdInternalEvent(sd_event_source *s, int fd, uint32_t revents, void *userdata);
        static int onSdEventPrepare(sd_event_source *s, void *userdata);
#endif

        struct EventFd
        {
            EventFd();
            ~EventFd();
            void notify();
            bool clear();

            int fd{-1};
        };

        struct MatchInfo
        {
            message_handler callback;
            message_handler installCallback;
            Connection& connection;
            Slot slot;
        };

        // sd-event integration
        struct SdEvent
        {
            Slot sdEvent;
            Slot sdTimeEventSource;
            Slot sdIoEventSource;
            Slot sdInternalEventSource;
        };

    private:
        std::unique_ptr<ISdBus> sdbus_;
        BusPtr bus_;
        std::thread asyncLoopThread_;
        EventFd loopExitFd_; // To wake up event loop I/O polling to exit
        EventFd eventFd_; // To wake up event loop I/O polling to re-enter poll with fresh PollData values
        std::vector<Slot> floatingMatchRules_;
        std::unique_ptr<SdEvent> sdEvent_; // Integration of systemd sd-event event loop implementation
    };

}

#endif /* SDBUS_CXX_INTERNAL_CONNECTION_H_ */
