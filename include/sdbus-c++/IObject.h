/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2024 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file IObject.h
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

#ifndef SDBUS_CXX_IOBJECT_H_
#define SDBUS_CXX_IOBJECT_H_

#include <sdbus-c++/VTableItems.h>
#include <sdbus-c++/ConvenienceApiClasses.h>
#include <sdbus-c++/TypeTraits.h>
#include <sdbus-c++/Flags.h>
#include <functional>
#include <string>
#include <memory>
#include <vector>

// Forward declarations
namespace sdbus {
    class Signal;
    class IConnection;
}

namespace sdbus {

    /********************************************//**
     * @class IObject
     *
     * IObject class represents a D-Bus object instance identified by a specific object path.
     * D-Bus object provides its interfaces, methods, signals and properties on a bus
     * identified by a specific bus name.
     *
     * All IObject member methods throw @c sdbus::Error in case of D-Bus or sdbus-c++ error.
     * The IObject class has been designed as thread-aware. However, the operation of
     * creating and sending asynchronous method replies, as well as creating and emitting
     * signals, is thread-safe by design.
     *
     ***********************************************/
    class IObject
    {
    public:
        virtual ~IObject() = default;

        /*!
         * @brief Adds a declaration of methods, properties and signals of the object at a given interface
         *
         * @param[in] interfaceName Name of an interface the the vtable is registered for
         * @param[in] items Individual instances of VTable item structures
         *
         * This method is used to declare attributes for the object under the given interface.
         * Parameter `items' represents a vtable definition that may contain method declarations
         * (using MethodVTableItem struct), property declarations (using PropertyVTableItem
         * struct), signal declarations (using SignalVTableItem struct), or global interface
         * flags (using InterfaceFlagsVTableItem struct).
         *
         * An interface can have any number of vtables attached to it.
         *
         * Consult manual pages for underlying `sd_bus_add_object_vtable` function for more information.
         *
         * The method can be called at any time during object's lifetime. For each vtable an internal
         * match slot is created and its lifetime is tied to the lifetime of the Object instance.
         *
         * The function provides strong exception guarantee. The state of the object remains
         * unmodified in face of an exception.
         *
         * @throws sdbus::Error in case of failure
         */
        template < typename... VTableItems
                 , typename = std::enable_if_t<(is_one_of_variants_types<VTableItem, std::decay_t<VTableItems>> && ...)> >
        void addVTable(std::string interfaceName, VTableItems&&... items);

        /*!
         * @brief Adds a declaration of methods, properties and signals of the object at a given interface
         *
         * @param[in] interfaceName Name of an interface the the vtable is registered for
         * @param[in] vtable A list of individual descriptions in the form of VTable item instances
         *
         * This method is used to declare attributes for the object under the given interface.
         * The `vtable' parameter may contain method declarations (using MethodVTableItem struct),
         * property declarations (using PropertyVTableItem struct), signal declarations (using
         * SignalVTableItem struct), or global interface flags (using InterfaceFlagsVTableItem struct).
         *
         * An interface can have any number of vtables attached to it.
         *
         * Consult manual pages for underlying `sd_bus_add_object_vtable` function for more information.
         *
         * The method can be called at any time during object's lifetime. For each vtable an internal
         * match slot is created and its lifetime is tied to the lifetime of the Object instance.
         *
         * The function provides strong exception guarantee. The state of the object remains
         * unmodified in face of an exception.
         *
         * @throws sdbus::Error in case of failure
         */
        virtual void addVTable(std::string interfaceName, std::vector<VTableItem> vtable) = 0;

        /*!
         * @brief Adds a declaration of methods, properties and signals of the object at a given interface
         *
         * @param[in] interfaceName Name of an interface the the vtable is registered for
         * @param[in] vtable A list of individual descriptions in the form of VTable item instances
         *
         * This method is used to declare attributes for the object under the given interface.
         * The `vtable' parameter may contain method declarations (using MethodVTableItem struct),
         * property declarations (using PropertyVTableItem struct), signal declarations (using
         * SignalVTableItem struct), or global interface flags (using InterfaceFlagsVTableItem struct).
         *
         * An interface can have any number of vtables attached to it.
         *
         * Consult manual pages for underlying `sd_bus_add_object_vtable` function for more information.
         *
         * The method can be called at any time during object's lifetime. For each vtable an internal
         * match slot is created and is returned to the caller. The returned slot should be destroyed
         * when the vtable is not needed anymore. This allows for "dynamic" object API where vtables
         * can be added or removed by the user at runtime.
         *
         * The function provides strong exception guarantee. The state of the object remains
         * unmodified in face of an exception.
         *
         * @throws sdbus::Error in case of failure
         */
        [[nodiscard]] virtual Slot addVTable(std::string interfaceName, std::vector<VTableItem> vtable, return_slot_t) = 0;

        /*!
         * @brief A little more convenient overload of addVTable() above
         *
         * This version allows method chaining for a little safer and more readable VTable registration.
         *
         * See addVTable() overloads above for detailed documentation.
         */
        template < typename... VTableItems
                 , typename = std::enable_if_t<(is_one_of_variants_types<VTableItem, std::decay_t<VTableItems>> && ...)> >
        [[nodiscard]] VTableAdder addVTable(VTableItems&&... items);

        /*!
         * @brief A little more convenient overload of addVTable() above
         *
         * This version allows method chaining for a little safer and more readable VTable registration.
         *
         * See addVTable() overloads above for detailed documentation.
         */
        [[nodiscard]] VTableAdder addVTable(std::vector<VTableItem> vtable);

        /*!
         * @brief Unregisters object's API and removes object from the bus
         *
         * This method unregisters the object, its interfaces, methods, signals and properties
         * from the bus. Unregistration is done automatically also in object's destructor. This
         * method makes sense if, in the process of object removal, we need to make sure that
         * callbacks are unregistered explicitly before the final destruction of the object instance.
         *
         * @throws sdbus::Error in case of failure
         */
        virtual void unregister() = 0;

        /*!
         * @brief Creates a signal message
         *
         * @param[in] interfaceName Name of an interface that the signal belongs under
         * @param[in] signalName Name of the signal
         * @return A signal message
         *
         * Serialize signal arguments into the returned message and emit the signal by passing
         * the message with serialized arguments to the @c emitSignal function.
         * Alternatively, use higher-level API @c emitSignal(const std::string& signalName) defined below.
         *
         * @throws sdbus::Error in case of failure
         */
        [[nodiscard]] virtual Signal createSignal(const std::string& interfaceName, const std::string& signalName) = 0;

        /*!
         * @brief Emits signal for this object path
         *
         * @param[in] message Signal message to be sent out
         *
         * Note: To avoid messing with messages, use higher-level API defined below.
         *
         * @throws sdbus::Error in case of failure
         */
        virtual void emitSignal(const sdbus::Signal& message) = 0;

        /*!
         * @brief Emits PropertyChanged signal for specified properties under a given interface of this object path
         *
         * @param[in] interfaceName Name of an interface that properties belong to
         * @param[in] propNames Names of properties that will be included in the PropertiesChanged signal
         *
         * @throws sdbus::Error in case of failure
         */
        virtual void emitPropertiesChangedSignal(const std::string& interfaceName, const std::vector<std::string>& propNames) = 0;

        /*!
         * @brief Emits PropertyChanged signal for all properties on a given interface of this object path
         *
         * @param[in] interfaceName Name of an interface
         *
         * @throws sdbus::Error in case of failure
         */
        virtual void emitPropertiesChangedSignal(const std::string& interfaceName) = 0;

        /*!
         * @brief Emits InterfacesAdded signal on this object path
         *
         * This emits an InterfacesAdded signal on this object path, by iterating all registered
         * interfaces on the path. All properties are queried and included in the signal.
         * This call is equivalent to emitInterfacesAddedSignal() with an explicit list of
         * registered interfaces. However, unlike emitInterfacesAddedSignal(interfaces), this
         * call can figure out the list of supported interfaces itself. Furthermore, it properly
         * adds the builtin org.freedesktop.DBus.* interfaces.
         *
         * @throws sdbus::Error in case of failure
         */
        virtual void emitInterfacesAddedSignal() = 0;

        /*!
         * @brief Emits InterfacesAdded signal on this object path
         *
         * This emits an InterfacesAdded signal on this object path with explicitly provided list
         * of registered interfaces. Since v2.0, sdbus-c++ supports dynamically addable/removable
         * object interfaces and their vtables, so this method now makes more sense.
         *
         * @throws sdbus::Error in case of failure
         */
        virtual void emitInterfacesAddedSignal(const std::vector<std::string>& interfaces) = 0;

        /*!
         * @brief Emits InterfacesRemoved signal on this object path
         *
         * This is like sd_bus_emit_object_added(), but emits an InterfacesRemoved signal on this
         * object path. This only includes any registered interfaces but skips the properties.
         * This function shall be called (just) before destroying the object.
         *
         * @throws sdbus::Error in case of failure
         */
        virtual void emitInterfacesRemovedSignal() = 0;

        /*!
         * @brief Emits InterfacesRemoved signal on this object path
         *
         * This emits an InterfacesRemoved signal on the given path with explicitly provided list
         * of registered interfaces. Since v2.0, sdbus-c++ supports dynamically addable/removable
         * object interfaces and their vtables, so this method now makes more sense.
         *
         * @throws sdbus::Error in case of failure
         */
        virtual void emitInterfacesRemovedSignal(const std::vector<std::string>& interfaces) = 0;

        /*!
         * @brief Adds an ObjectManager interface at the path of this D-Bus object
         *
         * Creates an ObjectManager interface at the specified object path on
         * the connection. This is a convenient way to interrogate a connection
         * to see what objects it has.
         *
         * @throws sdbus::Error in case of failure
         */
        virtual void addObjectManager() = 0;

        /*!
         * @brief Removes an ObjectManager interface from the path of this D-Bus object
         *
         * @throws sdbus::Error in case of failure
         */
        virtual void removeObjectManager() = 0;

        /*!
         * @brief Tests whether ObjectManager interface is added at the path of this D-Bus object
         * @return True if ObjectManager interface is there, false otherwise
         */
        [[nodiscard]] virtual bool hasObjectManager() const = 0;

        /*!
         * @brief Provides D-Bus connection used by the object
         *
         * @return Reference to the D-Bus connection
         */
        [[nodiscard]] virtual sdbus::IConnection& getConnection() const = 0;

        /*!
         * @brief Emits signal on D-Bus
         *
         * @param[in] signalName Name of the signal
         * @return A helper object for convenient emission of signals
         *
         * This is a high-level, convenience way of emitting D-Bus signals that abstracts
         * from the D-Bus message concept. Signal arguments are automatically serialized
         * in a message and D-Bus signatures automatically deduced from the provided native arguments.
         *
         * Example of use:
         * @code
         * int arg1 = ...;
         * double arg2 = ...;
         * object_.emitSignal("fooSignal").onInterface("com.kistler.foo").withArguments(arg1, arg2);
         * @endcode
         *
         * @throws sdbus::Error in case of failure
         */
        [[nodiscard]] SignalEmitter emitSignal(const std::string& signalName);

        /*!
         * @brief Returns object path of the underlying DBus object
         */
        [[nodiscard]] virtual const std::string& getObjectPath() const = 0;

        /*!
         * @brief Provides access to the currently processed D-Bus message
         *
         * This method provides access to the currently processed incoming D-Bus message.
         * "Currently processed" means that the registered callback handler(s) for that message
         * are being invoked. This method is meant to be called from within a callback handler
         * (e.g. from a D-Bus signal handler, or async method reply handler, etc.). In such a case it is
         * guaranteed to return a valid D-Bus message instance for which the handler is called.
         * If called from other contexts/threads, it may return a valid or invalid message, depending
         * on whether a message was processed or not at the time of the call.
         *
         * @return Currently processed D-Bus message
         */
        [[nodiscard]] virtual Message getCurrentlyProcessedMessage() const = 0;
    };

    // Out-of-line member definitions

    inline SignalEmitter IObject::emitSignal(const std::string& signalName)
    {
        return SignalEmitter(*this, signalName);
    }

    template <typename... VTableItems, typename>
    void IObject::addVTable(std::string interfaceName, VTableItems&&... items)
    {
        addVTable(std::move(interfaceName), {std::forward<VTableItems>(items)...});
    }

    template <typename... VTableItems, typename>
    VTableAdder IObject::addVTable(VTableItems&&... items)
    {
        return addVTable(std::vector<VTableItem>{std::forward<VTableItems>(items)...});
    }

    inline VTableAdder IObject::addVTable(std::vector<VTableItem> vtable)
    {
        return VTableAdder(*this, std::move(vtable));
    }

    /*!
     * @brief Creates instance representing a D-Bus object
     *
     * @param[in] connection D-Bus connection to be used by the object
     * @param[in] objectPath Path of the D-Bus object
     * @return Pointer to the object representation instance
     *
     * The provided connection will be used by the object to export methods,
     * issue signals and provide properties.
     *
     * Creating a D-Bus object instance is (thread-)safe even upon the connection
     * which is already running its I/O event loop.
     *
     * Code example:
     * @code
     * auto proxy = sdbus::createObject(connection, "/com/kistler/foo");
     * @endcode
     */
    [[nodiscard]] std::unique_ptr<sdbus::IObject> createObject(sdbus::IConnection& connection, std::string objectPath);

}

#include <sdbus-c++/VTableItems.inl>
#include <sdbus-c++/ConvenienceApiClasses.inl>

#endif /* SDBUS_CXX_IOBJECT_H_ */
