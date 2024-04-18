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
        void forInterface(InterfaceName interfaceName);
        void forInterface(std::string interfaceName);
        [[nodiscard]] Slot forInterface(InterfaceName interfaceName, return_slot_t);
        [[nodiscard]] Slot forInterface(std::string interfaceName, return_slot_t);

    private:
        friend IObject;
        VTableAdder(IObject& object, std::vector<VTableItem> vtable);

    private:
        IObject& object_;
        std::vector<VTableItem> vtable_;
    };

    class SignalEmitter
    {
    public:
        SignalEmitter& onInterface(const InterfaceName& interfaceName);
        SignalEmitter& onInterface(const std::string& interfaceName);
        SignalEmitter& onInterface(const char* interfaceName);
        template <typename... _Args> void withArguments(_Args&&... args);

        SignalEmitter(SignalEmitter&& other) = default;
        ~SignalEmitter() noexcept(false);

    private:
        friend IObject;
        SignalEmitter(IObject& object, const SignalName& signalName);
        SignalEmitter(IObject& object, const char* signalName);

    private:
        IObject& object_;
        const char* signalName_;
        Signal signal_;
        int exceptions_{}; // Number of active exceptions when SignalEmitter is constructed
    };

    class MethodInvoker
    {
    public:
        MethodInvoker& onInterface(const InterfaceName& interfaceName);
        MethodInvoker& onInterface(const std::string& interfaceName);
        MethodInvoker& onInterface(const char* interfaceName);
        MethodInvoker& withTimeout(uint64_t usec);
        template <typename _Rep, typename _Period>
        MethodInvoker& withTimeout(const std::chrono::duration<_Rep, _Period>& timeout);
        template <typename... _Args> MethodInvoker& withArguments(_Args&&... args);
        template <typename... _Args> void storeResultsTo(_Args&... args);
        void dontExpectReply();

        MethodInvoker(MethodInvoker&& other) = default;
        ~MethodInvoker() noexcept(false);

    private:
        friend IProxy;
        MethodInvoker(IProxy& proxy, const MethodName& methodName);
        MethodInvoker(IProxy& proxy, const char* methodName);

    private:
        IProxy& proxy_;
        const char* methodName_;
        uint64_t timeout_{};
        MethodCall method_;
        int exceptions_{}; // Number of active exceptions when MethodInvoker is constructed
        bool methodCalled_{};
    };

    class AsyncMethodInvoker
    {
    public:
        AsyncMethodInvoker& onInterface(const InterfaceName& interfaceName);
        AsyncMethodInvoker& onInterface(const std::string& interfaceName);
        AsyncMethodInvoker& onInterface(const char* interfaceName);
        AsyncMethodInvoker& withTimeout(uint64_t usec);
        template <typename _Rep, typename _Period>
        AsyncMethodInvoker& withTimeout(const std::chrono::duration<_Rep, _Period>& timeout);
        template <typename... _Args> AsyncMethodInvoker& withArguments(_Args&&... args);
        template <typename _Function> PendingAsyncCall uponReplyInvoke(_Function&& callback);
        template <typename _Function> [[nodiscard]] Slot uponReplyInvoke(_Function&& callback, return_slot_t);
        // Returned future will be std::future<void> for no (void) D-Bus method return value
        //                      or std::future<T> for single D-Bus method return value
        //                      or std::future<std::tuple<...>> for multiple method return values
        template <typename... _Args> std::future<future_return_t<_Args...>> getResultAsFuture();

    private:
        friend IProxy;
        AsyncMethodInvoker(IProxy& proxy, const MethodName& methodName);
        AsyncMethodInvoker(IProxy& proxy, const char* methodName);
        template <typename _Function> async_reply_handler makeAsyncReplyHandler(_Function&& callback);

    private:
        IProxy& proxy_;
        const char* methodName_;
        uint64_t timeout_{};
        MethodCall method_;
    };

    class SignalSubscriber
    {
    public:
        SignalSubscriber& onInterface(const InterfaceName& interfaceName);
        SignalSubscriber& onInterface(const std::string& interfaceName);
        SignalSubscriber& onInterface(const char* interfaceName);
        template <typename _Function> void call(_Function&& callback);
        template <typename _Function> [[nodiscard]] Slot call(_Function&& callback, return_slot_t);

    private:
        friend IProxy;
        SignalSubscriber(IProxy& proxy, const SignalName& signalName);
        SignalSubscriber(IProxy& proxy, const char* signalName);
        template <typename _Function> signal_handler makeSignalHandler(_Function&& callback);

    private:
        IProxy& proxy_;
        const char* signalName_;
        const char* interfaceName_{};
    };

    class PropertyGetter
    {
    public:
        Variant onInterface(std::string_view interfaceName);

    private:
        friend IProxy;
        PropertyGetter(IProxy& proxy, std::string_view propertyName);

        static constexpr const char* DBUS_PROPERTIES_INTERFACE_NAME = "org.freedesktop.DBus.Properties";

    private:
        IProxy& proxy_;
        std::string_view propertyName_;
    };

    class AsyncPropertyGetter
    {
    public:
        AsyncPropertyGetter& onInterface(std::string_view interfaceName);
        template <typename _Function> PendingAsyncCall uponReplyInvoke(_Function&& callback);
        template <typename _Function> [[nodiscard]] Slot uponReplyInvoke(_Function&& callback, return_slot_t);
        std::future<Variant> getResultAsFuture();

    private:
        friend IProxy;
        AsyncPropertyGetter(IProxy& proxy, std::string_view propertyName);

        static constexpr const char* DBUS_PROPERTIES_INTERFACE_NAME = "org.freedesktop.DBus.Properties";

    private:
        IProxy& proxy_;
        std::string_view propertyName_;
        std::string_view interfaceName_;
    };

    class PropertySetter
    {
    public:
        PropertySetter& onInterface(std::string_view interfaceName);
        template <typename _Value> void toValue(const _Value& value);
        template <typename _Value> void toValue(const _Value& value, dont_expect_reply_t);
        void toValue(const Variant& value);
        void toValue(const Variant& value, dont_expect_reply_t);

    private:
        friend IProxy;
        PropertySetter(IProxy& proxy, std::string_view propertyName);

        static constexpr const char* DBUS_PROPERTIES_INTERFACE_NAME = "org.freedesktop.DBus.Properties";

    private:
        IProxy& proxy_;
        std::string_view propertyName_;
        std::string_view interfaceName_;
    };

    class AsyncPropertySetter
    {
    public:
        AsyncPropertySetter& onInterface(std::string_view interfaceName);
        template <typename _Value> AsyncPropertySetter& toValue(_Value&& value);
        AsyncPropertySetter& toValue(Variant value);
        template <typename _Function> PendingAsyncCall uponReplyInvoke(_Function&& callback);
        template <typename _Function> [[nodiscard]] Slot uponReplyInvoke(_Function&& callback, return_slot_t);
        std::future<void> getResultAsFuture();

    private:
        friend IProxy;
        AsyncPropertySetter(IProxy& proxy, std::string_view propertyName);

        static constexpr const char* DBUS_PROPERTIES_INTERFACE_NAME = "org.freedesktop.DBus.Properties";

    private:
        IProxy& proxy_;
        std::string_view propertyName_;
        std::string_view interfaceName_;
        Variant value_;
    };

    class AllPropertiesGetter
    {
    public:
        std::map<PropertyName, Variant> onInterface(std::string_view interfaceName);

    private:
        friend IProxy;
        AllPropertiesGetter(IProxy& proxy);

        static constexpr const char* DBUS_PROPERTIES_INTERFACE_NAME = "org.freedesktop.DBus.Properties";

    private:
        IProxy& proxy_;
    };

    class AsyncAllPropertiesGetter
    {
    public:
        AsyncAllPropertiesGetter& onInterface(std::string_view interfaceName);
        template <typename _Function> PendingAsyncCall uponReplyInvoke(_Function&& callback);
        template <typename _Function> [[nodiscard]] Slot uponReplyInvoke(_Function&& callback, return_slot_t);
        std::future<std::map<PropertyName, Variant>> getResultAsFuture();

    private:
        friend IProxy;
        AsyncAllPropertiesGetter(IProxy& proxy);

        static constexpr const char* DBUS_PROPERTIES_INTERFACE_NAME = "org.freedesktop.DBus.Properties";

    private:
        IProxy& proxy_;
        std::string_view interfaceName_;
    };

} // namespace sdbus

#endif /* SDBUS_CXX_CONVENIENCEAPICLASSES_H_ */
