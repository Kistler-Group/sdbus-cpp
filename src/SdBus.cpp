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
#include <unistd.h>
#include <sys/eventfd.h>

namespace sdbus::internal {

sd_bus_message* SdBus::sd_bus_message_ref(sd_bus_message *m)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_message_ref(m);
}

sd_bus_message* SdBus::sd_bus_message_unref(sd_bus_message *m)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_message_unref(m);
}

int SdBus::sd_bus_send(sd_bus *bus, sd_bus_message *m, uint64_t *cookie)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_send(bus, m, cookie);
}

int SdBus::sd_bus_call(sd_bus *bus, sd_bus_message *m, uint64_t usec, sd_bus_error *ret_error, sd_bus_message **reply)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_call(bus, m, usec, ret_error, reply);
}

int SdBus::sd_bus_call_async(sd_bus *bus, sd_bus_slot **slot, sd_bus_message *m, sd_bus_message_handler_t callback, void *userdata, uint64_t usec)
{
    std::lock_guard lock(sdbusMutex_);

    if (usec == 0 || usec == UINT64_MAX) {
        return ::sd_bus_call_async(bus, slot, m, callback, userdata, usec);
    }

    if (!bus) {
        bus = ::sd_bus_message_get_bus(m);
    }

    auto prevTimeout = uint64_t(0);
    ::sd_bus_get_timeout(bus, &prevTimeout);

    auto r = ::sd_bus_call_async(bus, slot, m, callback, userdata, usec);
    if (r < 0) {
        return r;
    }

    auto currentTimeout = UINT64_MAX;
    ::sd_bus_get_timeout(bus, &currentTimeout);

    if (currentTimeout < prevTimeout) {
        notifyEventFdLocked(bus);
    }

    return r;
}

int SdBus::sd_bus_message_new(sd_bus *bus, sd_bus_message **m, uint8_t type)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_message_new(bus, m, type);
}

int SdBus::sd_bus_message_new_method_call(sd_bus *bus, sd_bus_message **m, const char *destination, const char *path, const char *interface, const char *member)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_message_new_method_call(bus, m, destination, path, interface, member);
}

int SdBus::sd_bus_message_new_signal(sd_bus *bus, sd_bus_message **m, const char *path, const char *interface, const char *member)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_message_new_signal(bus, m, path, interface, member);
}

int SdBus::sd_bus_message_new_method_return(sd_bus_message *call, sd_bus_message **m)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_message_new_method_return(call, m);
}

int SdBus::sd_bus_message_new_method_error(sd_bus_message *call, sd_bus_message **m, const sd_bus_error *e)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_message_new_method_error(call, m, e);
}

int SdBus::sd_bus_set_method_call_timeout(sd_bus *bus, uint64_t usec)
{
#if LIBSYSTEMD_VERSION>=240
    std::lock_guard lock(sdbusMutex_);

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
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_get_method_call_timeout(bus, ret);
#else
    (void)bus;
    (void)ret;
    throw sdbus::Error(SD_BUS_ERROR_NOT_SUPPORTED, "Getting general method call timeout not supported by underlying version of libsystemd");
#endif
}

int SdBus::sd_bus_emit_properties_changed_strv(sd_bus *bus, const char *path, const char *interface, char **names)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_emit_properties_changed_strv(bus, path, interface, names);
}

int SdBus::sd_bus_emit_object_added(sd_bus *bus, const char *path)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_emit_object_added(bus, path);
}

int SdBus::sd_bus_emit_object_removed(sd_bus *bus, const char *path)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_emit_object_removed(bus, path);
}

int SdBus::sd_bus_emit_interfaces_added_strv(sd_bus *bus, const char *path, char **interfaces)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_emit_interfaces_added_strv(bus, path, interfaces);
}

int SdBus::sd_bus_emit_interfaces_removed_strv(sd_bus *bus, const char *path, char **interfaces)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_emit_interfaces_removed_strv(bus, path, interfaces);
}

int SdBus::sd_bus_open(sd_bus **ret)
{
    auto r = ::sd_bus_open(ret);
    if (r < 0) {
        return r;
    }
    allocEventFd(*ret);
    return r;
}

void SdBus::dropEventFd(sd_bus *bus) {
    std::unique_lock lock(sdbusMutex_);
    auto node = eventFds_.extract(bus);
    lock.unlock();
    if (!node.empty()) {
        close(node.mapped());
    }
}

void SdBus::allocEventFd(sd_bus *bus)
{
    auto fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    std::lock_guard lock(sdbusMutex_);
    if (fd >= 0 && !eventFds_.emplace(bus, fd).second) {
        close(fd);
    }
}

void SdBus::notifyEventFdLocked(sd_bus *bus)
{
    if (auto it = eventFds_.find(bus); it != eventFds_.end())
    {
        uint64_t value = 1;
        const auto cnt = write(it->second, &value, sizeof(value));
        (void) cnt; // TODO: notifyEventFdLocked: Process write failure
    }
}

int SdBus::sd_bus_open_user(sd_bus **ret)
{
    auto r = ::sd_bus_open_user(ret);
    if (r < 0) {
        return r;
    }
    allocEventFd(*ret);
    return r;
}

int SdBus::sd_bus_open_system(sd_bus **ret)
{
    auto r = ::sd_bus_open_system(ret);
    if (r < 0) {
        return r;
    }
    allocEventFd(*ret);
    return r;
}

int SdBus::sd_bus_open_system_remote(sd_bus **ret, const char *host)
{
    auto r = ::sd_bus_open_system_remote(ret, host);
    if (r < 0) {
        return r;
    }
    allocEventFd(*ret);
    return r;
}

int SdBus::sd_bus_request_name(sd_bus *bus, const char *name, uint64_t flags)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_request_name(bus, name, flags);
}

int SdBus::sd_bus_release_name(sd_bus *bus, const char *name)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_release_name(bus, name);
}

int SdBus::sd_bus_get_unique_name(sd_bus *bus, const char **name)
{
    std::lock_guard lock(sdbusMutex_);
    return ::sd_bus_get_unique_name(bus, name);
}

int SdBus::sd_bus_add_object_vtable(sd_bus *bus, sd_bus_slot **slot, const char *path, const char *interface, const sd_bus_vtable *vtable, void *userdata)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_add_object_vtable(bus, slot, path, interface,  vtable, userdata);
}

int SdBus::sd_bus_add_object_manager(sd_bus *bus, sd_bus_slot **slot, const char *path)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_add_object_manager(bus, slot, path);
}

int SdBus::sd_bus_add_match(sd_bus *bus, sd_bus_slot **slot, const char *match, sd_bus_message_handler_t callback, void *userdata)
{
    std::lock_guard lock(sdbusMutex_);

    return :: sd_bus_add_match(bus, slot, match, callback, userdata);
}

sd_bus_slot* SdBus::sd_bus_slot_unref(sd_bus_slot *slot)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_slot_unref(slot);
}

int SdBus::sd_bus_process(sd_bus *bus, sd_bus_message **r)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_process(bus, r);
}

int SdBus::sd_bus_get_poll_data(sd_bus *bus, PollData* data)
{
    std::lock_guard lock(sdbusMutex_);
    auto r = ::sd_bus_get_fd(bus);
    if (r < 0)
        return r;
    data->fd = r;
    auto it = eventFds_.find(bus);
    data->eventFd = it != eventFds_.end() ? it->second : -1;

    r = ::sd_bus_get_events(bus);
    if (r < 0)
        return r;
    data->events = static_cast<short int>(r);

    r = ::sd_bus_get_timeout(bus, &data->timeout_usec);
    if (r < 0)
        return r;

    // Despite what is documented, the timeout returned by ::sd_bus_get_timeout is an absolute time of linux's CLOCK_MONOTONIC clock.
    // Values 0 and uint64_t(-1) are used as sentinels meaning "non-blocking poll" and "no-timeout", respectively.
    if (data->timeout_usec != UINT64_MAX && data->timeout_usec != 0)
    {
        struct timespec ts{};
        r = clock_gettime(CLOCK_MONOTONIC, &ts);
        if (r < 0) {
            return r;
        }

        const auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::seconds(ts.tv_sec) + std::chrono::nanoseconds(ts.tv_nsec));
        const auto now_us = static_cast<uint64_t>(now.count());
        const auto to_us = data->timeout_usec > now_us ? data->timeout_usec - now_us : 0;

        data->timeout_usec = to_us;
    }

    return r;
}

int SdBus::sd_bus_flush(sd_bus *bus)
{
    return ::sd_bus_flush(bus);
}

sd_bus* SdBus::sd_bus_flush_close_unref(sd_bus *bus)
{
    dropEventFd(bus);
    return ::sd_bus_flush_close_unref(bus);
}

int SdBus::sd_bus_message_set_destination(sd_bus_message *m, const char *destination)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_message_set_destination(m, destination);
}

int SdBus::sd_bus_query_sender_creds(sd_bus_message *m, uint64_t mask, sd_bus_creds **c)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_query_sender_creds(m, mask, c);
}

sd_bus_creds* SdBus::sd_bus_creds_unref(sd_bus_creds *c)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_creds_unref(c);
}

int SdBus::sd_bus_creds_get_pid(sd_bus_creds *c, pid_t *pid)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_creds_get_pid(c, pid);
}

int SdBus::sd_bus_creds_get_uid(sd_bus_creds *c, uid_t *uid)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_creds_get_uid(c, uid);
}

int SdBus::sd_bus_creds_get_euid(sd_bus_creds *c, uid_t *euid)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_creds_get_euid(c, euid);
}

int SdBus::sd_bus_creds_get_gid(sd_bus_creds *c, gid_t *gid)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_creds_get_gid(c, gid);
}

int SdBus::sd_bus_creds_get_egid(sd_bus_creds *c, uid_t *egid)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_creds_get_egid(c, egid);
}

int SdBus::sd_bus_creds_get_supplementary_gids(sd_bus_creds *c, const gid_t **gids)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_creds_get_supplementary_gids(c, gids);
}

int SdBus::sd_bus_creds_get_selinux_context(sd_bus_creds *c, const char **label)
{
    std::lock_guard lock(sdbusMutex_);

    return ::sd_bus_creds_get_selinux_context(c, label);
}

}
