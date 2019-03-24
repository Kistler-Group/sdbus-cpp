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
#include "SdBus.h"
#include <sdbus-c++/Message.h>
#include <sdbus-c++/Error.h>
#include "ScopeGuard.h"
#include <systemd/sd-bus.h>
#include <unistd.h>
#include <poll.h>
#include <sys/eventfd.h>

namespace sdbus { namespace internal {

Connection::Connection(Connection::BusType type, std::unique_ptr<ISdBus>&& interface)
    : iface_(std::move(interface))
    , busType_(type)
{
    assert(iface_ != nullptr);

    auto bus = openBus(busType_);
    bus_.reset(bus);

    finishHandshake(bus);

    loopExitFd_ = createProcessingLoopExitDescriptor();
}

Connection::~Connection()
{
    leaveProcessingLoop();
    closeProcessingLoopExitDescriptor(loopExitFd_);
}

void Connection::requestName(const std::string& name)
{
    auto r = iface_->sd_bus_request_name(bus_.get(), name.c_str(), 0);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to request bus name", -r);
}

void Connection::releaseName(const std::string& name)
{
    auto r = iface_->sd_bus_release_name(bus_.get(), name.c_str());
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to release bus name", -r);
}

void Connection::enterProcessingLoop()
{
    while (true)
    {
        auto processed = processPendingRequest();
        if (processed)
            continue; // Process next one

        auto success = waitForNextRequest();
        if (!success)
            break; // Exit processing loop
    }
}

void Connection::enterProcessingLoopAsync()
{
    if (!asyncLoopThread_.joinable())
        asyncLoopThread_ = std::thread([this](){ enterProcessingLoop(); });
}

void Connection::leaveProcessingLoop()
{
    notifyProcessingLoopToExit();
    joinWithProcessingLoop();
}

const ISdBus& Connection::getSdBusInterface() const
{
    return *iface_.get();
}

ISdBus& Connection::getSdBusInterface()
{
    return *iface_.get();
}

sd_bus_slot* Connection::addObjectVTable( const std::string& objectPath
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

    return slot;
}

void Connection::removeObjectVTable(sd_bus_slot* vtableHandle)
{
    iface_->sd_bus_slot_unref(vtableHandle);
}

MethodCall Connection::createMethodCall( const std::string& destination
                                       , const std::string& objectPath
                                       , const std::string& interfaceName
                                       , const std::string& methodName ) const
{
    sd_bus_message *sdbusMsg{};

    auto r = iface_->sd_bus_message_new_method_call( bus_.get()
                                                   , &sdbusMsg
                                                   , destination.c_str()
                                                   , objectPath.c_str()
                                                   , interfaceName.c_str()
                                                   , methodName.c_str() );

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to create method call", -r);

    return MethodCall{sdbusMsg, iface_.get(), adopt_message};
}

Signal Connection::createSignal( const std::string& objectPath
                               , const std::string& interfaceName
                               , const std::string& signalName ) const
{
    sd_bus_message *sdbusSignal{};

    auto r = iface_->sd_bus_message_new_signal( bus_.get()
                                              , &sdbusSignal
                                              , objectPath.c_str()
                                              , interfaceName.c_str()
                                              , signalName.c_str() );

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to create signal", -r);

    return Signal{sdbusSignal, iface_.get(), adopt_message};
}

sd_bus_slot* Connection::registerSignalHandler( const std::string& objectPath
                                              , const std::string& interfaceName
                                              , const std::string& signalName
                                              , sd_bus_message_handler_t callback
                                              , void* userData )
{
    sd_bus_slot *slot{};

    auto filter = composeSignalMatchFilter(objectPath, interfaceName, signalName);
    auto r = iface_->sd_bus_add_match(bus_.get(), &slot, filter.c_str(), callback, userData);

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to register signal handler", -r);

    return slot;
}

void Connection::unregisterSignalHandler(sd_bus_slot* handlerCookie)
{
    iface_->sd_bus_slot_unref(handlerCookie);
}

sd_bus* Connection::openBus(Connection::BusType type)
{
    sd_bus* bus{};
    int r = 0;
    if (type == BusType::eSystem)
        r = iface_->sd_bus_open_system(&bus);
    else if (type == BusType::eSession)
        r = iface_->sd_bus_open_user(&bus);
    else
        assert(false);

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

    auto r = iface_->sd_bus_flush(bus);

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

    int r = iface_->sd_bus_process(bus, nullptr);

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to process bus requests", -r);

    return r > 0;
}

bool Connection::waitForNextRequest()
{
    auto bus = bus_.get();

    assert(bus != nullptr);
    assert(loopExitFd_ != 0);

    ISdBus::PollData sdbusPollData;
    auto r = iface_->sd_bus_get_poll_data(bus, &sdbusPollData);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus poll data", -r);

    struct pollfd fds[] = {{sdbusPollData.fd, sdbusPollData.events, 0}, {loopExitFd_, POLLIN, 0}};
    auto fdsCount = sizeof(fds)/sizeof(fds[0]);

    auto timeout = sdbusPollData.timeout_usec == (uint64_t) -1 ? (uint64_t)-1 : (sdbusPollData.timeout_usec+999)/1000;
    r = poll(fds, fdsCount, timeout);

    if (r < 0 && errno == EINTR)
        return true; // Try again

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to wait on the bus", -errno);

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
    auto interface = std::make_unique<sdbus::internal::SdBus>();
    assert(interface != nullptr);
    return std::make_unique<sdbus::internal::Connection>( sdbus::internal::Connection::BusType::eSystem
                                                        , std::move(interface));
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
    assert(interface != nullptr);
    return std::make_unique<sdbus::internal::Connection>( sdbus::internal::Connection::BusType::eSession
                                                        , std::move(interface));
}

std::unique_ptr<sdbus::IConnection> createSessionBusConnection(const std::string& name)
{
    auto conn = createSessionBusConnection();
    conn->requestName(name);
    return conn;
}

}
