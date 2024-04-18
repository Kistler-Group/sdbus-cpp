/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2024 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
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
#include <string_view>
#include <map>
#include <vector>

namespace sdbus {

    // Proxy for peer
    class Peer_proxy
    {
        static inline const char* INTERFACE_NAME = "org.freedesktop.DBus.Peer";

    protected:
        Peer_proxy(sdbus::IProxy& proxy)
            : proxy_(&proxy)
        {
        }

        Peer_proxy(const Peer_proxy&) = delete;
        Peer_proxy& operator=(const Peer_proxy&) = delete;
        Peer_proxy(Peer_proxy&&) = default;
        Peer_proxy& operator=(Peer_proxy&&) = default;

        ~Peer_proxy() = default;

        void registerProxy()
        {
        }

    public:
        void Ping()
        {
            proxy_->callMethod("Ping").onInterface(INTERFACE_NAME);
        }

        std::string GetMachineId()
        {
            std::string machineUUID;
            proxy_->callMethod("GetMachineId").onInterface(INTERFACE_NAME).storeResultsTo(machineUUID);
            return machineUUID;
        }

    private:
        sdbus::IProxy* proxy_;
    };

    // Proxy for introspection
    class Introspectable_proxy
    {
        static inline const char* INTERFACE_NAME = "org.freedesktop.DBus.Introspectable";

    protected:
        Introspectable_proxy(sdbus::IProxy& proxy)
            : proxy_(&proxy)
        {
        }

        Introspectable_proxy(const Introspectable_proxy&) = delete;
        Introspectable_proxy& operator=(const Introspectable_proxy&) = delete;
        Introspectable_proxy(Introspectable_proxy&&) = default;
        Introspectable_proxy& operator=(Introspectable_proxy&&) = default;

        ~Introspectable_proxy() = default;

        void registerProxy()
        {
        }

    public:
        std::string Introspect()
        {
            std::string xml;
            proxy_->callMethod("Introspect").onInterface(INTERFACE_NAME).storeResultsTo(xml);
            return xml;
        }

    private:
        sdbus::IProxy* proxy_;
    };

    // Proxy for properties
    class Properties_proxy
    {
        static inline const char* INTERFACE_NAME = "org.freedesktop.DBus.Properties";

    protected:
        Properties_proxy(sdbus::IProxy& proxy)
            : proxy_(&proxy)
        {
        }

        Properties_proxy(const Properties_proxy&) = delete;
        Properties_proxy& operator=(const Properties_proxy&) = delete;
        Properties_proxy(Properties_proxy&&) = default;
        Properties_proxy& operator=(Properties_proxy&&) = default;

        ~Properties_proxy() = default;

        void registerProxy()
        {
            proxy_
                ->uponSignal("PropertiesChanged")
                .onInterface(INTERFACE_NAME)
                .call([this]( const InterfaceName& interfaceName
                            , const std::map<PropertyName, sdbus::Variant>& changedProperties
                            , const std::vector<PropertyName>& invalidatedProperties )
                            {
                                this->onPropertiesChanged(interfaceName, changedProperties, invalidatedProperties);
                            });
        }

        virtual void onPropertiesChanged( const InterfaceName& interfaceName
                                        , const std::map<PropertyName, sdbus::Variant>& changedProperties
                                        , const std::vector<PropertyName>& invalidatedProperties ) = 0;

    public:
        sdbus::Variant Get(const InterfaceName& interfaceName, const PropertyName& propertyName)
        {
            return proxy_->getProperty(propertyName).onInterface(interfaceName);
        }

        sdbus::Variant Get(std::string_view interfaceName, std::string_view propertyName)
        {
            return proxy_->getProperty(propertyName).onInterface(interfaceName);
        }

        template <typename _Function>
        PendingAsyncCall GetAsync(const InterfaceName& interfaceName, const PropertyName& propertyName, _Function&& callback)
        {
            return proxy_->getPropertyAsync(propertyName).onInterface(interfaceName).uponReplyInvoke(std::forward<_Function>(callback));
        }

        template <typename _Function>
        [[nodiscard]] Slot GetAsync(const InterfaceName& interfaceName, const PropertyName& propertyName, _Function&& callback, return_slot_t)
        {
            return proxy_->getPropertyAsync(propertyName).onInterface(interfaceName).uponReplyInvoke(std::forward<_Function>(callback), return_slot);
        }

        template <typename _Function>
        PendingAsyncCall GetAsync(std::string_view interfaceName, std::string_view propertyName, _Function&& callback)
        {
            return proxy_->getPropertyAsync(propertyName).onInterface(interfaceName).uponReplyInvoke(std::forward<_Function>(callback));
        }

        template <typename _Function>
        [[nodiscard]] Slot GetAsync(std::string_view interfaceName, std::string_view propertyName, _Function&& callback, return_slot_t)
        {
            return proxy_->getPropertyAsync(propertyName).onInterface(interfaceName).uponReplyInvoke(std::forward<_Function>(callback), return_slot);
        }

        std::future<sdbus::Variant> GetAsync(const InterfaceName& interfaceName, const PropertyName& propertyName, with_future_t)
        {
            return proxy_->getPropertyAsync(propertyName).onInterface(interfaceName).getResultAsFuture();
        }

        std::future<sdbus::Variant> GetAsync(std::string_view interfaceName, std::string_view propertyName, with_future_t)
        {
            return proxy_->getPropertyAsync(propertyName).onInterface(interfaceName).getResultAsFuture();
        }

        void Set(const InterfaceName& interfaceName, const PropertyName& propertyName, const sdbus::Variant& value)
        {
            proxy_->setProperty(propertyName).onInterface(interfaceName).toValue(value);
        }

        void Set(std::string_view interfaceName, const std::string_view propertyName, const sdbus::Variant& value)
        {
            proxy_->setProperty(propertyName).onInterface(interfaceName).toValue(value);
        }

        void Set(const InterfaceName& interfaceName, const PropertyName& propertyName, const sdbus::Variant& value, dont_expect_reply_t)
        {
            proxy_->setProperty(propertyName).onInterface(interfaceName).toValue(value, dont_expect_reply);
        }

        void Set(std::string_view interfaceName, const std::string_view propertyName, const sdbus::Variant& value, dont_expect_reply_t)
        {
            proxy_->setProperty(propertyName).onInterface(interfaceName).toValue(value, dont_expect_reply);
        }

        template <typename _Function>
        PendingAsyncCall SetAsync(const InterfaceName& interfaceName, const PropertyName& propertyName, const sdbus::Variant& value, _Function&& callback)
        {
            return proxy_->setPropertyAsync(propertyName).onInterface(interfaceName).toValue(value).uponReplyInvoke(std::forward<_Function>(callback));
        }

        template <typename _Function>
        PendingAsyncCall SetAsync(const InterfaceName& interfaceName, const PropertyName& propertyName, const sdbus::Variant& value, _Function&& callback, return_slot_t)
        {
            return proxy_->setPropertyAsync(propertyName).onInterface(interfaceName).toValue(value).uponReplyInvoke(std::forward<_Function>(callback), return_slot);
        }

        template <typename _Function>
        PendingAsyncCall SetAsync(std::string_view interfaceName, std::string_view propertyName, const sdbus::Variant& value, _Function&& callback)
        {
            return proxy_->setPropertyAsync(propertyName).onInterface(interfaceName).toValue(value).uponReplyInvoke(std::forward<_Function>(callback));
        }

        template <typename _Function>
        PendingAsyncCall SetAsync(std::string_view interfaceName, std::string_view propertyName, const sdbus::Variant& value, _Function&& callback, return_slot_t)
        {
            return proxy_->setPropertyAsync(propertyName).onInterface(interfaceName).toValue(value).uponReplyInvoke(std::forward<_Function>(callback), return_slot);
        }

        std::future<void> SetAsync(const InterfaceName& interfaceName, const PropertyName& propertyName, const sdbus::Variant& value, with_future_t)
        {
            return proxy_->setPropertyAsync(propertyName).onInterface(interfaceName).toValue(value).getResultAsFuture();
        }

        std::future<void> SetAsync(std::string_view interfaceName, std::string_view propertyName, const sdbus::Variant& value, with_future_t)
        {
            return proxy_->setPropertyAsync(propertyName).onInterface(interfaceName).toValue(value).getResultAsFuture();
        }

        std::map<PropertyName, sdbus::Variant> GetAll(const InterfaceName& interfaceName)
        {
            return proxy_->getAllProperties().onInterface(interfaceName);
        }

        std::map<PropertyName, sdbus::Variant> GetAll(std::string_view interfaceName)
        {
            return proxy_->getAllProperties().onInterface(interfaceName);
        }

        template <typename _Function>
        PendingAsyncCall GetAllAsync(const InterfaceName& interfaceName, _Function&& callback)
        {
            return proxy_->getAllPropertiesAsync().onInterface(interfaceName).uponReplyInvoke(std::forward<_Function>(callback));
        }

        template <typename _Function>
        PendingAsyncCall GetAllAsync(const InterfaceName& interfaceName, _Function&& callback, return_slot_t)
        {
            return proxy_->getAllPropertiesAsync().onInterface(interfaceName).uponReplyInvoke(std::forward<_Function>(callback), return_slot);
        }

        template <typename _Function>
        PendingAsyncCall GetAllAsync(std::string_view interfaceName, _Function&& callback)
        {
            return proxy_->getAllPropertiesAsync().onInterface(interfaceName).uponReplyInvoke(std::forward<_Function>(callback));
        }

        template <typename _Function>
        PendingAsyncCall GetAllAsync(std::string_view interfaceName, _Function&& callback, return_slot_t)
        {
            return proxy_->getAllPropertiesAsync().onInterface(interfaceName).uponReplyInvoke(std::forward<_Function>(callback), return_slot);
        }

        std::future<std::map<PropertyName, sdbus::Variant>> GetAllAsync(const InterfaceName& interfaceName, with_future_t)
        {
            return proxy_->getAllPropertiesAsync().onInterface(interfaceName).getResultAsFuture();
        }

        std::future<std::map<PropertyName, sdbus::Variant>> GetAllAsync(std::string_view interfaceName, with_future_t)
        {
            return proxy_->getAllPropertiesAsync().onInterface(interfaceName).getResultAsFuture();
        }

    private:
        sdbus::IProxy* proxy_;
    };

    // Proxy for object manager
    class ObjectManager_proxy
    {
        static inline const char* INTERFACE_NAME = "org.freedesktop.DBus.ObjectManager";

    protected:
        ObjectManager_proxy(sdbus::IProxy& proxy)
            : proxy_(&proxy)
        {
        }

        ObjectManager_proxy(const ObjectManager_proxy&) = delete;
        ObjectManager_proxy& operator=(const ObjectManager_proxy&) = delete;
        ObjectManager_proxy(ObjectManager_proxy&&) = default;
        ObjectManager_proxy& operator=(ObjectManager_proxy&&) = default;

        ~ObjectManager_proxy() = default;

        void registerProxy()
        {
            proxy_
                ->uponSignal("InterfacesAdded")
                .onInterface(INTERFACE_NAME)
                .call([this]( const sdbus::ObjectPath& objectPath
                            , const std::map<sdbus::InterfaceName, std::map<PropertyName, sdbus::Variant>>& interfacesAndProperties )
                            {
                                this->onInterfacesAdded(objectPath, interfacesAndProperties);
                            });

            proxy_->uponSignal("InterfacesRemoved")
                .onInterface(INTERFACE_NAME)
                .call([this]( const sdbus::ObjectPath& objectPath
                            , const std::vector<sdbus::InterfaceName>& interfaces )
                            {
                                this->onInterfacesRemoved(objectPath, interfaces);
                            });
        }

        virtual void onInterfacesAdded( const sdbus::ObjectPath& objectPath
                                      , const std::map<sdbus::InterfaceName, std::map<PropertyName, sdbus::Variant>>& interfacesAndProperties) = 0;
        virtual void onInterfacesRemoved( const sdbus::ObjectPath& objectPath
                                        , const std::vector<sdbus::InterfaceName>& interfaces) = 0;

    public:
        std::map<sdbus::ObjectPath, std::map<sdbus::InterfaceName, std::map<PropertyName, sdbus::Variant>>> GetManagedObjects()
        {
            std::map<sdbus::ObjectPath, std::map<sdbus::InterfaceName, std::map<PropertyName, sdbus::Variant>>> objectsInterfacesAndProperties;
            proxy_->callMethod("GetManagedObjects").onInterface(INTERFACE_NAME).storeResultsTo(objectsInterfacesAndProperties);
            return objectsInterfacesAndProperties;
        }

    private:
        sdbus::IProxy* proxy_;
    };

    // Adaptors for the above-listed standard D-Bus interfaces are not necessary because the functionality
    // is provided by underlying libsystemd implementation. The exception is Properties_adaptor,
    // ObjectManager_adaptor and ManagedObject_adaptor, which provide convenience functionality to emit signals.

    // Adaptor for properties
    class Properties_adaptor
    {
        static inline const char* INTERFACE_NAME = "org.freedesktop.DBus.Properties";

    protected:
        Properties_adaptor(sdbus::IObject& object) : object_(&object)
        {
        }

        Properties_adaptor(const Properties_adaptor&) = delete;
        Properties_adaptor& operator=(const Properties_adaptor&) = delete;
        Properties_adaptor(Properties_adaptor&&) = default;
        Properties_adaptor& operator=(Properties_adaptor&&) = default;

        ~Properties_adaptor() = default;

        void registerAdaptor()
        {
        }

    public:
        void emitPropertiesChangedSignal(const InterfaceName& interfaceName, const std::vector<PropertyName>& properties)
        {
            object_->emitPropertiesChangedSignal(interfaceName, properties);
        }

        void emitPropertiesChangedSignal(const char* interfaceName, const std::vector<PropertyName>& properties)
        {
            object_->emitPropertiesChangedSignal(interfaceName, properties);
        }

        void emitPropertiesChangedSignal(const InterfaceName& interfaceName)
        {
            object_->emitPropertiesChangedSignal(interfaceName);
        }

        void emitPropertiesChangedSignal(const char* interfaceName)
        {
            object_->emitPropertiesChangedSignal(interfaceName);
        }

    private:
        sdbus::IObject* object_;
    };

    /*!
     * @brief Object Manager Convenience Adaptor
     *
     * Adding this class as _Interfaces.. template parameter of class AdaptorInterfaces
     * implements the *GetManagedObjects()* method of the [org.freedesktop.DBus.ObjectManager.GetManagedObjects](https://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-objectmanager)
     * interface.
     *
     * Note that there can be multiple object managers in a path hierarchy. InterfacesAdded/InterfacesRemoved
     * signals are sent from the closest object manager at either the same path or the closest parent path of an object.
     */
    class ObjectManager_adaptor
    {
        static inline const char* INTERFACE_NAME = "org.freedesktop.DBus.ObjectManager";

    protected:
        explicit ObjectManager_adaptor(sdbus::IObject& object) : object_(&object)
        {
        }

        ObjectManager_adaptor(const ObjectManager_adaptor&) = delete;
        ObjectManager_adaptor& operator=(const ObjectManager_adaptor&) = delete;
        ObjectManager_adaptor(ObjectManager_adaptor&&) = default;
        ObjectManager_adaptor& operator=(ObjectManager_adaptor&&) = default;

        ~ObjectManager_adaptor() = default;

        void registerAdaptor()
        {
            object_->addObjectManager();
        }

    private:
        sdbus::IObject* object_;
    };

    /*!
     * @brief Managed Object Convenience Adaptor
     *
     * Adding this class as _Interfaces.. template parameter of class AdaptorInterfaces
     * will extend the resulting object adaptor with emitInterfacesAddedSignal()/emitInterfacesRemovedSignal()
     * according to org.freedesktop.DBus.ObjectManager.InterfacesAdded/.InterfacesRemoved.
     *
     * Note that objects which implement this adaptor require an object manager (e.g via ObjectManager_adaptor) to be
     * instantiated on one of it's parent object paths or the same path. InterfacesAdded/InterfacesRemoved
     * signals are sent from the closest object manager at either the same path or the closest parent path of an object.
     */
    class ManagedObject_adaptor
    {
    protected:
        explicit ManagedObject_adaptor(sdbus::IObject& object)
            : object_(&object)
        {
        }

        ManagedObject_adaptor(const ManagedObject_adaptor&) = delete;
        ManagedObject_adaptor& operator=(const ManagedObject_adaptor&) = delete;
        ManagedObject_adaptor(ManagedObject_adaptor&&) = default;
        ManagedObject_adaptor& operator=(ManagedObject_adaptor&&) = default;

        ~ManagedObject_adaptor() = default;

        void registerAdaptor()
        {
        }

    public:
        /*!
         * @brief Emits InterfacesAdded signal for this object path
         *
         * See IObject::emitInterfacesAddedSignal().
         */
        void emitInterfacesAddedSignal()
        {
            object_->emitInterfacesAddedSignal();
        }

        /*!
         * @brief Emits InterfacesAdded signal for this object path
         *
         * See IObject::emitInterfacesAddedSignal().
         */
        void emitInterfacesAddedSignal(const std::vector<sdbus::InterfaceName>& interfaces)
        {
            object_->emitInterfacesAddedSignal(interfaces);
        }

        /*!
         * @brief Emits InterfacesRemoved signal for this object path
         *
         * See IObject::emitInterfacesRemovedSignal().
         */
        void emitInterfacesRemovedSignal()
        {
            object_->emitInterfacesRemovedSignal();
        }

        /*!
         * @brief Emits InterfacesRemoved signal for this object path
         *
         * See IObject::emitInterfacesRemovedSignal().
         */
        void emitInterfacesRemovedSignal(const std::vector<InterfaceName>& interfaces)
        {
            object_->emitInterfacesRemovedSignal(interfaces);
        }

    private:
        sdbus::IObject* object_;
    };

}

#endif /* SDBUS_CXX_STANDARDINTERFACES_H_ */
