/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 *
 * @file Interfaces.h
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

#ifndef SDBUS_CXX_INTERFACES_H_
#define SDBUS_CXX_INTERFACES_H_

#include <sdbus-c++/IObject.h>
#include <sdbus-c++/IObjectProxy.h>
#include <cassert>
#include <string>
#include <memory>

// Forward declarations
namespace sdbus {
    class IConnection;
}

namespace sdbus {

    template <typename _Object>
    class ObjectHolder
    {
    protected:
        ObjectHolder(std::unique_ptr<_Object>&& object)
            : object_(std::move(object))
        {
        }

        const _Object& getObject() const
        {
            assert(object_ != nullptr);
            return *object_;
        }

        _Object& getObject()
        {
            assert(object_ != nullptr);
            return *object_;
        }

    private:
        std::unique_ptr<_Object> object_;
    };

    /********************************************//**
     * @class Interfaces
     *
     * A helper template class that a user class representing a D-Bus object
     * should inherit from, providing as template arguments the adaptor
     * classes representing D-Bus interfaces that the object implements.
     *
     ***********************************************/
    template <typename... _Interfaces>
    class Interfaces
        : private ObjectHolder<IObject>
        , public _Interfaces...
    {
    public:
        Interfaces(IConnection& connection, std::string objectPath)
            : ObjectHolder<IObject>(createObject(connection, std::move(objectPath)))
            , _Interfaces(getObject())...
        {
            getObject().finishRegistration();
        }
    };

    /********************************************//**
     * @class Interfaces
     *
     * A helper template class that a user class representing a proxy of a
     * D-Bus object should inherit from, providing as template arguments the proxy
     * classes representing object D-Bus interfaces that the object implements.
     *
     ***********************************************/
    template <typename... _Interfaces>
    class ProxyInterfaces
        : private ObjectHolder<IObjectProxy>
        , public _Interfaces...
    {
    public:
        ProxyInterfaces(std::string destination, std::string objectPath)
            : ObjectHolder<IObjectProxy>(createObjectProxy(std::move(destination), std::move(objectPath)))
            , _Interfaces(getObject())...
        {
            getObject().finishRegistration();
        }

        ProxyInterfaces(IConnection& connection, std::string destination, std::string objectPath)
            : ObjectHolder<IObjectProxy>(createObjectProxy(connection, std::move(destination), std::move(objectPath)))
            , _Interfaces(getObject())...
        {
            getObject().finishRegistration();
        }
    };

}

#endif /* SDBUS_CXX_INTERFACES_H_ */
