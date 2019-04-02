/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 *
 * @file ConvenienceClasses.cpp
 *
 * Created on: Jan 19, 2017
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

#include <sdbus-c++/ConvenienceClasses.h>
#include <sdbus-c++/IObject.h>
#include <sdbus-c++/IObjectProxy.h>
#include <string>
#include <exception>

namespace sdbus {

MethodRegistrator::MethodRegistrator(IObject& object, const std::string& methodName)
    : object_(object)
    , methodName_(methodName)
    , exceptions_(std::uncaught_exceptions()) // Needs C++17
{
}

MethodRegistrator::~MethodRegistrator() noexcept(false) // since C++11, destructors must
{                                                       // explicitly be allowed to throw
    // Don't register the method if MethodRegistrator threw an exception in one of its methods
    if (std::uncaught_exceptions() != exceptions_)
        return;

    SDBUS_THROW_ERROR_IF(interfaceName_.empty(), "DBus interface not specified when registering a DBus method", EINVAL);
    SDBUS_THROW_ERROR_IF(!methodCallback_, "Method handler not specified when registering a DBus method", EINVAL);

    // registerMethod() can throw. But as the MethodRegistrator shall always be used as an unnamed,
    // temporary object, i.e. not as a stack-allocated object, the double-exception situation
    // shall never happen. I.e. it should not happen that this destructor is directly called
    // in the stack-unwinding process of another flying exception (which would lead to immediate
    // termination). It can be called indirectly in the destructor of another object, but that's
    // fine and safe provided that the caller catches exceptions thrown from here.
    // Therefore, we can allow registerMethod() to throw even if we are in the destructor.
    // Bottomline is, to be on the safe side, the caller must take care of catching and reacting
    // to the exception thrown from here if the caller is a destructor itself.
    object_.registerMethod(interfaceName_, methodName_, inputSignature_, outputSignature_, std::move(methodCallback_), flags_);
}

SignalRegistrator::SignalRegistrator(IObject& object, const std::string& signalName)
    : object_(object)
    , signalName_(signalName)
    , exceptions_(std::uncaught_exceptions()) // Needs C++17
{
}

SignalRegistrator::~SignalRegistrator() noexcept(false) // since C++11, destructors must
{                                                       // explicitly be allowed to throw
    // Don't register the signal if SignalRegistrator threw an exception in one of its methods
    if (std::uncaught_exceptions() != exceptions_)
        return;

    SDBUS_THROW_ERROR_IF(interfaceName_.empty(), "DBus interface not specified when registering a DBus signal", EINVAL);

    // registerSignal() can throw. But as the SignalRegistrator shall always be used as an unnamed,
    // temporary object, i.e. not as a stack-allocated object, the double-exception situation
    // shall never happen. I.e. it should not happen that this destructor is directly called
    // in the stack-unwinding process of another flying exception (which would lead to immediate
    // termination). It can be called indirectly in the destructor of another object, but that's
    // fine and safe provided that the caller catches exceptions thrown from here.
    // Therefore, we can allow registerSignal() to throw even if we are in the destructor.
    // Bottomline is, to be on the safe side, the caller must take care of catching and reacting
    // to the exception thrown from here if the caller is a destructor itself.
    object_.registerSignal(interfaceName_, signalName_, signalSignature_, flags_);
}


PropertyRegistrator::PropertyRegistrator(IObject& object, const std::string& propertyName)
    : object_(object)
    , propertyName_(propertyName)
    , exceptions_(std::uncaught_exceptions())
{
}

PropertyRegistrator::~PropertyRegistrator() noexcept(false) // since C++11, destructors must
{                                                           // explicitly be allowed to throw
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
                            , std::move(setter_)
                            , flags_ );
}


InterfaceFlagsSetter::InterfaceFlagsSetter(IObject& object, const std::string& interfaceName)
    : object_(object)
    , interfaceName_(interfaceName)
    , exceptions_(std::uncaught_exceptions())
{
}

InterfaceFlagsSetter::~InterfaceFlagsSetter() noexcept(false) // since C++11, destructors must
{                                                                    // explicitly be allowed to throw
    // Don't set any flags if InterfaceFlagsSetter threw an exception in one of its methods
    if (std::uncaught_exceptions() != exceptions_)
        return;

    SDBUS_THROW_ERROR_IF(interfaceName_.empty(), "DBus interface not specified when setting its flags", EINVAL);

    // setInterfaceFlags() can throw. But as the InterfaceFlagsSetter shall always be used as an unnamed,
    // temporary object, i.e. not as a stack-allocated object, the double-exception situation
    // shall never happen. I.e. it should not happen that this destructor is directly called
    // in the stack-unwinding process of another flying exception (which would lead to immediate
    // termination). It can be called indirectly in the destructor of another object, but that's
    // fine and safe provided that the caller catches exceptions thrown from here.
    // Therefore, we can allow setInterfaceFlags() to throw even if we are in the destructor.
    // Bottomline is, to be on the safe side, the caller must take care of catching and reacting
    // to the exception thrown from here if the caller is a destructor itself.
    object_.setInterfaceFlags( std::move(interfaceName_)
                             , std::move(flags_) );
}


SignalEmitter::SignalEmitter(IObject& object, const std::string& signalName)
    : object_(object)
    , signalName_(signalName)
    , exceptions_(std::uncaught_exceptions()) // Needs C++17
{
}

SignalEmitter::~SignalEmitter() noexcept(false) // since C++11, destructors must
{                                               // explicitly be allowed to throw
    // Don't emit the signal if SignalEmitter threw an exception in one of its methods
    if (std::uncaught_exceptions() != exceptions_)
        return;

    SDBUS_THROW_ERROR_IF(!signal_.isValid(), "DBus interface not specified when emitting a DBus signal", EINVAL);

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


MethodInvoker::MethodInvoker(IObjectProxy& objectProxy, const std::string& methodName)
    : objectProxy_(objectProxy)
    , methodName_(methodName)
    , exceptions_(std::uncaught_exceptions()) // Needs C++17
{
}

MethodInvoker::~MethodInvoker() noexcept(false) // since C++11, destructors must
{                                               // explicitly be allowed to throw
    // Don't call the method if it has been called already or if MethodInvoker
    // threw an exception in one of its methods
    if (methodCalled_ || std::uncaught_exceptions() != exceptions_)
        return;

    SDBUS_THROW_ERROR_IF(!method_.isValid(), "DBus interface not specified when calling a DBus method", EINVAL);

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

}
