/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2024 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
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
#include <sdbus-c++/TypeTraits.h>
#include <string>
#include <memory>
#include <functional>
#include <chrono>
#include <future>

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
         * @brief Calls method on the remote D-Bus object
         *
         * @param[in] message Message representing a method call
         * @param[in] timeout Timeout for dbus call in microseconds
         * @return A method reply message
         *
         * The call does not block if the method call has dont-expect-reply flag set. In that case,
         * the call returns immediately and the return value is an empty, invalid method reply.
         *
         * The call blocks otherwise, waiting for the remote peer to send back a reply or an error,
         * or until the call times out.
         *
         * While blocking, other concurrent operations (in other threads) on the underlying bus
         * connection are stalled until the call returns. This is not an issue in vast majority of
         * (simple, single-threaded) applications. In asynchronous, multi-threaded designs involving
         * shared bus connections, this may be an issue. It is advised to instead use an asynchronous
         * callMethod() function overload, which does not block the bus connection, or do the synchronous
         * call from another Proxy instance created just before the call and then destroyed (which is
         * anyway quite a typical approach in D-Bus implementations). Such proxy instance must have
         * its own bus connection. So-called light-weight proxies (ones created with `dont_run_event_loop_thread`
         * tag are designed for exactly that purpose.
         *
         * Note: To avoid messing with messages, use API on a higher level of abstraction defined below.
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
         * @brief Calls method on the D-Bus object asynchronously
         *
         * @param[in] message Message representing an async method call
         * @param[in] asyncReplyCallback Handler for the async reply
         * @param[in] timeout Timeout for dbus call in microseconds
         * @return Cookie for the the pending asynchronous call
         *
         * This is a callback-based way of asynchronously calling a remote D-Bus method.
         *
         * The call itself is non-blocking. It doesn't wait for the reply. Once the reply arrives,
         * the provided async reply handler will get invoked from the context of the bus
         * connection I/O event loop thread.
         *
         * Note: To avoid messing with messages, use API on a higher level of abstraction defined below.
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
         * @brief Calls method on the D-Bus object asynchronously
         *
         * @param[in] message Message representing an async method call
         * @param[in] Tag denoting a std::future-based overload
         * @return Future object providing access to the future method reply message
         *
         * This is a std::future-based way of asynchronously calling a remote D-Bus method.
         *
         * The call itself is non-blocking. It doesn't wait for the reply. Once the reply arrives,
         * the provided future object will be set to contain the reply (or sdbus::Error
         * in case the remote method threw an exception).
         *
         * Note: To avoid messing with messages, use higher-level API defined below.
         *
         * @throws sdbus::Error in case of failure
         */
        virtual std::future<MethodReply> callMethod(const MethodCall& message, with_future_t) = 0;

        /*!
         * @brief Calls method on the D-Bus object asynchronously, with custom timeout
         *
         * @param[in] message Message representing an async method call
         * @param[in] timeout Method call timeout
         * @param[in] Tag denoting a std::future-based overload
         * @return Future object providing access to the future method reply message
         *
         * This is a std::future-based way of asynchronously calling a remote D-Bus method.
         *
         * The call itself is non-blocking. It doesn't wait for the reply. Once the reply arrives,
         * the provided future object will be set to contain the reply (or sdbus::Error
         * in case the remote method threw an exception, or the call timed out).
         *
         * Note: To avoid messing with messages, use higher-level API defined below.
         *
         * @throws sdbus::Error in case of failure
         */
        virtual std::future<MethodReply> callMethod(const MethodCall& message, uint64_t timeout, with_future_t) = 0;

        /*!
         * @copydoc IProxy::callMethod(const MethodCall&,uint64_t,with_future_t)
         */
        template <typename _Rep, typename _Period>
        std::future<MethodReply> callMethod( const MethodCall& message
                                           , const std::chrono::duration<_Rep, _Period>& timeout
                                           , with_future_t );

        /*!
         * @brief Registers a handler for the desired signal emitted by the D-Bus object
         *
         * @param[in] interfaceName Name of an interface that the signal belongs to
         * @param[in] signalName Name of the signal
         * @param[in] signalHandler Callback that implements the body of the signal handler
         *
         * A signal can be subscribed to and unsubscribed from at any time during proxy
         * lifetime. The subscription is active immediately after the call.
         *
         * @throws sdbus::Error in case of failure
         */
        virtual void registerSignalHandler( const std::string& interfaceName
                                          , const std::string& signalName
                                          , signal_handler signalHandler ) = 0;

        /*!
         * @brief Registers a handler for the desired signal emitted by the D-Bus object
         *
         * @param[in] interfaceName Name of an interface that the signal belongs to
         * @param[in] signalName Name of the signal
         * @param[in] signalHandler Callback that implements the body of the signal handler
         *
         * @return RAII-style slot handle representing the ownership of the subscription
         *
         * A signal can be subscribed to and unsubscribed from at any time during proxy
         * lifetime. The subscription is active immediately after the call. The subscription
         * is unregistered when the client destroys the returned slot object.
         *
         * @throws sdbus::Error in case of failure
         */
        [[nodiscard]] virtual Slot registerSignalHandler( const std::string& interfaceName
                                                        , const std::string& signalName
                                                        , signal_handler signalHandler
                                                        , request_slot_t ) = 0;

        /*!
         * @brief Unregisters proxy's signal handlers and stops receiving replies to pending async calls
         *
         * Unregistration is done automatically also in proxy's destructor. This method makes
         * sense if, in the process of proxy removal, we need to make sure that callbacks
         * are unregistered explicitly before the final destruction of the proxy instance.
         *
         * @throws sdbus::Error in case of failure
         */
        virtual void unregister() = 0;

        /*!
         * @brief Calls method on the D-Bus object
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
         * @brief Calls method on the D-Bus object asynchronously
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
         * @brief Registers signal handler for a given signal of the D-Bus object
         *
         * @param[in] signalName Name of the signal
         * @return A helper object for convenient registration of the signal handler
         *
         * This is a high-level, convenience way of registering to D-Bus signals that abstracts
         * from the D-Bus message concept. Signal arguments are automatically serialized
         * in a message and D-Bus signatures automatically deduced from the parameters
         * of the provided native signal callback.
         *
         * A signal can be subscribed to and unsubscribed from at any time during proxy
         * lifetime. The subscription is active immediately after the call.
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
         * @brief Gets value of a property of the D-Bus object
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
         * @brief Gets value of a property of the D-Bus object asynchronously
         *
         * @param[in] propertyName Name of the property
         * @return A helper object for convenient asynchronous getting of property value
         *
         * This is a high-level, convenience way of reading D-Bus property values that abstracts
         * from the D-Bus message concept.
         *
         * Example of use:
         * @code
         * std::future<sdbus::Variant> state = object.getPropertyAsync("state").onInterface("com.kistler.foo").getResultAsFuture();
         * auto callback = [](const sdbus::Error* err, const sdbus::Variant& value){ ... };
         * object.getPropertyAsync("state").onInterface("com.kistler.foo").uponReplyInvoke(std::move(callback));
         * @endcode
         *
         * @throws sdbus::Error in case of failure
         */
        [[nodiscard]] AsyncPropertyGetter getPropertyAsync(const std::string& propertyName);

        /*!
         * @brief Sets value of a property of the D-Bus object
         *
         * @param[in] propertyName Name of the property
         * @return A helper object for convenient setting of property value
         *
         * This is a high-level, convenience way of writing D-Bus property values that abstracts
         * from the D-Bus message concept.
         * Setting property value with NoReply flag is also supported.
         *
         * Example of use:
         * @code
         * int state = ...;
         * object_.setProperty("state").onInterface("com.kistler.foo").toValue(state);
         * // Or we can just send the set message call without waiting for the reply
         * object_.setProperty("state").onInterface("com.kistler.foo").toValue(state, dont_expect_reply);
         * @endcode
         *
         * @throws sdbus::Error in case of failure
         */
        [[nodiscard]] PropertySetter setProperty(const std::string& propertyName);

        /*!
         * @brief Sets value of a property of the D-Bus object asynchronously
         *
         * @param[in] propertyName Name of the property
         * @return A helper object for convenient asynchronous setting of property value
         *
         * This is a high-level, convenience way of writing D-Bus property values that abstracts
         * from the D-Bus message concept.
         *
         * Example of use:
         * @code
         * int state = ...;
         * // We can wait until the set operation finishes by waiting on the future
         * std::future<void> res = object_.setPropertyAsync("state").onInterface("com.kistler.foo").toValue(state).getResultAsFuture();
         * @endcode
         *
         * @throws sdbus::Error in case of failure
         */
        [[nodiscard]] AsyncPropertySetter setPropertyAsync(const std::string& propertyName);

        /*!
         * @brief Gets values of all properties of the D-Bus object
         *
         * @return A helper object for convenient getting of properties' values
         *
         * This is a high-level, convenience way of reading D-Bus properties' values that abstracts
         * from the D-Bus message concept.
         *
         * Example of use:
         * @code
         * auto props = object.getAllProperties().onInterface("com.kistler.foo");
         * @endcode
         *
         * @throws sdbus::Error in case of failure
         */
        [[nodiscard]] AllPropertiesGetter getAllProperties();

        /*!
         * @brief Gets values of all properties of the D-Bus object asynchronously
         *
         * @return A helper object for convenient asynchronous getting of properties' values
         *
         * This is a high-level, convenience way of reading D-Bus properties' values that abstracts
         * from the D-Bus message concept.
         *
         * Example of use:
         * @code
         * auto callback = [](const sdbus::Error* err, const std::map<std::string, Variant>>& properties){ ... };
         * auto props = object.getAllPropertiesAsync().onInterface("com.kistler.foo").uponReplyInvoke(std::move(callback));
         * @endcode
         *
         * @throws sdbus::Error in case of failure
         */
        [[nodiscard]] AsyncAllPropertiesGetter getAllPropertiesAsync();

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
        virtual Message getCurrentlyProcessedMessage() const = 0;
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

    template <typename _Rep, typename _Period>
    inline std::future<MethodReply> IProxy::callMethod( const MethodCall& message
                                                      , const std::chrono::duration<_Rep, _Period>& timeout
                                                      , with_future_t )
    {
        auto microsecs = std::chrono::duration_cast<std::chrono::microseconds>(timeout);
        return callMethod(message, microsecs.count(), with_future);
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

    inline AsyncPropertyGetter IProxy::getPropertyAsync(const std::string& propertyName)
    {
        return AsyncPropertyGetter(*this, propertyName);
    }

    inline PropertySetter IProxy::setProperty(const std::string& propertyName)
    {
        return PropertySetter(*this, propertyName);
    }

    inline AsyncPropertySetter IProxy::setPropertyAsync(const std::string& propertyName)
    {
        return AsyncPropertySetter(*this, propertyName);
    }

    inline AllPropertiesGetter IProxy::getAllProperties()
    {
        return AllPropertiesGetter(*this);
    }

    inline AsyncAllPropertiesGetter IProxy::getAllPropertiesAsync()
    {
        return AsyncAllPropertiesGetter(*this);
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
     * The destination parameter may be an empty string (useful e.g. in case of direct
     * D-Bus connections to a custom server bus).
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
     * The destination parameter may be an empty string (useful e.g. in case of direct
     * D-Bus connections to a custom server bus).
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
     * @param[in] connection D-Bus connection to be used by the proxy object
     * @param[in] destination Bus name that provides the remote D-Bus object
     * @param[in] objectPath Path of the remote D-Bus object
     * @return Pointer to the object proxy instance
     *
     * The provided connection will be used by the proxy to issue calls against the object.
     * The Object proxy becomes an exclusive owner of this connection, but will not start
     * an event loop thread on this connection. This is cheap construction and is suitable
     * for short-lived proxies created just to execute simple synchronous D-Bus calls and
     * then destroyed. Such blocking request-reply calls will work without an event loop
     * (but signals, async calls, etc. won't).
     *
     * The destination parameter may be an empty string (useful e.g. in case of direct
     * D-Bus connections to a custom server bus).
     *
     * Code example:
     * @code
     * auto proxy = sdbus::createProxy(std::move(connection), "com.kistler.foo", "/com/kistler/foo", sdbus::dont_run_event_loop_thread);
     * @endcode
     */
    [[nodiscard]] std::unique_ptr<sdbus::IProxy> createProxy( std::unique_ptr<sdbus::IConnection>&& connection
                                                            , std::string destination
                                                            , std::string objectPath
                                                            , dont_run_event_loop_thread_t );

    /*!
     * @brief Creates a proxy object for a specific remote D-Bus object
     *
     * @param[in] destination Bus name that provides the remote D-Bus object
     * @param[in] objectPath Path of the remote D-Bus object
     * @return Pointer to the object proxy instance
     *
     * No D-Bus connection is provided here, so the object proxy will create and manage
     * his own connection, and will automatically start an event loop upon that connection
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

    /*!
     * @brief Creates a proxy object for a specific remote D-Bus object
     *
     * @param[in] destination Bus name that provides the remote D-Bus object
     * @param[in] objectPath Path of the remote D-Bus object
     * @return Pointer to the object proxy instance
     *
     * No D-Bus connection is provided here, so the object proxy will create and manage
     * his own connection, but it will not start an event loop thread. This is cheap
     * construction and is suitable for short-lived proxies created just to execute simple
     * synchronous D-Bus calls and then destroyed. Such blocking request-reply calls
     * will work without an event loop (but signals, async calls, etc. won't).
     *
     * Code example:
     * @code
     * auto proxy = sdbus::createProxy("com.kistler.foo", "/com/kistler/foo", sdbus::dont_run_event_loop_thread );
     * @endcode
     */
    [[nodiscard]] std::unique_ptr<sdbus::IProxy> createProxy( std::string destination
                                                            , std::string objectPath
                                                            , dont_run_event_loop_thread_t );

}

#include <sdbus-c++/ConvenienceApiClasses.inl>

#endif /* SDBUS_CXX_IPROXY_H_ */
