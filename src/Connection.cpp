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
#include "SdBus.h"
#include "MessageUtils.h"
#include "Utils.h"
#include <sdbus-c++/Message.h>
#include <sdbus-c++/Error.h>
#include "ScopeGuard.h"
#include <systemd/sd-bus.h>
#include <unistd.h>
#include <poll.h>
#include <sys/eventfd.h>

namespace sdbus::internal {

Connection::Connection(std::unique_ptr<ISdBus>&& interface, const BusFactory& busFactory)
    : iface_(std::move(interface))
    , bus_(openBus(busFactory))
{
    assert(iface_ != nullptr);
}

Connection::Connection(std::unique_ptr<ISdBus>&& interface, default_bus_t)
    : Connection(std::move(interface), [this](sd_bus** bus){ return iface_->sd_bus_open(bus); })
{
}

Connection::Connection(std::unique_ptr<ISdBus>&& interface, system_bus_t)
    : Connection(std::move(interface), [this](sd_bus** bus){ return iface_->sd_bus_open_system(bus); })
{
}

Connection::Connection(std::unique_ptr<ISdBus>&& interface, session_bus_t)
    : Connection(std::move(interface), [this](sd_bus** bus){ return iface_->sd_bus_open_user(bus); })
{
}

Connection::Connection(std::unique_ptr<ISdBus>&& interface, custom_session_bus_t, const std::string& address)
        : Connection(std::move(interface), [&](sd_bus** bus) { return iface_->sd_bus_open_user_with_address(bus, address.c_str()); })
{
}

Connection::Connection(std::unique_ptr<ISdBus>&& interface, remote_system_bus_t, const std::string& host)
    : Connection(std::move(interface), [this, &host](sd_bus** bus){ return iface_->sd_bus_open_system_remote(bus, host.c_str()); })
{
}

Connection::Connection(std::unique_ptr<ISdBus>&& interface, private_bus_t, const std::string& address)
    : Connection(std::move(interface), [&](sd_bus** bus) { return iface_->sd_bus_open_direct(bus, address.c_str()); })
{
}

Connection::Connection(std::unique_ptr<ISdBus>&& interface, server_bus_t, int fd)
    : Connection(std::move(interface), [&](sd_bus** bus) { return iface_->sd_bus_open_server(bus, fd); })
{
}

Connection::Connection(std::unique_ptr<ISdBus>&& interface, pseudo_bus_t)
    : iface_(std::move(interface))
    , bus_(openPseudoBus())
{
    assert(iface_ != nullptr);
}

Connection::~Connection()
{
    Connection::leaveEventLoop();
}

void Connection::requestName(const std::string& name)
{
    SDBUS_CHECK_SERVICE_NAME(name);

    auto r = iface_->sd_bus_request_name(bus_.get(), name.c_str(), 0);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to request bus name", -r);
}

void Connection::releaseName(const std::string& name)
{
    auto r = iface_->sd_bus_release_name(bus_.get(), name.c_str());
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to release bus name", -r);
}

std::string Connection::getUniqueName() const
{
    const char* unique = nullptr;
    auto r = iface_->sd_bus_get_unique_name(bus_.get(), &unique);
    SDBUS_THROW_ERROR_IF(r < 0 || unique == nullptr, "Failed to get unique bus name", -r);
    return unique;
}

void Connection::enterEventLoop()
{
    loopThreadId_ = std::this_thread::get_id();
    SCOPE_EXIT{ loopThreadId_ = std::thread::id{}; };

    std::lock_guard guard(loopMutex_);

    while (true)
    {
        auto processed = processPendingRequest();
        if (processed)
            continue; // Process next one

        auto success = waitForNextRequest();
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
    auto r = iface_->sd_bus_get_poll_data(bus_.get(), &pollData);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus poll data", -r);

    return {pollData.fd, pollData.events, pollData.timeout_usec};
}

const ISdBus& Connection::getSdBusInterface() const
{
    return *iface_.get();
}

ISdBus& Connection::getSdBusInterface()
{
    return *iface_.get();
}

void Connection::addObjectManager(const std::string& objectPath)
{
    Connection::addObjectManager(objectPath, floating_slot);
}

void Connection::addObjectManager(const std::string& objectPath, floating_slot_t)
{
    auto r = iface_->sd_bus_add_object_manager(bus_.get(), nullptr, objectPath.c_str());

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to add object manager", -r);
}

Slot Connection::addObjectManager(const std::string& objectPath, request_slot_t)
{
    sd_bus_slot *slot{};

    auto r = iface_->sd_bus_add_object_manager(bus_.get(), &slot, objectPath.c_str());

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to add object manager", -r);

    return {slot, [this](void *slot){ iface_->sd_bus_slot_unref((sd_bus_slot*)slot); }};
}

void Connection::setMethodCallTimeout(uint64_t timeout)
{
    auto r = iface_->sd_bus_set_method_call_timeout(bus_.get(), timeout);

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to set method call timeout", -r);
}

uint64_t Connection::getMethodCallTimeout() const
{
    uint64_t timeout;

    auto r = iface_->sd_bus_get_method_call_timeout(bus_.get(), &timeout);

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get method call timeout", -r);

    return timeout;
}

Slot Connection::addMatch(const std::string& match, message_handler callback)
{
    auto matchInfo = std::make_unique<MatchInfo>(MatchInfo{std::move(callback), *this, {}});

    auto messageHandler = [](sd_bus_message *sdbusMessage, void *userData, sd_bus_error */*retError*/) -> int
    {
        auto* matchInfo = static_cast<MatchInfo*>(userData);
        auto message = Message::Factory::create<PlainMessage>(sdbusMessage, &matchInfo->connection.getSdBusInterface());
        matchInfo->callback(message);
        return 0;
    };

    auto r = iface_->sd_bus_add_match(bus_.get(), &matchInfo->slot, match.c_str(), std::move(messageHandler), matchInfo.get());
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to add match", -r);

    return {matchInfo.release(), [this](void *ptr)
    {
        auto* matchInfo = static_cast<MatchInfo*>(ptr);
        iface_->sd_bus_slot_unref(matchInfo->slot);
        std::default_delete<MatchInfo>{}(matchInfo);
    }};
}

void Connection::addMatch(const std::string& match, message_handler callback, floating_slot_t)
{
    floatingMatchRules_.push_back(addMatch(match, std::move(callback)));
}

Slot Connection::addObjectVTable( const std::string& objectPath
                                , const std::string& interfaceName
                                , const sd_bus_vtable* vtable
                                , void* userData )
{
    sd_bus_slot *slot{};

    auto r = iface_->sd_bus_add_object_vtable( bus_.get()
                                             , &slot
                                             , objectPath.c_str()
                                             , interfaceName.c_str()
                                             , vtable
                                             , userData );

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to register object vtable", -r);

    return {slot, [this](void *slot){ iface_->sd_bus_slot_unref((sd_bus_slot*)slot); }};
}

PlainMessage Connection::createPlainMessage() const
{
    sd_bus_message* sdbusMsg{};

    auto r = iface_->sd_bus_message_new(bus_.get(), &sdbusMsg, _SD_BUS_MESSAGE_TYPE_INVALID);

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to create a plain message", -r);

    return Message::Factory::create<PlainMessage>(sdbusMsg, iface_.get(), adopt_message);
}

MethodCall Connection::createMethodCall( const std::optional<std::string>& destination
                                       , const std::string& objectPath
                                       , const std::string& interfaceName
                                       , const std::string& methodName ) const
{
    sd_bus_message *sdbusMsg{};

    auto r = iface_->sd_bus_message_new_method_call( bus_.get()
                                                   , &sdbusMsg
                                                   , destination ? (*destination).c_str() : nullptr
                                                   , objectPath.c_str()
                                                   , interfaceName.c_str()
                                                   , methodName.c_str() );

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to create method call", -r);

    return Message::Factory::create<MethodCall>(sdbusMsg, iface_.get(), this, adopt_message);
}

Signal Connection::createSignal( const std::string& objectPath
                               , const std::string& interfaceName
                               , const std::string& signalName ) const
{
    sd_bus_message *sdbusMsg{};

    auto r = iface_->sd_bus_message_new_signal( bus_.get()
                                              , &sdbusMsg
                                              , objectPath.c_str()
                                              , interfaceName.c_str()
                                              , signalName.c_str() );

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to create signal", -r);

    return Message::Factory::create<Signal>(sdbusMsg, iface_.get(), adopt_message);
}

void Connection::emitPropertiesChangedSignal( const std::string& objectPath
                                            , const std::string& interfaceName
                                            , const std::vector<std::string>& propNames )
{
    auto names = to_strv(propNames);

    auto r = iface_->sd_bus_emit_properties_changed_strv( bus_.get()
                                                        , objectPath.c_str()
                                                        , interfaceName.c_str()
                                                        , propNames.empty() ? nullptr : &names[0] );

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to emit PropertiesChanged signal", -r);
}

void Connection::emitInterfacesAddedSignal(const std::string& objectPath)
{
    auto r = iface_->sd_bus_emit_object_added(bus_.get(), objectPath.c_str());

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to emit InterfacesAdded signal for all registered interfaces", -r);
}

void Connection::emitInterfacesAddedSignal( const std::string& objectPath
                                          , const std::vector<std::string>& interfaces )
{
    auto names = to_strv(interfaces);

    auto r = iface_->sd_bus_emit_interfaces_added_strv( bus_.get()
                                                      , objectPath.c_str()
                                                      , interfaces.empty() ? nullptr : &names[0] );

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to emit InterfacesAdded signal", -r);
}

void Connection::emitInterfacesRemovedSignal(const std::string& objectPath)
{
    auto r = iface_->sd_bus_emit_object_removed(bus_.get(), objectPath.c_str());

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to emit InterfacesRemoved signal for all registered interfaces", -r);
}

void Connection::emitInterfacesRemovedSignal( const std::string& objectPath
                                            , const std::vector<std::string>& interfaces )
{
    auto names = to_strv(interfaces);

    auto r = iface_->sd_bus_emit_interfaces_removed_strv( bus_.get()
                                                        , objectPath.c_str()
                                                        , interfaces.empty() ? nullptr : &names[0] );

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to emit InterfacesRemoved signal", -r);
}

Slot Connection::registerSignalHandler( const std::optional<std::string>& sender
                                      , const std::string& objectPath
                                      , const std::string& interfaceName
                                      , const std::string& signalName
                                      , sd_bus_message_handler_t callback
                                      , void* userData )
{
    sd_bus_slot *slot{};

    // Alternatively to our own composeSignalMatchFilter() implementation, we could use sd_bus_match_signal() from
    // https://www.freedesktop.org/software/systemd/man/sd_bus_add_match.html .
    // But this would require libsystemd v237 or higher.
    auto filter = composeSignalMatchFilter(sender, objectPath, interfaceName, signalName);
    auto r = iface_->sd_bus_add_match(bus_.get(), &slot, filter.c_str(), callback, userData);

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to register signal handler", -r);

    return {slot, [this](void *slot){ iface_->sd_bus_slot_unref((sd_bus_slot*)slot); }};
}

MethodReply Connection::tryCallMethodSynchronously(const MethodCall& message, uint64_t timeout)
{
    auto loopThreadId = loopThreadId_.load(std::memory_order_relaxed);

    // Is the loop not yet on? => Go make synchronous call
    while (loopThreadId == std::thread::id{})
    {
        // Did the loop begin in the meantime? Or try_lock() failed spuriously?
        if (!loopMutex_.try_lock())
        {
            loopThreadId = loopThreadId_.load(std::memory_order_relaxed);
            continue;
        }

        // Synchronous D-Bus call
        std::lock_guard guard(loopMutex_, std::adopt_lock);
        return message.send(timeout);
    }

    // Is the loop on and we are in the same thread? => Go for synchronous call
    if (loopThreadId == std::this_thread::get_id())
    {
        assert(!loopMutex_.try_lock());
        return message.send(timeout);
    }

    return {};
}

Connection::BusPtr Connection::openBus(const BusFactory& busFactory)
{
    sd_bus* bus{};
    int r = busFactory(&bus);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to open bus", -r);

    BusPtr busPtr{bus, [this](sd_bus* bus){ return iface_->sd_bus_flush_close_unref(bus); }};
    finishHandshake(busPtr.get());
    return busPtr;
}

Connection::BusPtr Connection::openPseudoBus()
{
    sd_bus* bus{};

    int r = iface_->sd_bus_new(&bus);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to open pseudo bus", -r);

    (void)iface_->sd_bus_start(bus);
    // It is expected that sd_bus_start has failed here, returning -EINVAL, due to having
    // not set a bus address, but it will leave the bus in an OPENING state, which enables
    // us to create plain D-Bus messages as a local data storage (for Variant, for example),
    // without dependency on real IPC communication with the D-Bus broker daemon.
    SDBUS_THROW_ERROR_IF(r < 0 && r != -EINVAL, "Failed to start pseudo bus", -r);

    return {bus, [this](sd_bus* bus){ return iface_->sd_bus_close_unref(bus); }};
}

void Connection::finishHandshake(sd_bus* bus)
{
    // Process all requests that are part of the initial handshake,
    // like processing the Hello message response, authentication etc.,
    // to avoid connection authentication timeout in dbus daemon.

    assert(bus != nullptr);

    auto r = iface_->sd_bus_flush(bus);

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to flush bus on opening", -r);
}

void Connection::notifyEventLoop(int fd) const
{
    assert(fd >= 0);

    uint64_t value = 1;
    auto r = write(fd, &value, sizeof(value));
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to notify event loop", -errno);
}

void Connection::notifyEventLoopToExit() const
{
    notifyEventLoop(loopExitFd_.fd);
}

void Connection::notifyEventLoopNewTimeout() const
{
    // The extra notifications for new timeouts are only needed if calls are made asynchronously to the event loop.
    // Are we in the same thread as the event loop? Note that it's ok to fail this check because the event loop isn't yet started.
    if (loopThreadId_.load(std::memory_order_relaxed) == std::this_thread::get_id())
        return;

    // Get the new timeout from sd-bus
    auto sdbusPollData = getEventLoopPollData();
    if (sdbusPollData.timeout_usec < activeTimeout_.load(std::memory_order_relaxed))
        notifyEventLoop(eventFd_.fd);
}

void Connection::clearEventLoopNotification(int fd) const
{
    uint64_t value{};
    auto r = read(fd, &value, sizeof(value));
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to read from the event descriptor", -errno);
}

void Connection::joinWithEventLoop()
{
    if (asyncLoopThread_.joinable())
        asyncLoopThread_.join();
}

bool Connection::processPendingRequest()
{
    auto bus = bus_.get();
    assert(bus != nullptr);

    int r = iface_->sd_bus_process(bus, nullptr);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to process bus requests", -r);

    return r > 0;
}

bool Connection::waitForNextRequest()
{
    assert(bus_ != nullptr);
    assert(eventFd_.fd >= 0);

    auto sdbusPollData = getEventLoopPollData();
    struct pollfd fds[] = {
            {sdbusPollData.fd, sdbusPollData.events, 0},
            {eventFd_.fd, POLLIN, 0},
            {loopExitFd_.fd, POLLIN, 0}
    };
    auto fdsCount = sizeof(fds)/sizeof(fds[0]);

    auto timeout = sdbusPollData.getPollTimeout();
    activeTimeout_.store(sdbusPollData.timeout_usec, std::memory_order_relaxed);
    auto r = poll(fds, fdsCount, timeout);

    if (r < 0 && errno == EINTR)
        return true; // Try again

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to wait on the bus", -errno);

    // new timeout notification
    if (fds[1].revents & POLLIN)
    {
        clearEventLoopNotification(fds[1].fd);
    }
    // loop exit notification
    if (fds[2].revents & POLLIN)
    {
        clearEventLoopNotification(fds[2].fd);
        return false;
    }

    return true;
}

std::string Connection::composeSignalMatchFilter( const std::optional<std::string> &sender
                                                , const std::string &objectPath
                                                , const std::string &interfaceName
                                                , const std::string &signalName )
{
    std::string filter;

    filter += "type='signal',";
    if (sender)
        filter += "sender='" + *sender + "',";
    filter += "interface='" + interfaceName + "',";
    filter += "member='" + signalName + "',";
    filter += "path='" + objectPath + "'";

    return filter;
}

std::vector</*const */char*> Connection::to_strv(const std::vector<std::string>& strings)
{
    std::vector</*const */char*> strv;
    for (auto& str : strings)
        strv.push_back(const_cast<char*>(str.c_str()));
    strv.push_back(nullptr);
    return strv;
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

} // namespace sdbus::internal

namespace sdbus {

std::optional<std::chrono::microseconds> IConnection::PollData::getRelativeTimeout() const
{
    constexpr auto zero = std::chrono::microseconds::zero();
    if (timeout_usec == 0)
        return zero;
    else if (timeout_usec == UINT64_MAX)
        return std::nullopt;

    // We need C so that we use the same clock as the underlying sd-bus lib.
    // We use POSIX's clock_gettime in favour of std::chrono::steady_clock to ensure this.
    struct timespec ts{};
    auto r = clock_gettime(CLOCK_MONOTONIC, &ts);
    SDBUS_THROW_ERROR_IF(r < 0, "clock_gettime failed: ", -errno);
    auto now = std::chrono::nanoseconds(ts.tv_nsec) + std::chrono::seconds(ts.tv_sec);
    auto absTimeout = std::chrono::microseconds(timeout_usec);
    auto result = std::chrono::duration_cast<std::chrono::microseconds>(absTimeout - now);
    return std::max(result, zero);
}

int IConnection::PollData::getPollTimeout() const
{
    auto timeout = getRelativeTimeout();
    return timeout ? static_cast<int>(std::chrono::ceil<std::chrono::milliseconds>(timeout.value()).count()) : -1;
}

} // namespace sdbus

namespace sdbus::internal {

std::unique_ptr<sdbus::internal::IConnection> createConnection()
{
    auto connection = sdbus::createConnection();
    SCOPE_EXIT{ connection.release(); };
    auto connectionInternal = dynamic_cast<sdbus::internal::IConnection*>(connection.get());
    return std::unique_ptr<sdbus::internal::IConnection>(connectionInternal);
}

std::unique_ptr<sdbus::internal::IConnection> createPseudoConnection()
{
    auto interface = std::make_unique<sdbus::internal::SdBus>();
    return std::make_unique<sdbus::internal::Connection>(std::move(interface), Connection::pseudo_bus);
}

} // namespace sdbus::internal

namespace sdbus {

using internal::Connection;

std::unique_ptr<sdbus::IConnection> createConnection()
{
    return createSystemBusConnection();
}

std::unique_ptr<sdbus::IConnection> createConnection(const std::string& name)
{
    return createSystemBusConnection(name);
}

std::unique_ptr<sdbus::IConnection> createDefaultBusConnection()
{
    auto interface = std::make_unique<sdbus::internal::SdBus>();
    return std::make_unique<sdbus::internal::Connection>(std::move(interface), Connection::default_bus);
}

std::unique_ptr<sdbus::IConnection> createDefaultBusConnection(const std::string& name)
{
    auto conn = createDefaultBusConnection();
    conn->requestName(name);
    return conn;
}

std::unique_ptr<sdbus::IConnection> createSystemBusConnection()
{
    auto interface = std::make_unique<sdbus::internal::SdBus>();
    return std::make_unique<sdbus::internal::Connection>(std::move(interface), Connection::system_bus);
}

std::unique_ptr<sdbus::IConnection> createSystemBusConnection(const std::string& name)
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

std::unique_ptr<sdbus::IConnection> createSessionBusConnection(const std::string& name)
{
    auto conn = createSessionBusConnection();
    conn->requestName(name);
    return conn;
}

std::unique_ptr<sdbus::IConnection> createSessionBusConnectionWithAddress(const std::string &address)
{
    auto interface = std::make_unique<sdbus::internal::SdBus>();
    assert(interface != nullptr);
    return std::make_unique<sdbus::internal::Connection>(std::move(interface), Connection::custom_session_bus, address);
}

std::unique_ptr<sdbus::IConnection> createRemoteSystemBusConnection(const std::string& host)
{
    auto interface = std::make_unique<sdbus::internal::SdBus>();
    return std::make_unique<sdbus::internal::Connection>(std::move(interface), Connection::remote_system_bus, host);
}

std::unique_ptr<sdbus::IConnection> createDirectBusConnection(const std::string &address)
{
    auto interface = std::make_unique<sdbus::internal::SdBus>();
    assert(interface != nullptr);
    return std::make_unique<sdbus::internal::Connection>(std::move(interface), Connection::private_bus, address);
}

std::unique_ptr<sdbus::IConnection> createServerBus(int fd)
{
    auto interface = std::make_unique<sdbus::internal::SdBus>();
    assert(interface != nullptr);
    return std::make_unique<sdbus::internal::Connection>(std::move(interface), Connection::server_bus, fd);
}

} // namespace sdbus
