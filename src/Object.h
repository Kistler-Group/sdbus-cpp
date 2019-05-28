/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
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
#include <systemd/sd-bus.h>
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include <cassert>

namespace sdbus {
namespace internal {

    class Object
        : public IObject
    {
    public:
        Object(sdbus::internal::IConnection& connection, std::string objectPath);

        void registerMethod( const std::string& interfaceName
                           , const std::string& methodName
                           , const std::string& inputSignature
                           , const std::string& outputSignature
                           , method_callback methodCallback
                           , Flags flags ) override;

        void registerSignal( const std::string& interfaceName
                           , const std::string& signalName
                           , const std::string& signature
                           , Flags flags ) override;

        void registerProperty( const std::string& interfaceName
                             , const std::string& propertyName
                             , const std::string& signature
                             , property_get_callback getCallback
                             , Flags flags ) override;

        void registerProperty( const std::string& interfaceName
                             , const std::string& propertyName
                             , const std::string& signature
                             , property_get_callback getCallback
                             , property_set_callback setCallback
                             , Flags flags ) override;

        void setInterfaceFlags(const std::string& interfaceName, Flags flags) override;

        void finishRegistration() override;
        void unregister() override;

        sdbus::Signal createSignal(const std::string& interfaceName, const std::string& signalName) override;
        void emitSignal(const sdbus::Signal& message) override;

        sdbus::IConnection& getConnection() const override;

        void addObjectManager() override;
        void removeObjectManager() override;
        bool hasObjectManager() const override;

    private:
        using InterfaceName = std::string;
        struct InterfaceData
        {
            using MethodName = std::string;
            struct MethodData
            {
                std::string inputArgs_;
                std::string outputArgs_;
                method_callback callback_;
                Flags flags_;
            };
            std::map<MethodName, MethodData> methods_;
            using SignalName = std::string;
            struct SignalData
            {
                std::string signature_;
                Flags flags_;
            };
            std::map<SignalName, SignalData> signals_;
            using PropertyName = std::string;
            struct PropertyData
            {
                std::string signature_;
                property_get_callback getCallback_;
                property_set_callback setCallback_;
                Flags flags_;
            };
            std::map<PropertyName, PropertyData> properties_;
            std::vector<sd_bus_vtable> vtable_;
            Flags flags_;

            SlotPtr slot_;
        };

        static const std::vector<sd_bus_vtable>& createInterfaceVTable(InterfaceData& interfaceData);
        static void registerMethodsToVTable(const InterfaceData& interfaceData, std::vector<sd_bus_vtable>& vtable);
        static void registerSignalsToVTable(const InterfaceData& interfaceData, std::vector<sd_bus_vtable>& vtable);
        static void registerPropertiesToVTable(const InterfaceData& interfaceData, std::vector<sd_bus_vtable>& vtable);
        void activateInterfaceVTable( const std::string& interfaceName
                                    , InterfaceData& interfaceData
                                    , const std::vector<sd_bus_vtable>& vtable );

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
        std::map<InterfaceName, InterfaceData> interfaces_;
        SlotPtr objectManagerSlot_;
    };

}
}

#endif /* SDBUS_CXX_INTERNAL_OBJECT_H_ */
