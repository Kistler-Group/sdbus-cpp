/**
 * (C) 2016 - 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2019 Stanislav Angelovic <angelovic.s@gmail.com>
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

#include <string>
#include <memory>
#include <chrono>
#include <cstdint>
#include <vector>

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
        struct PollData
        {
            int fd;
            short int events;
            uint64_t timeout_usec;
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

        /*
        virtual uid_t getCredsUid(std::string bus_name) const = 0;
        virtual uid_t getCredsEuid(std::string bus_name) const = 0;
        virtual gid_t getCredsGid(std::string bus_name) const = 0;
        virtual std::vector<gid_t> getCredsSupplementaryGids(std::string bus_name) const = 0;
        */

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
         * @throws sdbus::Error in case of failure
         */
        virtual void addObjectManager(const std::string& objectPath) = 0;

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
         * @copydoc IConnection::enterEventLoop()
         *
         * @deprecated This function has been replaced by enterEventLoop()
         */
        [[deprecated("This function has been replaced by enterEventLoop()")]] void enterProcessingLoop();

        /*!
         * @copydoc IConnection::enterProcessingLoopAsync()
         *
         * @deprecated This function has been replaced by enterEventLoopAsync()
         */
        [[deprecated("This function has been replaced by enterEventLoopAsync()")]] void enterProcessingLoopAsync();

        /*!
         * @copydoc IConnection::leaveProcessingLoop()
         *
         * @deprecated This function has been replaced by leaveEventLoop()
         */
        [[deprecated("This function has been replaced by leaveEventLoop()")]] void leaveProcessingLoop();

        /*!
         * @copydoc IConnection::getProcessLoopPollData()
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
     * @brief Creates/opens D-Bus system connection
     *
     * @return Connection instance
     *
     * @throws sdbus::Error in case of failure
     */
    [[nodiscard]] std::unique_ptr<sdbus::IConnection> createConnection();

    /*!
     * @brief Creates/opens D-Bus system connection with a name
     *
     * @param[in] name Name to request on the connection after its opening
     * @return Connection instance
     *
     * @throws sdbus::Error in case of failure
     */
    [[nodiscard]] std::unique_ptr<sdbus::IConnection> createConnection(const std::string& name);

    /*!
     * @brief Creates/opens D-Bus system connection
     *
     * @return Connection instance
     *
     * @throws sdbus::Error in case of failure
     */
    [[nodiscard]] std::unique_ptr<sdbus::IConnection> createSystemBusConnection();

    /*!
     * @brief Creates/opens D-Bus system connection with a name
     *
     * @param[in] name Name to request on the connection after its opening
     * @return Connection instance
     *
     * @throws sdbus::Error in case of failure
     */
    [[nodiscard]] std::unique_ptr<sdbus::IConnection> createSystemBusConnection(const std::string& name);

    /*!
     * @brief Creates/opens D-Bus session connection
     *
     * @return Connection instance
     *
     * @throws sdbus::Error in case of failure
     */
    [[nodiscard]] std::unique_ptr<sdbus::IConnection> createSessionBusConnection();

    /*!
     * @brief Creates/opens D-Bus session connection with a name
     *
     * @param[in] name Name to request on the connection after its opening
     * @return Connection instance
     *
     * @throws sdbus::Error in case of failure
     */
    [[nodiscard]] std::unique_ptr<sdbus::IConnection> createSessionBusConnection(const std::string& name);

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
