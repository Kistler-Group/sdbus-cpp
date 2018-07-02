/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 *
 * @file IObject.h
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

#ifndef SDBUS_CXX_IOBJECT_H_
#define SDBUS_CXX_IOBJECT_H_

#include <sdbus-c++/ConvenienceClasses.h>
#include <sdbus-c++/TypeTraits.h>
#include <functional>
#include <string>
#include <memory>

// Forward declarations
namespace sdbus {
    class Signal;
    class IConnection;
}

namespace sdbus {

    /********************************************//**
     * @class IObject
     *
     * An interface to D-Bus object. Provides API for registration
     * of methods, signals, properties, and for emitting signals.
     *
     * All methods throw @c sdbus::Error in case of failure. The class is
     * thread-aware, but not thread-safe.
     *
     ***********************************************/
    class IObject
    {
    public:
        /*!
        * @brief Registers method that the object will provide on D-Bus
        *
        * @param[in] interfaceName Name of an interface that the method will belong to
        * @param[in] methodName Name of the method
        * @param[in] inputSignature D-Bus signature of method input parameters
        * @param[in] outputSignature D-Bus signature of method output parameters
        * @param[in] methodCallback Callback that implements the body of the method
        *
        * @throws sdbus::Error in case of failure
        */
        virtual void registerMethod( const std::string& interfaceName
                                   , const std::string& methodName
                                   , const std::string& inputSignature
                                   , const std::string& outputSignature
                                   , method_callback methodCallback ) = 0;

        /*!
        * @brief Registers method that the object will provide on D-Bus
        *
        * @param[in] interfaceName Name of an interface that the method will belong to
        * @param[in] methodName Name of the method
        * @param[in] inputSignature D-Bus signature of method input parameters
        * @param[in] outputSignature D-Bus signature of method output parameters
        * @param[in] asyncMethodCallback Callback that implements the body of the method
        *
        * This overload register a method callback that will have a freedom to execute
        * its body in asynchronous contexts, and send the results from those contexts.
        * This can help in e.g. long operations, which then don't block the D-Bus processing
        * loop thread.
        *
        * @throws sdbus::Error in case of failure
        */
        virtual void registerMethod( const std::string& interfaceName
                                   , const std::string& methodName
                                   , const std::string& inputSignature
                                   , const std::string& outputSignature
                                   , async_method_callback asyncMethodCallback ) = 0;

        /*!
        * @brief Registers signal that the object will emit on D-Bus
        *
        * @param[in] interfaceName Name of an interface that the signal will fall under
        * @param[in] signalName Name of the signal
        * @param[in] signature D-Bus signature of signal parameters
        *
        * @throws sdbus::Error in case of failure
        */
        virtual void registerSignal( const std::string& interfaceName
                                   , const std::string& signalName
                                   , const std::string& signature ) = 0;

        /*!
        * @brief Registers read-only property that the object will provide on D-Bus
        *
        * @param[in] interfaceName Name of an interface that the property will fall under
        * @param[in] propertyName Name of the property
        * @param[in] signature D-Bus signature of property parameters
        * @param[in] getCallback Callback that implements the body of the property getter
        *
        * @throws sdbus::Error in case of failure
        */
        virtual void registerProperty( const std::string& interfaceName
                                     , const std::string& propertyName
                                     , const std::string& signature
                                     , property_get_callback getCallback ) = 0;

        /*!
        * @brief Registers read/write property that the object will provide on D-Bus
        *
        * @param[in] interfaceName Name of an interface that the property will fall under
        * @param[in] propertyName Name of the property
        * @param[in] signature D-Bus signature of property parameters
        * @param[in] getCallback Callback that implements the body of the property getter
        * @param[in] setCallback Callback that implements the body of the property setter
        *
        * @throws sdbus::Error in case of failure
        */
        virtual void registerProperty( const std::string& interfaceName
                                     , const std::string& propertyName
                                     , const std::string& signature
                                     , property_get_callback getCallback
                                     , property_set_callback setCallback ) = 0;

        /*!
        * @brief Finishes the registration and exports object API on D-Bus
        *
        * The method exports all up to now registered methods, signals and properties on D-Bus.
        * Must be called only once, after all methods, signals and properties have been registered.
        *
        * @throws sdbus::Error in case of failure
        */
        virtual void finishRegistration() = 0;

        /*!
        * @brief Creates a signal message
        *
        * @param[in] interfaceName Name of an interface that the signal belongs under
        * @param[in] signalName Name of the signal
        * @return A signal message
        *
        * Serialize signal arguments into the returned message and emit the signal by passing
        * the message with serialized arguments to the @c emitSignal function.
        * Alternatively, use higher-level API @c emitSignal(const std::string& signalName) defined below.
        *
        * @throws sdbus::Error in case of failure
        */
        virtual Signal createSignal(const std::string& interfaceName, const std::string& signalName) = 0;

        /*!
        * @brief Emits signal on D-Bus
        *
        * @param[in] message Signal message to be sent out
        *
        * Note: To avoid messing with messages, use higher-level API defined below.
        *
        * @throws sdbus::Error in case of failure
        */
        virtual void emitSignal(const sdbus::Signal& message) = 0;

        /*!
        * @brief Registers method that the object will provide on D-Bus
        *
        * @param[in] methodName Name of the method
        * @return A helper object for convenient registration of the method
        *
        * This is a high-level, convenience way of registering D-Bus methods that abstracts
        * from the D-Bus message concept. Method arguments/return value are automatically (de)serialized
        * in a message and D-Bus signatures automatically deduced from the parameters and return type
        * of the provided native method implementation callback.
        *
        * Example of use:
        * @code
        * object.registerMethod("doFoo").onInterface("com.kistler.foo").implementedAs([this](int value){ return this->doFoo(value); });
        * @endcode
        *
        * @throws sdbus::Error in case of failure
        */
        MethodRegistrator registerMethod(const std::string& methodName);

        /*!
        * @brief Registers signal that the object will provide on D-Bus
        *
        * @param[in] signalName Name of the signal
        * @return A helper object for convenient registration of the signal
        *
        * This is a high-level, convenience way of registering D-Bus signals that abstracts
        * from the D-Bus message concept. Signal arguments are automatically (de)serialized
        * in a message and D-Bus signatures automatically deduced from the provided native parameters.
        *
        * Example of use:
        * @code
        * object.registerSignal("paramChange").onInterface("com.kistler.foo").withParameters<std::map<int32_t, std::string>>();
        * @endcode
        *
        * @throws sdbus::Error in case of failure
        */
        SignalRegistrator registerSignal(const std::string& signalName);

        /*!
        * @brief Registers property that the object will provide on D-Bus
        *
        * @param[in] propertyName Name of the property
        * @return A helper object for convenient registration of the property
        *
        * This is a high-level, convenience way of registering D-Bus properties that abstracts
        * from the D-Bus message concept. Property arguments are automatically (de)serialized
        * in a message and D-Bus signatures automatically deduced from the provided native callbacks.
        *
        * Example of use:
        * @code
        * object_.registerProperty("state").onInterface("com.kistler.foo").withGetter([this](){ return this->state(); });
        * @endcode
        *
        * @throws sdbus::Error in case of failure
        */
        PropertyRegistrator registerProperty(const std::string& propertyName);

        /*!
        * @brief Emits signal on D-Bus
        *
        * @param[in] signalName Name of the signal
        * @return A helper object for convenient emission of signals
        *
        * This is a high-level, convenience way of emitting D-Bus signals that abstracts
        * from the D-Bus message concept. Signal arguments are automatically serialized
        * in a message and D-Bus signatures automatically deduced from the provided native arguments.
        *
        * Example of use:
        * @code
        * int arg1 = ...;
        * double arg2 = ...;
        * object_.emitSignal("fooSignal").onInterface("com.kistler.foo").withArguments(arg1, arg2);
        * @endcode
        *
        * @throws sdbus::Error in case of failure
        */
        SignalEmitter emitSignal(const std::string& signalName);

        virtual ~IObject() = 0;
    };

    inline MethodRegistrator IObject::registerMethod(const std::string& methodName)
    {
        return MethodRegistrator(*this, methodName);
    }

    inline SignalRegistrator IObject::registerSignal(const std::string& signalName)
    {
        return SignalRegistrator(*this, std::move(signalName));
    }

    inline PropertyRegistrator IObject::registerProperty(const std::string& propertyName)
    {
        return PropertyRegistrator(*this, std::move(propertyName));
    }

    inline SignalEmitter IObject::emitSignal(const std::string& signalName)
    {
        return SignalEmitter(*this, signalName);
    }

    inline IObject::~IObject() {}

    /*!
    * @brief Creates instance representing a D-Bus object
    *
    * @param[in] connection D-Bus connection to be used by the object
    * @param[in] objectPath Path of the D-Bus object
    * @return Pointer to the object representation instance
    *
    * The provided connection will be used by the object to export methods,
    * issue signals and provide properties.
    *
    * Code example:
    * @code
    * auto proxy = sdbus::createObject(connection, "/com/kistler/foo");
    * @endcode
    */
    std::unique_ptr<sdbus::IObject> createObject(sdbus::IConnection& connection, std::string objectPath);

}

#include <sdbus-c++/ConvenienceClasses.inl>

#endif /* SDBUS_CXX_IOBJECT_H_ */
