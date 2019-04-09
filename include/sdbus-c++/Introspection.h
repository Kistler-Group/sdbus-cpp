/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 *
 * @file Introspection.h
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

#ifndef SDBUS_CXX_INTROSPECTION_H_
#define SDBUS_CXX_INTROSPECTION_H_

#include <sdbus-c++/IObject.h>
#include <sdbus-c++/IProxy.h>
#include <string>

namespace sdbus {

    // Proxy for introspection
    class introspectable_proxy
    {
        static constexpr const char* interfaceName = "org.freedesktop.DBus.Introspectable";

    protected:
        introspectable_proxy(sdbus::IProxy& object)
            : object_(object)
        {
        }

    public:
        std::string Introspect()
        {
            std::string xml;
            object_.callMethod("Introspect").onInterface(interfaceName).storeResultsTo(xml);
            return xml;
        }

    private:
        sdbus::IProxy& object_;
    };

    // Adaptor is not necessary if we want to rely on sdbus-provided introspection
//    class introspectable_adaptor
//    {
//        static constexpr const char* interfaceName = "org.freedesktop.DBus.Introspectable";
//
//    protected:
//        introspectable_adaptor(sdbus::IObject& object)
//            : object_(object)
//        {
//            object_.registerMethod("Introspect").onInterface(interfaceName).implementedAs([this](){ return object_.introspect(); });
//        }
//
//    public:
//        std::string introspect()
//        {
//            std::string xml;
//            object_.callMethod("Introspect").onInterface(interfaceName).storeResultsTo(xml);
//            return xml;
//        }
//
//    private:
//        sdbus::IObject& object_;
//    };

}

#endif /* SDBUS_CXX_INTROSPECTION_H_ */
