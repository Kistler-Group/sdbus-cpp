/**
 * (C) 2016 - 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2019 Stanislav Angelovic <angelovic.s@gmail.com>
 *
 * @file IProxy.h
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

#ifndef SDBUS_CXX_IPROXY_H_
#define SDBUS_CXX_IPROXY_H_

#include <sdbus-c++/ConvenienceApiClasses.h>
#include <string>
#include <memory>
#include <functional>
#include <chrono>

// Forward declarations
namespace sdbus {
    class MethodCall;
    class MethodReply;
    class IConnection;
    class PendingAsyncCall;
    namespace internal {
        class Proxy;
    }
}

namespace sdbus {

    /********************************************//**
     * @class IProxy
     *
     * IProxy class represents a proxy object, which is a convenient local object created
     * to represent a remote D-Bus object in another process.
     * The proxy enables calling methods on remote objects, receiving signals from remote
     * objects, and getting/setting properties of remote objects.
     *
     * All IProxy member methods throw @c sdbus::Error in case of D-Bus or sdbus-c++ error.
     * The IProxy class has been designed as thread-aware. However, the operation of
     * creating and sending method calls (both synchronously and asynchronously) is
     * thread-safe by design.
     *
     ***********************************************/
    class IProxy
    {
    public:
        virtual ~IProxy() = default;

        /*!
         * @brief Creates a method call message
         *
         * @param[in] interfaceName Name of an interface that provides a given method
         * @param[in] methodName Name of the method
         * @return A method call message
         *
         * Serialize method arguments into the returned message and invoke the method by passing
         * the message with serialized arguments to the @c callMethod function.
         * Alternatively, use higher-level API @c callMethod(const std::string& methodName) defined below.
         *
         * @throws sdbus::Error in case of failure
         */
        virtual MethodCall createMethodCall(const std::string& interfaceName, const std::string& methodName) = 0;

        /*!
         * @brief Calls method on the proxied D-Bus object
         *
         * @param[in] message Message representing a method call
         * @param[in] timeout Timeout for dbus call in microseconds
         * @return A method reply message
         *
         * Normally, the call is blocking, i.e. it waits for the remote method to finish with either
         * a return value or an error.
         *
         * If the method call argument is set to not expect reply, the call will not wait for the remote
         * method to finish, i.e. the call will be non-blocking, and the function will return an empty,
         * invalid MethodReply object (representing void).
         *
         * Note: To avoid messing with messages, use higher-level API defined below.
         *
         * @throws sdbus::Error in case of failure
         */
        virtual MethodReply callMethod(const MethodCall& message, uint64_t timeout = 0) = 0;

        /*!
         * @copydoc IProxy::callMethod(const MethodCall&,uint64_t)
         */
        template <typename _Rep, typename _Period>
        MethodReply callMethod(const MethodCall& message, const std::chrono::duration<_Rep, _Period>& timeout);

        /*!
         * @brief Calls method on the proxied D-Bus object asynchronously
         *
         * @param[in] message Message representing an async method call
         * @param[in] asyncReplyCallback Handler for the async reply
         * @param[in] timeout Timeout for dbus call in microseconds
         * @return Cookie for the the pending asynchronous call
         *
         * The call is non-blocking. It doesn't wait for the reply. Once the reply arrives,
         * the provided async reply handler will get invoked from the context of the connection
         * I/O event loop thread.
         *
         * Note: To avoid messing with messages, use higher-level API defined below.
         *
         * @throws sdbus::Error in case of failure
         */
        virtual PendingAsyncCall callMethod(const MethodCall& message, async_reply_handler asyncReplyCallback, uint64_t timeout = 0) = 0;

        /*!
         * @copydoc IProxy::callMethod(const MethodCall&,async_reply_handler,uint64_t)
         */
        template <typename _Rep, typename _Period>
        PendingAsyncCall callMethod(const MethodCall& message, async_reply_handler asyncReplyCallback, const std::chrono::duration<_Rep, _Period>& timeout);

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
         * @brief Unregisters proxy's signal handlers and stops receving replies to pending async calls
         *
         * Unregistration is done automatically also in proxy's destructor. This method makes
         * sense if, in the process of proxy removal, we need to make sure that callbacks
         * are unregistered explicitly before the final destruction of the proxy instance.
         *
         * @throws sdbus::Error in case of failure
         */
        virtual void unregister() = 0;

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
        [[nodiscard]] MethodInvoker callMethod(const std::string& methodName);

        /*!
         * @brief Calls method on the proxied D-Bus object asynchronously
         *
         * @param[in] methodName Name of the method
         * @return A helper object for convenient asynchronous invocation of the method
         *
         * This is a high-level, convenience way of calling D-Bus methods that abstracts
         * from the D-Bus message concept. Method arguments/return value are automatically (de)serialized
         * in a message and D-Bus signatures automatically deduced from the provided native arguments
         * and return values.
         *
         * Example of use:
         * @code
         * int a = ..., b = ...;
         * object_.callMethodAsync("multiply").onInterface(INTERFACE_NAME).withArguments(a, b).uponReplyInvoke([](int result)
         * {
         *     std::cout << "Got result of multiplying " << a << " and " << b << ": " << result << std::endl;
         * });
         * @endcode
         *
         * @throws sdbus::Error in case of failure
         */
        [[nodiscard]] AsyncMethodInvoker callMethodAsync(const std::string& methodName);

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
        [[nodiscard]] SignalSubscriber uponSignal(const std::string& signalName);

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
        [[nodiscard]] PropertyGetter getProperty(const std::string& propertyName);

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
        [[nodiscard]] PropertySetter setProperty(const std::string& propertyName);

        /*!
         * @brief Provides D-Bus connection used by the proxy
         *
         * @return Reference to the D-Bus connection
         */
        virtual sdbus::IConnection& getConnection() const = 0;

        /*!
         * @brief Returns object path of the underlying DBus object
         */
        virtual const std::string& getObjectPath() const = 0;

        /*!
         * @brief Provides currently processed D-Bus message
         *
         * This method provides immutable access to the currently processed incoming D-Bus message.
         * "Currently processed" means that the registered callback handler(s) for that message
         * are being invoked. This method is meant to be called from within a callback handler
         * (e.g. from a D-Bus signal handler, or async method reply handler, etc.). In such a case it is
         * guaranteed to return a valid pointer to the D-Bus message for which the handler is called.
         * If called from other contexts/threads, it may return a nonzero pointer or a nullptr, depending
         * on whether a message was processed at the time of call or not, but the value is nondereferencable,
         * since the pointed-to message may have gone in the meantime.
         *
         * @return A pointer to the currently processed D-Bus message
         */
        virtual const Message* getCurrentlyProcessedMessage() const = 0;
    };

    /********************************************//**
     * @class PendingAsyncCall
     *
     * PendingAsyncCall represents a simple handle type to cancel the delivery
     * of the asynchronous D-Bus call result to the application.
     *
     * The handle is lifetime-independent from the originating Proxy object.
     * It's safe to call its methods even after the Proxy has gone.
     *
     ***********************************************/
    class PendingAsyncCall
    {
    public:
        PendingAsyncCall() = default;

        /*!
         * @brief Cancels the delivery of the pending asynchronous call result
         *
         * This function effectively removes the callback handler registered to the
         * async D-Bus method call result delivery. Does nothing if the call was
         * completed already, or if the originating Proxy object has gone meanwhile.
         */
        void cancel();

        /*!
         * @brief Answers whether the asynchronous call is still pending
         *
         * @return True if the call is pending, false if the call has been fully completed
         *
         * Pending call in this context means a call whose results have not arrived, or
         * have arrived and are currently being processed by the callback handler.
         */
        bool isPending() const;

    private:
        friend internal::Proxy;
        PendingAsyncCall(std::weak_ptr<void> callData);

    private:
        std::weak_ptr<void> callData_;
    };

    // Out-of-line member definitions

    template <typename _Rep, typename _Period>
    inline MethodReply IProxy::callMethod(const MethodCall& message, const std::chrono::duration<_Rep, _Period>& timeout)
    {
        auto microsecs = std::chrono::duration_cast<std::chrono::microseconds>(timeout);
        return callMethod(message, microsecs.count());
    }

    template <typename _Rep, typename _Period>
    inline PendingAsyncCall IProxy::callMethod(const MethodCall& message, async_reply_handler asyncReplyCallback, const std::chrono::duration<_Rep, _Period>& timeout)
    {
        auto microsecs = std::chrono::duration_cast<std::chrono::microseconds>(timeout);
        return callMethod(message, std::move(asyncReplyCallback), microsecs.count());
    }

    inline MethodInvoker IProxy::callMethod(const std::string& methodName)
    {
        return MethodInvoker(*this, methodName);
    }

    inline AsyncMethodInvoker IProxy::callMethodAsync(const std::string& methodName)
    {
        return AsyncMethodInvoker(*this, methodName);
    }

    inline SignalSubscriber IProxy::uponSignal(const std::string& signalName)
    {
        return SignalSubscriber(*this, signalName);
    }

    inline PropertyGetter IProxy::getProperty(const std::string& propertyName)
    {
        return PropertyGetter(*this, propertyName);
    }

    inline PropertySetter IProxy::setProperty(const std::string& propertyName)
    {
        return PropertySetter(*this, propertyName);
    }

    /*!
     * @brief Creates a proxy object for a specific remote D-Bus object
     *
     * @param[in] connection D-Bus connection to be used by the proxy object
     * @param[in] destination Bus name that provides the remote D-Bus object
     * @param[in] objectPath Path of the remote D-Bus object
     * @return Pointer to the proxy object instance
     *
     * The provided connection will be used by the proxy to issue calls against the object,
     * and signals, if any, will be subscribed to on this connection. The caller still
     * remains the owner of the connection (the proxy just keeps a reference to it), and
     * should make sure that an I/O event loop is running on that connection, so the proxy
     * may receive incoming signals and asynchronous method replies.
     *
     * Code example:
     * @code
     * auto proxy = sdbus::createProxy(connection, "com.kistler.foo", "/com/kistler/foo");
     * @endcode
     */
    [[nodiscard]] std::unique_ptr<sdbus::IProxy> createProxy( sdbus::IConnection& connection
                                                            , std::string destination
                                                            , std::string objectPath );

    /*!
     * @brief Creates a proxy object for a specific remote D-Bus object
     *
     * @param[in] connection D-Bus connection to be used by the proxy object
     * @param[in] destination Bus name that provides the remote D-Bus object
     * @param[in] objectPath Path of the remote D-Bus object
     * @return Pointer to the object proxy instance
     *
     * The provided connection will be used by the proxy to issue calls against the object,
     * and signals, if any, will be subscribed to on this connection. The Object proxy becomes
     * an exclusive owner of this connection, and will automatically start a procesing loop
     * upon that connection in a separate internal thread. Handlers for incoming signals and
     * asynchronous method replies will be executed in the context of that thread.
     *
     * Code example:
     * @code
     * auto proxy = sdbus::createProxy(std::move(connection), "com.kistler.foo", "/com/kistler/foo");
     * @endcode
     */
    [[nodiscard]] std::unique_ptr<sdbus::IProxy> createProxy( std::unique_ptr<sdbus::IConnection>&& connection
                                                            , std::string destination
                                                            , std::string objectPath );

    /*!
     * @brief Creates a proxy object for a specific remote D-Bus object
     *
     * @param[in] destination Bus name that provides the remote D-Bus object
     * @param[in] objectPath Path of the remote D-Bus object
     * @return Pointer to the object proxy instance
     *
     * No D-Bus connection is provided here, so the object proxy will create and manage
     * his own connection, and will automatically start a procesing loop upon that connection
     * in a separate internal thread. Handlers for incoming signals and asynchronous
     * method replies will be executed in the context of that thread.
     *
     * Code example:
     * @code
     * auto proxy = sdbus::createProxy("com.kistler.foo", "/com/kistler/foo");
     * @endcode
     */
    [[nodiscard]] std::unique_ptr<sdbus::IProxy> createProxy( std::string destination
                                                            , std::string objectPath );

}

#include <sdbus-c++/ConvenienceApiClasses.inl>

#endif /* SDBUS_CXX_IPROXY_H_ */
