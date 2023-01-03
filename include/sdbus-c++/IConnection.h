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
#include <string>
#include <memory>
#include <chrono>
#include <cstdint>
#include <optional>

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
        /*!
         * Poll Data for external event loop implementations.
         *
         * To integrate sdbus with your app's own custom event handling system
         * you can use this method to query which file descriptors, poll events
         * and timeouts you should add to your app's poll(2), or select(2)
         * call in your main event loop.
         *
         * If you are unsure what this all means then use
         * enterEventLoop() or enterEventLoopAsync() instead.
         *
         * See: getEventLoopPollData()
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
             * Absolute timeout value in micro seconds and based of CLOCK_MONOTONIC.
             */
            uint64_t timeout_usec;

            /*!
             * Get the event poll timeout.
             *
             * The timeout is an absolute value based of CLOCK_MONOTONIC.
             *
             * @return a duration since the CLOCK_MONOTONIC epoch started.
             */
            [[nodiscard]] std::chrono::microseconds getAbsoluteTimeout() const
            {
                return std::chrono::microseconds(timeout_usec);
            }

            /*!
             * Get the timeout as relative value from now
             *
             * @return std::nullopt if the timeout is indefinite. A duration otherwise.
             */
            [[nodiscard]] std::optional<std::chrono::microseconds> getRelativeTimeout() const;

            /*!
             * Get a converted, relative timeout which can be passed as argument 'timeout' to poll(2)
             *
             * @return -1 if the timeout is indefinite. 0 if the poll(2) shouldn't block. An integer in milli
             * seconds otherwise.
             */
            [[nodiscard]] int getPollTimeout() const;
        };

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
         * @brief Retrieve the unique name of a connection. E.g. ":1.xx"
         *
         * @throws sdbus::Error in case of failure
         */
        virtual std::string getUniqueName() const = 0;

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
        [[deprecated("Use one of other addObjectManager overloads")]] virtual void addObjectManager(const std::string& objectPath) = 0;

        /*!
         * @brief Returns fd, I/O events and timeout data you can pass to poll
         *
         * To integrate sdbus with your app's own custom event handling system
         * (without the requirement of an extra thread), you can use this
         * method to query which file descriptors, poll events and timeouts you
         * should add to your app's poll call in your main event loop. If these
         * file descriptors signal, then you should call processPendingRequest
         * to process the event. This means that all of sdbus's callbacks will
         * arrive on your app's main event thread (opposed to on a thread created
         * by sdbus-c++). If you are unsure what this all means then use
         * enterEventLoop() or enterEventLoopAsync() instead.
         *
         * To integrate sdbus-c++ into a gtk app, pass the file descriptor returned
         * by this method to g_main_context_add_poll.
         *
         * @throws sdbus::Error in case of failure
         */
        virtual PollData getEventLoopPollData() const = 0;

        /*!
         * @brief Process a pending request
         *
         * @returns true if an event was processed, false if poll should be called
         *
         * Processes a single dbus event. All of sdbus-c++'s callbacks will be called
         * from within this method. This method should ONLY be used in conjuction
         * with getEventLoopPollData().
         * This method returns true if an I/O message was processed. This you can try
         * to call this method again before going to poll on I/O events. The method
         * returns false if no operations were pending, and the caller should then
         * poll for I/O events before calling this method again.
         * enterEventLoop() and enterEventLoopAsync() will call this method for you,
         * so there is no need to call it when using these. If you are unsure what
         * this all means then don't use this method.
         *
         * @throws sdbus::Error in case of failure
         */
        virtual bool processPendingRequest() = 0;

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
        virtual uint64_t getMethodCallTimeout() const = 0;

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
         * @brief Adds a match rule for incoming message dispatching
         *
         * @param[in] match Match expression to filter incoming D-Bus message
         * @param[in] callback Callback handler to be called upon incoming D-Bus message matching the rule
         * @return RAII-style slot handle representing the ownership of the subscription
         *
         * The method installs a match rule for messages received on the specified bus connection.
         * The syntax of the match rule expression passed in match is described in the D-Bus specification.
         * The specified handler function callback is called for each incoming message matching the specified
         * expression. The match is installed synchronously when connected to a bus broker, i.e. the call
         * sends a control message requested the match to be added to the broker and waits until the broker
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
         * @brief Adds a floating match rule for incoming message dispatching
         *
         * @param[in] match Match expression to filter incoming D-Bus message
         * @param[in] callback Callback handler to be called upon incoming D-Bus message matching the rule
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
         * @copydoc IConnection::enterEventLoop()
         *
         * @deprecated This function has been replaced by enterEventLoop()
         */
        [[deprecated("This function has been replaced by enterEventLoop()")]] void enterProcessingLoop();

        /*!
         * @copydoc IConnection::enterEventLoopAsync()
         *
         * @deprecated This function has been replaced by enterEventLoopAsync()
         */
        [[deprecated("This function has been replaced by enterEventLoopAsync()")]] void enterProcessingLoopAsync();

        /*!
         * @copydoc IConnection::leaveEventLoop()
         *
         * @deprecated This function has been replaced by leaveEventLoop()
         */
        [[deprecated("This function has been replaced by leaveEventLoop()")]] void leaveProcessingLoop();

        /*!
         * @copydoc IConnection::getEventLoopPollData()
         *
         * @deprecated This function has been replaced by getEventLoopPollData()
         */
        [[deprecated("This function has been replaced by getEventLoopPollData()")]] PollData getProcessLoopPollData() const;
    };

    template <typename _Rep, typename _Period>
    inline void IConnection::setMethodCallTimeout(const std::chrono::duration<_Rep, _Period>& timeout)
    {
        auto microsecs = std::chrono::duration_cast<std::chrono::microseconds>(timeout);
        return setMethodCallTimeout(microsecs.count());
    }

    inline void IConnection::enterProcessingLoop()
    {
        enterEventLoop();
    }

    inline void IConnection::enterProcessingLoopAsync()
    {
        enterEventLoopAsync();
    }

    inline void IConnection::leaveProcessingLoop()
    {
        leaveEventLoop();
    }

    inline IConnection::PollData IConnection::getProcessLoopPollData() const
    {
        return getEventLoopPollData();
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
}

#endif /* SDBUS_CXX_ICONNECTION_H_ */
