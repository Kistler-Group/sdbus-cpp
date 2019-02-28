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

    loopExitFd_ = createProcessingLoopExitDescriptor();
    std::cerr << "Created eventfd " << loopExitFd_ << " of " << this << std::endl;
}

Connection::~Connection()
{
    leaveProcessingLoop();
    std::cerr << "Closing eventfd " << loopExitFd_ << " of " << this << std::endl;
    closeProcessingLoopExitDescriptor(loopExitFd_);
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
    std::unique_lock<std::recursive_mutex> lock(busMutex_);

    while (true)
    {
        auto processed = processPendingRequest();
        if (processed)
            continue; // Process next one

        lock.unlock();
        SCOPE_EXIT{ lock.lock(); };
        auto success = waitForNextRequest();
        if (!success)
            break; // Exit processing loop

        // Not necesary anymore with bus mutex
        //if (success.asyncMsgsToProcess)
        //    processUserRequests();
    }
}

void Connection::enterProcessingLoopAsync()
{
    std::cerr << "--> enterProcessingLoopAsync() for connection " << this << std::endl;
    // TODO: Check that joinable() means a valid non-empty thread
    if (!asyncLoopThread_.joinable())
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

MethodCall Connection::createMethodCall( const std::string& destination
                                       , const std::string& objectPath
                                       , const std::string& interfaceName
                                       , const std::string& methodName ) const
{
    // Note: It should be safe even without locking busMutex_ here

    sd_bus_message *sdbusMsg{};

    // Returned message will become an owner of sdbusMsg
    SCOPE_EXIT{ sd_bus_message_unref(sdbusMsg); };

    // It is thread-safe to create a message this way
    auto r = sd_bus_message_new_method_call( bus_.get()
                                           , &sdbusMsg
                                           , destination.c_str()
                                           , objectPath.c_str()
                                           , interfaceName.c_str()
                                           , methodName.c_str() );

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to create method call", -r);

    return MethodCall(sdbusMsg);
}

Signal Connection::createSignal( const std::string& objectPath
                               , const std::string& interfaceName
                               , const std::string& signalName ) const
{
    // Note: It should be safe even without locking busMutex_ here

    sd_bus_message *sdbusSignal{};

    // Returned message will become an owner of sdbusSignal
    SCOPE_EXIT{ sd_bus_message_unref(sdbusSignal); };

    // It is thread-safe to create a message this way
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
    std::lock_guard<std::recursive_mutex> lock(busMutex_);

    sd_bus_slot *slot{};

    auto filter = composeSignalMatchFilter(objectPath, interfaceName, signalName);
    auto r = sd_bus_add_match(bus_.get(), &slot, filter.c_str(), callback, userData);
    std::cerr << "Registered signal " << signalName << " with slot " << slot << std::endl;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to register signal handler", -r);

    return slot;
};

void Connection::unregisterSignalHandler(void* handlerCookie)
{
    std::lock_guard<std::recursive_mutex> lock(busMutex_);

    sd_bus_slot_unref((sd_bus_slot *)handlerCookie);
}

MethodReply Connection::callMethod(const MethodCall& message)
{
    std::lock_guard<std::recursive_mutex> lock(busMutex_);

    return message.send();
}

void Connection::callMethod(const AsyncMethodCall& message, void* callback, void* userData)
{
    std::lock_guard<std::recursive_mutex> lock(busMutex_);

    message.send(callback, userData);
}

void Connection::sendMethodReply(const MethodReply& message)
{
    std::lock_guard<std::recursive_mutex> lock(busMutex_);

    message.send();
}

void Connection::emitSignal(const Signal& message)
{
    std::lock_guard<std::recursive_mutex> lock(busMutex_);

    message.send();
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

int Connection::createProcessingLoopExitDescriptor()
{
    auto r = eventfd(0, EFD_SEMAPHORE | EFD_CLOEXEC | EFD_NONBLOCK);

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to create event object", -errno);

    return r;
}

void Connection::closeProcessingLoopExitDescriptor(int fd)
{
    close(fd);
}

void Connection::notifyProcessingLoopToExit()
{
    assert(loopExitFd_ >= 0);

    uint64_t value = 1;
    auto r = write(loopExitFd_, &value, sizeof(value));
    std::cerr << "Wrote to notification fd " << loopExitFd_ << std::endl;
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to notify processing loop", -errno);
}

void Connection::clearExitNotification()
{
    uint64_t value{};
    auto r = read(loopExitFd_, &value, sizeof(value));
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to read from the event descriptor", -errno);
}

void Connection::joinWithProcessingLoop()
{
    if (asyncLoopThread_.joinable())
        asyncLoopThread_.join();
}

bool Connection::processPendingRequest()
{
    auto bus = bus_.get();

    assert(bus != nullptr);

    int r = sd_bus_process(bus, nullptr);

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to process bus requests", -r);

    return r > 0;
}

bool Connection::waitForNextRequest()
{
    auto bus = bus_.get();

    assert(bus != nullptr);
    assert(loopExitFd_ != 0);

    auto r = sd_bus_get_fd(bus);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus descriptor", -r);
    auto sdbusFd = r;

    r = sd_bus_get_events(bus);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus events", -r);
    short int sdbusEvents = r;

    uint64_t usec;
    sd_bus_get_timeout(bus, &usec);

    struct pollfd fds[] = {{sdbusFd, sdbusEvents, 0}, {loopExitFd_, POLLIN | POLLHUP | POLLERR | POLLNVAL, 0}};
    auto fdsCount = sizeof(fds)/sizeof(fds[0]);

    std::cerr << "[lt] Going to poll on fs " << sdbusFd << " with events " << sdbusEvents << ", and fs " << loopExitFd_ << " with timeout " << usec << " and fdscount == " << fdsCount << std::endl;
    r = poll(fds, fdsCount, usec == (uint64_t) -1 ? -1 : (usec+999)/1000);

    if (r < 0 && errno == EINTR)
    {
        std::cerr << "<<<>>>> GOT EINTR" << std::endl;
        return true; // Try again
    }

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to wait on the bus", -errno);

    if ((fds[1].revents & POLLHUP) || (fds[1].revents & POLLERR) || ((fds[1].revents & POLLNVAL)))
    {
        std::cerr << "!!!!!!!!!! Something went wrong on polling" << std::endl;
    }
    if (fds[1].revents & POLLIN)
    {
        clearExitNotification();
        return false;
    }

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
