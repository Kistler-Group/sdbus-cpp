/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2022 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file ConvenienceApiClasses.h
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

#ifndef SDBUS_CXX_CONVENIENCEAPICLASSES_H_
#define SDBUS_CXX_CONVENIENCEAPICLASSES_H_

#include <sdbus-c++/Message.h>
#include <sdbus-c++/TypeTraits.h>
#include <sdbus-c++/Types.h>
#include <sdbus-c++/Flags.h>
#include <string>
#include <vector>
#include <type_traits>
#include <chrono>
#include <future>
#include <cstdint>

// Forward declarations
namespace sdbus {
    class IObject;
    class IProxy;
    class Error;
    class PendingAsyncCall;
}

namespace sdbus {

    class MethodRegistrator
    {
    public:
        MethodRegistrator(IObject& object, std::string methodName);
        MethodRegistrator(MethodRegistrator&& other) = default;
        ~MethodRegistrator() noexcept(false);

        MethodRegistrator& onInterface(std::string interfaceName);
        template <typename _Function> MethodRegistrator& implementedAs(_Function&& callback);
        MethodRegistrator& withInputParamNames(std::vector<std::string> paramNames);
        template <typename... _String> MethodRegistrator& withInputParamNames(_String... paramNames);
        MethodRegistrator& withOutputParamNames(std::vector<std::string> paramNames);
        template <typename... _String> MethodRegistrator& withOutputParamNames(_String... paramNames);
        MethodRegistrator& markAsDeprecated();
        MethodRegistrator& markAsPrivileged();
        MethodRegistrator& withNoReply();

    private:
        IObject& object_;
        std::string methodName_;
        std::string interfaceName_;
        std::string inputSignature_;
        std::vector<std::string> inputParamNames_;
        std::string outputSignature_;
        std::vector<std::string> outputParamNames_;
        method_callback methodCallback_;
        Flags flags_;
        int exceptions_{}; // Number of active exceptions when SignalRegistrator is constructed
    };

    class SignalRegistrator
    {
    public:
        SignalRegistrator(IObject& object, std::string signalName);
        SignalRegistrator(SignalRegistrator&& other) = default;
        ~SignalRegistrator() noexcept(false);

        SignalRegistrator& onInterface(std::string interfaceName);
        template <typename... _Args> SignalRegistrator& withParameters();
        template <typename... _Args> SignalRegistrator& withParameters(std::vector<std::string> paramNames);
        template <typename... _Args, typename... _String> SignalRegistrator& withParameters(_String... paramNames);
        SignalRegistrator& markAsDeprecated();

    private:
        IObject& object_;
        std::string signalName_;
        std::string interfaceName_;
        std::string signalSignature_;
        std::vector<std::string> paramNames_;
        Flags flags_;
        int exceptions_{}; // Number of active exceptions when SignalRegistrator is constructed
    };

    class PropertyRegistrator
    {
    public:
        PropertyRegistrator(IObject& object, const std::string& propertyName);
        PropertyRegistrator(PropertyRegistrator&& other) = default;
        ~PropertyRegistrator() noexcept(false);

        PropertyRegistrator& onInterface(std::string interfaceName);
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
        MethodInvoker(IProxy& proxy, const std::string& methodName);
        MethodInvoker(MethodInvoker&& other) = default;
        ~MethodInvoker() noexcept(false);

        MethodInvoker& onInterface(const std::string& interfaceName);
        MethodInvoker& withTimeout(uint64_t usec);
        template <typename _Rep, typename _Period>
        MethodInvoker& withTimeout(const std::chrono::duration<_Rep, _Period>& timeout);
        template <typename... _Args> MethodInvoker& withArguments(_Args&&... args);
        template <typename... _Args> void storeResultsTo(_Args&... args);

        void dontExpectReply();

    private:
        IProxy& proxy_;
        const std::string& methodName_;
        uint64_t timeout_{};
        MethodCall method_;
        int exceptions_{}; // Number of active exceptions when MethodInvoker is constructed
        bool methodCalled_{};
    };

    class AsyncMethodInvoker
    {
    public:
        AsyncMethodInvoker(IProxy& proxy, const std::string& methodName);
        AsyncMethodInvoker& onInterface(const std::string& interfaceName);
        AsyncMethodInvoker& withTimeout(uint64_t usec);
        template <typename _Rep, typename _Period>
        AsyncMethodInvoker& withTimeout(const std::chrono::duration<_Rep, _Period>& timeout);
        template <typename... _Args> AsyncMethodInvoker& withArguments(_Args&&... args);
        template <typename _Function> PendingAsyncCall uponReplyInvoke(_Function&& callback);
        // Returned future will be std::future<void> for no (void) D-Bus method return value
        //                      or std::future<T> for single D-Bus method return value
        //                      or std::future<std::tuple<...>> for multiple method return values
        template <typename... _Args> std::future<future_return_t<_Args...>> getResultAsFuture();

    private:
        IProxy& proxy_;
        const std::string& methodName_;
        uint64_t timeout_{};
        MethodCall method_;
    };

    class SignalSubscriber
    {
    public:
        SignalSubscriber(IProxy& proxy, const std::string& signalName);
        SignalSubscriber& onInterface(std::string interfaceName);
        template <typename _Function> void call(_Function&& callback);

    private:
        IProxy& proxy_;
        const std::string& signalName_;
        std::string interfaceName_;
    };

    class SignalUnsubscriber
    {
    public:
        SignalUnsubscriber(IProxy& proxy, const std::string& signalName);
        void onInterface(const std::string& interfaceName);

    private:
        IProxy& proxy_;
        const std::string& signalName_;
    };

    class PropertyGetter
    {
    public:
        PropertyGetter(IProxy& proxy, const std::string& propertyName);
        Variant onInterface(const std::string& interfaceName);

    private:
        IProxy& proxy_;
        const std::string& propertyName_;
    };

    class AsyncPropertyGetter
    {
    public:
        AsyncPropertyGetter(IProxy& proxy, const std::string& propertyName);
        AsyncPropertyGetter& onInterface(const std::string& interfaceName);
        template <typename _Function> PendingAsyncCall uponReplyInvoke(_Function&& callback);
        std::future<Variant> getResultAsFuture();

    private:
        IProxy& proxy_;
        const std::string& propertyName_;
        const std::string* interfaceName_{};
    };

    class PropertySetter
    {
    public:
        PropertySetter(IProxy& proxy, const std::string& propertyName);
        PropertySetter& onInterface(const std::string& interfaceName);
        template <typename _Value> void toValue(const _Value& value);
        template <typename _Value> void toValue(const _Value& value, dont_expect_reply_t);
        void toValue(const Variant& value);
        void toValue(const Variant& value, dont_expect_reply_t);

    private:
        IProxy& proxy_;
        const std::string& propertyName_;
        const std::string* interfaceName_{};
    };

    class AsyncPropertySetter
    {
    public:
        AsyncPropertySetter(IProxy& proxy, const std::string& propertyName);
        AsyncPropertySetter& onInterface(const std::string& interfaceName);
        template <typename _Value> AsyncPropertySetter& toValue(_Value&& value);
        AsyncPropertySetter& toValue(Variant value);
        template <typename _Function> PendingAsyncCall uponReplyInvoke(_Function&& callback);
        std::future<void> getResultAsFuture();

    private:
        IProxy& proxy_;
        const std::string& propertyName_;
        const std::string* interfaceName_{};
        Variant value_;
    };

    class AllPropertiesGetter
    {
    public:
        AllPropertiesGetter(IProxy& proxy);
        std::map<std::string, Variant> onInterface(const std::string& interfaceName);

    private:
        IProxy& proxy_;
    };

    class AsyncAllPropertiesGetter
    {
    public:
        AsyncAllPropertiesGetter(IProxy& proxy);
        AsyncAllPropertiesGetter& onInterface(const std::string& interfaceName);
        template <typename _Function> PendingAsyncCall uponReplyInvoke(_Function&& callback);
        std::future<std::map<std::string, Variant>> getResultAsFuture();

    private:
        IProxy& proxy_;
        const std::string* interfaceName_{};
    };

}

#endif /* SDBUS_CXX_CONVENIENCEAPICLASSES_H_ */
