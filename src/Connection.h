/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2026 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
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

#include "sdbus-c++/IConnection.h"

#include "sdbus-c++/Message.h"

#include "IConnection.h"
#include "ISdBus.h"

#include <memory>
#include <string>
#include SDBUS_HEADER
#include <thread>
#include <vector>

// Forward declarations
struct sd_event_source;
namespace sdbus {
    class ObjectPath;
    class InterfaceName;
    class BusName;
    using ServiceName = BusName;
    class MemberName;
    using MethodName = MemberName;
    using SignalName = MemberName;
    using PropertyName = MemberName;
} // namespace sdbus

namespace sdbus::internal {

    class Connection final
        : public IConnection
    {
    public:
        // Bus type tags
        struct default_bus_t{};
        static constexpr default_bus_t default_bus{};
        struct system_bus_t{};
        static constexpr system_bus_t system_bus{};
        struct session_bus_t{};
        static constexpr session_bus_t session_bus{};
        struct custom_session_bus_t{};
        static constexpr custom_session_bus_t custom_session_bus{};
        struct remote_system_bus_t{};
        static constexpr remote_system_bus_t remote_system_bus{};
        struct private_bus_t{};
        static constexpr private_bus_t private_bus{};
        struct server_bus_t{};
        static constexpr server_bus_t server_bus{};
        struct sdbus_bus_t{}; // A bus connection created directly from existing sd_bus instance
        static constexpr sdbus_bus_t sdbus_bus{};
        struct pseudo_bus_t{}; // A bus connection that is not really established with D-Bus daemon
        static constexpr pseudo_bus_t pseudo_bus{};

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
        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;
        Connection(Connection&&) = delete;
        Connection& operator=(Connection&&) = delete;
        ~Connection() override;

        void requestName(const ServiceName & name) override;
        void releaseName(const ServiceName& name) override;
        [[nodiscard]] BusName getUniqueName() const override;
        void enterEventLoop() override;
        void enterEventLoopAsync() override;
        void leaveEventLoop() override;
        [[nodiscard]] PollData getEventLoopPollData() const override;
        bool processPendingEvent() override;
        Message getCurrentlyProcessedMessage() const override;

        void addObjectManager(const ObjectPath& objectPath) override;
        Slot addObjectManager(const ObjectPath& objectPath, return_slot_t) override;

        void setMethodCallTimeout(uint64_t timeout) override;
        [[nodiscard]] uint64_t getMethodCallTimeout() const override;

        void addMatch(const std::string& match, message_handler callback) override;
        [[nodiscard]] Slot addMatch(const std::string& match, message_handler callback, return_slot_t) override;
        void addMatchAsync(const std::string& match, message_handler callback, message_handler installCallback) override;
        [[nodiscard]] Slot addMatchAsync( const std::string& match
                                        , message_handler callback
                                        , message_handler installCallback
                                        , return_slot_t ) override;

        void attachSdEventLoop(sd_event *event, int priority) override;
        void detachSdEventLoop() override;
        sd_event *getSdEventLoop() override;

        Slot addObjectVTable( const ObjectPath& objectPath
                            , const InterfaceName& interfaceName
                            , const sd_bus_vtable* vtable
                            , void* userData
                            , return_slot_t ) override;

        [[nodiscard]] PlainMessage createPlainMessage() const override;
        [[nodiscard]] MethodCall createMethodCall( const ServiceName& destination
                                                 , const ObjectPath& objectPath
                                                 , const InterfaceName& interfaceName
                                                 , const MethodName& methodName ) const override;
        [[nodiscard]] MethodCall createMethodCall( const char* destination
                                                 , const char* objectPath
                                                 , const char* interfaceName
                                                 , const char* methodName ) const override;
        [[nodiscard]] Signal createSignal( const ObjectPath& objectPath
                                         , const InterfaceName& interfaceName
                                         , const SignalName& signalName ) const override;
        [[nodiscard]] Signal createSignal( const char* objectPath
                                         , const char* interfaceName
                                         , const char* signalName ) const override;

        void emitPropertiesChangedSignal( const ObjectPath& objectPath
                                        , const InterfaceName& interfaceName
                                        , const std::vector<PropertyName>& propNames ) override;
        void emitPropertiesChangedSignal( const char* objectPath
                                        , const char* interfaceName
                                        , const std::vector<PropertyName>& propNames ) override;
        void emitInterfacesAddedSignal(const ObjectPath& objectPath) override;
        void emitInterfacesAddedSignal( const ObjectPath& objectPath
                                      , const std::vector<InterfaceName>& interfaces ) override;
        void emitInterfacesRemovedSignal(const ObjectPath& objectPath) override;
        void emitInterfacesRemovedSignal( const ObjectPath& objectPath
                                        , const std::vector<InterfaceName>& interfaces ) override;

        Slot registerSignalHandler( const char* sender
                                  , const char* objectPath
                                  , const char* interfaceName
                                  , const char* signalName
                                  , sd_bus_message_handler_t callback
                                  , void* userData
                                  , return_slot_t ) override;

        sd_bus_message* incrementMessageRefCount(sd_bus_message* sdbusMsg) override;
        sd_bus_message* decrementMessageRefCount(sd_bus_message* sdbusMsg) override;

        int querySenderCredentials(sd_bus_message* sdbusMsg, uint64_t mask, sd_bus_creds **creds) override;
        sd_bus_creds* incrementCredsRefCount(sd_bus_creds* creds) override;
        sd_bus_creds* decrementCredsRefCount(sd_bus_creds* creds) override;

        sd_bus_message* callMethod(sd_bus_message* sdbusMsg, uint64_t timeout) override;
        Slot callMethodAsync(sd_bus_message* sdbusMsg, sd_bus_message_handler_t callback, void* userData, uint64_t timeout, return_slot_t) override;
        void sendMessage(sd_bus_message* sdbusMsg) override;

        sd_bus_message* createMethodReply(sd_bus_message* sdbusMsg) override;
        sd_bus_message* createErrorReplyMessage(sd_bus_message* sdbusMsg, const Error& error) override;

    private:
        using BusFactory = std::function<int(sd_bus**)>;
        using BusPtr = std::unique_ptr<sd_bus, std::function<sd_bus*(sd_bus*)>>;
        Connection(std::unique_ptr<ISdBus>&& interface, const BusFactory& busFactory);

        BusPtr openBus(const std::function<int(sd_bus**)>& busFactory);
        BusPtr openPseudoBus();
        void finishHandshake(sd_bus* bus);
        bool waitForNextEvent();

        [[nodiscard]] bool arePendingMessagesInQueues() const;

        void notifyEventLoopToExit();
        void notifyEventLoopToWakeUpFromPoll();
        void wakeUpEventLoopIfMessagesInQueue();
        void joinWithEventLoop();

        template <typename StringBasedType>
        static std::vector</*const */char*> to_strv(const std::vector<StringBasedType>& strings);

        static int sdbus_match_callback(sd_bus_message *sdbusMessage, void *userData, sd_bus_error *retError);
        static int sdbus_match_install_callback(sd_bus_message *sdbusMessage, void *userData, sd_bus_error *retError);

    
#ifndef SDBUS_basu // sd_event integration is not supported if instead of libsystemd we are based on basu
        static Slot createSdEventSlot(sd_event *event);
        Slot createSdTimeEventSourceSlot(sd_event *event, int priority);
        Slot createSdIoEventSourceSlot(sd_event *event, int fd, int priority);
        Slot createSdInternalEventSourceSlot(sd_event *event, int fd, int priority);
        static void deleteSdEventSource(sd_event_source *source);

        static int onSdTimerEvent(sd_event_source *source, uint64_t usec, void *userdata);
        static int onSdIoEvent(sd_event_source *source, int fd, uint32_t revents, void *userdata);
        static int onSdInternalEvent(sd_event_source *source, int fd, uint32_t revents, void *userdata);
        static int onSdEventPrepare(sd_event_source *source, void *userdata);
#endif

        struct EventFd
        {
            EventFd();
            EventFd(const EventFd&) = delete;
            EventFd& operator=(const EventFd&) = delete;
            EventFd(EventFd&&) = delete;
            EventFd& operator=(EventFd&&) = delete;
            ~EventFd();
            void notify();
            bool clear();

            int fd{-1};
        };

        struct MatchInfo
        {
            message_handler callback;
            message_handler installCallback;
            Connection& connection; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
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

        std::unique_ptr<ISdBus> sdbus_;
        BusPtr bus_;
        std::thread asyncLoopThread_;
        EventFd loopExitFd_; // To wake up event loop I/O polling to exit
        EventFd eventFd_; // To wake up event loop I/O polling to re-enter poll with fresh PollData values
        std::vector<Slot> floatingMatchRules_;
        std::unique_ptr<SdEvent> sdEvent_; // Integration of systemd sd-event event loop implementation
    };

} // namespace sdbus::internal

#endif /* SDBUS_CXX_INTERNAL_CONNECTION_H_ */
