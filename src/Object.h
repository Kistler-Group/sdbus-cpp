/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2024 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file Object.h
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

#ifndef SDBUS_CXX_INTERNAL_OBJECT_H_
#define SDBUS_CXX_INTERNAL_OBJECT_H_

#include <sdbus-c++/IObject.h>
#include "IConnection.h"
#include SDBUS_HEADER
#include <string>
#include <list>
#include <vector>
#include <functional>
#include <memory>
#include <cassert>

namespace sdbus::internal {

    class Object
        : public IObject
    {
    public:
        Object(sdbus::internal::IConnection& connection, std::string objectPath);

        void addVTable(std::string interfaceName, std::vector<VTableItem> vtable) override;
        Slot addVTable(std::string interfaceName, std::vector<VTableItem> vtable, request_slot_t) override;
        void unregister() override;

        sdbus::Signal createSignal(const std::string& interfaceName, const std::string& signalName) override;
        void emitSignal(const sdbus::Signal& message) override;
        void emitPropertiesChangedSignal(const std::string& interfaceName, const std::vector<std::string>& propNames) override;
        void emitPropertiesChangedSignal(const std::string& interfaceName) override;
        void emitInterfacesAddedSignal() override;
        void emitInterfacesAddedSignal(const std::vector<std::string>& interfaces) override;
        void emitInterfacesRemovedSignal() override;
        void emitInterfacesRemovedSignal(const std::vector<std::string>& interfaces) override;

        void addObjectManager() override;
        void removeObjectManager() override;
        bool hasObjectManager() const override;

        sdbus::IConnection& getConnection() const override;
        const std::string& getObjectPath() const override;
        Message getCurrentlyProcessedMessage() const override;

    private:
        // A vtable record comprising methods, signals, properties, flags.
        // Once created, it cannot be modified. Only new vtables records can be added.
        // An interface can have any number of vtables attached to it, not only one.
        struct VTable
        {
            std::string interfaceName;
            Flags interfaceFlags;

            struct MethodItem
            {
                std::string name;
                std::string inputArgs;
                std::string outputArgs;
                std::string paramNames;
                method_callback callback;
                Flags flags;
            };
            // Array of method records sorted by method name
            std::vector<MethodItem> methods;

            struct SignalItem
            {
                std::string name;
                std::string signature;
                std::string paramNames;
                Flags flags;
            };
            // Array of signal records sorted by signal name
            std::vector<SignalItem> signals;

            struct PropertyItem
            {
                std::string name;
                std::string signature;
                property_get_callback getCallback;
                property_set_callback setCallback;
                Flags flags;
            };
            // Array of signal records sorted by signal name
            std::vector<PropertyItem> properties;

            // VTable structure in format required by sd-bus API
            std::vector<sd_bus_vtable> sdbusVTable;

            // Back-reference to the owning object from sd-bus callback handlers
            Object* object{};

            // This is intentionally the last member, because it must be destructed first,
            // releasing callbacks above before the callbacks themselves are destructed.
            Slot slot;
        };

        VTable createInternalVTable(std::string interfaceName, std::vector<VTableItem> vtable);
        void writeInterfaceFlagsToVTable(InterfaceFlagsVTableItem flags, VTable& vtable);
        void writeMethodRecordToVTable(MethodVTableItem method, VTable& vtable);
        void writeSignalRecordToVTable(SignalVTableItem signal, VTable& vtable);
        void writePropertyRecordToVTable(PropertyVTableItem property, VTable& vtable);

        std::vector<sd_bus_vtable> createInternalSdBusVTable(const VTable& vtable);
        static void startSdBusVTable(const Flags& interfaceFlags, std::vector<sd_bus_vtable>& vtable);
        static void writeMethodRecordToSdBusVTable(const VTable::MethodItem& method, std::vector<sd_bus_vtable>& vtable);
        static void writeSignalRecordToSdBusVTable(const VTable::SignalItem& signal, std::vector<sd_bus_vtable>& vtable);
        static void writePropertyRecordToSdBusVTable(const VTable::PropertyItem& property, std::vector<sd_bus_vtable>& vtable);
        static void finalizeSdBusVTable(std::vector<sd_bus_vtable>& vtable);

        static const VTable::MethodItem* findMethod(const VTable& vtable, const std::string& methodName);
        static const VTable::PropertyItem* findProperty(const VTable& vtable, const std::string& propertyName);

        static std::string paramNamesToString(const std::vector<std::string>& paramNames);

        static int sdbus_method_callback(sd_bus_message *sdbusMessage, void *userData, sd_bus_error *retError);
        static int sdbus_property_get_callback( sd_bus *bus
                                              , const char *objectPath
                                              , const char *interface
                                              , const char *property
                                              , sd_bus_message *sdbusReply
                                              , void *userData
                                              , sd_bus_error *retError );
        static int sdbus_property_set_callback( sd_bus *bus
                                              , const char *objectPath
                                              , const char *interface
                                              , const char *property
                                              , sd_bus_message *sdbusValue
                                              , void *userData
                                              , sd_bus_error *retError );

    private:
        sdbus::internal::IConnection& connection_;
        std::string objectPath_;
        std::vector<Slot> vtables_;
        Slot objectManagerSlot_;
    };

}

#endif /* SDBUS_CXX_INTERNAL_OBJECT_H_ */
