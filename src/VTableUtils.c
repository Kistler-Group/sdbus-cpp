/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 *
 * @file VTableUtils.c
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

#include "VTableUtils.h"
#include <systemd/sd-bus.h>

sd_bus_vtable createVTableStartItem()
{
    struct sd_bus_vtable vtableStart = SD_BUS_VTABLE_START(0);
    return vtableStart;
}

sd_bus_vtable createVTableMethodItem( const char *member
                                    , const char *signature
                                    , const char *result
                                    , sd_bus_message_handler_t handler
                                    , bool noReply )
{
    uint64_t flags = SD_BUS_VTABLE_UNPRIVILEGED | (noReply ? SD_BUS_VTABLE_METHOD_NO_REPLY : 0ULL);
    struct sd_bus_vtable vtableItem = SD_BUS_METHOD(member, signature, result, handler, flags);
    return vtableItem;
}

sd_bus_vtable createVTableSignalItem( const char *member
                                    , const char *signature )
{
    struct sd_bus_vtable vtableItem = SD_BUS_SIGNAL(member, signature, 0);
    return vtableItem;
}

sd_bus_vtable createVTablePropertyItem( const char *member
                                      , const char *signature
                                      , sd_bus_property_get_t getter
                                      , bool isConst)
{
    unsigned long long sdbusFlags = 0ULL;

    if (isConst)
        sdbusFlags |= SD_BUS_VTABLE_PROPERTY_CONST;

    struct sd_bus_vtable vtableItem = SD_BUS_PROPERTY(member, signature, getter, 0, sdbusFlags);
    return vtableItem;
}

sd_bus_vtable createVTableWritablePropertyItem( const char *member
                                              , const char *signature
                                              , sd_bus_property_get_t getter
                                              , sd_bus_property_set_t setter
                                              , bool emitsChange
                                              , bool emitsInvalidation)
{
    unsigned long long sdbusFlags = 0ULL;

    if (emitsChange) {
        sdbusFlags |= SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE;
    } else {
        if (emitsInvalidation) {
            sdbusFlags |= SD_BUS_VTABLE_PROPERTY_EMITS_INVALIDATION;
        }
    }

    struct sd_bus_vtable vtableItem = SD_BUS_WRITABLE_PROPERTY(member, signature, getter, setter, 0, sdbusFlags);
    return vtableItem;
}

sd_bus_vtable createVTableEndItem()
{
    struct sd_bus_vtable vtableEnd = SD_BUS_VTABLE_END;
    return vtableEnd;
}
