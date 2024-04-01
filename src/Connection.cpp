/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2022 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file Connection.cpp
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

#include "Connection.h"

#include "sdbus-c++/Error.h"
#include "sdbus-c++/Message.h"
#include "sdbus-c++/Types.h"

#include "MessageUtils.h"
#include "ScopeGuard.h"
#include "SdBus.h"
#include "Utils.h"

#include <poll.h>
#include <sys/eventfd.h>
#include SDBUS_HEADER
#ifndef SDBUS_basu // sd_event integration is not supported in basu-based sdbus-c++
#include <systemd/sd-event.h>
#endif
#include <unistd.h>

namespace sdbus::internal {

Connection::Connection(std::unique_ptr<ISdBus>&& interface, const BusFactory& busFactory)
    : sdbus_(std::move(interface))
    , bus_(openBus(busFactory))
{
    assert(sdbus_ != nullptr);
}

Connection::Connection(std::unique_ptr<ISdBus>&& interface, default_bus_t)
    : Connection(std::move(interface), [this](sd_bus** bus){ return sdbus_->sd_bus_open(bus); })
{
}

Connection::Connection(std::unique_ptr<ISdBus>&& interface, system_bus_t)
    : Connection(std::move(interface), [this](sd_bus** bus){ return sdbus_->sd_bus_open_system(bus); })
{
}

Connection::Connection(std::unique_ptr<ISdBus>&& interface, session_bus_t)
    : Connection(std::move(interface), [this](sd_bus** bus){ return sdbus_->sd_bus_open_user(bus); })
{
}

Connection::Connection(std::unique_ptr<ISdBus>&& interface, custom_session_bus_t, const std::string& address)
        : Connection(std::move(interface), [&](sd_bus** bus) { return sdbus_->sd_bus_open_user_with_address(bus, address.c_str()); })
{
}

Connection::Connection(std::unique_ptr<ISdBus>&& interface, remote_system_bus_t, const std::string& host)
    : Connection(std::move(interface), [this, &host](sd_bus** bus){ return sdbus_->sd_bus_open_system_remote(bus, host.c_str()); })
{
}

Connection::Connection(std::unique_ptr<ISdBus>&& interface, private_bus_t, const std::string& address)
    : Connection(std::move(interface), [&](sd_bus** bus) { return sdbus_->sd_bus_open_direct(bus, address.c_str()); })
{
}

Connection::Connection(std::unique_ptr<ISdBus>&& interface, private_bus_t, int fd)
        : Connection(std::move(interface), [&](sd_bus** bus) { return sdbus_->sd_bus_open_direct(bus, fd); })
{
}

Connection::Connection(std::unique_ptr<ISdBus>&& interface, server_bus_t, int fd)
    : Connection(std::move(interface), [&](sd_bus** bus) { return sdbus_->sd_bus_open_server(bus, fd); })
{
}

Connection::Connection(std::unique_ptr<ISdBus>&& interface, sdbus_bus_t, sd_bus *bus)
        : Connection(std::move(interface), [&](sd_bus** b) { *b = bus; return 0; })
{
}

Connection::Connection(std::unique_ptr<ISdBus>&& interface, pseudo_bus_t)
    : sdbus_(std::move(interface))
    , bus_(openPseudoBus())
{
    assert(sdbus_ != nullptr);
}

Connection::~Connection()
{
    Connection::leaveEventLoop();
}

void Connection::requestName(const ServiceName& name)
{
    SDBUS_CHECK_SERVICE_NAME(name);

    auto r = sdbus_->sd_bus_request_name(bus_.get(), name.c_str(), 0);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to request bus name", -r);

    // In some cases we need to explicitly notify the event loop
    // to process messages that may have arrived while executing the call
    wakeUpEventLoopIfMessagesInQueue();
}

void Connection::releaseName(const ServiceName& name)
{
    auto r = sdbus_->sd_bus_release_name(bus_.get(), name.c_str());
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to release bus name", -r);

    // In some cases we need to explicitly notify the event loop
    // to process messages that may have arrived while executing the call
    wakeUpEventLoopIfMessagesInQueue();
}

BusName Connection::getUniqueName() const
{
    const char* name{};
    auto r = sdbus_->sd_bus_get_unique_name(bus_.get(), &name);
    SDBUS_THROW_ERROR_IF(r < 0 || name == nullptr, "Failed to get unique bus name", -r);
    return BusName{name};
}

void Connection::enterEventLoop()
{
    while (true)
    {
        // Process one pending event
        (void)processPendingEvent();

        // And go to poll(), which wakes us up right away
        // if there's another pending event, or sleeps otherwise.
        auto success = waitForNextEvent();
        if (!success)
            break; // Exit I/O event loop
    }
}

void Connection::enterEventLoopAsync()
{
    if (!asyncLoopThread_.joinable())
        asyncLoopThread_ = std::thread([this](){ enterEventLoop(); });
}

void Connection::leaveEventLoop()
{
    notifyEventLoopToExit();
    joinWithEventLoop();
}

Connection::PollData Connection::getEventLoopPollData() const
{
    ISdBus::PollData pollData{};
    auto r = sdbus_->sd_bus_get_poll_data(bus_.get(), &pollData);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus poll data", -r);

    assert(eventFd_.fd >= 0);

    auto timeout = pollData.timeout_usec == UINT64_MAX ? std::chrono::microseconds::max() : std::chrono::microseconds(pollData.timeout_usec);

    return {pollData.fd, pollData.events, timeout, eventFd_.fd};
}

const ISdBus& Connection::getSdBusInterface() const
{
    return *sdbus_.get();
}

ISdBus& Connection::getSdBusInterface()
{
    return *sdbus_.get();
}

void Connection::addObjectManager(const ObjectPath& objectPath, floating_slot_t)
{
    auto r = sdbus_->sd_bus_add_object_manager(bus_.get(), nullptr, objectPath.c_str());

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to add object manager", -r);
}

Slot Connection::addObjectManager(const ObjectPath& objectPath, return_slot_t)
{
    sd_bus_slot *slot{};

    auto r = sdbus_->sd_bus_add_object_manager(bus_.get(), &slot, objectPath.c_str());

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to add object manager", -r);

    return {slot, [this](void *slot){ sdbus_->sd_bus_slot_unref((sd_bus_slot*)slot); }};
}

void Connection::setMethodCallTimeout(uint64_t timeout)
{
    auto r = sdbus_->sd_bus_set_method_call_timeout(bus_.get(), timeout);

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to set method call timeout", -r);
}

uint64_t Connection::getMethodCallTimeout() const
{
    uint64_t timeout;

    auto r = sdbus_->sd_bus_get_method_call_timeout(bus_.get(), &timeout);

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get method call timeout", -r);

    return timeout;
}

Slot Connection::addMatch(const std::string& match, message_handler callback)
{
    SDBUS_THROW_ERROR_IF(!callback, "Invalid match callback handler provided", EINVAL);

    auto matchInfo = std::make_unique<MatchInfo>(MatchInfo{std::move(callback), {}, *this, {}});

    sd_bus_slot *slot{};
    auto r = sdbus_->sd_bus_add_match(bus_.get(), &slot, match.c_str(), &Connection::sdbus_match_callback, matchInfo.get());
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to add match", -r);

    matchInfo->slot = {slot, [this](void *slot){ sdbus_->sd_bus_slot_unref((sd_bus_slot*)slot); }};

    return {matchInfo.release(), [](void *ptr){ delete static_cast<MatchInfo*>(ptr); }};
}

void Connection::addMatch(const std::string& match, message_handler callback, floating_slot_t)
{
    floatingMatchRules_.push_back(addMatch(match, std::move(callback)));
}

Slot Connection::addMatchAsync(const std::string& match, message_handler callback, message_handler installCallback)
{
    SDBUS_THROW_ERROR_IF(!callback, "Invalid match callback handler provided", EINVAL);

    sd_bus_message_handler_t sdbusInstallCallback = installCallback ? &Connection::sdbus_match_install_callback : nullptr;
    auto matchInfo = std::make_unique<MatchInfo>(MatchInfo{std::move(callback), std::move(installCallback), *this, {}});

    sd_bus_slot *slot{};
    auto r = sdbus_->sd_bus_add_match_async( bus_.get()
                                           , &slot
                                           , match.c_str()
                                           , &Connection::sdbus_match_callback
                                           , sdbusInstallCallback
                                           , matchInfo.get());
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to add match", -r);

    matchInfo->slot = {slot, [this](void *slot){ sdbus_->sd_bus_slot_unref((sd_bus_slot*)slot); }};

    return {matchInfo.release(), [](void *ptr){ delete static_cast<MatchInfo*>(ptr); }};
}

void Connection::addMatchAsync(const std::string& match, message_handler callback, message_handler installCallback, floating_slot_t)
{
    floatingMatchRules_.push_back(addMatchAsync(match, std::move(callback), std::move(installCallback)));
}

void Connection::attachSdEventLoop(sd_event *event, int priority)
{
#ifndef SDBUS_basu
    auto pollData = getEventLoopPollData();

    auto sdEvent = createSdEventSlot(event);
    auto sdTimeEventSource = createSdTimeEventSourceSlot(event, priority);
    auto sdIoEventSource = createSdIoEventSourceSlot(event, pollData.fd, priority);
    auto sdInternalEventSource = createSdInternalEventSourceSlot(event, pollData.eventFd, priority);

    sdEvent_ = std::make_unique<SdEvent>(SdEvent{ std::move(sdEvent)
                                                , std::move(sdTimeEventSource)
                                                , std::move(sdIoEventSource)
                                                , std::move(sdInternalEventSource) });
#else
    (void)event;
    (void)priority;
    SDBUS_THROW_ERROR("sd_event integration is not supported on this platform", EOPNOTSUPP);
#endif
}

void Connection::detachSdEventLoop()
{
    sdEvent_.reset();
}

sd_event *Connection::getSdEventLoop()
{
    return sdEvent_ ? static_cast<sd_event*>(sdEvent_->sdEvent.get()) : nullptr;
}

#ifndef SDBUS_basu

Slot Connection::createSdEventSlot(sd_event *event)
{
    // Get default event if no event is provided by the caller
    if (event != nullptr)
        event = sd_event_ref(event);
    else
        (void)sd_event_default(&event);
    SDBUS_THROW_ERROR_IF(!event, "Invalid sd_event handle", EINVAL);

    return Slot{event, [](void* event){ sd_event_unref((sd_event*)event); }};
}

Slot Connection::createSdTimeEventSourceSlot(sd_event *event, int priority)
{
    sd_event_source *timeEventSource{};
    auto r = sd_event_add_time(event, &timeEventSource, CLOCK_MONOTONIC, 0, 0, onSdTimerEvent, this);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to add timer event", -r);
    Slot sdTimeEventSource{timeEventSource, [](void* source){ deleteSdEventSource((sd_event_source*)source); }};

    r = sd_event_source_set_priority(timeEventSource, priority);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to set time event priority", -r);

    r = sd_event_source_set_description(timeEventSource, "bus-time");
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to set time event description", -r);

    return sdTimeEventSource;
}

Slot Connection::createSdIoEventSourceSlot(sd_event *event, int fd, int priority)
{
    sd_event_source *ioEventSource{};
    auto r = sd_event_add_io(event, &ioEventSource, fd, 0, onSdIoEvent, this);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to add io event", -r);
    Slot sdIoEventSource{ioEventSource, [](void* source){ deleteSdEventSource((sd_event_source*)source); }};

    r = sd_event_source_set_prepare(ioEventSource, onSdEventPrepare);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to set prepare callback for IO event", -r);

    r = sd_event_source_set_priority(ioEventSource, priority);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to set priority for IO event", -r);

    r = sd_event_source_set_description(ioEventSource, "bus-input");
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to set priority for IO event", -r);

    return sdIoEventSource;
}

Slot Connection::createSdInternalEventSourceSlot(sd_event *event, int fd, int priority)
{
    sd_event_source *internalEventSource{};
    auto r = sd_event_add_io(event, &internalEventSource, fd, 0, onSdInternalEvent, this);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to add internal event", -r);
    Slot sdInternalEventSource{internalEventSource, [](void* source){ deleteSdEventSource((sd_event_source*)source); }};

    // sd-event loop calls prepare callbacks for all event sources, not just for the one that fired now.
    // So since onSdEventPrepare is already registered on ioEventSource, we don't need to duplicate it here.
    //r = sd_event_source_set_prepare(internalEventSource, onSdEventPrepare);
    //SDBUS_THROW_ERROR_IF(r < 0, "Failed to set prepare callback for internal event", -r);

    r = sd_event_source_set_priority(internalEventSource, priority);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to set priority for internal event", -r);

    r = sd_event_source_set_description(internalEventSource, "internal-event");
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to set priority for IO event", -r);

    return sdInternalEventSource;
}

int Connection::onSdTimerEvent(sd_event_source */*s*/, uint64_t /*usec*/, void *userdata)
{
    auto connection = static_cast<Connection*>(userdata);
    assert(connection != nullptr);

    (void)connection->processPendingEvent();

    return 1;
}

int Connection::onSdIoEvent(sd_event_source */*s*/, int /*fd*/, uint32_t /*revents*/, void *userdata)
{
    auto connection = static_cast<Connection*>(userdata);
    assert(connection != nullptr);

    (void)connection->processPendingEvent();

    return 1;
}

int Connection::onSdInternalEvent(sd_event_source */*s*/, int /*fd*/, uint32_t /*revents*/, void *userdata)
{
    auto connection = static_cast<Connection*>(userdata);
    assert(connection != nullptr);

    // It's not really necessary to processPendingEvent() here. We just clear the event fd.
    // The sd-event loop will before the next poll call prepare callbacks for all event sources,
    // including I/O bus fd. This will get up-to-date poll timeout, which will be zero if there
    // are pending D-Bus messages in the read queue, which will immediately wake up next poll
    // and go to onSdIoEvent() handler, which calls processPendingEvent(). Viola.
    // For external event loops that only have access to public sdbus-c++ API, processPendingEvent()
    // is the only option to clear event fd (it comes at a little extra cost but on the other hand
    // the solution is simpler for clients -- we don't provide an extra method for just clearing
    // the event fd. There is one method for both fd's -- and that's processPendingEvent().

    // Kept here so that potential readers know what to do in their custom external event loops.
    //(void)connection->processPendingEvent();

    connection->eventFd_.clear();

    return 1;
}

int Connection::onSdEventPrepare(sd_event_source */*s*/, void *userdata)
{
    auto connection = static_cast<Connection*>(userdata);
    assert(connection != nullptr);

    auto sdbusPollData = connection->getEventLoopPollData();

    // Set poll events to watch out for on I/O fd
    auto* sdIoEventSource = static_cast<sd_event_source*>(connection->sdEvent_->sdIoEventSource.get());
    auto r = sd_event_source_set_io_events(sdIoEventSource, sdbusPollData.events);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to set poll events for IO event source", -r);

    // Set poll events to watch out for on internal event fd
    auto* sdInternalEventSource = static_cast<sd_event_source*>(connection->sdEvent_->sdInternalEventSource.get());
    r = sd_event_source_set_io_events(sdInternalEventSource, POLLIN);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to set poll events for internal event source", -r);

    // Set current timeout to the time event source (it may be zero if there are messages in the sd-bus queues to be processed)
    auto* sdTimeEventSource = static_cast<sd_event_source*>(connection->sdEvent_->sdTimeEventSource.get());
    r = sd_event_source_set_time(sdTimeEventSource, static_cast<uint64_t>(sdbusPollData.timeout.count()));
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to set timeout for time event source", -r);
    // In case the timeout is infinite, we disable the timer in the sd_event loop.
    // This prevents a syscall error, where `timerfd_settime` returns `EINVAL`,
    // because the value is too big. See #324 for details
    r = sd_event_source_set_enabled(sdTimeEventSource, sdbusPollData.timeout != sdbusPollData.timeout.max() ? SD_EVENT_ONESHOT : SD_EVENT_OFF);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to enable time event source", -r);

    return 1;
}

void Connection::deleteSdEventSource(sd_event_source *s)
{
#if LIBSYSTEMD_VERSION>=243
    sd_event_source_disable_unref(s);
#else
    sd_event_source_set_enabled(s, SD_EVENT_OFF);
    sd_event_source_unref(s);
#endif
}

#endif // SDBUS_basu

Slot Connection::addObjectVTable( const ObjectPath& objectPath
                                , const InterfaceName& interfaceName
                                , const sd_bus_vtable* vtable
                                , void* userData )
{
    sd_bus_slot *slot{};

    auto r = sdbus_->sd_bus_add_object_vtable( bus_.get()
                                             , &slot
                                             , objectPath.c_str()
                                             , interfaceName.c_str()
                                             , vtable
                                             , userData );

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to register object vtable", -r);

    return {slot, [this](void *slot){ sdbus_->sd_bus_slot_unref((sd_bus_slot*)slot); }};
}

PlainMessage Connection::createPlainMessage() const
{
    sd_bus_message* sdbusMsg{};

    auto r = sdbus_->sd_bus_message_new(bus_.get(), &sdbusMsg, _SD_BUS_MESSAGE_TYPE_INVALID);

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to create a plain message", -r);

    return Message::Factory::create<PlainMessage>(sdbusMsg, sdbus_.get(), adopt_message);
}

MethodCall Connection::createMethodCall( const ServiceName& destination
                                       , const ObjectPath& objectPath
                                       , const InterfaceName& interfaceName
                                       , const MethodName& methodName ) const
{
    sd_bus_message *sdbusMsg{};

    auto r = sdbus_->sd_bus_message_new_method_call( bus_.get()
                                                   , &sdbusMsg
                                                   , destination.empty() ? nullptr : destination.c_str()
                                                   , objectPath.c_str()
                                                   , interfaceName.c_str()
                                                   , methodName.c_str() );

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to create method call", -r);

    return Message::Factory::create<MethodCall>(sdbusMsg, sdbus_.get(), adopt_message);
}

Signal Connection::createSignal( const ObjectPath& objectPath
                               , const InterfaceName& interfaceName
                               , const SignalName& signalName ) const
{
    sd_bus_message *sdbusMsg{};

    auto r = sdbus_->sd_bus_message_new_signal( bus_.get()
                                              , &sdbusMsg
                                              , objectPath.c_str()
                                              , interfaceName.c_str()
                                              , signalName.c_str() );

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to create signal", -r);

    return Message::Factory::create<Signal>(sdbusMsg, sdbus_.get(), adopt_message);
}

MethodReply Connection::callMethod(const MethodCall& message, uint64_t timeout)
{
    // If the call expects reply, this call will block the bus connection from
    // serving other messages until the reply arrives or the call times out.
    auto reply = message.send(timeout);

    // Wake up event loop to process messages that may have arrived in the meantime...
    wakeUpEventLoopIfMessagesInQueue();

    return reply;
}

void Connection::callMethod(const MethodCall& message, void* callback, void* userData, uint64_t timeout, floating_slot_t)
{
    // TODO: Think of ways of optimizing these three locking/unlocking of sdbus mutex (merge into one call?)
    auto timeoutBefore = getEventLoopPollData().timeout;
    message.send(callback, userData, timeout, floating_slot);
    auto timeoutAfter = getEventLoopPollData().timeout;

    // An event loop may wait in poll with timeout `t1', while in another thread an async call is made with
    // timeout `t2'. If `t2' < `t1', then we have to wake up the event loop thread to update its poll timeout.
    if (timeoutAfter < timeoutBefore)
        notifyEventLoopToWakeUpFromPoll();
}

Slot Connection::callMethod(const MethodCall& message, void* callback, void* userData, uint64_t timeout)
{
    // TODO: Think of ways of optimizing these three locking/unlocking of sdbus mutex (merge into one call?)
    auto timeoutBefore = getEventLoopPollData().timeout;
    auto slot = message.send(callback, userData, timeout);
    auto timeoutAfter = getEventLoopPollData().timeout;

    // An event loop may wait in poll with timeout `t1', while in another thread an async call is made with
    // timeout `t2'. If `t2' < `t1', then we have to wake up the event loop thread to update its poll timeout.
    if (timeoutAfter < timeoutBefore)
        notifyEventLoopToWakeUpFromPoll();

    return slot;
}

void Connection::emitPropertiesChangedSignal( const ObjectPath& objectPath
                                            , const InterfaceName& interfaceName
                                            , const std::vector<PropertyName>& propNames )
{
    auto names = to_strv(propNames);

    auto r = sdbus_->sd_bus_emit_properties_changed_strv( bus_.get()
                                                        , objectPath.c_str()
                                                        , interfaceName.c_str()
                                                        , propNames.empty() ? nullptr : &names[0] );

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to emit PropertiesChanged signal", -r);
}

void Connection::emitInterfacesAddedSignal(const ObjectPath& objectPath)
{
    auto r = sdbus_->sd_bus_emit_object_added(bus_.get(), objectPath.c_str());

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to emit InterfacesAdded signal for all registered interfaces", -r);
}

void Connection::emitInterfacesAddedSignal( const ObjectPath& objectPath
                                          , const std::vector<InterfaceName>& interfaces )
{
    auto names = to_strv(interfaces);

    auto r = sdbus_->sd_bus_emit_interfaces_added_strv( bus_.get()
                                                      , objectPath.c_str()
                                                      , interfaces.empty() ? nullptr : &names[0] );

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to emit InterfacesAdded signal", -r);
}

void Connection::emitInterfacesRemovedSignal(const ObjectPath& objectPath)
{
    auto r = sdbus_->sd_bus_emit_object_removed(bus_.get(), objectPath.c_str());

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to emit InterfacesRemoved signal for all registered interfaces", -r);
}

void Connection::emitInterfacesRemovedSignal( const ObjectPath& objectPath
                                            , const std::vector<InterfaceName>& interfaces )
{
    auto names = to_strv(interfaces);

    auto r = sdbus_->sd_bus_emit_interfaces_removed_strv( bus_.get()
                                                        , objectPath.c_str()
                                                        , interfaces.empty() ? nullptr : &names[0] );

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to emit InterfacesRemoved signal", -r);
}

Slot Connection::registerSignalHandler( const ServiceName& sender
                                      , const ObjectPath& objectPath
                                      , const InterfaceName& interfaceName
                                      , const SignalName& signalName
                                      , sd_bus_message_handler_t callback
                                      , void* userData )
{
    sd_bus_slot *slot{};

    auto r = sdbus_->sd_bus_match_signal( bus_.get()
                                        , &slot
                                        , !sender.empty() ? sender.c_str() : nullptr
                                        , !objectPath.empty() ? objectPath.c_str() : nullptr
                                        , !interfaceName.empty() ? interfaceName.c_str() : nullptr
                                        , !signalName.empty() ? signalName.c_str() : nullptr
                                        , callback
                                        , userData );

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to register signal handler", -r);

    return {slot, [this](void *slot){ sdbus_->sd_bus_slot_unref((sd_bus_slot*)slot); }};
}

Connection::BusPtr Connection::openBus(const BusFactory& busFactory)
{
    sd_bus* bus{};
    int r = busFactory(&bus);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to open bus", -r);

    BusPtr busPtr{bus, [this](sd_bus* bus){ return sdbus_->sd_bus_flush_close_unref(bus); }};
    finishHandshake(busPtr.get());
    return busPtr;
}

Connection::BusPtr Connection::openPseudoBus()
{
    sd_bus* bus{};

    int r = sdbus_->sd_bus_new(&bus);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to open pseudo bus", -r);

    (void)sdbus_->sd_bus_start(bus);
    // It is expected that sd_bus_start has failed here, returning -EINVAL, due to having
    // not set a bus address, but it will leave the bus in an OPENING state, which enables
    // us to create plain D-Bus messages as a local data storage (for Variant, for example),
    // without dependency on real IPC communication with the D-Bus broker daemon.
    SDBUS_THROW_ERROR_IF(r < 0 && r != -EINVAL, "Failed to start pseudo bus", -r);

    return {bus, [this](sd_bus* bus){ return sdbus_->sd_bus_close_unref(bus); }};
}

void Connection::finishHandshake(sd_bus* bus)
{
    // Process all requests that are part of the initial handshake,
    // like processing the Hello message response, authentication etc.,
    // to avoid connection authentication timeout in dbus daemon.

    assert(bus != nullptr);

    auto r = sdbus_->sd_bus_flush(bus);

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to flush bus on opening", -r);
}

void Connection::notifyEventLoopToExit()
{
    loopExitFd_.notify();
}

void Connection::notifyEventLoopToWakeUpFromPoll()
{
    eventFd_.notify();
}

void Connection::wakeUpEventLoopIfMessagesInQueue()
{
    // When doing a sync call, other D-Bus messages may have arrived, waiting in the read queue.
    // In case an event loop is inside a poll in another thread, or an external event loop polls in the
    // same thread but as an unrelated event source, then we need to wake up the poll explicitly so the
    // event loop 1. processes all messages in the read queue, 2. updates poll timeout before next poll.
    if (arePendingMessagesInReadQueue())
        notifyEventLoopToWakeUpFromPoll();
}

void Connection::joinWithEventLoop()
{
    if (asyncLoopThread_.joinable())
        asyncLoopThread_.join();
}

bool Connection::processPendingEvent()
{
    auto bus = bus_.get();
    assert(bus != nullptr);

    int r = sdbus_->sd_bus_process(bus, nullptr);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to process bus requests", -r);

    // In correct use of sdbus-c++ API, r can be 0 only when processPendingEvent()
    // is called from an external event loop as a reaction to event fd being signalled.
    // If there are no more D-Bus messages to process, we know we have to clear event fd.
    if (r == 0)
        eventFd_.clear();

    return r > 0;
}

bool Connection::waitForNextEvent()
{
    assert(bus_ != nullptr);
    assert(loopExitFd_.fd >= 0);
    assert(eventFd_.fd >= 0);

    auto sdbusPollData = getEventLoopPollData();
    struct pollfd fds[] = { {sdbusPollData.fd, sdbusPollData.events, 0}
                          , {eventFd_.fd, POLLIN, 0}
                          , {loopExitFd_.fd, POLLIN, 0} };
    constexpr auto fdsCount = sizeof(fds)/sizeof(fds[0]);

    auto timeout = sdbusPollData.getPollTimeout();
    auto r = poll(fds, fdsCount, timeout);

    if (r < 0 && errno == EINTR)
        return true; // Try again

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to wait on the bus", -errno);

    // Wake up notification, in order that we re-enter poll with freshly read PollData (namely, new poll timeout thereof)
    if (fds[1].revents & POLLIN)
    {
        auto cleared = eventFd_.clear();
        SDBUS_THROW_ERROR_IF(!cleared, "Failed to read from the event descriptor", -errno);
        // Go poll() again, but with up-to-date timeout (which will wake poll() up right away if there are messages to process)
        return waitForNextEvent();
    }
    // Loop exit notification
    if (fds[2].revents & POLLIN)
    {
        auto cleared = loopExitFd_.clear();
        SDBUS_THROW_ERROR_IF(!cleared, "Failed to read from the loop exit descriptor", -errno);
        return false;
    }

    return true;
}

bool Connection::arePendingMessagesInReadQueue() const
{
    uint64_t readQueueSize{};

    auto r = sdbus_->sd_bus_get_n_queued_read(bus_.get(), &readQueueSize);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get number of pending messages in read queue", -r);

    return readQueueSize > 0;
}

Message Connection::getCurrentlyProcessedMessage() const
{
    auto* sdbusMsg = sdbus_->sd_bus_get_current_message(bus_.get());

    return Message::Factory::create<Message>(sdbusMsg, sdbus_.get());
}

template <typename StringBasedType>
std::vector</*const */char*> Connection::to_strv(const std::vector<StringBasedType>& strings)
{
    std::vector</*const */char*> strv;
    for (auto& str : strings)
        strv.push_back(const_cast<char*>(str.c_str()));
    strv.push_back(nullptr);
    return strv;
}

int Connection::sdbus_match_callback(sd_bus_message *sdbusMessage, void *userData, sd_bus_error *retError)
{
    auto* matchInfo = static_cast<MatchInfo*>(userData);
    assert(matchInfo != nullptr);
    assert(matchInfo->callback);

    auto message = Message::Factory::create<PlainMessage>(sdbusMessage, &matchInfo->connection.getSdBusInterface());

    auto ok = invokeHandlerAndCatchErrors([&](){ matchInfo->callback(std::move(message)); }, retError);

    return ok ? 0 : -1;
}

int Connection::sdbus_match_install_callback(sd_bus_message *sdbusMessage, void *userData, sd_bus_error *retError)
{
    auto* matchInfo = static_cast<MatchInfo*>(userData);
    assert(matchInfo != nullptr);
    assert(matchInfo->installCallback);

    auto message = Message::Factory::create<PlainMessage>(sdbusMessage, &matchInfo->connection.getSdBusInterface());

    auto ok = invokeHandlerAndCatchErrors([&](){ matchInfo->installCallback(std::move(message)); }, retError);

    return ok ? 0 : -1;
}

Connection::EventFd::EventFd()
{
    fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    SDBUS_THROW_ERROR_IF(fd < 0, "Failed to create event object", -errno);
}

Connection::EventFd::~EventFd()
{
    assert(fd >= 0);
    close(fd);
}

void Connection::EventFd::notify()
{
    assert(fd >= 0);
    auto r = eventfd_write(fd, 1);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to notify event descriptor", -errno);
}

bool Connection::EventFd::clear()
{
    assert(fd >= 0);

    uint64_t value{};
    auto r = eventfd_read(fd, &value);
    return r >= 0;
}

} // namespace sdbus::internal

namespace sdbus {

std::chrono::microseconds IConnection::PollData::getRelativeTimeout() const
{
    constexpr auto zero = std::chrono::microseconds::zero();
    constexpr auto max = std::chrono::microseconds::max();
    using internal::now;

    if (timeout == zero)
        return zero;
    else if (timeout == max)
        return max;
    else
        return std::max(std::chrono::duration_cast<std::chrono::microseconds>(timeout - now()), zero);
}

int IConnection::PollData::getPollTimeout() const
{
    const auto relativeTimeout = getRelativeTimeout();

    if (relativeTimeout == decltype(relativeTimeout)::max())
        return -1;
    else
        return static_cast<int>(std::chrono::ceil<std::chrono::milliseconds>(relativeTimeout).count());
}

} // namespace sdbus

namespace sdbus::internal {

std::unique_ptr<sdbus::internal::IConnection> createPseudoConnection()
{
    auto interface = std::make_unique<sdbus::internal::SdBus>();
    return std::make_unique<sdbus::internal::Connection>(std::move(interface), Connection::pseudo_bus);
}

} // namespace sdbus::internal

namespace sdbus {

using internal::Connection;

std::unique_ptr<sdbus::IConnection> createBusConnection()
{
    auto interface = std::make_unique<sdbus::internal::SdBus>();
    return std::make_unique<sdbus::internal::Connection>(std::move(interface), Connection::default_bus);
}

std::unique_ptr<sdbus::IConnection> createBusConnection(const ServiceName& name)
{
    auto conn = createBusConnection();
    conn->requestName(name);
    return conn;
}

std::unique_ptr<sdbus::IConnection> createSystemBusConnection()
{
    auto interface = std::make_unique<sdbus::internal::SdBus>();
    return std::make_unique<sdbus::internal::Connection>(std::move(interface), Connection::system_bus);
}

std::unique_ptr<sdbus::IConnection> createSystemBusConnection(const ServiceName& name)
{
    auto conn = createSystemBusConnection();
    conn->requestName(name);
    return conn;
}

std::unique_ptr<sdbus::IConnection> createSessionBusConnection()
{
    auto interface = std::make_unique<sdbus::internal::SdBus>();
    return std::make_unique<sdbus::internal::Connection>(std::move(interface), Connection::session_bus);
}

std::unique_ptr<sdbus::IConnection> createSessionBusConnection(const ServiceName& name)
{
    auto conn = createSessionBusConnection();
    conn->requestName(name);
    return conn;
}

std::unique_ptr<sdbus::IConnection> createSessionBusConnectionWithAddress(const std::string &address)
{
    auto interface = std::make_unique<sdbus::internal::SdBus>();
    return std::make_unique<sdbus::internal::Connection>(std::move(interface), Connection::custom_session_bus, address);
}

std::unique_ptr<sdbus::IConnection> createRemoteSystemBusConnection(const std::string& host)
{
    auto interface = std::make_unique<sdbus::internal::SdBus>();
    return std::make_unique<sdbus::internal::Connection>(std::move(interface), Connection::remote_system_bus, host);
}

std::unique_ptr<sdbus::IConnection> createDirectBusConnection(const std::string& address)
{
    auto interface = std::make_unique<sdbus::internal::SdBus>();
    return std::make_unique<sdbus::internal::Connection>(std::move(interface), Connection::private_bus, address);
}

std::unique_ptr<sdbus::IConnection> createDirectBusConnection(int fd)
{
    auto interface = std::make_unique<sdbus::internal::SdBus>();
    return std::make_unique<sdbus::internal::Connection>(std::move(interface), Connection::private_bus, fd);
}

std::unique_ptr<sdbus::IConnection> createServerBus(int fd)
{
    auto interface = std::make_unique<sdbus::internal::SdBus>();
    return std::make_unique<sdbus::internal::Connection>(std::move(interface), Connection::server_bus, fd);
}

std::unique_ptr<sdbus::IConnection> createBusConnection(sd_bus *bus)
{
    SDBUS_THROW_ERROR_IF(bus == nullptr, "Invalid bus argument", EINVAL);

    auto interface = std::make_unique<sdbus::internal::SdBus>();
    return std::make_unique<sdbus::internal::Connection>(std::move(interface), Connection::sdbus_bus, bus);
}

} // namespace sdbus
