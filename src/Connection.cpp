/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
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
#include <sdbus-c++/Message.h>
#include <sdbus-c++/Error.h>
#include "ScopeGuard.h"
#include <systemd/sd-bus.h>
#include <unistd.h>
#include <poll.h>
#include <sys/eventfd.h>

namespace {
    std::map<sdbus::internal::Connection::BusType, int(*)(sd_bus **)> busTypeToFactory
    {
        {sdbus::internal::Connection::BusType::eSystem, &sd_bus_open_system},
        {sdbus::internal::Connection::BusType::eSession, &sd_bus_open_user}
    };
}

namespace sdbus { namespace internal {

Connection::Connection(Connection::BusType type)
    : busType_(type)
{
    sd_bus* bus{};
    auto r = busTypeToFactory[busType_](&bus);
    if (r < 0)
        SDBUS_THROW_ERROR("Failed to open system bus", -r);

    bus_.reset(bus);

    // Process all requests that are part of the initial handshake,
    // like processing the Hello message response, authentication etc.,
    // to avoid connection authentication timeout in dbus daemon.
    r = sd_bus_flush(bus_.get());
    if (r < 0)
        SDBUS_THROW_ERROR("Failed to flush system bus on opening", -r);

    r = eventfd(0, EFD_SEMAPHORE | EFD_CLOEXEC);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to create event object", -errno);
    runFd_ = r;
}

Connection::~Connection()
{
    leaveProcessingLoop();
    close(runFd_);
}

void Connection::requestName(const std::string& name)
{
    auto r = sd_bus_request_name(bus_.get(), name.c_str(), 0);
    if (r < 0)
        SDBUS_THROW_ERROR("Failed to request bus name", -r);
}

void Connection::releaseName(const std::string& name)
{
    auto r = sd_bus_release_name(bus_.get(), name.c_str());
    if (r < 0)
        SDBUS_THROW_ERROR("Failed to release bus name", -r);
}

void Connection::enterProcessingLoop()
{
    int semaphoreFd = runFd_;
    short int semaphoreEvents = POLLIN;

    while (true)
    {
        /* Process requests */
        int r = sd_bus_process(bus_.get(), nullptr);
        SDBUS_THROW_ERROR_IF(r < 0, "Failed to process bus requests", -r);
        if (r > 0) /* we processed a request, try to process another one, right-away */
            continue;

        r = sd_bus_get_fd(bus_.get());
        SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus descriptor", -r);
        auto sdbusFd = r;

        r = sd_bus_get_events(bus_.get());
        SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus descriptor", -r);
        short int sdbusEvents = r;

        struct pollfd fds[] = {{sdbusFd, sdbusEvents, 0}, {semaphoreFd, semaphoreEvents, 0}};

        /* Wait for the next request to process */
        uint64_t usec;
        sd_bus_get_timeout(bus_.get(), &usec);

        auto fdsCount = sizeof(fds)/sizeof(fds[0]);
        r = poll(fds, fdsCount, usec == (uint64_t) -1 ? -1 : (usec+999)/1000);
        SDBUS_THROW_ERROR_IF(r < 0, "Failed to wait on the bus", -errno);

        if (fds[1].revents & POLLIN)
            break;
    }
}

void Connection::enterProcessingLoopAsync()
{
    asyncLoopThread_ = std::thread([this](){ enterProcessingLoop(); });
}

void Connection::leaveProcessingLoop()
{
    assert(runFd_ >= 0);
    uint64_t value = 1;
    write(runFd_, &value, sizeof(value));

    if (asyncLoopThread_.joinable())
        asyncLoopThread_.join();
}

void* Connection::addObjectVTable( const std::string& objectPath
                                 , const std::string& interfaceName
                                 , const void* vtable
                                 , void* userData )
{
    sd_bus_slot *slot{};
    auto r = sd_bus_add_object_vtable( bus_.get()
                                     , &slot
                                     , objectPath.c_str()
                                     , interfaceName.c_str()
                                     , static_cast<const sd_bus_vtable*>(vtable)
                                     , userData );
    if (r < 0)
        SDBUS_THROW_ERROR("Failed to register object vtable", -r);

    return slot;
}

void Connection::removeObjectVTable(void* vtableHandle)
{
    sd_bus_slot_unref((sd_bus_slot *)vtableHandle);
}

sdbus::Message Connection::createMethodCall( const std::string& destination
                                           , const std::string& objectPath
                                           , const std::string& interfaceName
                                           , const std::string& methodName ) const
{
    sd_bus_message *sdbusMsg{};
    SCOPE_EXIT{ sd_bus_message_unref(sdbusMsg); }; // Returned message will become an owner of sdbusMsg
    auto r = sd_bus_message_new_method_call( bus_.get()
                                           , &sdbusMsg
                                           , destination.c_str()
                                           , objectPath.c_str()
                                           , interfaceName.c_str()
                                           , methodName.c_str() );
    if (r < 0)
        SDBUS_THROW_ERROR("Failed to create method call", -r);

    return Message(sdbusMsg, Message::Type::eMethodCall);
}

sdbus::Message Connection::createSignal( const std::string& objectPath
                                       , const std::string& interfaceName
                                       , const std::string& signalName ) const
{
    sd_bus_message *sdbusSignal{};
    SCOPE_EXIT{ sd_bus_message_unref(sdbusSignal); }; // Returned message will become an owner of sdbusSignal
    auto r = sd_bus_message_new_signal( bus_.get()
                                      , &sdbusSignal
                                      , objectPath.c_str()
                                      , interfaceName.c_str()
                                      , signalName.c_str() );
    if (r < 0)
        SDBUS_THROW_ERROR("Failed to create signal", -r);

    return Message(sdbusSignal, Message::Type::eSignal);
}

void* Connection::registerSignalHandler( const std::string& objectPath
                                       , const std::string& interfaceName
                                       , const std::string& signalName
                                       , sd_bus_message_handler_t callback
                                       , void* userData )
{
    sd_bus_slot *slot{};
    auto filter = composeSignalMatchFilter(objectPath, interfaceName, signalName);
    auto r = sd_bus_add_match(bus_.get(), &slot, filter.c_str(), callback, userData);
    if (r < 0)
        SDBUS_THROW_ERROR("Failed to register signal handler", -r);

    return slot;
}

void Connection::unregisterSignalHandler(void* handlerCookie)
{
    sd_bus_slot_unref((sd_bus_slot *)handlerCookie);
}

std::unique_ptr<sdbus::internal::IConnection> Connection::clone() const
{
    return std::make_unique<sdbus::internal::Connection>(busType_);
}

std::string Connection::composeSignalMatchFilter( const std::string& objectPath
                                                , const std::string& interfaceName
                                                , const std::string& signalName )
{
    std::string filter;
    filter += "type='signal',";
    filter += "interface='" + interfaceName + "',";
    filter += "member='" + signalName + "',";
    filter += "path='" + objectPath + "'";
    return filter;
}

}}

namespace sdbus {

std::unique_ptr<sdbus::IConnection> createConnection()
{
    return createSystemBusConnection();
}

std::unique_ptr<sdbus::IConnection> createConnection(const std::string& name)
{
    return createSystemBusConnection(name);
}

std::unique_ptr<sdbus::IConnection> createSystemBusConnection()
{
    return std::make_unique<sdbus::internal::Connection>(sdbus::internal::Connection::BusType::eSystem);
}

std::unique_ptr<sdbus::IConnection> createSystemBusConnection(const std::string& name)
{
    auto conn = createSystemBusConnection();
    conn->requestName(name);
    return conn;
}

std::unique_ptr<sdbus::IConnection> createSessionBusConnection()
{
    return std::make_unique<sdbus::internal::Connection>(sdbus::internal::Connection::BusType::eSession);
}

std::unique_ptr<sdbus::IConnection> createSessionBusConnection(const std::string& name)
{
    auto conn = createSessionBusConnection();
    conn->requestName(name);
    return conn;
}

}
