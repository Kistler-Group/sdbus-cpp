/**
 * (C) 2016 - 2024 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file VTableItems.h
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

#ifndef SDBUS_CXX_VTABLEITEMS_H_
#define SDBUS_CXX_VTABLEITEMS_H_

#include <sdbus-c++/Flags.h>
#include <sdbus-c++/Types.h>
#include <sdbus-c++/TypeTraits.h>

#include <string>
#include <variant>
#include <vector>

namespace sdbus {

    struct MethodVTableItem
    {
        template <typename _Function> MethodVTableItem& implementedAs(_Function&& callback);
        MethodVTableItem& withInputParamNames(std::vector<std::string> names);
        template <typename... _String> MethodVTableItem& withInputParamNames(_String... names);
        MethodVTableItem& withOutputParamNames(std::vector<std::string> names);
        template <typename... _String> MethodVTableItem& withOutputParamNames(_String... names);
        MethodVTableItem& markAsDeprecated();
        MethodVTableItem& markAsPrivileged();
        MethodVTableItem& withNoReply();

        MethodName name;
        Signature inputSignature;
        std::vector<std::string> inputParamNames;
        Signature outputSignature;
        std::vector<std::string> outputParamNames;
        method_callback callbackHandler;
        Flags flags;
    };

    MethodVTableItem registerMethod(MethodName methodName);
    MethodVTableItem registerMethod(std::string methodName);

    struct SignalVTableItem
    {
        template <typename... _Args> SignalVTableItem& withParameters();
        template <typename... _Args> SignalVTableItem& withParameters(std::vector<std::string> names);
        template <typename... _Args, typename... _String> SignalVTableItem& withParameters(_String... names);
        SignalVTableItem& markAsDeprecated();

        SignalName name;
        Signature signature;
        std::vector<std::string> paramNames;
        Flags flags;
    };

    SignalVTableItem registerSignal(SignalName signalName);
    SignalVTableItem registerSignal(std::string signalName);

    struct PropertyVTableItem
    {
        template <typename _Function> PropertyVTableItem& withGetter(_Function&& callback);
        template <typename _Function> PropertyVTableItem& withSetter(_Function&& callback);
        PropertyVTableItem& markAsDeprecated();
        PropertyVTableItem& markAsPrivileged();
        PropertyVTableItem& withUpdateBehavior(Flags::PropertyUpdateBehaviorFlags behavior);

        PropertyName name;
        Signature signature;
        property_get_callback getter;
        property_set_callback setter;
        Flags flags;
    };

    PropertyVTableItem registerProperty(PropertyName propertyName);
    PropertyVTableItem registerProperty(std::string propertyName);

    struct InterfaceFlagsVTableItem
    {
        InterfaceFlagsVTableItem& markAsDeprecated();
        InterfaceFlagsVTableItem& markAsPrivileged();
        InterfaceFlagsVTableItem& withNoReplyMethods();
        InterfaceFlagsVTableItem& withPropertyUpdateBehavior(Flags::PropertyUpdateBehaviorFlags behavior);

        Flags flags;
    };

    InterfaceFlagsVTableItem setInterfaceFlags();

    using VTableItem = std::variant<MethodVTableItem, SignalVTableItem, PropertyVTableItem, InterfaceFlagsVTableItem>;

} // namespace sdbus

#endif /* SDBUS_CXX_VTABLEITEMS_H_ */
