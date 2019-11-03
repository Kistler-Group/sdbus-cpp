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
    };

    template <typename _Rep, typename _Period>
    inline void IConnection::setMethodCallTimeout(const std::chrono::duration<_Rep, _Period>& timeout)
    {
        auto microsecs = std::chrono::duration_cast<std::chrono::microseconds>(timeout);
        return setMethodCallTimeout(microsecs.count());
    }

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

}

#endif /* SDBUS_CXX_ICONNECTION_H_ */
