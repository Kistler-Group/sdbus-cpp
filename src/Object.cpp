/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2022 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
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

void Object::registerMethod( const std::string& interfaceName
                           , std::string methodName
                           , std::string inputSignature
                           , std::string outputSignature
                           , method_callback methodCallback
                           , Flags flags )
{
    registerMethod( interfaceName
                  , std::move(methodName)
                  , std::move(inputSignature)
                  , {}
                  , std::move(outputSignature)
                  , {}
                  , std::move(methodCallback)
                  , std::move(flags) );
}

void Object::registerMethod( const std::string& interfaceName
                           , std::string methodName
                           , std::string inputSignature
                           , const std::vector<std::string>& inputNames
                           , std::string outputSignature
                           , const std::vector<std::string>& outputNames
                           , method_callback methodCallback
                           , Flags flags )
{
    SDBUS_CHECK_INTERFACE_NAME(interfaceName);
    SDBUS_CHECK_MEMBER_NAME(methodName);
    SDBUS_THROW_ERROR_IF(!methodCallback, "Invalid method callback provided", EINVAL);

    auto& interface = getInterface(interfaceName);
    InterfaceData::MethodData methodData{ std::move(inputSignature)
                                        , std::move(outputSignature)
                                        , paramNamesToString(inputNames) + paramNamesToString(outputNames)
                                        , std::move(methodCallback)
                                        , std::move(flags) };
    auto inserted = interface.methods.emplace(std::move(methodName), std::move(methodData)).second;

    SDBUS_THROW_ERROR_IF(!inserted, "Failed to register method: method already exists", EINVAL);
}

void Object::registerSignal( const std::string& interfaceName
                           , std::string signalName
                           , std::string signature
                           , Flags flags )
{
    registerSignal(interfaceName, std::move(signalName), std::move(signature), {}, std::move(flags));
}

void Object::registerSignal( const std::string& interfaceName
                           , std::string signalName
                           , std::string signature
                           , const std::vector<std::string>& paramNames
                           , Flags flags )
{
    SDBUS_CHECK_INTERFACE_NAME(interfaceName);
    SDBUS_CHECK_MEMBER_NAME(signalName);

    auto& interface = getInterface(interfaceName);

    InterfaceData::SignalData signalData{std::move(signature), paramNamesToString(paramNames), std::move(flags)};
    auto inserted = interface.signals.emplace(std::move(signalName), std::move(signalData)).second;

    SDBUS_THROW_ERROR_IF(!inserted, "Failed to register signal: signal already exists", EINVAL);
}

void Object::registerProperty( const std::string& interfaceName
                             , std::string propertyName
                             , std::string signature
                             , property_get_callback getCallback
                             , Flags flags )
{
    registerProperty( interfaceName
                    , std::move(propertyName)
                    , std::move(signature)
                    , std::move(getCallback)
                    , {}
                    , std::move(flags) );
}

void Object::registerProperty( const std::string& interfaceName
                             , std::string propertyName
                             , std::string signature
                             , property_get_callback getCallback
                             , property_set_callback setCallback
                             , Flags flags )
{
    SDBUS_CHECK_INTERFACE_NAME(interfaceName);
    SDBUS_CHECK_MEMBER_NAME(propertyName);
    SDBUS_THROW_ERROR_IF(!getCallback && !setCallback, "Invalid property callbacks provided", EINVAL);

    auto& interface = getInterface(interfaceName);

    InterfaceData::PropertyData propertyData{ std::move(signature)
                                            , std::move(getCallback)
                                            , std::move(setCallback)
                                            , std::move(flags) };
    auto inserted = interface.properties.emplace(std::move(propertyName), std::move(propertyData)).second;

    SDBUS_THROW_ERROR_IF(!inserted, "Failed to register property: property already exists", EINVAL);
}

void Object::setInterfaceFlags(const std::string& interfaceName, Flags flags)
{
    auto& interface = getInterface(interfaceName);
    interface.flags = flags;
}

void Object::finishRegistration()
{
    for (auto& item : interfaces_)
    {
        const auto& interfaceName = item.first;
        auto& interfaceData = item.second;

        const auto& vtable = createInterfaceVTable(interfaceData);
        activateInterfaceVTable(interfaceName, interfaceData, vtable);
    }
}

void Object::unregister()
{
    interfaces_.clear();
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

Object::InterfaceData& Object::getInterface(const std::string& interfaceName)
{
    return interfaces_.emplace(interfaceName, *this).first->second;
}

const std::vector<sd_bus_vtable>& Object::createInterfaceVTable(InterfaceData& interfaceData)
{
    auto& vtable = interfaceData.vtable;
    assert(vtable.empty());

    vtable.push_back(createVTableStartItem(interfaceData.flags.toSdBusInterfaceFlags()));
    registerMethodsToVTable(interfaceData, vtable);
    registerSignalsToVTable(interfaceData, vtable);
    registerPropertiesToVTable(interfaceData, vtable);
    vtable.push_back(createVTableEndItem());

    return vtable;
}

void Object::registerMethodsToVTable(const InterfaceData& interfaceData, std::vector<sd_bus_vtable>& vtable)
{
    for (const auto& item : interfaceData.methods)
    {
        const auto& methodName = item.first;
        const auto& methodData = item.second;

        vtable.push_back(createVTableMethodItem( methodName.c_str()
                                               , methodData.inputArgs.c_str()
                                               , methodData.outputArgs.c_str()
                                               , methodData.paramNames.c_str()
                                               , &Object::sdbus_method_callback
                                               , methodData.flags.toSdBusMethodFlags() ));
    }
}

void Object::registerSignalsToVTable(const InterfaceData& interfaceData, std::vector<sd_bus_vtable>& vtable)
{
    for (const auto& item : interfaceData.signals)
    {
        const auto& signalName = item.first;
        const auto& signalData = item.second;

        vtable.push_back(createVTableSignalItem( signalName.c_str()
                                               , signalData.signature.c_str()
                                               , signalData.paramNames.c_str()
                                               , signalData.flags.toSdBusSignalFlags() ));
    }
}

void Object::registerPropertiesToVTable(const InterfaceData& interfaceData, std::vector<sd_bus_vtable>& vtable)
{
    for (const auto& item : interfaceData.properties)
    {
        const auto& propertyName = item.first;
        const auto& propertyData = item.second;

        if (!propertyData.setCallback)
            vtable.push_back(createVTablePropertyItem( propertyName.c_str()
                                                     , propertyData.signature.c_str()
                                                     , &Object::sdbus_property_get_callback
                                                     , propertyData.flags.toSdBusPropertyFlags() ));
        else
            vtable.push_back(createVTableWritablePropertyItem( propertyName.c_str()
                                                             , propertyData.signature.c_str()
                                                             , &Object::sdbus_property_get_callback
                                                             , &Object::sdbus_property_set_callback
                                                             , propertyData.flags.toSdBusWritablePropertyFlags() ));
    }
}

void Object::activateInterfaceVTable( const std::string& interfaceName
                                    , InterfaceData& interfaceData
                                    , const std::vector<sd_bus_vtable>& vtable )
{
    interfaceData.slot = connection_.addObjectVTable(objectPath_, interfaceName, &vtable[0], &interfaceData);
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
    auto* interfaceData = static_cast<InterfaceData*>(userData);
    assert(interfaceData != nullptr);
    auto& object = interfaceData->object;

    auto message = Message::Factory::create<MethodCall>(sdbusMessage, &object.connection_.getSdBusInterface());

    auto& callback = interfaceData->methods[message.getMemberName()].callback;
    assert(callback);

    try
    {
        callback(std::move(message));
    }
    catch (const Error& e)
    {
        sd_bus_error_set(retError, e.getName().c_str(), e.getMessage().c_str());
    }

    return 1;
}

int Object::sdbus_property_get_callback( sd_bus */*bus*/
                                       , const char */*objectPath*/
                                       , const char */*interface*/
                                       , const char *property
                                       , sd_bus_message *sdbusReply
                                       , void *userData
                                       , sd_bus_error *retError )
{
    auto* interfaceData = static_cast<InterfaceData*>(userData);
    assert(interfaceData != nullptr);
    auto& object = interfaceData->object;

    auto& callback = interfaceData->properties[property].getCallback;
    // Getter can be empty - the case of "write-only" property
    if (!callback)
    {
        sd_bus_error_set(retError, "org.freedesktop.DBus.Error.Failed", "Cannot read property as it is write-only");
        return 1;
    }

    auto reply = Message::Factory::create<PropertyGetReply>(sdbusReply, &object.connection_.getSdBusInterface());

    try
    {
        callback(reply);
    }
    catch (const Error& e)
    {
        sd_bus_error_set(retError, e.getName().c_str(), e.getMessage().c_str());
    }

    return 1;
}

int Object::sdbus_property_set_callback( sd_bus */*bus*/
                                       , const char */*objectPath*/
                                       , const char */*interface*/
                                       , const char *property
                                       , sd_bus_message *sdbusValue
                                       , void *userData
                                       , sd_bus_error *retError )
{
    auto* interfaceData = static_cast<InterfaceData*>(userData);
    assert(interfaceData != nullptr);
    auto& object = interfaceData->object;

    auto& callback = interfaceData->properties[property].setCallback;
    assert(callback);

    auto value = Message::Factory::create<PropertySetCall>(sdbusValue, &object.connection_.getSdBusInterface());

    try
    {
        callback(std::move(value));
    }
    catch (const Error& e)
    {
        sd_bus_error_set(retError, e.getName().c_str(), e.getMessage().c_str());
    }

    return 1;
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
