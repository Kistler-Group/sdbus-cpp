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

    notificationFd_ = createLoopNotificationDescriptor();
    std::cerr << "Created eventfd " << notificationFd_ << " of " << this << std::endl;
}

Connection::~Connection()
{
    leaveProcessingLoop();
    std::cerr << "Closing eventfd " << notificationFd_ << " of " << this << std::endl;
    closeLoopNotificationDescriptor(notificationFd_);
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
    loopThreadId_ = std::this_thread::get_id();

    std::lock_guard<std::mutex> guard(loopMutex_);

    while (true)
    {
        auto processed = processPendingRequest();
        if (processed)
            continue; // Process next one

        auto success = waitForNextRequest();
        if (!success)
            break; // Exit processing loop
        if (success.asyncMsgsToProcess)
            processUserRequests();
    }

    loopThreadId_ = std::thread::id{};
}

void Connection::enterProcessingLoopAsync()
{
    std::cerr << "--> enterProcessingLoopAsync() for connection " << this << std::endl;
    // TODO: Check that joinable() means a valid non-empty thread
    //if (!asyncLoopThread_.joinable())
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


template<typename _Callable, typename... _Args, std::enable_if_t<std::is_void<function_result_t<_Callable>>::value, int>>
inline auto Connection::tryExecuteSync(_Callable&& fnc, const _Args&... args)
{
    std::thread::id loopThreadId = loopThreadId_.load(std::memory_order_relaxed);

    // Is the loop not yet on? => Go make synchronous call
    while (loopThreadId == std::thread::id{})
    {
        // Did the loop begin in the meantime? Or try_lock() failed spuriously?
        if (!loopMutex_.try_lock())
        {
            loopThreadId = loopThreadId_.load(std::memory_order_relaxed);
            continue;
        }

        // Synchronous call
        std::lock_guard<std::mutex> guard(loopMutex_, std::adopt_lock);
        sdbus::invoke(std::forward<_Callable>(fnc), args...);
        return true;
    }

    // Is the loop on and we are in the same thread? => Go for synchronous call
    if (loopThreadId == std::this_thread::get_id())
    {
        assert(!loopMutex_.try_lock());
        sdbus::invoke(std::forward<_Callable>(fnc), args...);
        return true;
    }

    return false;
}

template<typename _Callable, typename... _Args, std::enable_if_t<!std::is_void<function_result_t<_Callable>>::value, int>>
inline auto Connection::tryExecuteSync(_Callable&& fnc, const _Args&... args)
{
    std::thread::id loopThreadId = loopThreadId_.load(std::memory_order_relaxed);

    // Is the loop not yet on? => Go make synchronous call
    while (loopThreadId == std::thread::id{})
    {
        // Did the loop begin in the meantime? Or try_lock() failed spuriously?
        if (!loopMutex_.try_lock())
        {
            loopThreadId = loopThreadId_.load(std::memory_order_relaxed);
            continue;
        }

        // Synchronous call
        std::lock_guard<std::mutex> guard(loopMutex_, std::adopt_lock);
        return std::make_pair(true, sdbus::invoke(std::forward<_Callable>(fnc), args...));
    }

    // Is the loop on and we are in the same thread? => Go for synchronous call
    if (loopThreadId == std::this_thread::get_id())
    {
        assert(!loopMutex_.try_lock());
        return std::make_pair(true, sdbus::invoke(std::forward<_Callable>(fnc), args...));
    }

    return std::make_pair(false, function_result_t<_Callable>{});
}

template<typename _Callable, typename... _Args, std::enable_if_t<std::is_void<function_result_t<_Callable>>::value, int>>
inline void Connection::executeAsync(_Callable&& fnc, const _Args&... args)
{
    std::promise<void> result;
    auto future = result.get_future();

    queueUserRequest([fnc = std::forward<_Callable>(fnc), args..., &result]()
    {
        SCOPE_EXIT_NAMED(onSdbusError){ result.set_exception(std::current_exception()); };

        std::cerr << "  [lt] ... Invoking void request from within event loop thread..." << std::endl;
        sdbus::invoke(fnc, args...);
        std::cerr << "  [lt] Request invoked" << std::endl;
        result.set_value();

        onSdbusError.dismiss();
    });

    // Wait for the the processing loop thread to process the request
    future.get();
}

template<typename _Callable, typename... _Args, std::enable_if_t<!std::is_void<function_result_t<_Callable>>::value, int>>
inline auto Connection::executeAsync(_Callable&& fnc, const _Args&... args)
{
    std::promise<function_result_t<_Callable>> result;
    auto future = result.get_future();

    queueUserRequest([fnc = std::forward<_Callable>(fnc), args..., &result]()
    {
        SCOPE_EXIT_NAMED(onSdbusError){ result.set_exception(std::current_exception()); };

        std::cerr << "  [lt] ... Invoking request from within event loop thread..." << std::endl;
        auto returnValue = sdbus::invoke(fnc, args...);
        std::cerr << "  [lt] Request invoked and got result" << std::endl;
        result.set_value(returnValue);

        onSdbusError.dismiss();
    });

    // Wait for the reply from the processing loop thread
    return future.get();
}

template<typename _Callable, typename... _Args>
inline void Connection::executeAsyncAndDontWaitForResult(_Callable&& fnc, const _Args&... args)
{
    queueUserRequest([fnc = std::forward<_Callable>(fnc), args...]()
    {
        sdbus::invoke(fnc, args...);
    });
}

void* Connection::registerSignalHandler( const std::string& objectPath
                                       , const std::string& interfaceName
                                       , const std::string& signalName
                                       , sd_bus_message_handler_t callback
                                       , void* userData )
{
    auto registerSignalHandler = [this]( const std::string& objectPath
                                       , const std::string& interfaceName
                                       , const std::string& signalName
                                       , sd_bus_message_handler_t callback
                                       , void* userData )
    {
        sd_bus_slot *slot{};

        auto filter = composeSignalMatchFilter(objectPath, interfaceName, signalName);
        auto r = sd_bus_add_match(bus_.get(), &slot, filter.c_str(), callback, userData);
        std::cerr << "Registered signal " << signalName << " with slot " << slot << std::endl;

        SDBUS_THROW_ERROR_IF(r < 0, "Failed to register signal handler", -r);

        return slot;
    };

    std::cerr << "Trying to register signal " << signalName << " synchronously..." << std::endl;
    auto result = tryExecuteSync(registerSignalHandler, objectPath, interfaceName, signalName, callback, userData);
    if (!result.first) std::cerr << "  ... Nope, going async way" << std::endl;
    return result.first ? result.second
                        : executeAsync(registerSignalHandler, objectPath, interfaceName, signalName, callback, userData);
}

void Connection::unregisterSignalHandler(void* handlerCookie)
{
    auto result = tryExecuteSync(sd_bus_slot_unref, (sd_bus_slot *)handlerCookie);
//    if (!result.first)
//        executeAsync(sd_bus_slot_unref, (sd_bus_slot *)handlerCookie);
    if (result.first)
    {
        std::cerr << "Synchronously unregistered signal " << handlerCookie << ": " << result.second << std::endl;
        return;
    }
    auto slot = executeAsync(sd_bus_slot_unref, (sd_bus_slot *)handlerCookie);
    std::cerr << "Asynchronously unregistered signal " << handlerCookie << ": " << slot << std::endl;
}

MethodReply Connection::callMethod(const MethodCall& message)
{
    std::cerr << "Trying to call method synchronously..." << std::endl;
    auto result = tryExecuteSync(&MethodCall::send, message);
    if (!result.first) std::cerr << "  ... Nope, going async way" << std::endl;
    return result.first ? result.second
                        : executeAsync(&MethodCall::send, message);
}

void Connection::callMethod(const AsyncMethodCall& message, void* callback, void* userData)
{
    auto result = tryExecuteSync(&AsyncMethodCall::send, message, callback, userData);
    if (!result)
        executeAsyncAndDontWaitForResult(&AsyncMethodCall::send, message, callback, userData);
}

void Connection::sendMethodReply(const MethodReply& message)
{
    auto result = tryExecuteSync(&MethodReply::send, message);
    if (!result)
        executeAsyncAndDontWaitForResult(&MethodReply::send, message);
}

void Connection::emitSignal(const Signal& message)
{
    auto result = tryExecuteSync(&Signal::send, message);
    if (!result)
        executeAsyncAndDontWaitForResult(&Signal::send, message);
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

int Connection::createLoopNotificationDescriptor()
{
    auto r = eventfd(0, EFD_SEMAPHORE | EFD_CLOEXEC | EFD_NONBLOCK);

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to create event object", -errno);

    return r;
}

void Connection::closeLoopNotificationDescriptor(int fd)
{
    close(fd);
}

void Connection::notifyProcessingLoop()
{
    assert(notificationFd_ >= 0);

    for (int i = 0; i < 1; ++i)
    {
        //std::this_thread::sleep_for(std::chrono::milliseconds(5));
        uint64_t value = 1;
        auto r = write(notificationFd_, &value, sizeof(value));
        std::cerr << "Wrote to notification fd " << notificationFd_ << std::endl;
        SDBUS_THROW_ERROR_IF(r < 0, "Failed to notify processing loop", -errno);
    }
}

void Connection::notifyProcessingLoopToExit()
{
    exitLoopThread_ = true;

    notifyProcessingLoop();
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

void Connection::queueUserRequest(UserRequest&& request)
{
    {
        std::lock_guard<std::mutex> guard(userRequestsMutex_);
        userRequests_.push(std::move(request));
        std::cerr << "Pushed to user request queue. Size: " << userRequests_.size() << std::endl;
    }
    notifyProcessingLoop();
}

void Connection::processUserRequests()
{
    std::lock_guard<std::mutex> guard(userRequestsMutex_);
    while (!userRequests_.empty())
    {
        auto& request = userRequests_.front();
        request();
        userRequests_.pop();
    }
}

Connection::WaitResult Connection::waitForNextRequest()
{
    auto bus = bus_.get();

    assert(bus != nullptr);
    assert(notificationFd_ != 0);

    auto r = sd_bus_get_fd(bus);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus descriptor", -r);
    auto sdbusFd = r;

    r = sd_bus_get_events(bus);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus events", -r);
    short int sdbusEvents = r;

    uint64_t usec;
    sd_bus_get_timeout(bus, &usec);

    struct pollfd fds[] = {{sdbusFd, sdbusEvents, 0}, {notificationFd_, POLLIN | POLLHUP | POLLERR | POLLNVAL, 0}};
    auto fdsCount = sizeof(fds)/sizeof(fds[0]);

    std::cerr << "[lt] Going to poll on fs " << sdbusFd << ", " << notificationFd_ << " with timeout " << usec << " and fdscount == " << fdsCount << std::endl;
    r = poll(fds, fdsCount, usec == (uint64_t) -1 ? -1 : (usec+999)/1000);

    if (r < 0 && errno == EINTR)
    {
        std::cerr << "<<<>>>> GOT EINTR" << std::endl;
        return {true, false}; // Try again
    }

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to wait on the bus", -errno);

    if ((fds[1].revents & POLLHUP) || (fds[1].revents & POLLERR) || ((fds[1].revents & POLLNVAL)))
    {
        std::cerr << "!!!!!!!!!! Something went wrong on polling" << std::endl;
    }
    if (fds[1].revents & POLLIN)
    {
        if (exitLoopThread_)
            return {false, false}; // Got exit notification

        // Otherwise we have some user requests to process
        std::cerr << "Loop found it has some async requests to process" << std::endl;

        uint64_t value{};
        auto r = read(notificationFd_, &value, sizeof(value));
        SDBUS_THROW_ERROR_IF(r < 0, "Failed to read from the event descriptor", -errno);

        return {false, true};
    }

    return {true, false};
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
