/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 *
 * @file IObjectProxy.h
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

#ifndef SDBUS_CXX_IOBJECTPROXY_H_
#define SDBUS_CXX_IOBJECTPROXY_H_

#include <sdbus-c++/ConvenienceClasses.h>
#include <string>
#include <memory>
#include <functional>

// Forward declarations
namespace sdbus {
    class Message;
    class IConnection;
}

namespace sdbus {

    /********************************************//**
     * @class IObjectProxy
     *
     * An interface to D-Bus object proxy. Provides API for calling
     * methods, getting/setting properties, and for registering to signals.
     *
     * All methods throw @c sdbus::Error in case of failure. The class is
     * thread-aware, but not thread-safe.
     *
     ***********************************************/
    class IObjectProxy
    {
    public:
        /*!
        * @brief Creates a method call message
        *
        * @param[in] interfaceName Name of an interface that the method is defined under
        * @param[in] methodName Name of the method
        * @return A method call message message
        *
        * Serialize method arguments into the returned message and invoke the method by passing
        * the message with serialized arguments to the @c callMethod function.
        * Alternatively, use higher-level API @c callMethod(const std::string& methodName) defined below.
        *
        * @throws sdbus::Error in case of failure
        */
        virtual Message createMethodCall(const std::string& interfaceName, const std::string& methodName) = 0;

        /*!
        * @brief Calls method on the proxied D-Bus object
        *
        * @param[in] message Message representing a method call
        *
        * Note: To avoid messing with messages, use higher-level API defined below.
        *
        * @throws sdbus::Error in case of failure
        */
        virtual Message callMethod(const sdbus::Message& message) = 0;

        /*!
        * @brief Registers a handler for the desired signal emitted by the proxied D-Bus object
        *
        * @param[in] interfaceName Name of an interface that the signal belongs to
        * @param[in] signalName Name of the signal
        * @param[in] signalHandler Callback that implements the body of the signal handler
        *
        * @throws sdbus::Error in case of failure
        */
        virtual void registerSignalHandler( const std::string& interfaceName
                                          , const std::string& signalName
                                          , signal_handler signalHandler ) = 0;

        /*!
        * @brief Finishes the registration of signal handlers
        *
        * The method physically subscribes to the desired signals.
        * Must be called only once, after all signals have been registered already.
        *
        * @throws sdbus::Error in case of failure
        */
        virtual void finishRegistration() = 0;

        /*!
        * @brief Calls method on the proxied D-Bus object
        *
        * @param[in] methodName Name of the method
        * @return A helper object for convenient invocation of the method
        *
        * This is a high-level, convenience way of calling D-Bus methods that abstracts
        * from the D-Bus message concept. Method arguments/return value are automatically (de)serialized
        * in a message and D-Bus signatures automatically deduced from the provided native arguments
        * and return values.
        *
        * Example of use:
        * @code
        * int result, a = ..., b = ...;
        * object_.callMethod("multiply").onInterface(INTERFACE_NAME).withArguments(a, b).storeResultsTo(result);
        * @endcode
        *
        * @throws sdbus::Error in case of failure
        */
        MethodInvoker callMethod(const std::string& methodName);

        /*!
        * @brief Registers signal handler for a given signal of the proxied D-Bus object
        *
        * @param[in] signalName Name of the signal
        * @return A helper object for convenient registration of the signal handler
        *
        * This is a high-level, convenience way of registering to D-Bus signals that abstracts
        * from the D-Bus message concept. Signal arguments are automatically serialized
        * in a message and D-Bus signatures automatically deduced from the parameters
        * of the provided native signal callback.
        *
        * Example of use:
        * @code
        * object_.uponSignal("fooSignal").onInterface("com.kistler.foo").call([this](int arg1, double arg2){ this->onFooSignal(arg1, arg2); });
        * @endcode
        *
        * @throws sdbus::Error in case of failure
        */
        SignalSubscriber uponSignal(const std::string& signalName);

        /*!
        * @brief Gets value of a property of the proxied D-Bus object
        *
        * @param[in] propertyName Name of the property
        * @return A helper object for convenient getting of property value
        *
        * This is a high-level, convenience way of reading D-Bus property values that abstracts
        * from the D-Bus message concept. sdbus::Variant is returned which shall then be converted
        * to the real property type (implicit conversion is supported).
        *
        * Example of use:
        * @code
        * int state = object.getProperty("state").onInterface("com.kistler.foo");
        * @endcode
        *
        * @throws sdbus::Error in case of failure
        */
        PropertyGetter getProperty(const std::string& propertyName);

        /*!
        * @brief Sets value of a property of the proxied D-Bus object
        *
        * @param[in] propertyName Name of the property
        * @return A helper object for convenient setting of property value
        *
        * This is a high-level, convenience way of writing D-Bus property values that abstracts
        * from the D-Bus message concept.
        *
        * Example of use:
        * @code
        * int state = ...;
        * object_.setProperty("state").onInterface("com.kistler.foo").toValue(state);
        * @endcode
        *
        * @throws sdbus::Error in case of failure
        */
        PropertySetter setProperty(const std::string& propertyName);

        virtual ~IObjectProxy() = 0;
    };

    inline MethodInvoker IObjectProxy::callMethod(const std::string& methodName)
    {
        return MethodInvoker(*this, methodName);
    }

    inline SignalSubscriber IObjectProxy::uponSignal(const std::string& signalName)
    {
        return SignalSubscriber(*this, signalName);
    }

    inline PropertyGetter IObjectProxy::getProperty(const std::string& propertyName)
    {
        return PropertyGetter(*this, propertyName);
    }

    inline PropertySetter IObjectProxy::setProperty(const std::string& propertyName)
    {
        return PropertySetter(*this, propertyName);
    }

    inline IObjectProxy::~IObjectProxy() {}

    /*!
    * @brief Creates object proxy instance
    *
    * @param[in] connection D-Bus connection to be used by the proxy object
    * @param[in] destination Bus name that provides a D-Bus object
    * @param[in] objectPath Path of the D-Bus object
    * @return Pointer to the object proxy instance
    *
    * The provided connection will be used by the proxy to issue calls against the object,
    * and signals, if any, will be subscribed to on this connection.
    *
    * Code example:
    * @code
    * auto proxy = sdbus::createObjectProxy(connection, "com.kistler.foo", "/com/kistler/foo");
    * @endcode
    */
    std::unique_ptr<sdbus::IObjectProxy> createObjectProxy( sdbus::IConnection& connection
                                                          , std::string destination
                                                          , std::string objectPath );

    /*!
    * @brief Creates object proxy instance
    *
    * @param[in] connection D-Bus connection to be used by the proxy object
    * @param[in] destination Bus name that provides a D-Bus object
    * @param[in] objectPath Path of the D-Bus object
    * @return Pointer to the object proxy instance
    *
    * The provided connection will be used by the proxy to issue calls against the object,
    * and signals, if any, will be subscribed to on this connection. Object proxy becomes
    * an exclusive owner of this connection.
    *
    * Code example:
    * @code
    * auto proxy = sdbus::createObjectProxy(std::move(connection), "com.kistler.foo", "/com/kistler/foo");
    * @endcode
    */
    std::unique_ptr<sdbus::IObjectProxy> createObjectProxy( std::unique_ptr<sdbus::IConnection>&& connection
                                                          , std::string destination
                                                          , std::string objectPath );

    /*!
    * @brief Creates object proxy instance that uses its own D-Bus connection
    *
    * @param[in] destination Bus name that provides a D-Bus object
    * @param[in] objectPath Path of the D-Bus object
    * @return Pointer to the object proxy instance
    *
    * This factory overload creates a proxy that manages its own D-Bus connection(s).
    *
    * Code example:
    * @code
    * auto proxy = sdbus::createObjectProxy(connection, "com.kistler.foo", "/com/kistler/foo");
    * @endcode
    */
    std::unique_ptr<sdbus::IObjectProxy> createObjectProxy( std::string destination
                                                          , std::string objectPath );

}

#include <sdbus-c++/ConvenienceClasses.inl>

#endif /* SDBUS_CXX_IOBJECTPROXY_H_ */
