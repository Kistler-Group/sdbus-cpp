/**
 * (C) 2016 - 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2019 Stanislav Angelovic <angelovic.s@gmail.com>
 *
 * @file ConvenienceApiClasses.inl
 *
 * Created on: Dec 19, 2016
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

#ifndef SDBUS_CPP_CONVENIENCEAPICLASSES_INL_
#define SDBUS_CPP_CONVENIENCEAPICLASSES_INL_

#include <sdbus-c++/IObject.h>
#include <sdbus-c++/IProxy.h>
#include <sdbus-c++/Message.h>
#include <sdbus-c++/MethodResult.h>
#include <sdbus-c++/Types.h>
#include <sdbus-c++/TypeTraits.h>
#include <sdbus-c++/Error.h>
#include <string>
#include <tuple>
#include <exception>
#include <cassert>

namespace sdbus {

    /*** ----------------- ***/
    /*** MethodRegistrator ***/
    /*** ----------------- ***/

    inline MethodRegistrator::MethodRegistrator(IObject& object, const std::string& methodName)
        : object_(object)
        , methodName_(methodName)
        , exceptions_(std::uncaught_exceptions())
    {
    }

    inline MethodRegistrator::~MethodRegistrator() noexcept(false) // since C++11, destructors must
    {                                                              // explicitly be allowed to throw
        // Don't register the method if MethodRegistrator threw an exception in one of its methods
        if (std::uncaught_exceptions() != exceptions_)
            return;

        assert(!interfaceName_.empty()); // onInterface() must be placed/called prior to this function
        assert(methodCallback_); // implementedAs() must be placed/called prior to this function

        // registerMethod() can throw. But as the MethodRegistrator shall always be used as an unnamed,
        // temporary object, i.e. not as a stack-allocated object, the double-exception situation
        // shall never happen. I.e. it should not happen that this destructor is directly called
        // in the stack-unwinding process of another flying exception (which would lead to immediate
        // termination). It can be called indirectly in the destructor of another object, but that's
        // fine and safe provided that the caller catches exceptions thrown from here.
        // Therefore, we can allow registerMethod() to throw even if we are in the destructor.
        // Bottomline is, to be on the safe side, the caller must take care of catching and reacting
        // to the exception thrown from here if the caller is a destructor itself.
        object_.registerMethod( interfaceName_
                              , std::move(methodName_)
                              , std::move(inputSignature_)
                              , std::move(inputParamNames_)
                              , std::move(outputSignature_)
                              , std::move(outputParamNames_)
                              , std::move(methodCallback_)
                              , std::move(flags_));
    }

    inline MethodRegistrator& MethodRegistrator::onInterface(std::string interfaceName)
    {
        interfaceName_ = std::move(interfaceName);

        return *this;
    }

    template <typename _Function>
    MethodRegistrator& MethodRegistrator::implementedAs(_Function&& callback)
    {
        inputSignature_ = signature_of_function_input_arguments<_Function>::str();
        outputSignature_ = signature_of_function_output_arguments<_Function>::str();
        methodCallback_ = [callback = std::forward<_Function>(callback)](MethodCall call)
        {
            // Create a tuple of callback input arguments' types, which will be used
            // as a storage for the argument values deserialized from the message.
            tuple_of_function_input_arg_types_t<_Function> inputArgs;

            // Deserialize input arguments from the message into the tuple.
            call >> inputArgs;

            if constexpr (!is_async_method_v<_Function>)
            {
                // Invoke callback with input arguments from the tuple.
                auto ret = sdbus::apply(callback, inputArgs);

                // Store output arguments to the reply message and send it back.
                auto reply = call.createReply();
                reply << ret;
                reply.send();
            }
            else
            {
                // Invoke callback with input arguments from the tuple and with result object to be set later
                using AsyncResult = typename function_traits<_Function>::async_result_t;
                sdbus::apply(callback, AsyncResult{std::move(call)}, std::move(inputArgs));
            }
        };

        return *this;
    }

    inline MethodRegistrator& MethodRegistrator::withInputParamNames(std::vector<std::string> paramNames)
    {
        inputParamNames_ = std::move(paramNames);

        return *this;
    }

    template <typename... _String>
    inline MethodRegistrator& MethodRegistrator::withInputParamNames(_String... paramNames)
    {
        static_assert(std::conjunction_v<std::is_convertible<_String, std::string>...>, "Parameter names must be (convertible to) strings");

        return withInputParamNames({paramNames...});
    }

    inline MethodRegistrator& MethodRegistrator::withOutputParamNames(std::vector<std::string> paramNames)
    {
        outputParamNames_ = std::move(paramNames);

        return *this;
    }

    template <typename... _String>
    inline MethodRegistrator& MethodRegistrator::withOutputParamNames(_String... paramNames)
    {
        static_assert(std::conjunction_v<std::is_convertible<_String, std::string>...>, "Parameter names must be (convertible to) strings");

        return withOutputParamNames({paramNames...});
    }

    inline MethodRegistrator& MethodRegistrator::markAsDeprecated()
    {
        flags_.set(Flags::DEPRECATED);

        return *this;
    }

    inline MethodRegistrator& MethodRegistrator::markAsPrivileged()
    {
        flags_.set(Flags::PRIVILEGED);

        return *this;
    }

    inline MethodRegistrator& MethodRegistrator::withNoReply()
    {
        flags_.set(Flags::METHOD_NO_REPLY);

        return *this;
    }

    /*** ----------------- ***/
    /*** SignalRegistrator ***/
    /*** ----------------- ***/

    inline SignalRegistrator::SignalRegistrator(IObject& object, const std::string& signalName)
        : object_(object)
        , signalName_(signalName)
        , exceptions_(std::uncaught_exceptions())
    {
    }

    inline SignalRegistrator::~SignalRegistrator() noexcept(false) // since C++11, destructors must
    {                                                              // explicitly be allowed to throw
        // Don't register the signal if SignalRegistrator threw an exception in one of its methods
        if (std::uncaught_exceptions() != exceptions_)
            return;

        assert(!interfaceName_.empty()); // onInterface() must be placed/called prior to this function

        // registerSignal() can throw. But as the SignalRegistrator shall always be used as an unnamed,
        // temporary object, i.e. not as a stack-allocated object, the double-exception situation
        // shall never happen. I.e. it should not happen that this destructor is directly called
        // in the stack-unwinding process of another flying exception (which would lead to immediate
        // termination). It can be called indirectly in the destructor of another object, but that's
        // fine and safe provided that the caller catches exceptions thrown from here.
        // Therefore, we can allow registerSignal() to throw even if we are in the destructor.
        // Bottomline is, to be on the safe side, the caller must take care of catching and reacting
        // to the exception thrown from here if the caller is a destructor itself.
        object_.registerSignal( interfaceName_
                              , std::move(signalName_)
                              , std::move(signalSignature_)
                              , std::move(paramNames_)
                              , std::move(flags_) );
    }

    inline SignalRegistrator& SignalRegistrator::onInterface(std::string interfaceName)
    {
        interfaceName_ = std::move(interfaceName);

        return *this;
    }

    template <typename... _Args>
    inline SignalRegistrator& SignalRegistrator::withParameters()
    {
        signalSignature_ = signature_of_function_input_arguments<void(_Args...)>::str();

        return *this;
    }

    template <typename... _Args>
    inline SignalRegistrator& SignalRegistrator::withParameters(std::vector<std::string> paramNames)
    {
        paramNames_ = std::move(paramNames);

        return withParameters<_Args...>();
    }

    template <typename... _Args, typename... _String>
    inline SignalRegistrator& SignalRegistrator::withParameters(_String... paramNames)
    {
        static_assert(std::conjunction_v<std::is_convertible<_String, std::string>...>, "Parameter names must be (convertible to) strings");
        static_assert(sizeof...(_Args) == sizeof...(_String), "Numbers of signal parameters and their names don't match");

        return withParameters<_Args...>({paramNames...});
    }

    inline SignalRegistrator& SignalRegistrator::markAsDeprecated()
    {
        flags_.set(Flags::DEPRECATED);

        return *this;
    }

    /*** ------------------- ***/
    /*** PropertyRegistrator ***/
    /*** ------------------- ***/

    inline PropertyRegistrator::PropertyRegistrator(IObject& object, const std::string& propertyName)
        : object_(object)
        , propertyName_(propertyName)
        , exceptions_(std::uncaught_exceptions())
    {
    }

    inline PropertyRegistrator::~PropertyRegistrator() noexcept(false) // since C++11, destructors must
    {                                                                  // explicitly be allowed to throw
        // Don't register the property if PropertyRegistrator threw an exception in one of its methods
        if (std::uncaught_exceptions() != exceptions_)
            return;

        assert(!interfaceName_.empty()); // onInterface() must be placed/called prior to this function

        // registerProperty() can throw. But as the PropertyRegistrator shall always be used as an unnamed,
        // temporary object, i.e. not as a stack-allocated object, the double-exception situation
        // shall never happen. I.e. it should not happen that this destructor is directly called
        // in the stack-unwinding process of another flying exception (which would lead to immediate
        // termination). It can be called indirectly in the destructor of another object, but that's
        // fine and safe provided that the caller catches exceptions thrown from here.
        // Therefore, we can allow registerProperty() to throw even if we are in the destructor.
        // Bottomline is, to be on the safe side, the caller must take care of catching and reacting
        // to the exception thrown from here if the caller is a destructor itself.
        object_.registerProperty( interfaceName_
                                , propertyName_
                                , propertySignature_
                                , std::move(getter_)
                                , std::move(setter_)
                                , flags_ );
    }

    inline PropertyRegistrator& PropertyRegistrator::onInterface(std::string interfaceName)
    {
        interfaceName_ = std::move(interfaceName);

        return *this;
    }

    template <typename _Function>
    inline PropertyRegistrator& PropertyRegistrator::withGetter(_Function&& callback)
    {
        static_assert(function_traits<_Function>::arity == 0, "Property getter function must not take any arguments");
        static_assert(!std::is_void<function_result_t<_Function>>::value, "Property getter function must return property value");

        if (propertySignature_.empty())
            propertySignature_ = signature_of_function_output_arguments<_Function>::str();

        getter_ = [callback = std::forward<_Function>(callback)](PropertyGetReply& reply)
        {
            // Get the propety value and serialize it into the pre-constructed reply message
            reply << callback();
        };

        return *this;
    }

    template <typename _Function>
    inline PropertyRegistrator& PropertyRegistrator::withSetter(_Function&& callback)
    {
        static_assert(function_traits<_Function>::arity == 1, "Property setter function must take one parameter - the property value");
        static_assert(std::is_void<function_result_t<_Function>>::value, "Property setter function must not return any value");

        if (propertySignature_.empty())
            propertySignature_ = signature_of_function_input_arguments<_Function>::str();

        setter_ = [callback = std::forward<_Function>(callback)](PropertySetCall& call)
        {
            // Default-construct property value
            using property_type = function_argument_t<_Function, 0>;
            std::decay_t<property_type> property;

            // Deserialize property value from the incoming call message
            call >> property;

            // Invoke setter with the value
            callback(property);
        };

        return *this;
    }

    inline PropertyRegistrator& PropertyRegistrator::markAsDeprecated()
    {
        flags_.set(Flags::DEPRECATED);

        return *this;
    }

    inline PropertyRegistrator& PropertyRegistrator::markAsPrivileged()
    {
        flags_.set(Flags::PRIVILEGED);

        return *this;
    }

    inline PropertyRegistrator& PropertyRegistrator::withUpdateBehavior(Flags::PropertyUpdateBehaviorFlags behavior)
    {
        flags_.set(behavior);

        return *this;
    }

    /*** -------------------- ***/
    /*** InterfaceFlagsSetter ***/
    /*** -------------------- ***/

    inline InterfaceFlagsSetter::InterfaceFlagsSetter(IObject& object, const std::string& interfaceName)
        : object_(object)
        , interfaceName_(interfaceName)
        , exceptions_(std::uncaught_exceptions())
    {
    }

    inline InterfaceFlagsSetter::~InterfaceFlagsSetter() noexcept(false) // since C++11, destructors must
    {                                                                    // explicitly be allowed to throw
        // Don't set any flags if InterfaceFlagsSetter threw an exception in one of its methods
        if (std::uncaught_exceptions() != exceptions_)
            return;

        // setInterfaceFlags() can throw. But as the InterfaceFlagsSetter shall always be used as an unnamed,
        // temporary object, i.e. not as a stack-allocated object, the double-exception situation
        // shall never happen. I.e. it should not happen that this destructor is directly called
        // in the stack-unwinding process of another flying exception (which would lead to immediate
        // termination). It can be called indirectly in the destructor of another object, but that's
        // fine and safe provided that the caller catches exceptions thrown from here.
        // Therefore, we can allow setInterfaceFlags() to throw even if we are in the destructor.
        // Bottomline is, to be on the safe side, the caller must take care of catching and reacting
        // to the exception thrown from here if the caller is a destructor itself.
        object_.setInterfaceFlags(interfaceName_, std::move(flags_));
    }

    inline InterfaceFlagsSetter& InterfaceFlagsSetter::markAsDeprecated()
    {
        flags_.set(Flags::DEPRECATED);

        return *this;
    }

    inline InterfaceFlagsSetter& InterfaceFlagsSetter::markAsPrivileged()
    {
        flags_.set(Flags::PRIVILEGED);

        return *this;
    }

    inline InterfaceFlagsSetter& InterfaceFlagsSetter::withNoReplyMethods()
    {
        flags_.set(Flags::METHOD_NO_REPLY);

        return *this;
    }

    inline InterfaceFlagsSetter& InterfaceFlagsSetter::withPropertyUpdateBehavior(Flags::PropertyUpdateBehaviorFlags behavior)
    {
        flags_.set(behavior);

        return *this;
    }

    /*** ------------- ***/
    /*** SignalEmitter ***/
    /*** ------------- ***/

    inline SignalEmitter::SignalEmitter(IObject& object, const std::string& signalName)
        : object_(object)
        , signalName_(signalName)
        , exceptions_(std::uncaught_exceptions())
    {
    }

    inline SignalEmitter::~SignalEmitter() noexcept(false) // since C++11, destructors must
    {                                                      // explicitly be allowed to throw
        // Don't emit the signal if SignalEmitter threw an exception in one of its methods
        if (std::uncaught_exceptions() != exceptions_)
            return;

        // emitSignal() can throw. But as the SignalEmitter shall always be used as an unnamed,
        // temporary object, i.e. not as a stack-allocated object, the double-exception situation
        // shall never happen. I.e. it should not happen that this destructor is directly called
        // in the stack-unwinding process of another flying exception (which would lead to immediate
        // termination). It can be called indirectly in the destructor of another object, but that's
        // fine and safe provided that the caller catches exceptions thrown from here.
        // Therefore, we can allow emitSignal() to throw even if we are in the destructor.
        // Bottomline is, to be on the safe side, the caller must take care of catching and reacting
        // to the exception thrown from here if the caller is a destructor itself.
        object_.emitSignal(signal_);
    }

    inline SignalEmitter& SignalEmitter::onInterface(const std::string& interfaceName)
    {
        signal_ = object_.createSignal(interfaceName, signalName_);

        return *this;
    }

    template <typename... _Args>
    inline void SignalEmitter::withArguments(_Args&&... args)
    {
        assert(signal_.isValid()); // onInterface() must be placed/called prior to withArguments()

        detail::serialize_pack(signal_, std::forward<_Args>(args)...);
    }

    /*** ------------- ***/
    /*** MethodInvoker ***/
    /*** ------------- ***/

    inline MethodInvoker::MethodInvoker(IProxy& proxy, const std::string& methodName)
        : proxy_(proxy)
        , methodName_(methodName)
        , exceptions_(std::uncaught_exceptions())
    {
    }

    inline MethodInvoker::~MethodInvoker() noexcept(false) // since C++11, destructors must
    {                                                      // explicitly be allowed to throw
        // Don't call the method if it has been called already or if MethodInvoker
        // threw an exception in one of its methods
        if (methodCalled_ || std::uncaught_exceptions() != exceptions_)
            return;

        // callMethod() can throw. But as the MethodInvoker shall always be used as an unnamed,
        // temporary object, i.e. not as a stack-allocated object, the double-exception situation
        // shall never happen. I.e. it should not happen that this destructor is directly called
        // in the stack-unwinding process of another flying exception (which would lead to immediate
        // termination). It can be called indirectly in the destructor of another object, but that's
        // fine and safe provided that the caller catches exceptions thrown from here.
        // Therefore, we can allow callMethod() to throw even if we are in the destructor.
        // Bottomline is, to be on the safe side, the caller must take care of catching and reacting
        // to the exception thrown from here if the caller is a destructor itself.
        proxy_.callMethod(method_, timeout_);
    }

    inline MethodInvoker& MethodInvoker::onInterface(const std::string& interfaceName)
    {
        method_ = proxy_.createMethodCall(interfaceName, methodName_);

        return *this;
    }

    inline MethodInvoker& MethodInvoker::withTimeout(uint64_t usec)
    {
        timeout_ = usec;

        return *this;
    }

    template <typename _Rep, typename _Period>
    inline MethodInvoker& MethodInvoker::withTimeout(const std::chrono::duration<_Rep, _Period>& timeout)
    {
        auto microsecs = std::chrono::duration_cast<std::chrono::microseconds>(timeout);
        return withTimeout(microsecs.count());
    }

    template <typename... _Args>
    inline MethodInvoker& MethodInvoker::withArguments(_Args&&... args)
    {
        assert(method_.isValid()); // onInterface() must be placed/called prior to this function

        detail::serialize_pack(method_, std::forward<_Args>(args)...);

        return *this;
    }

    template <typename... _Args>
    inline void MethodInvoker::storeResultsTo(_Args&... args)
    {
        assert(method_.isValid()); // onInterface() must be placed/called prior to this function

        auto reply = proxy_.callMethod(method_, timeout_);
        methodCalled_ = true;

        detail::deserialize_pack(reply, args...);
    }

    inline void MethodInvoker::dontExpectReply()
    {
        assert(method_.isValid()); // onInterface() must be placed/called prior to this function

        method_.dontExpectReply();
    }

    /*** ------------------ ***/
    /*** AsyncMethodInvoker ***/
    /*** ------------------ ***/

    inline AsyncMethodInvoker::AsyncMethodInvoker(IProxy& proxy, const std::string& methodName)
        : proxy_(proxy)
        , methodName_(methodName)
    {
    }

    inline AsyncMethodInvoker& AsyncMethodInvoker::onInterface(const std::string& interfaceName)
    {
        method_ = proxy_.createMethodCall(interfaceName, methodName_);

        return *this;
    }

    inline AsyncMethodInvoker& AsyncMethodInvoker::withTimeout(uint64_t usec)
    {
        timeout_ = usec;

        return *this;
    }

    template <typename _Rep, typename _Period>
    inline AsyncMethodInvoker& AsyncMethodInvoker::withTimeout(const std::chrono::duration<_Rep, _Period>& timeout)
    {
        auto microsecs = std::chrono::duration_cast<std::chrono::microseconds>(timeout);
        return withTimeout(microsecs.count());
    }

    template <typename... _Args>
    inline AsyncMethodInvoker& AsyncMethodInvoker::withArguments(_Args&&... args)
    {
        assert(method_.isValid()); // onInterface() must be placed/called prior to this function

        detail::serialize_pack(method_, std::forward<_Args>(args)...);

        return *this;
    }

    template <typename _Function>
    PendingAsyncCall AsyncMethodInvoker::uponReplyInvoke(_Function&& callback)
    {
        assert(method_.isValid()); // onInterface() must be placed/called prior to this function

        auto asyncReplyHandler = [callback = std::forward<_Function>(callback)](MethodReply& reply, const Error* error)
        {
            // Create a tuple of callback input arguments' types, which will be used
            // as a storage for the argument values deserialized from the message.
            tuple_of_function_input_arg_types_t<_Function> args;

            // Deserialize input arguments from the message into the tuple (if no error occurred).
            if (error == nullptr)
            {
                try
                {
                    reply >> args;
                }
                catch (const Error& e)
                {
                    // Catch message unpack exceptions and pass them to the callback
                    // in the expected manner to avoid propagating them up the call
                    // stack to the event loop.
                    sdbus::apply(callback, &e, args);
                    return;
                }
            }

            // Invoke callback with input arguments from the tuple.
            sdbus::apply(callback, error, args);
        };

        return proxy_.callMethod(method_, std::move(asyncReplyHandler), timeout_);
    }

    /*** ---------------- ***/
    /*** SignalSubscriber ***/
    /*** ---------------- ***/

    inline SignalSubscriber::SignalSubscriber(IProxy& proxy, const std::string& signalName)
        : proxy_(proxy)
        , signalName_(signalName)
    {
    }

    inline SignalSubscriber& SignalSubscriber::onInterface(std::string interfaceName)
    {
        interfaceName_ = std::move(interfaceName);

        return *this;
    }

    template <typename _Function>
    inline void SignalSubscriber::call(_Function&& callback)
    {
        assert(!interfaceName_.empty()); // onInterface() must be placed/called prior to this function

        proxy_.registerSignalHandler( interfaceName_
                                    , signalName_
                                    , [callback = std::forward<_Function>(callback)](Signal& signal)
        {
            // Create a tuple of callback input arguments' types, which will be used
            // as a storage for the argument values deserialized from the signal message.
            tuple_of_function_input_arg_types_t<_Function> signalArgs;

            // The signal handler can take pure signal parameters only, or an additional `const Error*` as its first
            // parameter. In the former case, if the deserialization fails (e.g. due to signature mismatch),
            // the failure is ignored (and signal simply dropped). In the latter case, the deserialization failure
            // will be communicated as a non-zero Error pointer to the client's signal handler.
            if constexpr (has_error_param_v<_Function>)
            {
                // Deserialize input arguments from the signal message into the tuple
                try
                {
                    signal >> signalArgs;
                }
                catch (const sdbus::Error& e)
                {
                    // Invoke callback with error argument and input arguments from the tuple.
                    sdbus::apply(callback, &e, signalArgs);
                }

                // Invoke callback with no error and input arguments from the tuple.
                sdbus::apply(callback, nullptr, signalArgs);
            }
            else
            {
                // Deserialize input arguments from the signal message into the tuple
                signal >> signalArgs;

                // Invoke callback with input arguments from the tuple.
                sdbus::apply(callback, signalArgs);
            }
        });
    }

    /*** ------------------ ***/
    /*** SignalUnsubscriber ***/
    /*** ------------------ ***/

    inline SignalUnsubscriber::SignalUnsubscriber(IProxy& proxy, const std::string& signalName)
        : proxy_(proxy)
        , signalName_(signalName)
    {
    }

    inline void SignalUnsubscriber::onInterface(std::string interfaceName)
    {
        proxy_.unregisterSignalHandler(interfaceName, signalName_);
    }

    /*** -------------- ***/
    /*** PropertyGetter ***/
    /*** -------------- ***/

    inline PropertyGetter::PropertyGetter(IProxy& proxy, const std::string& propertyName)
        : proxy_(proxy)
        , propertyName_(propertyName)
    {
    }

    inline sdbus::Variant PropertyGetter::onInterface(const std::string& interfaceName)
    {
        sdbus::Variant var;
        proxy_
            .callMethod("Get")
            .onInterface("org.freedesktop.DBus.Properties")
            .withArguments(interfaceName, propertyName_)
            .storeResultsTo(var);
        return var;
    }

    /*** -------------- ***/
    /*** PropertySetter ***/
    /*** -------------- ***/

    inline PropertySetter::PropertySetter(IProxy& proxy, const std::string& propertyName)
        : proxy_(proxy)
        , propertyName_(propertyName)
    {
    }

    inline PropertySetter& PropertySetter::onInterface(std::string interfaceName)
    {
        interfaceName_ = std::move(interfaceName);

        return *this;
    }

    template <typename _Value>
    inline void PropertySetter::toValue(const _Value& value)
    {
        PropertySetter::toValue(sdbus::Variant{value});
    }

    inline void PropertySetter::toValue(const sdbus::Variant& value)
    {
        assert(!interfaceName_.empty()); // onInterface() must be placed/called prior to this function

        proxy_
            .callMethod("Set")
            .onInterface("org.freedesktop.DBus.Properties")
            .withArguments(interfaceName_, propertyName_, value);
    }

}

#endif /* SDBUS_CPP_CONVENIENCEAPICLASSES_INL_ */
