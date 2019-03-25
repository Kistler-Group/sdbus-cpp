/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 *
 * @file ConvenienceClasses.h
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

#ifndef SDBUS_CXX_CONVENIENCECLASSES_H_
#define SDBUS_CXX_CONVENIENCECLASSES_H_

#include <sdbus-c++/Message.h>
#include <sdbus-c++/TypeTraits.h>
#include <sdbus-c++/Flags.h>
#include <string>
#include <type_traits>

// Forward declarations
namespace sdbus {
    class IObject;
    class IObjectProxy;
    class Variant;
    class Error;
}

namespace sdbus {

    class MethodRegistrator
    {
    public:
        MethodRegistrator(IObject& object, const std::string& methodName);
        MethodRegistrator(MethodRegistrator&& other) = default;
        MethodRegistrator& operator=(MethodRegistrator&& other) = default;
        ~MethodRegistrator() noexcept(false);

        MethodRegistrator& onInterface(const std::string& interfaceName);
        template <typename _Function>
        std::enable_if_t<!is_async_method_v<_Function>, MethodRegistrator&> implementedAs(_Function&& callback);
        template <typename _Function>
        std::enable_if_t<is_async_method_v<_Function>, MethodRegistrator&> implementedAs(_Function&& callback);
        MethodRegistrator& markAsDeprecated();
        MethodRegistrator& markAsPrivileged();
        MethodRegistrator& withNoReply();

    private:
        IObject& object_;
        const std::string& methodName_;
        std::string interfaceName_;
        std::string inputSignature_;
        std::string outputSignature_;
        method_callback syncCallback_;
        async_method_callback asyncCallback_;
        Flags flags_;
        int exceptions_{}; // Number of active exceptions when SignalRegistrator is constructed
    };

    class SignalRegistrator
    {
    public:
        SignalRegistrator(IObject& object, const std::string& signalName);
        SignalRegistrator(SignalRegistrator&& other) = default;
        SignalRegistrator& operator=(SignalRegistrator&& other) = default;
        ~SignalRegistrator() noexcept(false);

        SignalRegistrator& onInterface(std::string interfaceName);
        template <typename... _Args> SignalRegistrator& withParameters();
        SignalRegistrator& markAsDeprecated();

    private:
        IObject& object_;
        const std::string& signalName_;
        std::string interfaceName_;
        std::string signalSignature_;
        Flags flags_;
        int exceptions_{}; // Number of active exceptions when SignalRegistrator is constructed
    };

    class PropertyRegistrator
    {
    public:
        PropertyRegistrator(IObject& object, const std::string& propertyName);
        PropertyRegistrator(PropertyRegistrator&& other) = default;
        PropertyRegistrator& operator=(PropertyRegistrator&& other) = default;
        ~PropertyRegistrator() noexcept(false);

        PropertyRegistrator& onInterface(const std::string& interfaceName);
        template <typename _Function> PropertyRegistrator& withGetter(_Function&& callback);
        template <typename _Function> PropertyRegistrator& withSetter(_Function&& callback);
        PropertyRegistrator& markAsDeprecated();
        PropertyRegistrator& markAsPrivileged();
        PropertyRegistrator& withUpdateBehavior(Flags::PropertyUpdateBehaviorFlags behavior);

    private:
        IObject& object_;
        const std::string& propertyName_;
        std::string interfaceName_;
        std::string propertySignature_;
        property_get_callback getter_;
        property_set_callback setter_;
        Flags flags_;
        int exceptions_{}; // Number of active exceptions when PropertyRegistrator is constructed
    };

    class InterfaceFlagsSetter
    {
    public:
        InterfaceFlagsSetter(IObject& object, const std::string& interfaceName);
        InterfaceFlagsSetter(InterfaceFlagsSetter&& other) = default;
        InterfaceFlagsSetter& operator=(InterfaceFlagsSetter&& other) = default;
        ~InterfaceFlagsSetter() noexcept(false);

        InterfaceFlagsSetter& markAsDeprecated();
        InterfaceFlagsSetter& markAsPrivileged();
        InterfaceFlagsSetter& withNoReplyMethods();
        InterfaceFlagsSetter& withPropertyUpdateBehavior(Flags::PropertyUpdateBehaviorFlags behavior);

    private:
        IObject& object_;
        const std::string& interfaceName_;
        Flags flags_;
        int exceptions_{}; // Number of active exceptions when InterfaceFlagsSetter is constructed
    };

    class SignalEmitter
    {
    public:
        SignalEmitter(IObject& object, const std::string& signalName);
        SignalEmitter(SignalEmitter&& other) = default;
        SignalEmitter& operator=(SignalEmitter&& other) = default;
        ~SignalEmitter() noexcept(false);
        SignalEmitter& onInterface(const std::string& interfaceName);
        template <typename... _Args> void withArguments(_Args&&... args);

    private:
        IObject& object_;
        const std::string& signalName_;
        Signal signal_;
        int exceptions_{}; // Number of active exceptions when SignalEmitter is constructed
    };

    class MethodInvoker
    {
    public:
        MethodInvoker(IObjectProxy& objectProxy, const std::string& methodName);
        MethodInvoker(MethodInvoker&& other) = default;
        MethodInvoker& operator=(MethodInvoker&& other) = default;
        ~MethodInvoker() noexcept(false);

        MethodInvoker& onInterface(const std::string& interfaceName);
        template <typename... _Args> MethodInvoker& withArguments(_Args&&... args);
        template <typename... _Args> void storeResultsTo(_Args&... args);

        void dontExpectReply();

    private:
        IObjectProxy& objectProxy_;
        const std::string& methodName_;
        MethodCall method_;
        int exceptions_{}; // Number of active exceptions when MethodInvoker is constructed
        bool methodCalled_{};
    };

    class AsyncMethodInvoker
    {
    public:
        AsyncMethodInvoker(IObjectProxy& objectProxy, const std::string& methodName);
        AsyncMethodInvoker& onInterface(const std::string& interfaceName);
        template <typename... _Args> AsyncMethodInvoker& withArguments(_Args&&... args);
        template <typename _Function> void uponReplyInvoke(_Function&& callback);

    private:
        IObjectProxy& objectProxy_;
        const std::string& methodName_;
        AsyncMethodCall method_;
    };

    class SignalSubscriber
    {
    public:
        SignalSubscriber(IObjectProxy& objectProxy, const std::string& signalName);
        SignalSubscriber& onInterface(const std::string& interfaceName);
        template <typename _Function> void call(_Function&& callback);

    private:
        IObjectProxy& objectProxy_;
        std::string signalName_;
        std::string interfaceName_;
    };

    class PropertyGetter
    {
    public:
        PropertyGetter(IObjectProxy& objectProxy, const std::string& propertyName);
        sdbus::Variant onInterface(const std::string& interfaceName);

    private:
        IObjectProxy& objectProxy_;
        std::string propertyName_;
    };

    class PropertySetter
    {
    public:
        PropertySetter(IObjectProxy& objectProxy, const std::string& propertyName);
        PropertySetter& onInterface(const std::string& interfaceName);
        template <typename _Value> void toValue(const _Value& value);

    private:
        IObjectProxy& objectProxy_;
        const std::string& propertyName_;
        std::string interfaceName_;
    };

}

#endif /* SDBUS_CXX_CONVENIENCECLASSES_H_ */
