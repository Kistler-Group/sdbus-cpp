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

//#include <cstdint>
#include <string>
#include <memory>

namespace sdbus {

    /********************************************//**
     * @class IConnection
     *
     * An interface to D-Bus bus connection. Incorporates implementation
     * of both synchronous and asynchronous processing loop.
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
         * @brief Enters the D-Bus processing loop
         *
         * The incoming D-Bus messages are processed in the loop. The method
         * blocks indefinitely, until unblocked via leaveProcessingLoop.
         *
         * @throws sdbus::Error in case of failure
         */
        virtual void enterProcessingLoop() = 0;

        /*!
         * @brief Enters the D-Bus processing loop in a separate thread
         *
         * The same as enterProcessingLoop, except that it doesn't block
         * because it runs the loop in a separate thread managed internally.
         */
        virtual void enterProcessingLoopAsync() = 0;

        /*!
         * @brief Leaves the D-Bus processing loop
         *
         * Ends the previously started processing loop.
         *
         * @throws sdbus::Error in case of failure
         */
        virtual void leaveProcessingLoop() = 0;

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
         * @brief Returns parameters you can pass to poll
         *
         * To integrate sdbus with your app's own custom event handling system
         * (without the requirement of an extra thread), you can use this
         * method to query which file descriptors, poll events and timeouts you
         * should add to your app's poll call in your main event loop. If these
         * file descriptors signal, then you should call processPendingRequest
         * to process the event. This means that all of sdbus's callbacks will
         * arrive on your app's main event thread (opposed to on a thread created
         * by sdbus-c++). If you are unsure what this all means then use
         * enterProcessingLoop() or enterProcessingLoopAsync() instead.
         *
         * To integrate sdbus-c++ into a gtk app, pass the file descriptor returned
         * by this method to g_main_context_add_poll.
         *
         * @throws sdbus::Error in case of failure
         */
        virtual PollData getProcessLoopPollData() = 0;

        /*!
         * @brief Process a pending request
         *
         * Processes a single dbus event. All of sdbus-c++'s callbacks will be called
         * from within this method. This method should ONLY be used in conjuction
         * with getProcessLoopPollData(). enterProcessingLoop() and
         * enterProcessingLoopAsync() will call this method for you, so there is no
         * need to call it when using these. If you are unsure what this all means then
         * don't use this method.
         *
         * @returns true if an event was processed
         * @throws sdbus::Error in case of failure
         */
        virtual bool processPendingRequest() = 0;
    };

    /*!
     * @brief Creates/opens D-Bus system connection
     *
     * @return Connection instance
     *
     * @throws sdbus::Error in case of failure
     */
    std::unique_ptr<sdbus::IConnection> createConnection();

    /*!
     * @brief Creates/opens D-Bus system connection with a name
     *
     * @param[in] name Name to request on the connection after its opening
     * @return Connection instance
     *
     * @throws sdbus::Error in case of failure
     */
    std::unique_ptr<sdbus::IConnection> createConnection(const std::string& name);

    /*!
     * @brief Creates/opens D-Bus system connection
     *
     * @return Connection instance
     *
     * @throws sdbus::Error in case of failure
     */
    std::unique_ptr<sdbus::IConnection> createSystemBusConnection();

    /*!
     * @brief Creates/opens D-Bus system connection with a name
     *
     * @param[in] name Name to request on the connection after its opening
     * @return Connection instance
     *
     * @throws sdbus::Error in case of failure
     */
    std::unique_ptr<sdbus::IConnection> createSystemBusConnection(const std::string& name);

    /*!
     * @brief Creates/opens D-Bus session connection
     *
     * @return Connection instance
     *
     * @throws sdbus::Error in case of failure
     */
    std::unique_ptr<sdbus::IConnection> createSessionBusConnection();

    /*!
     * @brief Creates/opens D-Bus session connection with a name
     *
     * @param[in] name Name to request on the connection after its opening
     * @return Connection instance
     *
     * @throws sdbus::Error in case of failure
     */
    std::unique_ptr<sdbus::IConnection> createSessionBusConnection(const std::string& name);

    /*!
     * @brief Creates/opens D-Bus system connection on a remote host using ssh
     *
     * @param[in] host Name of the host to connect
     * @return Connection instance
     *
     * @throws sdbus::Error in case of failure
     */
    std::unique_ptr<sdbus::IConnection> createRemoteSystemBusConnection(const std::string& host);
}

#endif /* SDBUS_CXX_ICONNECTION_H_ */
