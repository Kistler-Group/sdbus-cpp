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

namespace sdbus { namespace internal {

Connection::Connection(Connection::BusType type)
    : busType_(type)
{
    auto bus = openBus(busType_);
    bus_.reset(bus);

    finishHandshake(bus);

    exitLoopFd_ = createProcessingExitDescriptor();
}

Connection::~Connection()
{
    leaveProcessingLoop();
    closeProcessingExitDescriptor(exitLoopFd_);
}

void Connection::requestName(const std::string& name)
{
    auto r = sd_bus_request_name(bus_.get(), name.c_str(), 0);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to request bus name", -r);
}

void Connection::releaseName(const std::string& name)
{
    auto r = sd_bus_release_name(bus_.get(), name.c_str());
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to release bus name", -r);
}

void Connection::enterProcessingLoop()
{
    while (true)
    {
        auto processed = processPendingRequest(bus_.get());
        if (processed)
            continue; // Process next one

        auto success = waitForNextRequest(bus_.get(), exitLoopFd_);
        if (!success)
            break; // Exit processing loop
    }
}

void Connection::enterProcessingLoopAsync()
{
    asyncLoopThread_ = std::thread([this](){ enterProcessingLoop(); });
}

void Connection::leaveProcessingLoop()
{
    notifyProcessingLoopToExit();
    joinWithProcessingLoop();
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

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to register object vtable", -r);

    return slot;
}

void Connection::removeObjectVTable(void* vtableHandle)
{
    sd_bus_slot_unref((sd_bus_slot *)vtableHandle);
}

sdbus::MethodCall Connection::createMethodCall( const std::string& destination
                                              , const std::string& objectPath
                                              , const std::string& interfaceName
                                              , const std::string& methodName ) const
{
    sd_bus_message *sdbusMsg{};

    // Returned message will become an owner of sdbusMsg
    SCOPE_EXIT{ sd_bus_message_unref(sdbusMsg); };

    auto r = sd_bus_message_new_method_call( bus_.get()
                                           , &sdbusMsg
                                           , destination.c_str()
                                           , objectPath.c_str()
                                           , interfaceName.c_str()
                                           , methodName.c_str() );

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to create method call", -r);

    return MethodCall(sdbusMsg);
}

sdbus::Signal Connection::createSignal( const std::string& objectPath
                                      , const std::string& interfaceName
                                      , const std::string& signalName ) const
{
    sd_bus_message *sdbusSignal{};

    // Returned message will become an owner of sdbusSignal
    SCOPE_EXIT{ sd_bus_message_unref(sdbusSignal); };

    auto r = sd_bus_message_new_signal( bus_.get()
                                      , &sdbusSignal
                                      , objectPath.c_str()
                                      , interfaceName.c_str()
                                      , signalName.c_str() );

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to create signal", -r);

    return Signal(sdbusSignal);
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

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to register signal handler", -r);

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

sd_bus* Connection::openBus(Connection::BusType type)
{
    static std::map<sdbus::internal::Connection::BusType, int(*)(sd_bus **)> busTypeToFactory
    {
        {sdbus::internal::Connection::BusType::eSystem, &sd_bus_open_system},
        {sdbus::internal::Connection::BusType::eSession, &sd_bus_open_user}
    };

    sd_bus* bus{};

    auto r = busTypeToFactory[type](&bus);

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to open bus", -r);
    assert(bus != nullptr);

    return bus;
}

void Connection::finishHandshake(sd_bus* bus)
{
    // Process all requests that are part of the initial handshake,
    // like processing the Hello message response, authentication etc.,
    // to avoid connection authentication timeout in dbus daemon.

    assert(bus != nullptr);

    auto r = sd_bus_flush(bus);

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to flush bus on opening", -r);
}

int Connection::createProcessingExitDescriptor()
{
    // Mechanism for graceful termination of processing loop

    auto r = eventfd(0, EFD_SEMAPHORE | EFD_CLOEXEC);

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to create event object", -errno);

    return r;
}

void Connection::closeProcessingExitDescriptor(int fd)
{
    close(fd);
}

void Connection::notifyProcessingLoopToExit()
{
    assert(exitLoopFd_ >= 0);
    uint64_t value = 1;
    write(exitLoopFd_, &value, sizeof(value));
}

void Connection::joinWithProcessingLoop()
{
    if (asyncLoopThread_.joinable())
        asyncLoopThread_.join();
}

bool Connection::processPendingRequest(sd_bus* bus)
{
    assert(bus != nullptr);

    int r = sd_bus_process(bus, nullptr);

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to process bus requests", -r);

    return r > 0;
}

bool Connection::waitForNextRequest(sd_bus* bus, int exitFd)
{
    assert(bus != nullptr);
    assert(exitFd != 0);

    auto r = sd_bus_get_fd(bus);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus descriptor", -r);
    auto sdbusFd = r;

    r = sd_bus_get_events(bus);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus events", -r);
    short int sdbusEvents = r;

    uint64_t usec;
    sd_bus_get_timeout(bus, &usec);

    struct pollfd fds[] = {{sdbusFd, sdbusEvents, 0}, {exitFd, POLLIN, 0}};
    auto fdsCount = sizeof(fds)/sizeof(fds[0]);

    r = poll(fds, fdsCount, usec == (uint64_t) -1 ? -1 : (usec+999)/1000);

    if (r < 0 && errno == EINTR)
        return true; // Try again

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to wait on the bus", -errno);

    if (fds[1].revents & POLLIN)
        return false; // Got exit notification

    return true;
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
