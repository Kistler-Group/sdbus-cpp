/**
 * (C) 2023 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file VTableItems.inl
 *
 * Created on: Dec 14, 2023
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

#ifndef SDBUS_CPP_VTABLEITEMS_INL_
#define SDBUS_CPP_VTABLEITEMS_INL_

#include <sdbus-c++/TypeTraits.h>
#include <sdbus-c++/Error.h>
#include <type_traits>
#include <string>
#include <vector>

namespace sdbus {

    /*** -------------------- ***/
    /***  Method VTable Item  ***/
    /*** -------------------- ***/

    template <typename _Function>
    MethodVTableItem& MethodVTableItem::implementedAs(_Function&& callback)
    {
        inputSignature = signature_of_function_input_arguments<_Function>::str();
        outputSignature = signature_of_function_output_arguments<_Function>::str();
        callbackHandler = [callback = std::forward<_Function>(callback)](MethodCall call)
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

    inline MethodVTableItem& MethodVTableItem::withInputParamNames(std::vector<std::string> names)
    {
        inputParamNames = std::move(names);

        return *this;
    }

    template <typename... _String>
    inline MethodVTableItem& MethodVTableItem::withInputParamNames(_String... names)
    {
        static_assert(std::conjunction_v<std::is_convertible<_String, std::string>...>, "Parameter names must be (convertible to) strings");

        return withInputParamNames({names...});
    }

    inline MethodVTableItem& MethodVTableItem::withOutputParamNames(std::vector<std::string> names)
    {
        outputParamNames = std::move(names);

        return *this;
    }

    template <typename... _String>
    inline MethodVTableItem& MethodVTableItem::withOutputParamNames(_String... names)
    {
        static_assert(std::conjunction_v<std::is_convertible<_String, std::string>...>, "Parameter names must be (convertible to) strings");

        return withOutputParamNames({names...});
    }

    inline MethodVTableItem& MethodVTableItem::markAsDeprecated()
    {
        flags.set(Flags::DEPRECATED);

        return *this;
    }

    inline MethodVTableItem& MethodVTableItem::markAsPrivileged()
    {
        flags.set(Flags::PRIVILEGED);

        return *this;
    }

    inline MethodVTableItem& MethodVTableItem::withNoReply()
    {
        flags.set(Flags::METHOD_NO_REPLY);

        return *this;
    }

    inline MethodVTableItem registerMethod(std::string methodName)
    {
        return {std::move(methodName), {}, {}, {}, {}, {}, {}};
    }

    /*** -------------------- ***/
    /***  Signal VTable Item  ***/
    /*** -------------------- ***/

    template <typename... _Args>
    inline SignalVTableItem& SignalVTableItem::withParameters()
    {
        signature = signature_of_function_input_arguments<void(_Args...)>::str();

        return *this;
    }

    template <typename... _Args>
    inline SignalVTableItem& SignalVTableItem::withParameters(std::vector<std::string> names)
    {
        paramNames = std::move(names);

        return withParameters<_Args...>();
    }

    template <typename... _Args, typename... _String>
    inline SignalVTableItem& SignalVTableItem::withParameters(_String... names)
    {
        static_assert(std::conjunction_v<std::is_convertible<_String, std::string>...>, "Parameter names must be (convertible to) strings");
        static_assert(sizeof...(_Args) == sizeof...(_String), "Numbers of signal parameters and their names don't match");

        return withParameters<_Args...>({names...});
    }

    inline SignalVTableItem& SignalVTableItem::markAsDeprecated()
    {
        flags.set(Flags::DEPRECATED);

        return *this;
    }

    inline SignalVTableItem registerSignal(std::string signalName)
    {
        return {std::move(signalName), {}, {}, {}};
    }

    /*** -------------------- ***/
    /*** Property VTable Item ***/
    /*** -------------------- ***/

    template <typename _Function>
    inline PropertyVTableItem& PropertyVTableItem::withGetter(_Function&& callback)
    {
        static_assert(function_argument_count_v<_Function> == 0, "Property getter function must not take any arguments");
        static_assert(!std::is_void<function_result_t<_Function>>::value, "Property getter function must return property value");

        if (signature.empty())
            signature = signature_of_function_output_arguments<_Function>::str();

        getter = [callback = std::forward<_Function>(callback)](PropertyGetReply& reply)
        {
            // Get the propety value and serialize it into the pre-constructed reply message
            reply << callback();
        };

        return *this;
    }

    template <typename _Function>
    inline PropertyVTableItem& PropertyVTableItem::withSetter(_Function&& callback)
    {
        static_assert(function_argument_count_v<_Function> == 1, "Property setter function must take one parameter - the property value");
        static_assert(std::is_void<function_result_t<_Function>>::value, "Property setter function must not return any value");

        if (signature.empty())
            signature = signature_of_function_input_arguments<_Function>::str();

        setter = [callback = std::forward<_Function>(callback)](PropertySetCall call)
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

    inline PropertyVTableItem& PropertyVTableItem::markAsDeprecated()
    {
        flags.set(Flags::DEPRECATED);

        return *this;
    }

    inline PropertyVTableItem& PropertyVTableItem::markAsPrivileged()
    {
        flags.set(Flags::PRIVILEGED);

        return *this;
    }

    inline PropertyVTableItem& PropertyVTableItem::withUpdateBehavior(Flags::PropertyUpdateBehaviorFlags behavior)
    {
        flags.set(behavior);

        return *this;
    }

    inline PropertyVTableItem registerProperty(std::string propertyName)
    {
        return {std::move(propertyName), {}, {}, {}, {}};
    }

    /*** --------------------------- ***/
    /*** Interface Flags VTable Item ***/
    /*** --------------------------- ***/

    inline InterfaceFlagsVTableItem& InterfaceFlagsVTableItem::markAsDeprecated()
    {
        flags.set(Flags::DEPRECATED);

        return *this;
    }

    inline InterfaceFlagsVTableItem& InterfaceFlagsVTableItem::markAsPrivileged()
    {
        flags.set(Flags::PRIVILEGED);

        return *this;
    }

    inline InterfaceFlagsVTableItem& InterfaceFlagsVTableItem::withNoReplyMethods()
    {
        flags.set(Flags::METHOD_NO_REPLY);

        return *this;
    }

    inline InterfaceFlagsVTableItem& InterfaceFlagsVTableItem::withPropertyUpdateBehavior(Flags::PropertyUpdateBehaviorFlags behavior)
    {
        flags.set(behavior);

        return *this;
    }

    inline InterfaceFlagsVTableItem setInterfaceFlags()
    {
        return {};
    }

} // namespace sdbus

#endif /* SDBUS_CPP_VTABLEITEMS_INL_ */
