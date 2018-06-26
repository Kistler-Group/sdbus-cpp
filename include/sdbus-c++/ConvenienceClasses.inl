/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 *
 * @file ConvenienceClasses.inl
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

#ifndef SDBUS_CPP_CONVENIENCECLASSES_INL_
#define SDBUS_CPP_CONVENIENCECLASSES_INL_

#include <sdbus-c++/IObject.h>
#include <sdbus-c++/IObjectProxy.h>
#include <sdbus-c++/Message.h>
#include <sdbus-c++/MethodResult.h>
#include <sdbus-c++/Types.h>
#include <sdbus-c++/TypeTraits.h>
#include <sdbus-c++/Error.h>
#include <string>
#include <tuple>
/*#include <exception>*/

namespace sdbus {

    inline MethodRegistrator::MethodRegistrator(IObject& object, const std::string& methodName)
        : object_(object)
        , methodName_(methodName)
    {
    }

    inline MethodRegistrator& MethodRegistrator::onInterface(const std::string& interfaceName)
    {
        interfaceName_ = interfaceName;

        return *this;
    }

    template <typename _Function>
    inline std::enable_if_t<!is_async_method_v<_Function>> MethodRegistrator::implementedAs(_Function&& callback)
    {
        SDBUS_THROW_ERROR_IF(interfaceName_.empty(), "DBus interface not specified when registering a DBus method", EINVAL);

        object_.registerMethod( interfaceName_
                              , methodName_
                              , signature_of_function_input_arguments<_Function>::str()
                              , signature_of_function_output_arguments<_Function>::str()
                              , [callback = std::forward<_Function>(callback)](MethodCall& msg, MethodReply& reply)
        {
            // Create a tuple of callback input arguments' types, which will be used
            // as a storage for the argument values deserialized from the message.
            tuple_of_function_input_arg_types_t<_Function> inputArgs;

            // Deserialize input arguments from the message into the tuple
            msg >> inputArgs;

//            if constexpr (std::is_void<function_result_t<_Function>>::value)
//            {
//                apply(callback, inputArgs);
//            }
//            else
//            {
//                reply << apply(callback, inputArgs);
//            }

            // Invoke callback with input arguments from the tuple.
            // For callbacks returning a non-void value, `apply' also returns that value.
            // For callbacks returning void, `apply' returns an empty tuple.
            auto ret = apply(callback, inputArgs); // We don't yet have C++17's std::apply :-(

            // The return value is stored to the reply message.
            // In case of void functions, ret is an empty tuple and thus nothing is stored.
            reply << ret;
        });
    }

    template <typename _Function>
    inline std::enable_if_t<is_async_method_v<_Function>> MethodRegistrator::implementedAs(_Function&& callback)
    {
        SDBUS_THROW_ERROR_IF(interfaceName_.empty(), "DBus interface not specified when registering a DBus method", EINVAL);

        object_.registerMethod( interfaceName_
                              , methodName_
                              , signature_of_function_input_arguments<_Function>::str()
                              , signature_of_function_output_arguments<_Function>::str() //signature_of<last_function_argument_t<_Function>>::str() // because last argument contains output types
                              , [callback = std::forward<_Function>(callback)](MethodCall& msg, MethodResult result)
        {
            // Create a tuple of callback input arguments' types, which will be used
            // as a storage for the argument values deserialized from the message.
            tuple_of_function_input_arg_types_t<_Function> inputArgs;

            // Deserialize input arguments from the message into the tuple,
            // plus store the result object as a last item of the tuple.
            msg >> inputArgs;
            //static_assert(std::tuple_size<decltype(inputArgs)>::value > 0);
            //std::get<std::tuple_size<decltype(inputArgs)>::value-1>(inputArgs) = std::move(result);
            //std::get<0>(inputArgs) = std::move(result);

            // Invoke callback with input arguments from the tuple.
            apply(callback, std::move(result), inputArgs); // TODO: Use std::apply when switching to full C++17 support
        });
    }

    // Moved into the library to isolate from C++17 dependency
    /*
    inline SignalRegistrator::SignalRegistrator(IObject& object, std::string signalName)
        : object_(object)
        , signalName_(std::move(signalName))
        , exceptions_(std::uncaught_exceptions())
    {
    }

    inline SignalRegistrator::~SignalRegistrator() noexcept(false) // since C++11, destructors must
    {                                                              // explicitly be allowed to throw
        // Don't register the signal if SignalRegistrator threw an exception in one of its methods
        if (std::uncaught_exceptions() != exceptions_)
            return;

        if (interfaceName_.empty())
            throw sdbus::Exception("DBus interface not specified when registering a DBus signal");

        // registerSignal() can throw. But as the SignalRegistrator shall always be used as an unnamed,
        // temporary object, i.e. not as a stack-allocated object, the double-exception situation
        // shall never happen. I.e. it should not happen that this destructor is directly called
        // in the stack-unwinding process of another flying exception (which would lead to immediate
        // termination). It can be called indirectly in the destructor of another object, but that's
        // fine and safe provided that the caller catches exceptions thrown from here.
        // Therefore, we can allow registerSignal() to throw even if we are in the destructor.
        // Bottomline is, to be on the safe side, the caller must take care of catching and reacting
        // to the exception thrown from here if the caller is a destructor itself.
        object_.registerSignal(interfaceName_, signalName_, signalSignature_);
    }
    */

    inline SignalRegistrator& SignalRegistrator::onInterface(std::string interfaceName)
    {
        interfaceName_ = std::move(interfaceName);

        return *this;
    }

    template <typename... _Args>
    inline void SignalRegistrator::withParameters()
    {
        signalSignature_ = signature_of_function_input_arguments<void(_Args...)>::str();
    }


    // Moved into the library to isolate from C++17 dependency
    /*
    inline PropertyRegistrator::PropertyRegistrator(IObject& object, std::string propertyName)
        : object_(object)
        , propertyName_(std::move(propertyName))
        , exceptions_(std::uncaught_exceptions())
    {
    }

    inline PropertyRegistrator::~PropertyRegistrator() noexcept(false) // since C++11, destructors must
    {                                                                  // explicitly be allowed to throw
        // Don't register the property if PropertyRegistrator threw an exception in one of its methods
        if (std::uncaught_exceptions() != exceptions_)
            return;

        SDBUS_THROW_ERROR_IF(interfaceName_.empty(), "DBus interface not specified when registering a DBus property", EINVAL);

        // registerProperty() can throw. But as the PropertyRegistrator shall always be used as an unnamed,
        // temporary object, i.e. not as a stack-allocated object, the double-exception situation
        // shall never happen. I.e. it should not happen that this destructor is directly called
        // in the stack-unwinding process of another flying exception (which would lead to immediate
        // termination). It can be called indirectly in the destructor of another object, but that's
        // fine and safe provided that the caller catches exceptions thrown from here.
        // Therefore, we can allow registerProperty() to throw even if we are in the destructor.
        // Bottomline is, to be on the safe side, the caller must take care of catching and reacting
        // to the exception thrown from here if the caller is a destructor itself.
        object_.registerProperty( std::move(interfaceName_)
                                , std::move(propertyName_)
                                , std::move(propertySignature_)
                                , std::move(getter_)
                                , std::move(setter_) );
    }
    */

    inline PropertyRegistrator& PropertyRegistrator::onInterface(const std::string& interfaceName)
    {
        interfaceName_ = interfaceName;

        return *this;
    }

    template <typename _Function>
    inline PropertyRegistrator& PropertyRegistrator::withGetter(_Function&& callback)
    {
        static_assert(function_traits<_Function>::arity == 0, "Property getter function must not take any arguments");
        static_assert(!std::is_void<function_result_t<_Function>>::value, "Property getter function must return property value");

        if (propertySignature_.empty())
            propertySignature_ = signature_of_function_output_arguments<_Function>::str();

        getter_ = [callback = std::forward<_Function>(callback)](Message& msg)
        {
            // Get the propety value and serialize it into the message
            msg << callback();
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

        setter_ = [callback = std::forward<_Function>(callback)](Message& msg)
        {
            // Default-construct property value
            using property_type = function_argument_t<_Function, 0>;
            std::decay_t<property_type> property;

            // Deserialize property value from the message
            msg >> property;

            // Invoke setter with the value
            callback(property);
        };

        return *this;
    }


    // Moved into the library to isolate from C++17 dependency
    /*
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

        if (!signal_.isValid())
            throw sdbus::Exception("DBus interface not specified when emitting a DBus signal");

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
    */

    inline SignalEmitter& SignalEmitter::onInterface(const std::string& interfaceName)
    {
        signal_ = object_.createSignal(interfaceName, signalName_);

        return *this;
    }

    template <typename... _Args>
    inline void SignalEmitter::withArguments(_Args&&... args)
    {
        SDBUS_THROW_ERROR_IF(!signal_.isValid(), "DBus interface not specified when emitting a DBus signal", EINVAL);

        detail::serialize_pack(signal_, std::forward<_Args>(args)...);
    }


    // Moved into the library to isolate from C++17 dependency
    /*
    inline MethodInvoker::MethodInvoker(IObjectProxy& objectProxy, const std::string& methodName)
        : objectProxy_(objectProxy)
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

        if (!method_.isValid())
            throw sdbus::Exception("DBus interface not specified when calling a DBus method");

        // callMethod() can throw. But as the MethodInvoker shall always be used as an unnamed,
        // temporary object, i.e. not as a stack-allocated object, the double-exception situation
        // shall never happen. I.e. it should not happen that this destructor is directly called
        // in the stack-unwinding process of another flying exception (which would lead to immediate
        // termination). It can be called indirectly in the destructor of another object, but that's
        // fine and safe provided that the caller catches exceptions thrown from here.
        // Therefore, we can allow callMethod() to throw even if we are in the destructor.
        // Bottomline is, to be on the safe side, the caller must take care of catching and reacting
        // to the exception thrown from here if the caller is a destructor itself.
        objectProxy_.callMethod(method_);
    }
    */

    inline MethodInvoker& MethodInvoker::onInterface(const std::string& interfaceName)
    {
        method_ = objectProxy_.createMethodCall(interfaceName, methodName_);

        return *this;
    }

    template <typename... _Args>
    inline MethodInvoker& MethodInvoker::withArguments(_Args&&... args)
    {
        SDBUS_THROW_ERROR_IF(!method_.isValid(), "DBus interface not specified when calling a DBus method", EINVAL);

        detail::serialize_pack(method_, std::forward<_Args>(args)...);

        return *this;
    }

    template <typename... _Args>
    inline void MethodInvoker::storeResultsTo(_Args&... args)
    {
        SDBUS_THROW_ERROR_IF(!method_.isValid(), "DBus interface not specified when calling a DBus method", EINVAL);

        auto reply = objectProxy_.callMethod(method_);
        methodCalled_ = true;

        detail::deserialize_pack(reply, args...);
    }


    inline SignalSubscriber::SignalSubscriber(IObjectProxy& objectProxy, const std::string& signalName)
        : objectProxy_(objectProxy)
        , signalName_(signalName)
    {
    }

    inline SignalSubscriber& SignalSubscriber::onInterface(const std::string& interfaceName)
    {
        interfaceName_ = interfaceName;

        return *this;
    }

    template <typename _Function>
    inline void SignalSubscriber::call(_Function&& callback)
    {
        SDBUS_THROW_ERROR_IF(interfaceName_.empty(), "DBus interface not specified when subscribing to a signal", EINVAL);

        objectProxy_.registerSignalHandler( interfaceName_
                                          , signalName_
                                          , [callback = std::forward<_Function>(callback)](Signal& signal)
        {
            // Create a tuple of callback input arguments' types, which will be used
            // as a storage for the argument values deserialized from the signal message.
            tuple_of_function_input_arg_types_t<_Function> signalArgs;

            // Deserialize input arguments from the signal message into the tuple
            signal >> signalArgs;

            // Invoke callback with input arguments from the tuple.
            apply(callback, signalArgs); // We don't yet have C++17's std::apply :-(
        });
    }


    inline PropertyGetter::PropertyGetter(IObjectProxy& objectProxy, const std::string& propertyName)
        : objectProxy_(objectProxy)
        , propertyName_(propertyName)
    {
    }

    inline sdbus::Variant PropertyGetter::onInterface(const std::string& interfaceName)
    {
        sdbus::Variant var;
        objectProxy_
            .callMethod("Get")
            .onInterface("org.freedesktop.DBus.Properties")
            .withArguments(interfaceName, propertyName_)
            .storeResultsTo(var);
        return var;
    }


    inline PropertySetter::PropertySetter(IObjectProxy& objectProxy, const std::string& propertyName)
        : objectProxy_(objectProxy)
        , propertyName_(propertyName)
    {
    }

    inline PropertySetter& PropertySetter::onInterface(const std::string& interfaceName)
    {
        interfaceName_ = interfaceName;

        return *this;
    }

    template <typename _Value>
    inline void PropertySetter::toValue(const _Value& value)
    {
        SDBUS_THROW_ERROR_IF(interfaceName_.empty(), "DBus interface not specified when setting a property", EINVAL);

        objectProxy_
            .callMethod("Set")
            .onInterface("org.freedesktop.DBus.Properties")
            .withArguments(interfaceName_, propertyName_, sdbus::Variant{value});
    }

}

#endif /* SDBUS_CPP_CONVENIENCECLASSES_INL_ */
