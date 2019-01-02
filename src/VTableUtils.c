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

sd_bus_vtable createVTableStartItem(uint64_t flags)
{
    struct sd_bus_vtable vtableStart = SD_BUS_VTABLE_START(flags);
    return vtableStart;
}

sd_bus_vtable createVTableMethodItem( const char *member
                                    , const char *signature
                                    , const char *result
                                    , sd_bus_message_handler_t handler
                                    , uint64_t flags )
{
    struct sd_bus_vtable vtableItem = SD_BUS_METHOD(member, signature, result, handler, flags);
    return vtableItem;
}

sd_bus_vtable createVTableSignalItem( const char *member
                                    , const char *signature
                                    , uint64_t flags )
{
    struct sd_bus_vtable vtableItem = SD_BUS_SIGNAL(member, signature, flags);
    return vtableItem;
}

sd_bus_vtable createVTablePropertyItem( const char *member
                                      , const char *signature
                                      , sd_bus_property_get_t getter
                                      , uint64_t flags )
{
    struct sd_bus_vtable vtableItem = SD_BUS_PROPERTY(member, signature, getter, 0, flags);
    return vtableItem;
}

sd_bus_vtable createVTableWritablePropertyItem( const char *member
                                              , const char *signature
                                              , sd_bus_property_get_t getter
                                              , sd_bus_property_set_t setter
                                              , uint64_t flags )
{
    struct sd_bus_vtable vtableItem = SD_BUS_WRITABLE_PROPERTY(member, signature, getter, setter, 0, flags);
    return vtableItem;
}

sd_bus_vtable createVTableEndItem()
{
    struct sd_bus_vtable vtableEnd = SD_BUS_VTABLE_END;
    return vtableEnd;
}
