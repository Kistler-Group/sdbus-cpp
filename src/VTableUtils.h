/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2024 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file VTableUtils.h
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

#ifndef SDBUS_CXX_INTERNAL_VTABLEUTILS_H_
#define SDBUS_CXX_INTERNAL_VTABLEUTILS_H_

#include <stdbool.h>
#include SDBUS_HEADER

#ifdef __cplusplus
extern "C" {
#endif

sd_bus_vtable createSdBusVTableStartItem(uint64_t flags);
sd_bus_vtable createSdBusVTableMethodItem( const char *member
                                         , const char *signature
                                         , const char *result
                                         , const char *paramNames
                                         , sd_bus_message_handler_t handler
                                         , uint64_t flags );
sd_bus_vtable createSdBusVTableSignalItem( const char *member
                                         , const char *signature
                                         , const char *outnames
                                         , uint64_t flags );
sd_bus_vtable createSdBusVTableReadOnlyPropertyItem( const char *member
                                                   , const char *signature
                                                   , sd_bus_property_get_t getter
                                                   , uint64_t flags );
sd_bus_vtable createSdBusVTableWritablePropertyItem( const char *member
                                                   , const char *signature
                                                   , sd_bus_property_get_t getter
                                                   , sd_bus_property_set_t setter
                                                   , uint64_t flags );
sd_bus_vtable createSdBusVTableEndItem();

#ifdef __cplusplus
}
#endif

#endif /* SDBUS_CXX_INTERNAL_VTABLEUTILS_H_ */
