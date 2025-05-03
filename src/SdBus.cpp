/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2024 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
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
#include "sdbus-c++/Error.h" // NOLINT(misc-include-cleaner)
#include SDBUS_HEADER
#include <algorithm>
#include <cstdint>
#include <mutex>
#include <sys/types.h>

namespace sdbus::internal {

sd_bus_message* SdBus::sd_bus_message_ref(sd_bus_message *msg)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_message_ref(msg);
}

sd_bus_message* SdBus::sd_bus_message_unref(sd_bus_message *msg)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_message_unref(msg);
}

int SdBus::sd_bus_send(sd_bus *bus, sd_bus_message *msg, uint64_t *cookie)
{
    const std::lock_guard lock(sdbusMutex_);

    auto r = ::sd_bus_send(bus, msg, cookie);
    if (r < 0)
        return r;

    return r;
}

int SdBus::sd_bus_call(sd_bus *bus, sd_bus_message *msg, uint64_t usec, sd_bus_error *ret_error, sd_bus_message **reply)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_call(bus, msg, usec, ret_error, reply);
}

int SdBus::sd_bus_call_async(sd_bus *bus, sd_bus_slot **slot, sd_bus_message *msg, sd_bus_message_handler_t callback, void *userdata, uint64_t usec)
{
    const std::lock_guard lock(sdbusMutex_);

    auto r = ::sd_bus_call_async(bus, slot, msg, callback, userdata, usec);
    if (r < 0)
      return r;

    return r;
}

int SdBus::sd_bus_message_new(sd_bus *bus, sd_bus_message **msg, uint8_t type)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_message_new(bus, msg, type);
}

int SdBus::sd_bus_message_new_method_call(sd_bus *bus, sd_bus_message **msg, const char *destination, const char *path, const char *interface, const char *member)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_message_new_method_call(bus, msg, destination, path, interface, member);
}

int SdBus::sd_bus_message_new_signal(sd_bus *bus, sd_bus_message **msg, const char *path, const char *interface, const char *member)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_message_new_signal(bus, msg, path, interface, member);
}

int SdBus::sd_bus_message_new_method_return(sd_bus_message *call, sd_bus_message **msg)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_message_new_method_return(call, msg);
}

int SdBus::sd_bus_message_new_method_error(sd_bus_message *call, sd_bus_message **msg, const sd_bus_error *err)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_message_new_method_error(call, msg, err);
}

int SdBus::sd_bus_set_method_call_timeout(sd_bus *bus, uint64_t usec)
{
#if LIBSYSTEMD_VERSION>=240
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_set_method_call_timeout(bus, usec);
#else
    (void)bus;
    (void)usec;
    throw Error(Error::Name{SD_BUS_ERROR_NOT_SUPPORTED}, "Setting general method call timeout not supported by underlying version of libsystemd");
#endif
}

int SdBus::sd_bus_get_method_call_timeout(sd_bus *bus, uint64_t *ret)
{
#if LIBSYSTEMD_VERSION>=240
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_get_method_call_timeout(bus, ret);
#else
    (void)bus;
    (void)ret;
    throw Error(Error::Name{SD_BUS_ERROR_NOT_SUPPORTED}, "Getting general method call timeout not supported by underlying version of libsystemd");
#endif
}

int SdBus::sd_bus_emit_properties_changed_strv(sd_bus *bus, const char *path, const char *interface, char **names)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_emit_properties_changed_strv(bus, path, interface, names);
}

int SdBus::sd_bus_emit_object_added(sd_bus *bus, const char *path)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_emit_object_added(bus, path);
}

int SdBus::sd_bus_emit_object_removed(sd_bus *bus, const char *path)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_emit_object_removed(bus, path);
}

int SdBus::sd_bus_emit_interfaces_added_strv(sd_bus *bus, const char *path, char **interfaces)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_emit_interfaces_added_strv(bus, path, interfaces);
}

int SdBus::sd_bus_emit_interfaces_removed_strv(sd_bus *bus, const char *path, char **interfaces)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_emit_interfaces_removed_strv(bus, path, interfaces);
}

int SdBus::sd_bus_open(sd_bus **ret)
{
    return ::sd_bus_open(ret);
}

int SdBus::sd_bus_open_system(sd_bus **ret)
{
    return ::sd_bus_open_system(ret);
}

int SdBus::sd_bus_open_user(sd_bus **ret)
{
    return ::sd_bus_open_user(ret);
}

int SdBus::sd_bus_open_user_with_address(sd_bus **ret, const char* address)
{
    sd_bus* bus = nullptr;

    int r = ::sd_bus_new(&bus);
    if (r < 0)
        return r;

    r = ::sd_bus_set_address(bus, address);
    if (r < 0)
        return r;

    r = ::sd_bus_set_bus_client(bus, true); // NOLINT(readability-implicit-bool-conversion)
    if (r < 0)
        return r;

    // Copying behavior from
    // https://github.com/systemd/systemd/blob/fee6441601c979165ebcbb35472036439f8dad5f/src/libsystemd/sd-bus/sd-bus.c#L1381
    // Here, we make the bus as trusted
    r = ::sd_bus_set_trusted(bus, true); // NOLINT(readability-implicit-bool-conversion)
    if (r < 0)
        return r;

    r = ::sd_bus_start(bus);
    if (r < 0)
        return r;

    *ret = bus;

    return 0;
}

int SdBus::sd_bus_open_direct(sd_bus **ret, const char* address)
{
    sd_bus* bus = nullptr;

    int r = ::sd_bus_new(&bus);
    if (r < 0)
        return r;

    r = ::sd_bus_set_address(bus, address);
    if (r < 0)
        return r;

    r = ::sd_bus_start(bus);
    if (r < 0)
        return r;

    *ret = bus;

    return 0;
}

int SdBus::sd_bus_open_direct(sd_bus **ret, int fd)
{
    sd_bus* bus = nullptr;

    int r = ::sd_bus_new(&bus);
    if (r < 0)
        return r;

    r = ::sd_bus_set_fd(bus, fd, fd);
    if (r < 0)
        return r;

    r = ::sd_bus_start(bus);
    if (r < 0)
        return r;

    *ret = bus;

    return 0;
}

int SdBus::sd_bus_open_server(sd_bus **ret, int fd)
{
    sd_bus* bus = nullptr;

    int r = ::sd_bus_new(&bus);
    if (r < 0)
        return r;

    r = ::sd_bus_set_fd(bus, fd, fd);
    if (r < 0)
        return r;

    sd_id128_t id;
    r = ::sd_id128_randomize(&id);
    if (r < 0)
        return r;

    r = ::sd_bus_set_server(bus, true, id); // NOLINT(readability-implicit-bool-conversion)
    if (r < 0)
        return r;

    r = ::sd_bus_start(bus);
    if (r < 0)
        return r;

    *ret = bus;

    return 0;
}

int SdBus::sd_bus_open_system_remote(sd_bus **ret, const char *host)
{
#ifndef SDBUS_basu
    return ::sd_bus_open_system_remote(ret, host);
#else
    (void)ret;
    (void)host;
    // https://git.sr.ht/~emersion/basu/commit/01d33b244eb6
    return -EOPNOTSUPP;
#endif
}

int SdBus::sd_bus_request_name(sd_bus *bus, const char *name, uint64_t flags)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_request_name(bus, name, flags);
}

int SdBus::sd_bus_release_name(sd_bus *bus, const char *name)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_release_name(bus, name);
}

int SdBus::sd_bus_get_unique_name(sd_bus *bus, const char **name)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_get_unique_name(bus, name);
}

int SdBus::sd_bus_add_object_vtable(sd_bus *bus, sd_bus_slot **slot, const char *path, const char *interface, const sd_bus_vtable *vtable, void *userdata)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_add_object_vtable(bus, slot, path, interface,  vtable, userdata);
}

int SdBus::sd_bus_add_object_manager(sd_bus *bus, sd_bus_slot **slot, const char *path)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_add_object_manager(bus, slot, path);
}

int SdBus::sd_bus_add_match(sd_bus *bus, sd_bus_slot **slot, const char *match, sd_bus_message_handler_t callback, void *userdata)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_add_match(bus, slot, match, callback, userdata);
}

int SdBus::sd_bus_add_match_async(sd_bus *bus, sd_bus_slot **slot, const char *match, sd_bus_message_handler_t callback, sd_bus_message_handler_t install_callback, void *userdata)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_add_match_async(bus, slot, match, callback, install_callback, userdata);
}

int SdBus::sd_bus_match_signal(sd_bus *bus, sd_bus_slot **ret, const char *sender, const char *path, const char *interface, const char *member, sd_bus_message_handler_t callback, void *userdata)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_match_signal(bus, ret, sender, path, interface, member, callback, userdata);
}

sd_bus_slot* SdBus::sd_bus_slot_unref(sd_bus_slot *slot)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_slot_unref(slot);
}

int SdBus::sd_bus_new(sd_bus **ret)
{
    return ::sd_bus_new(ret);
}

int SdBus::sd_bus_start(sd_bus *bus)
{
    return ::sd_bus_start(bus);
}

int SdBus::sd_bus_process(sd_bus *bus, sd_bus_message **msg)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_process(bus, msg);
}

sd_bus_message* SdBus::sd_bus_get_current_message(sd_bus *bus)
{
    return ::sd_bus_get_current_message(bus);
}

int SdBus::sd_bus_get_poll_data(sd_bus *bus, PollData* data)
{
    const std::lock_guard lock(sdbusMutex_);

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

int SdBus::sd_bus_get_n_queued(sd_bus *bus, uint64_t *read, uint64_t* write) // NOLINT(bugprone-easily-swappable-parameters)
{
    const std::lock_guard lock(sdbusMutex_);

    auto r1 = ::sd_bus_get_n_queued_read(bus, read);
    auto r2 = ::sd_bus_get_n_queued_write(bus, write);

    return std::min(r1, r2);
}

int SdBus::sd_bus_flush(sd_bus *bus)
{
    return ::sd_bus_flush(bus);
}

sd_bus* SdBus::sd_bus_flush_close_unref(sd_bus *bus)
{
    return ::sd_bus_flush_close_unref(bus);
}

sd_bus* SdBus::sd_bus_close_unref(sd_bus *bus)
{
#if LIBSYSTEMD_VERSION>=241
    return ::sd_bus_close_unref(bus);
#else
    ::sd_bus_close(bus);
    return ::sd_bus_unref(bus);
#endif
}

int SdBus::sd_bus_message_set_destination(sd_bus_message *msg, const char *destination)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_message_set_destination(msg, destination);
}

int SdBus::sd_bus_query_sender_creds(sd_bus_message *msg, uint64_t mask, sd_bus_creds **creds)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_query_sender_creds(msg, mask, creds);
}

sd_bus_creds* SdBus::sd_bus_creds_ref(sd_bus_creds *creds)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_creds_ref(creds);
}

sd_bus_creds* SdBus::sd_bus_creds_unref(sd_bus_creds *creds)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_creds_unref(creds);
}

int SdBus::sd_bus_creds_get_pid(sd_bus_creds *creds, pid_t *pid)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_creds_get_pid(creds, pid);
}

int SdBus::sd_bus_creds_get_uid(sd_bus_creds *creds, uid_t *uid)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_creds_get_uid(creds, uid);
}

int SdBus::sd_bus_creds_get_euid(sd_bus_creds *creds, uid_t *euid)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_creds_get_euid(creds, euid);
}

int SdBus::sd_bus_creds_get_gid(sd_bus_creds *creds, gid_t *gid)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_creds_get_gid(creds, gid);
}

int SdBus::sd_bus_creds_get_egid(sd_bus_creds *creds, uid_t *egid)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_creds_get_egid(creds, egid);
}

int SdBus::sd_bus_creds_get_supplementary_gids(sd_bus_creds *creds, const gid_t **gids)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_creds_get_supplementary_gids(creds, gids);
}

int SdBus::sd_bus_creds_get_selinux_context(sd_bus_creds *creds, const char **label)
{
    const std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_creds_get_selinux_context(creds, label);
}

} // namespace sdbus::internal
