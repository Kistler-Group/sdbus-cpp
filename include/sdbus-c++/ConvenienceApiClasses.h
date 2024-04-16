/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2024 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
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
#include <sdbus-c++/VTableItems.h>

#include <chrono>
#include <cstdint>
#include <future>
#include <map>
#include <string>
#include <string_view>
#include <vector>

// Forward declarations
namespace sdbus {
    class IObject;
    class IProxy;
    class Error;
    class PendingAsyncCall;
}

namespace sdbus {

    class VTableAdder
    {
    public:
        VTableAdder(IObject& object, std::vector<VTableItem> vtable);
        void forInterface(InterfaceName interfaceName);
        void forInterface(std::string interfaceName);
        [[nodiscard]] Slot forInterface(InterfaceName interfaceName, return_slot_t);
        [[nodiscard]] Slot forInterface(std::string interfaceName, return_slot_t);

    private:
        IObject& object_;
        std::vector<VTableItem> vtable_;
    };

    class SignalEmitter
    {
    public:
        SignalEmitter(IObject& object, const SignalName& signalName);
        SignalEmitter(SignalEmitter&& other) = default;
        ~SignalEmitter() noexcept(false);
        SignalEmitter& onInterface(const InterfaceName& interfaceName);
        SignalEmitter& onInterface(const std::string& interfaceName);
        template <typename... _Args> void withArguments(_Args&&... args);

    private:
        IObject& object_;
        const SignalName& signalName_;
        Signal signal_;
        int exceptions_{}; // Number of active exceptions when SignalEmitter is constructed
    };

    class MethodInvoker
    {
    public:
        MethodInvoker(IProxy& proxy, const MethodName& methodName);
        MethodInvoker(MethodInvoker&& other) = default;
        ~MethodInvoker() noexcept(false);

        MethodInvoker& onInterface(const InterfaceName& interfaceName);
        MethodInvoker& onInterface(const std::string& interfaceName);
        MethodInvoker& withTimeout(uint64_t usec);
        template <typename _Rep, typename _Period>
        MethodInvoker& withTimeout(const std::chrono::duration<_Rep, _Period>& timeout);
        template <typename... _Args> MethodInvoker& withArguments(_Args&&... args);
        template <typename... _Args> void storeResultsTo(_Args&... args);

        void dontExpectReply();

    private:
        IProxy& proxy_;
        const MethodName& methodName_;
        uint64_t timeout_{};
        MethodCall method_;
        int exceptions_{}; // Number of active exceptions when MethodInvoker is constructed
        bool methodCalled_{};
    };

    class AsyncMethodInvoker
    {
    public:
        AsyncMethodInvoker(IProxy& proxy, const MethodName& methodName);
        AsyncMethodInvoker& onInterface(const InterfaceName& interfaceName);
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
        const MethodName& methodName_;
        uint64_t timeout_{};
        MethodCall method_;
    };

    class SignalSubscriber
    {
    public:
        SignalSubscriber(IProxy& proxy, const SignalName& signalName);
        SignalSubscriber& onInterface(InterfaceName interfaceName); // TODO: This could be const char*
        SignalSubscriber& onInterface(std::string interfaceName); // TODO: This could be const char*
        template <typename _Function> void call(_Function&& callback);
        template <typename _Function> [[nodiscard]] Slot call(_Function&& callback, return_slot_t);

    private:
        template <typename _Function> signal_handler makeSignalHandler(_Function&& callback);

    private:
        IProxy& proxy_;
        const SignalName& signalName_;
        InterfaceName interfaceName_;
    };

    class PropertyGetter
    {
    public:
        PropertyGetter(IProxy& proxy, std::string_view propertyName);
        Variant onInterface(std::string_view interfaceName);

    private:
        static inline const InterfaceName DBUS_PROPERTIES_INTERFACE_NAME{"org.freedesktop.DBus.Properties"};

    private:
        IProxy& proxy_;
        std::string_view propertyName_;
    };

    class AsyncPropertyGetter
    {
    public:
        AsyncPropertyGetter(IProxy& proxy, std::string_view propertyName);
        AsyncPropertyGetter& onInterface(std::string_view interfaceName);
        template <typename _Function> PendingAsyncCall uponReplyInvoke(_Function&& callback);
        std::future<Variant> getResultAsFuture();

    private:
        static inline const InterfaceName DBUS_PROPERTIES_INTERFACE_NAME{"org.freedesktop.DBus.Properties"};

    private:
        IProxy& proxy_;
        std::string_view propertyName_;
        std::string_view interfaceName_;
    };

    class PropertySetter
    {
    public:
        PropertySetter(IProxy& proxy, std::string_view propertyName);
        PropertySetter& onInterface(std::string_view interfaceName);
        template <typename _Value> void toValue(const _Value& value);
        template <typename _Value> void toValue(const _Value& value, dont_expect_reply_t);
        void toValue(const Variant& value);
        void toValue(const Variant& value, dont_expect_reply_t);

    private:
        static inline const InterfaceName DBUS_PROPERTIES_INTERFACE_NAME{"org.freedesktop.DBus.Properties"};

    private:
        IProxy& proxy_;
        std::string_view propertyName_;
        std::string_view interfaceName_;
    };

    class AsyncPropertySetter
    {
    public:
        AsyncPropertySetter(IProxy& proxy, std::string_view propertyName);
        AsyncPropertySetter& onInterface(std::string_view interfaceName);
        template <typename _Value> AsyncPropertySetter& toValue(_Value&& value);
        AsyncPropertySetter& toValue(Variant value);
        template <typename _Function> PendingAsyncCall uponReplyInvoke(_Function&& callback);
        std::future<void> getResultAsFuture();

    private:
        static inline const InterfaceName DBUS_PROPERTIES_INTERFACE_NAME{"org.freedesktop.DBus.Properties"};

    private:
        IProxy& proxy_;
        std::string_view propertyName_;
        std::string_view interfaceName_;
        Variant value_;
    };

    class AllPropertiesGetter
    {
    public:
        AllPropertiesGetter(IProxy& proxy);
        std::map<PropertyName, Variant> onInterface(std::string_view interfaceName);

    private:
        static inline const InterfaceName DBUS_PROPERTIES_INTERFACE_NAME{"org.freedesktop.DBus.Properties"};

    private:
        IProxy& proxy_;
    };

    class AsyncAllPropertiesGetter
    {
    public:
        AsyncAllPropertiesGetter(IProxy& proxy);
        AsyncAllPropertiesGetter& onInterface(std::string_view interfaceName);
        template <typename _Function> PendingAsyncCall uponReplyInvoke(_Function&& callback);
        std::future<std::map<PropertyName, Variant>> getResultAsFuture();

    private:
        static inline const InterfaceName DBUS_PROPERTIES_INTERFACE_NAME{"org.freedesktop.DBus.Properties"}; // TODO: Couldn't this be const char*?

    private:
        IProxy& proxy_;
        std::string_view interfaceName_;
    };

} // namespace sdbus

#endif /* SDBUS_CXX_CONVENIENCEAPICLASSES_H_ */
