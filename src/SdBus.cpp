/**
 * (C) 2016 - 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2019 Stanislav Angelovic <angelovic.s@gmail.com>
 *
 * @file SdBus.cpp
 * @author Ardazishvili Roman (ardazishvili.roman@yandex.ru)
 *
 * Created on: Mar 3, 2019
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

#include "SdBus.h"
#include <sdbus-c++/Error.h>

namespace sdbus { namespace internal {

sd_bus_message* SdBus::sd_bus_message_ref(sd_bus_message *m)
{
    std::unique_lock<std::recursive_mutex> lock(sdbusMutex_);

    return ::sd_bus_message_ref(m);
}

sd_bus_message* SdBus::sd_bus_message_unref(sd_bus_message *m)
{
    std::unique_lock<std::recursive_mutex> lock(sdbusMutex_);

    return ::sd_bus_message_unref(m);
}

int SdBus::sd_bus_send(sd_bus *bus, sd_bus_message *m, uint64_t *cookie)
{
    std::unique_lock<std::recursive_mutex> lock(sdbusMutex_);

    return ::sd_bus_send(bus, m, cookie);
}

int SdBus::sd_bus_call(sd_bus *bus, sd_bus_message *m, uint64_t usec, sd_bus_error *ret_error, sd_bus_message **reply)
{
    std::unique_lock<std::recursive_mutex> lock(sdbusMutex_);

    return ::sd_bus_call(bus, m, usec, ret_error, reply);
}

int SdBus::sd_bus_call_async(sd_bus *bus, sd_bus_slot **slot, sd_bus_message *m, sd_bus_message_handler_t callback, void *userdata, uint64_t usec)
{
    std::unique_lock<std::recursive_mutex> lock(sdbusMutex_);

    return ::sd_bus_call_async(bus, slot, m, callback, userdata, usec);
}

int SdBus::sd_bus_message_new_method_call(sd_bus *bus, sd_bus_message **m, const char *destination, const char *path, const char *interface, const char *member)
{
    std::unique_lock<std::recursive_mutex> lock(sdbusMutex_);

    return ::sd_bus_message_new_method_call(bus, m, destination, path, interface, member);
}

int SdBus::sd_bus_message_new_signal(sd_bus *bus, sd_bus_message **m, const char *path, const char *interface, const char *member)
{
    std::unique_lock<std::recursive_mutex> lock(sdbusMutex_);

    return ::sd_bus_message_new_signal(bus, m, path, interface, member);
}

int SdBus::sd_bus_message_new_method_return(sd_bus_message *call, sd_bus_message **m)
{
    std::unique_lock<std::recursive_mutex> lock(sdbusMutex_);

    return ::sd_bus_message_new_method_return(call, m);
}

int SdBus::sd_bus_message_new_method_error(sd_bus_message *call, sd_bus_message **m, const sd_bus_error *e)
{
    std::unique_lock<std::recursive_mutex> lock(sdbusMutex_);

    return ::sd_bus_message_new_method_error(call, m, e);
}

int SdBus::sd_bus_set_method_call_timeout(sd_bus *bus, uint64_t usec)
{
#if LIBSYSTEMD_VERSION>=240
    std::unique_lock<std::recursive_mutex> lock(sdbusMutex_);

    return ::sd_bus_set_method_call_timeout(bus, usec);
#else
    (void)bus;
    (void)usec;
    throw sdbus::Error(SD_BUS_ERROR_NOT_SUPPORTED, "Setting general method call timeout not supported by underlying version of libsystemd");
#endif
}

int SdBus::sd_bus_get_method_call_timeout(sd_bus *bus, uint64_t *ret)
{
#if LIBSYSTEMD_VERSION>=240
    std::unique_lock<std::recursive_mutex> lock(sdbusMutex_);

    return ::sd_bus_get_method_call_timeout(bus, ret);
#else
    (void)bus;
    (void)ret;
    throw sdbus::Error(SD_BUS_ERROR_NOT_SUPPORTED, "Getting general method call timeout not supported by underlying version of libsystemd");
#endif
}

int SdBus::sd_bus_emit_properties_changed_strv(sd_bus *bus, const char *path, const char *interface, char **names)
{
    std::unique_lock<std::recursive_mutex> lock(sdbusMutex_);

    return ::sd_bus_emit_properties_changed_strv(bus, path, interface, names);
}

int SdBus::sd_bus_emit_object_added(sd_bus *bus, const char *path)
{
    std::unique_lock<std::recursive_mutex> lock(sdbusMutex_);

    return ::sd_bus_emit_object_added(bus, path);
}

int SdBus::sd_bus_emit_object_removed(sd_bus *bus, const char *path)
{
    std::unique_lock<std::recursive_mutex> lock(sdbusMutex_);

    return ::sd_bus_emit_object_removed(bus, path);
}

int SdBus::sd_bus_emit_interfaces_added_strv(sd_bus *bus, const char *path, char **interfaces)
{
    std::unique_lock<std::recursive_mutex> lock(sdbusMutex_);

    return ::sd_bus_emit_interfaces_added_strv(bus, path, interfaces);
}

int SdBus::sd_bus_emit_interfaces_removed_strv(sd_bus *bus, const char *path, char **interfaces)
{
    std::unique_lock<std::recursive_mutex> lock(sdbusMutex_);

    return ::sd_bus_emit_interfaces_removed_strv(bus, path, interfaces);
}

int SdBus::sd_bus_open_user(sd_bus **ret)
{
    return ::sd_bus_open_user(ret);
}

int SdBus::sd_bus_open_system(sd_bus **ret)
{
    return ::sd_bus_open_system(ret);
}

int SdBus::sd_bus_open_system_remote(sd_bus **ret, const char *host)
{
    return ::sd_bus_open_system_remote(ret, host);
}

int SdBus::sd_bus_request_name(sd_bus *bus, const char *name, uint64_t flags)
{
    std::unique_lock<std::recursive_mutex> lock(sdbusMutex_);

    return ::sd_bus_request_name(bus, name, flags);
}

int SdBus::sd_bus_release_name(sd_bus *bus, const char *name)
{
    std::unique_lock<std::recursive_mutex> lock(sdbusMutex_);

    return ::sd_bus_release_name(bus, name);
}

int SdBus::sd_bus_add_object_vtable(sd_bus *bus, sd_bus_slot **slot, const char *path, const char *interface, const sd_bus_vtable *vtable, void *userdata)
{
    std::unique_lock<std::recursive_mutex> lock(sdbusMutex_);

    return ::sd_bus_add_object_vtable(bus, slot, path, interface,  vtable, userdata);
}

int SdBus::sd_bus_add_object_manager(sd_bus *bus, sd_bus_slot **slot, const char *path)
{
    std::unique_lock<std::recursive_mutex> lock(sdbusMutex_);

    return ::sd_bus_add_object_manager(bus, slot, path);
}

int SdBus::sd_bus_add_match(sd_bus *bus, sd_bus_slot **slot, const char *match, sd_bus_message_handler_t callback, void *userdata)
{
    std::unique_lock<std::recursive_mutex> lock(sdbusMutex_);

    return :: sd_bus_add_match(bus, slot, match, callback, userdata);
}

sd_bus_slot* SdBus::sd_bus_slot_unref(sd_bus_slot *slot)
{
    std::unique_lock<std::recursive_mutex> lock(sdbusMutex_);

    return ::sd_bus_slot_unref(slot);
}

int SdBus::sd_bus_process(sd_bus *bus, sd_bus_message **r)
{
    std::unique_lock<std::recursive_mutex> lock(sdbusMutex_);

    return ::sd_bus_process(bus, r);
}

int SdBus::sd_bus_get_poll_data(sd_bus *bus, PollData* data)
{
    std::unique_lock<std::recursive_mutex> lock(sdbusMutex_);

    auto r = ::sd_bus_get_fd(bus);
    if (r < 0)
        return r;
    data->fd = r;

    r = ::sd_bus_get_events(bus);
    if (r < 0)
        return r;
    data->events = static_cast<short int>(r);

    r = ::sd_bus_get_timeout(bus, &data->timeout_usec);

    return r;
}

int SdBus::sd_bus_flush(sd_bus *bus)
{
    return ::sd_bus_flush(bus);
}

sd_bus* SdBus::sd_bus_flush_close_unref(sd_bus *bus)
{
    return ::sd_bus_flush_close_unref(bus);
}

}}
