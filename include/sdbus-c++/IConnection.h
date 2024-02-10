/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2022 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file IConnection.h
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

#ifndef SDBUS_CXX_ICONNECTION_H_
#define SDBUS_CXX_ICONNECTION_H_

#include <sdbus-c++/TypeTraits.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

struct sd_bus;
struct sd_event;
namespace sdbus {
  class Message;
}

namespace sdbus {

    /********************************************//**
     * @class IConnection
     *
     * An interface to D-Bus bus connection. Incorporates implementation
     * of both synchronous and asynchronous D-Bus I/O event loop.
     *
     * All methods throw sdbus::Error in case of failure. All methods in
     * this class are thread-aware, but not thread-safe.
     *
     ***********************************************/
    class IConnection
    {
    public:
        struct PollData;

        virtual ~IConnection() = default;

        /*!
         * @brief Requests D-Bus name on the connection
         *
         * @param[in] name Name to request
         *
         * @throws sdbus::Error in case of failure
         */
        virtual void requestName(const std::string& name) = 0;

        /*!
         * @brief Releases D-Bus name on the connection
         *
         * @param[in] name Name to release
         *
         * @throws sdbus::Error in case of failure
         */
        virtual void releaseName(const std::string& name) = 0;

        /*!
         * @brief Retrieves the unique name of a connection. E.g. ":1.xx"
         *
         * @throws sdbus::Error in case of failure
         */
        [[nodiscard]] virtual std::string getUniqueName() const = 0;

        /*!
         * @brief Enters I/O event loop on this bus connection
         *
         * The incoming D-Bus messages are processed in the loop. The method
         * blocks indefinitely, until unblocked through leaveEventLoop().
         *
         * @throws sdbus::Error in case of failure
         */
        virtual void enterEventLoop() = 0;

        /*!
         * @brief Enters I/O event loop on this bus connection in a separate thread
         *
         * The same as enterEventLoop, except that it doesn't block
         * because it runs the loop in a separate, internally managed thread.
         */
        virtual void enterEventLoopAsync() = 0;

        /*!
         * @brief Leaves the I/O event loop running on this bus connection
         *
         * This causes the loop to exit and frees the thread serving the loop
         *
         * @throws sdbus::Error in case of failure
         */
        virtual void leaveEventLoop() = 0;

        /*!
         * @brief Attaches the bus connection to an sd-event event loop
         *
         * @param[in] event sd-event event loop object
         * @param[in] priority Specified priority
         *
         * @throws sdbus::Error in case of failure
         *
         * See `man sd_bus_attach_event'.
         */
        virtual void attachSdEventLoop(sd_event *event, int priority = 0) = 0;

        /*!
         * @brief Detaches the bus connection from an sd-event event loop
         *
         * @throws sdbus::Error in case of failure
         */
        virtual void detachSdEventLoop() = 0;

        /*!
         * @brief Gets current sd-event event loop for the bus connection
         *
         * @return Pointer to event loop object if attached, nullptr otherwise
         */
        virtual sd_event *getSdEventLoop() = 0;

        /*!
         * @brief Returns fd's, I/O events and timeout data to be used in an external event loop
         *
         * This function is useful to hook up a bus connection object with an
         * external (like GMainLoop, boost::asio, etc.) or manual event loop
         * involving poll() or a similar I/O polling call.
         *
         * Before **each** invocation of the I/O polling call, this function
         * should be invoked. Returned PollData::fd file descriptor should
         * be polled for the events indicated by PollData::events, and the I/O
         * call should block for that up to the returned PollData::timeout.
         *
         * Additionally, returned PollData::eventFd should be polled for POLLIN
         * events.
         *
         * After each I/O polling call the bus connection needs to process
         * incoming or outgoing data, by invoking processPendingEvent().
         *
         * Note that the returned timeout should be considered only a maximum
         * sleeping time. It is permissible (and even expected) that shorter
         * timeouts are used by the calling program, in case other event sources
         * are polled in the same event loop. Note that the returned time-value
         * is absolute, based of CLOCK_MONOTONIC and specified in microseconds.
         * Use PollData::getPollTimeout() to have the timeout value converted
         * in a form that can be passed to poll(2).
         *
         * The bus connection conveniently integrates sd-event event loop.
         * To attach the bus connection to an sd-event event loop, use
         * attachSdEventLoop() function.
         *
         * @throws sdbus::Error in case of failure
         */
        [[nodiscard]] virtual PollData getEventLoopPollData() const = 0;

        /*!
         * @brief Processes a pending event
         *
         * @returns True if an event was processed, false if no operations were pending
         *
         * This function drives the D-Bus connection. It processes pending I/O events.
         * Queued outgoing messages (or parts thereof) are sent out. Queued incoming
         * messages are dispatched to registered callbacks. Timeouts are recalculated.
         *
         * It returns false when no operations were pending and true if a message was
         * processed. When false is returned the caller should synchronously poll for
         * I/O events before calling into processPendingEvent() again.
         * Don't forget to call getEventLoopPollData() each time before the next poll.
         *
         * You don't need to directly call this method or getEventLoopPollData() method
         * when using convenient, internal bus connection event loops through
         * enterEventLoop() or enterEventLoopAsync() calls, or when the bus is
         * connected to an sd-event event loop through attachSdEventLoop().
         * It is invoked automatically when necessary.
         *
         * @throws sdbus::Error in case of failure
         */
        virtual bool processPendingEvent() = 0;

        /*!
         * @brief Provides access to the currently processed D-Bus message
         *
         * This method provides access to the currently processed incoming D-Bus message.
         * "Currently processed" means that the registered callback handler(s) for that message
         * are being invoked. This method is meant to be called from within a callback handler
         * (e.g. from a D-Bus signal handler, or async method reply handler, etc.). In such a case it is
         * guaranteed to return a valid D-Bus message instance for which the handler is called.
         * If called from other contexts/threads, it may return a valid or invalid message, depending
         * on whether a message was processed or not at the time of the call.
         *
         * @return Currently processed D-Bus message
         */
        [[nodiscard]] virtual Message getCurrentlyProcessedMessage() const = 0;

        /*!
         * @brief Sets general method call timeout
         *
         * @param[in] timeout Timeout value in microseconds
         *
         * General method call timeout is used for all method calls upon this connection.
         * Method call-specific timeout overrides this general setting.
         *
         * Supported by libsystemd>=v240.
         *
         * @throws sdbus::Error in case of failure
         */
        virtual void setMethodCallTimeout(uint64_t timeout) = 0;

        /*!
         * @copydoc IConnection::setMethodCallTimeout(uint64_t)
         */
        template <typename _Rep, typename _Period>
        void setMethodCallTimeout(const std::chrono::duration<_Rep, _Period>& timeout);

        /*!
         * @brief Gets general method call timeout
         *
         * @return Timeout value in microseconds
         *
         * Supported by libsystemd>=v240.
         *
         * @throws sdbus::Error in case of failure
         */
        [[nodiscard]] virtual uint64_t getMethodCallTimeout() const = 0;

        /*!
         * @brief Adds an ObjectManager at the specified D-Bus object path
         *
         * Creates an ObjectManager interface at the specified object path on
         * the connection. This is a convenient way to interrogate a connection
         * to see what objects it has.
         *
         * This call creates a floating registration. The ObjectManager will
         * be there for the object path until the connection is destroyed.
         *
         * Another, recommended way to add object managers is directly through
         * IObject API.
         *
         * @throws sdbus::Error in case of failure
         */
        virtual void addObjectManager(const std::string& objectPath, floating_slot_t) = 0;

        /*!
         * @brief Installs a match rule for messages received on this bus connection
         *
         * @param[in] match Match expression to filter incoming D-Bus message
         * @param[in] callback Callback handler to be called upon processing an inbound D-Bus message matching the rule
         * @return RAII-style slot handle representing the ownership of the subscription
         *
         * The method installs a match rule for messages received on the specified bus connection.
         * The syntax of the match rule expression passed in match is described in the D-Bus specification.
         * The specified handler function callback is called for each incoming message matching the specified
         * expression. The match is installed synchronously when connected to a bus broker, i.e. the call
         * sends a control message requesting the match to be added to the broker and waits until the broker
         * confirms the match has been installed successfully.
         *
         * Simply let go of the slot instance to uninstall the match rule from the bus connection. The slot
         * must not outlive the connection for the slot is associated with it.
         *
         * For more information, consult `man sd_bus_add_match`.
         *
         * @throws sdbus::Error in case of failure
         */
        [[nodiscard]] virtual Slot addMatch(const std::string& match, message_handler callback) = 0;

        /*!
         * @brief Installs a floating match rule for messages received on this bus connection
         *
         * @param[in] match Match expression to filter incoming D-Bus message
         * @param[in] callback Callback handler to be called upon processing an inbound D-Bus message matching the rule
         *
         * The method installs a floating match rule for messages received on the specified bus connection.
         * Floating means that the bus connection object owns the match rule, i.e. lifetime of the match rule
         * is bound to the lifetime of the bus connection.
         *
         * Refer to the @c addMatch(const std::string& match, message_handler callback) documentation for more
         * information.
         *
         * @throws sdbus::Error in case of failure
         */
        virtual void addMatch(const std::string& match, message_handler callback, floating_slot_t) = 0;

        /*!
         * @brief Asynchronously installs a match rule for messages received on this bus connection
         *
         * @param[in] match Match expression to filter incoming D-Bus message
         * @param[in] callback Callback handler to be called upon processing an inbound D-Bus message matching the rule
         * @param[in] installCallback Callback handler to be called upon processing an inbound D-Bus message matching the rule
         * @return RAII-style slot handle representing the ownership of the subscription
         *
         * This method operates the same as `addMatch()` above, just that it installs the match rule asynchronously,
         * in a non-blocking fashion. A request is sent to the broker, but the call does not wait for a response.
         * The `installCallback' callable is called when the response is later received, with the response message
         * from the broker as parameter. If it's an empty function object, a default implementation is used that
         * terminates the bus connection should installing the match fail.
         *
         * Refer to the @c addMatch(const std::string& match, message_handler callback) documentation, and consult
         * `man sd_bus_add_match`, for more information.
         *
         * @throws sdbus::Error in case of failure
         */
        [[nodiscard]] virtual Slot addMatchAsync(const std::string& match, message_handler callback, message_handler installCallback) = 0;

        /*!
         * @brief Asynchronously installs a floating match rule for messages received on this bus connection
         *
         * @param[in] match Match expression to filter incoming D-Bus message
         * @param[in] callback Callback handler to be called upon processing an inbound D-Bus message matching the rule
         * @param[in] installCallback Callback handler to be called upon processing an inbound D-Bus message matching the rule
         *
         * The method installs a floating match rule for messages received on the specified bus connection.
         * Floating means that the bus connection object owns the match rule, i.e. lifetime of the match rule
         * is bound to the lifetime of the bus connection.
         *
         * Refer to the @c addMatch(const std::string& match, message_handler callback, message_handler installCallback)
         * documentation for more information.
         *
         * @throws sdbus::Error in case of failure
         */
        virtual void addMatchAsync(const std::string& match, message_handler callback, message_handler installCallback, floating_slot_t) = 0;

        /*!
         * @struct PollData
         *
         * Carries poll data needed for integration with external event loop implementations.
         *
         * See getEventLoopPollData() for more info.
         */
        struct PollData
        {
            /*!
             * The read fd to be monitored by the event loop.
             */
            int fd;

            /*!
             * The events to use for poll(2) alongside fd.
             */
            short int events;

            /*!
             * Absolute timeout value in microseconds, based of CLOCK_MONOTONIC.
             *
             * Call getPollTimeout() to get timeout recalculated to relative timeout that can be passed to poll(2).
             */
            std::chrono::microseconds timeout;

            /*!
             * An additional event fd to be monitored by the event loop for POLLIN events.
             */
            int eventFd;

            /*!
             * Returns the timeout as relative value from now.
             *
             * Returned value is std::chrono::microseconds::max() if the timeout is indefinite.
             *
             * @return Relative timeout as a time duration
             */
            [[nodiscard]] std::chrono::microseconds getRelativeTimeout() const;

            /*!
             * Returns relative timeout in the form which can be passed as argument 'timeout' to poll(2)
             *
             * @return -1 if the timeout is indefinite. 0 if the poll(2) shouldn't block.
             *         An integer in milliseconds otherwise.
             */
            [[nodiscard]] int getPollTimeout() const;
        };
    };

    template <typename _Rep, typename _Period>
    inline void IConnection::setMethodCallTimeout(const std::chrono::duration<_Rep, _Period>& timeout)
    {
        auto microsecs = std::chrono::duration_cast<std::chrono::microseconds>(timeout);
        return setMethodCallTimeout(microsecs.count());
    }

    /*!
     * @brief Creates/opens D-Bus system bus connection
     *
     * @return Connection instance
     *
     * @throws sdbus::Error in case of failure
     */
    [[nodiscard]] std::unique_ptr<sdbus::IConnection> createConnection();

    /*!
     * @brief Creates/opens D-Bus system bus connection with a name
     *
     * @param[in] name Name to request on the connection after its opening
     * @return Connection instance
     *
     * @throws sdbus::Error in case of failure
     */
    [[nodiscard]] std::unique_ptr<sdbus::IConnection> createConnection(const std::string& name);

    /*!
     * @brief Creates/opens D-Bus session bus connection when in a user context, and a system bus connection, otherwise.
     *
     * @return Connection instance
     *
     * @throws sdbus::Error in case of failure
     */
    [[nodiscard]] std::unique_ptr<sdbus::IConnection> createDefaultBusConnection();

    /*!
     * @brief Creates/opens D-Bus session bus connection with a name when in a user context, and a system bus connection with a name, otherwise.
     *
     * @param[in] name Name to request on the connection after its opening
     * @return Connection instance
     *
     * @throws sdbus::Error in case of failure
     */
    [[nodiscard]] std::unique_ptr<sdbus::IConnection> createDefaultBusConnection(const std::string& name);

    /*!
     * @brief Creates/opens D-Bus system bus connection
     *
     * @return Connection instance
     *
     * @throws sdbus::Error in case of failure
     */
    [[nodiscard]] std::unique_ptr<sdbus::IConnection> createSystemBusConnection();

    /*!
     * @brief Creates/opens D-Bus system bus connection with a name
     *
     * @param[in] name Name to request on the connection after its opening
     * @return Connection instance
     *
     * @throws sdbus::Error in case of failure
     */
    [[nodiscard]] std::unique_ptr<sdbus::IConnection> createSystemBusConnection(const std::string& name);

    /*!
     * @brief Creates/opens D-Bus session bus connection
     *
     * @return Connection instance
     *
     * @throws sdbus::Error in case of failure
     */
    [[nodiscard]] std::unique_ptr<sdbus::IConnection> createSessionBusConnection();

    /*!
     * @brief Creates/opens D-Bus session bus connection with a name
     *
     * @param[in] name Name to request on the connection after its opening
     * @return Connection instance
     *
     * @throws sdbus::Error in case of failure
     */
    [[nodiscard]] std::unique_ptr<sdbus::IConnection> createSessionBusConnection(const std::string& name);

    /*!
     * @brief Creates/opens D-Bus session bus connection at a custom address
     *
     * @param[in] address ";"-separated list of addresses of bus brokers to try to connect
     * @return Connection instance
     *
     * @throws sdbus::Error in case of failure
     *
     * Consult manual pages for `sd_bus_set_address` of the underlying sd-bus library for more information.
     */
    [[nodiscard]] std::unique_ptr<sdbus::IConnection> createSessionBusConnectionWithAddress(const std::string& address);

    /*!
     * @brief Creates/opens D-Bus system connection on a remote host using ssh
     *
     * @param[in] host Name of the host to connect
     * @return Connection instance
     *
     * @throws sdbus::Error in case of failure
     */
    [[nodiscard]] std::unique_ptr<sdbus::IConnection> createRemoteSystemBusConnection(const std::string& host);

    /*!
     * @brief Opens direct D-Bus connection at a custom address
     *
     * @param[in] address ";"-separated list of addresses of bus brokers to try to connect to
     * @return Connection instance
     *
     * @throws sdbus::Error in case of failure
     */
    [[nodiscard]] std::unique_ptr<sdbus::IConnection> createDirectBusConnection(const std::string& address);

    /*!
     * @brief Opens direct D-Bus connection at the given file descriptor
     *
     * @param[in] fd File descriptor used to communicate directly from/to a D-Bus server
     * @return Connection instance
     *
     * The underlying sdbus-c++ connection instance takes over ownership of fd, so the caller can let it go.
     * If, however, the call throws an exception, the ownership of fd remains with the caller.
     *
     * @throws sdbus::Error in case of failure
     */
    [[nodiscard]] std::unique_ptr<sdbus::IConnection> createDirectBusConnection(int fd);

    /*!
     * @brief Opens direct D-Bus connection at fd as a server
     *
     * @param[in] fd File descriptor to use for server DBus connection
     * @return Server connection instance
     *
     * This creates a new, custom bus object in server mode. One can then call createDirectBusConnection()
     * on client side to connect to this bus.
     *
     * The underlying sdbus-c++ connection instance takes over ownership of fd, so the caller can let it go.
     * If, however, the call throws an exception, the ownership of fd remains with the caller.
     *
     * @throws sdbus::Error in case of failure
     */
    [[nodiscard]] std::unique_ptr<sdbus::IConnection> createServerBus(int fd);

    /*!
     * @brief Creates sdbus-c++ bus connection representation out of underlying sd_bus instance
     *
     * @param[in] bus File descriptor to use for server DBus connection
     * @return Connection instance
     *
     * This functions is helpful in cases where clients need a custom, tweaked configuration of their
     * bus object. Since sdbus-c++ does not provide C++ API for all bus connection configuration
     * functions of the underlying sd-bus library, clients can use these sd-bus functions themselves
     * to create and configure their sd_bus object, and create sdbus-c++ IConnection on top of it.
     *
     * The IConnection instance assumes unique ownership of the provided bus object. The bus object
     * must have been started by the client before this call.
     * The bus object will get flushed, closed, and unreffed when the IConnection instance is destroyed.
     *
     * @throws sdbus::Error in case of failure
     *
     * Code example:
     * @code
     * sd_bus* bus{};
     * ::sd_bus_new(&bus);
     * ::sd_bus_set_address(bus, address);
     * ::sd_bus_set_anonymous(bus, true);
     * ::sd_bus_start(bus);
     * auto con = sdbus::createBusConnection(bus); // IConnection consumes sd_bus object
     * @endcode
     */
    [[nodiscard]] std::unique_ptr<sdbus::IConnection> createBusConnection(sd_bus *bus);
}

#endif /* SDBUS_CXX_ICONNECTION_H_ */
