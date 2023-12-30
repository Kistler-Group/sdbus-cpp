/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2024 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file Object.cpp
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

#include "Object.h"
#include "MessageUtils.h"
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/Message.h>
#include <sdbus-c++/Error.h>
#include <sdbus-c++/MethodResult.h>
#include <sdbus-c++/Flags.h>
#include "ScopeGuard.h"
#include "IConnection.h"
#include "Utils.h"
#include "VTableUtils.h"
#include SDBUS_HEADER
#include <utility>
#include <cassert>

namespace sdbus::internal {

Object::Object(sdbus::internal::IConnection& connection, std::string objectPath)
    : connection_(connection), objectPath_(std::move(objectPath))
{
    SDBUS_CHECK_OBJECT_PATH(objectPath_);
}

void Object::addVTable(std::string interfaceName, std::vector<VTableItem> vtable)
{
    auto slot = Object::addVTable(std::move(interfaceName), std::move(vtable), request_slot);

    vtables_.push_back(std::move(slot));
}

Slot Object::addVTable(std::string interfaceName, std::vector<VTableItem> vtable, request_slot_t)
{
    SDBUS_CHECK_INTERFACE_NAME(interfaceName);

    // 1st pass -- create vtable structure for internal sdbus-c++ purposes
    auto internalVTable = std::make_unique<VTable>(createInternalVTable(std::move(interfaceName), std::move(vtable)));

    // 2nd pass -- from internal sdbus-c++ vtable, create vtable structure in format expected by underlying sd-bus library
    internalVTable->sdbusVTable = createInternalSdBusVTable(*internalVTable);

    // 3rd step -- register the vtable with sd-bus
    internalVTable->slot = connection_.addObjectVTable(objectPath_, internalVTable->interfaceName, &internalVTable->sdbusVTable[0], internalVTable.get());

    // Return vtable wrapped in a Slot object
    return {internalVTable.release(), [](void *ptr){ delete static_cast<VTable*>(ptr); }};
}

void Object::unregister()
{
    vtables_.clear();
    removeObjectManager();
}

sdbus::Signal Object::createSignal(const std::string& interfaceName, const std::string& signalName)
{
    return connection_.createSignal(objectPath_, interfaceName, signalName);
}

void Object::emitSignal(const sdbus::Signal& message)
{
    SDBUS_THROW_ERROR_IF(!message.isValid(), "Invalid signal message provided", EINVAL);

    message.send();
}

void Object::emitPropertiesChangedSignal(const std::string& interfaceName, const std::vector<std::string>& propNames)
{
    connection_.emitPropertiesChangedSignal(objectPath_, interfaceName, propNames);
}

void Object::emitPropertiesChangedSignal(const std::string& interfaceName)
{
    Object::emitPropertiesChangedSignal(interfaceName, {});
}

void Object::emitInterfacesAddedSignal()
{
    connection_.emitInterfacesAddedSignal(objectPath_);
}

void Object::emitInterfacesAddedSignal(const std::vector<std::string>& interfaces)
{
    connection_.emitInterfacesAddedSignal(objectPath_, interfaces);
}

void Object::emitInterfacesRemovedSignal()
{
    connection_.emitInterfacesRemovedSignal(objectPath_);
}

void Object::emitInterfacesRemovedSignal(const std::vector<std::string>& interfaces)
{
    connection_.emitInterfacesRemovedSignal(objectPath_, interfaces);
}

void Object::addObjectManager()
{
    objectManagerSlot_ = connection_.addObjectManager(objectPath_, request_slot);
}

void Object::removeObjectManager()
{
    objectManagerSlot_.reset();
}

bool Object::hasObjectManager() const
{
    return objectManagerSlot_ != nullptr;
}

sdbus::IConnection& Object::getConnection() const
{
    return connection_;
}

const std::string& Object::getObjectPath() const
{
    return objectPath_;
}

Message Object::getCurrentlyProcessedMessage() const
{
    return connection_.getCurrentlyProcessedMessage();
}

Object::VTable Object::createInternalVTable(std::string interfaceName, std::vector<VTableItem> vtable)
{
    VTable internalVTable;

    internalVTable.interfaceName = std::move(interfaceName);

    for (auto& vtableItem : vtable)
    {
        std::visit( overload{ [&](InterfaceFlagsVTableItem&& interfaceFlags){ writeInterfaceFlagsToVTable(std::move(interfaceFlags), internalVTable); }
                            , [&](MethodVTableItem&& method){ writeMethodRecordToVTable(std::move(method), internalVTable); }
                            , [&](SignalVTableItem&& signal){ writeSignalRecordToVTable(std::move(signal), internalVTable); }
                            , [&](PropertyVTableItem&& property){ writePropertyRecordToVTable(std::move(property), internalVTable); } }
                  , std::move(vtableItem) );
    }

    // Sort arrays so we can do fast searching for an item in sd-bus callback handlers
    std::sort(internalVTable.methods.begin(), internalVTable.methods.end(), [](const auto& a, const auto& b){ return a.name < b.name; });
    std::sort(internalVTable.signals.begin(), internalVTable.signals.end(), [](const auto& a, const auto& b){ return a.name < b.name; });
    std::sort(internalVTable.properties.begin(), internalVTable.properties.end(), [](const auto& a, const auto& b){ return a.name < b.name; });

    internalVTable.object = this;

    return internalVTable;
}

void Object::writeInterfaceFlagsToVTable(InterfaceFlagsVTableItem flags, VTable& vtable)
{
    vtable.interfaceFlags = std::move(flags.flags);
}

void Object::writeMethodRecordToVTable(MethodVTableItem method, VTable& vtable)
{
    SDBUS_CHECK_MEMBER_NAME(method.name);
    SDBUS_THROW_ERROR_IF(!method.callbackHandler, "Invalid method callback provided", EINVAL);

    vtable.methods.push_back({ std::move(method.name)
                             , std::move(method.inputSignature)
                             , std::move(method.outputSignature)
                             , paramNamesToString(method.inputParamNames) + paramNamesToString(method.outputParamNames)
                             , std::move(method.callbackHandler)
                             , std::move(method.flags) });
}

void Object::writeSignalRecordToVTable(SignalVTableItem signal, VTable& vtable)
{
    SDBUS_CHECK_MEMBER_NAME(signal.name);

    vtable.signals.push_back({ std::move(signal.name)
                             , std::move(signal.signature)
                             , paramNamesToString(signal.paramNames)
                             , std::move(signal.flags) });
}

void Object::writePropertyRecordToVTable(PropertyVTableItem property, VTable& vtable)
{
    SDBUS_CHECK_MEMBER_NAME(property.name);
    SDBUS_THROW_ERROR_IF(!property.getter && !property.setter, "Invalid property callbacks provided", EINVAL);

    vtable.properties.push_back({ std::move(property.name)
                                , std::move(property.signature)
                                , std::move(property.getter)
                                , std::move(property.setter)
                                , std::move(property.flags) });
}

std::vector<sd_bus_vtable> Object::createInternalSdBusVTable(const VTable& vtable)
{
    std::vector<sd_bus_vtable> sdbusVTable;

    startSdBusVTable(vtable.interfaceFlags, sdbusVTable);
    for (const auto& methodItem : vtable.methods)
        writeMethodRecordToSdBusVTable(methodItem, sdbusVTable);
    for (const auto& signalItem : vtable.signals)
        writeSignalRecordToSdBusVTable(signalItem, sdbusVTable);
    for (const auto& propertyItem : vtable.properties)
        writePropertyRecordToSdBusVTable(propertyItem, sdbusVTable);
    finalizeSdBusVTable(sdbusVTable);

    return sdbusVTable;
}

void Object::startSdBusVTable(const Flags& interfaceFlags, std::vector<sd_bus_vtable>& vtable)
{
    auto vtableItem = createSdBusVTableStartItem(interfaceFlags.toSdBusInterfaceFlags());
    vtable.push_back(std::move(vtableItem));
}

void Object::writeMethodRecordToSdBusVTable(const VTable::MethodItem& method, std::vector<sd_bus_vtable>& vtable)
{
    auto vtableItem = createSdBusVTableMethodItem( method.name.c_str()
                                                 , method.inputArgs.c_str()
                                                 , method.outputArgs.c_str()
                                                 , method.paramNames.c_str()
                                                 , &Object::sdbus_method_callback
                                                 , method.flags.toSdBusMethodFlags() );
    vtable.push_back(std::move(vtableItem));
}

void Object::writeSignalRecordToSdBusVTable(const VTable::SignalItem& signal, std::vector<sd_bus_vtable>& vtable)
{
    auto vtableItem = createSdBusVTableSignalItem( signal.name.c_str()
                                                 , signal.signature.c_str()
                                                 , signal.paramNames.c_str()
                                                 , signal.flags.toSdBusSignalFlags() );
    vtable.push_back(std::move(vtableItem));
}

void Object::writePropertyRecordToSdBusVTable(const VTable::PropertyItem& property, std::vector<sd_bus_vtable>& vtable)
{
    auto vtableItem = !property.setCallback
                    ? createSdBusVTableReadOnlyPropertyItem( property.name.c_str()
                                                           , property.signature.c_str()
                                                           , &Object::sdbus_property_get_callback
                                                           , property.flags.toSdBusPropertyFlags() )
                    : createSdBusVTableWritablePropertyItem( property.name.c_str()
                                                           , property.signature.c_str()
                                                           , &Object::sdbus_property_get_callback
                                                           , &Object::sdbus_property_set_callback
                                                           , property.flags.toSdBusWritablePropertyFlags() );
    vtable.push_back(std::move(vtableItem));
}

void Object::finalizeSdBusVTable(std::vector<sd_bus_vtable>& vtable)
{
    vtable.push_back(createSdBusVTableEndItem());
}

const Object::VTable::MethodItem* Object::findMethod(const VTable& vtable, const std::string& methodName)
{
    auto it = std::lower_bound(vtable.methods.begin(), vtable.methods.end(), methodName, [](const auto& methodItem, const auto& methodName)
    {
        return methodItem.name < methodName;
    });

    return it != vtable.methods.end() && it->name == methodName ? &*it : nullptr;
}

const Object::VTable::PropertyItem* Object::findProperty(const VTable& vtable, const std::string& propertyName)
{
    auto it = std::lower_bound(vtable.properties.begin(), vtable.properties.end(), propertyName, [](const auto& propertyItem, const auto& propertyName)
    {
        return propertyItem.name < propertyName;
    });

    return it != vtable.properties.end() && it->name == propertyName ? &*it : nullptr;
}

std::string Object::paramNamesToString(const std::vector<std::string>& paramNames)
{
    std::string names;
    for (const auto& name : paramNames)
        names += name + '\0';
    return names;
}

int Object::sdbus_method_callback(sd_bus_message *sdbusMessage, void *userData, sd_bus_error *retError)
{
    auto* vtable = static_cast<VTable*>(userData);
    assert(vtable != nullptr);
    assert(vtable->object != nullptr);

    auto message = Message::Factory::create<MethodCall>(sdbusMessage, &vtable->object->connection_.getSdBusInterface());

    const auto* methodItem = findMethod(*vtable, message.getMemberName());
    assert(methodItem != nullptr);
    assert(methodItem->callback);

    auto ok = invokeHandlerAndCatchErrors([&](){ methodItem->callback(std::move(message)); }, retError);

    return ok ? 1 : -1;
}

int Object::sdbus_property_get_callback( sd_bus */*bus*/
                                       , const char */*objectPath*/
                                       , const char */*interface*/
                                       , const char *property
                                       , sd_bus_message *sdbusReply
                                       , void *userData
                                       , sd_bus_error *retError )
{
    auto* vtable = static_cast<VTable*>(userData);
    assert(vtable != nullptr);
    assert(vtable->object != nullptr);

    const auto* propertyItem = findProperty(*vtable, property);
    assert(propertyItem != nullptr);

    // Getter may be empty - the case of "write-only" property
    if (!propertyItem->getCallback)
    {
        sd_bus_error_set(retError, "org.freedesktop.DBus.Error.Failed", "Cannot read property as it is write-only");
        return 1;
    }

    auto reply = Message::Factory::create<PropertyGetReply>(sdbusReply, &vtable->object->connection_.getSdBusInterface());

    auto ok = invokeHandlerAndCatchErrors([&](){ propertyItem->getCallback(reply); }, retError);

    return ok ? 1 : -1;
}

int Object::sdbus_property_set_callback( sd_bus */*bus*/
                                       , const char */*objectPath*/
                                       , const char */*interface*/
                                       , const char *property
                                       , sd_bus_message *sdbusValue
                                       , void *userData
                                       , sd_bus_error *retError )
{
    auto* vtable = static_cast<VTable*>(userData);
    assert(vtable != nullptr);
    assert(vtable->object != nullptr);

    const auto* propertyItem = findProperty(*vtable, property);
    assert(propertyItem != nullptr);
    assert(propertyItem->setCallback);

    auto value = Message::Factory::create<PropertySetCall>(sdbusValue, &vtable->object->connection_.getSdBusInterface());

    auto ok = invokeHandlerAndCatchErrors([&](){ propertyItem->setCallback(std::move(value)); }, retError);

    return ok ? 1 : -1;
}

}

namespace sdbus {

std::unique_ptr<sdbus::IObject> createObject(sdbus::IConnection& connection, std::string objectPath)
{
    auto* sdbusConnection = dynamic_cast<sdbus::internal::IConnection*>(&connection);
    SDBUS_THROW_ERROR_IF(!sdbusConnection, "Connection is not a real sdbus-c++ connection", EINVAL);

    return std::make_unique<sdbus::internal::Object>(*sdbusConnection, std::move(objectPath));
}

}
