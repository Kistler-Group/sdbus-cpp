/**
 * (C) 2016 - 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2019 Stanislav Angelovic <angelovic.s@gmail.com>
 *
 * @file StandardInterfaces.h
 *
 * Created on: Dec 13, 2016
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

#ifndef SDBUS_CXX_STANDARDINTERFACES_H_
#define SDBUS_CXX_STANDARDINTERFACES_H_

#include <sdbus-c++/IObject.h>
#include <sdbus-c++/IProxy.h>
#include <sdbus-c++/Types.h>
#include <string>
#include <map>
#include <vector>

namespace sdbus {

    // Proxy for peer
    class Peer_proxy
    {
        static constexpr const char* INTERFACE_NAME = "org.freedesktop.DBus.Peer";

    protected:
        Peer_proxy(sdbus::IProxy& proxy)
            : proxy_(proxy)
        {
        }

        ~Peer_proxy() = default;

    public:
        void Ping()
        {
            proxy_.callMethod("Ping").onInterface(INTERFACE_NAME);
        }

        std::string GetMachineId()
        {
            std::string machineUUID;
            proxy_.callMethod("GetMachineId").onInterface(INTERFACE_NAME).storeResultsTo(machineUUID);
            return machineUUID;
        }

    private:
        sdbus::IProxy& proxy_;
    };

    // Proxy for introspection
    class Introspectable_proxy
    {
        static constexpr const char* INTERFACE_NAME = "org.freedesktop.DBus.Introspectable";

    protected:
        Introspectable_proxy(sdbus::IProxy& proxy)
            : proxy_(proxy)
        {
        }

        ~Introspectable_proxy() = default;

    public:
        std::string Introspect()
        {
            std::string xml;
            proxy_.callMethod("Introspect").onInterface(INTERFACE_NAME).storeResultsTo(xml);
            return xml;
        }

    private:
        sdbus::IProxy& proxy_;
    };

    // Proxy for properties
    class Properties_proxy
    {
        static constexpr const char* INTERFACE_NAME = "org.freedesktop.DBus.Properties";

    protected:
        Properties_proxy(sdbus::IProxy& proxy)
            : proxy_(proxy)
        {
            proxy_
                .uponSignal("PropertiesChanged")
                .onInterface(INTERFACE_NAME)
                .call([this]( const std::string& interfaceName
                            , const std::map<std::string, sdbus::Variant>& changedProperties
                            , const std::vector<std::string>& invalidatedProperties )
                            {
                                this->onPropertiesChanged(interfaceName, changedProperties, invalidatedProperties);
                            });
        }

        ~Properties_proxy() = default;

        virtual void onPropertiesChanged( const std::string& interfaceName
                                        , const std::map<std::string, sdbus::Variant>& changedProperties
                                        , const std::vector<std::string>& invalidatedProperties ) = 0;

    public:
        sdbus::Variant Get(const std::string& interfaceName, const std::string& propertyName)
        {
            return proxy_.getProperty(propertyName).onInterface(interfaceName);
        }

        void Set(const std::string& interfaceName, const std::string& propertyName, const sdbus::Variant& value)
        {
            proxy_.setProperty(propertyName).onInterface(interfaceName).toValue(value);
        }

        std::map<std::string, sdbus::Variant> GetAll(const std::string& interfaceName)
        {
            std::map<std::string, sdbus::Variant> props;
            proxy_.callMethod("GetAll").onInterface(INTERFACE_NAME).withArguments(interfaceName).storeResultsTo(props);
            return props;
        }

    private:
        sdbus::IProxy& proxy_;
    };

    // Proxy for object manager
    class ObjectManager_proxy
    {
        static constexpr const char* INTERFACE_NAME = "org.freedesktop.DBus.ObjectManager";

    protected:
        ObjectManager_proxy(sdbus::IProxy& proxy)
            : proxy_(proxy)
        {
            proxy_
                .uponSignal("InterfacesAdded")
                .onInterface(INTERFACE_NAME)
                .call([this]( const sdbus::ObjectPath& objectPath
                            , const std::map<std::string, std::map<std::string, sdbus::Variant>>& interfacesAndProperties )
                            {
                                this->onInterfacesAdded(objectPath, interfacesAndProperties);
                            });

            proxy_
                .uponSignal("InterfacesRemoved")
                .onInterface(INTERFACE_NAME)
                .call([this]( const sdbus::ObjectPath& objectPath
                            , const std::vector<std::string>& interfaces )
                            {
                                this->onInterfacesRemoved(objectPath, interfaces);
                            });
        }

        ~ObjectManager_proxy() = default;

        virtual void onInterfacesAdded( const sdbus::ObjectPath& objectPath
                                      , const std::map<std::string, std::map<std::string, sdbus::Variant>>& interfacesAndProperties) = 0;
        virtual void onInterfacesRemoved( const sdbus::ObjectPath& objectPath
                                        , const std::vector<std::string>& interfaces) = 0;

    public:
        std::map<sdbus::ObjectPath, std::map<std::string, std::map<std::string, sdbus::Variant>>> GetManagedObjects()
        {
            std::map<sdbus::ObjectPath, std::map<std::string, std::map<std::string, sdbus::Variant>>> objectsInterfacesAndProperties;
            proxy_.callMethod("GetManagedObjects").onInterface(INTERFACE_NAME).storeResultsTo(objectsInterfacesAndProperties);
            return objectsInterfacesAndProperties;
        }

    private:
        sdbus::IProxy& proxy_;
    };

    // Adaptors for the above-listed standard D-Bus interfaces are not necessary because the functionality
    // is provided by underlying libsystemd implementation. The exception is Properties_adaptor and
    // ObjectManager_adaptor, which provide convenience functionality to emit signals.

    // Adaptor for properties
    class Properties_adaptor
    {
        static constexpr const char* INTERFACE_NAME = "org.freedesktop.DBus.Properties";

    protected:
        Properties_adaptor(sdbus::IObject& object)
            : object_(object)
        {
        }

        ~Properties_adaptor() = default;

    public:
        void emitPropertiesChangedSignal(const std::string& interfaceName, const std::vector<std::string>& properties)
        {
            object_.emitPropertiesChangedSignal(interfaceName, properties);
        }

        void emitPropertiesChangedSignal(const std::string& interfaceName)
        {
            object_.emitPropertiesChangedSignal(interfaceName);
        }

    private:
        sdbus::IObject& object_;
    };

    // Adaptor for object manager
    class ObjectManager_adaptor
    {
        static constexpr const char* INTERFACE_NAME = "org.freedesktop.DBus.ObjectManager";

    protected:
        ObjectManager_adaptor(sdbus::IObject& object)
            : object_(object)
        {
            object_.addObjectManager();
        }

        ~ObjectManager_adaptor() = default;

    public:
        void emitInterfacesAddedSignal()
        {
            object_.emitInterfacesAddedSignal();
        }

        void emitInterfacesAddedSignal(const std::vector<std::string>& interfaces)
        {
            object_.emitInterfacesAddedSignal(interfaces);
        }

        void emitInterfacesRemovedSignal()
        {
            object_.emitInterfacesRemovedSignal();
        }

        void emitInterfacesRemovedSignal(const std::vector<std::string>& interfaces)
        {
            object_.emitInterfacesRemovedSignal(interfaces);
        }

    private:
        sdbus::IObject& object_;
    };

}

#endif /* SDBUS_CXX_STANDARDINTERFACES_H_ */
