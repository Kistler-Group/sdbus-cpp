/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2022 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file ISdBus.h
 * @author Ardazishvili Roman (ardazishvili.roman@yandex.ru)
 *
 * Created on: Mar 12, 2019
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

#ifndef SDBUS_CXX_ISDBUS_H
#define SDBUS_CXX_ISDBUS_H

#include <systemd/sd-bus.h>

namespace sdbus::internal {

    class ISdBus
    {
    public:
        struct PollData
        {
            int fd;
            short int events;
            uint64_t timeout_usec;
        };

        virtual ~ISdBus() = default;

        virtual sd_bus_message* sd_bus_message_ref(sd_bus_message *m) = 0;
        virtual sd_bus_message* sd_bus_message_unref(sd_bus_message *m) = 0;

        virtual int sd_bus_send(sd_bus *bus, sd_bus_message *m, uint64_t *cookie) = 0;
        virtual int sd_bus_call(sd_bus *bus, sd_bus_message *m, uint64_t usec, sd_bus_error *ret_error, sd_bus_message **reply) = 0;
        virtual int sd_bus_call_async(sd_bus *bus, sd_bus_slot **slot, sd_bus_message *m, sd_bus_message_handler_t callback, void *userdata, uint64_t usec) = 0;

        virtual int sd_bus_message_new(sd_bus *bus, sd_bus_message **m, uint8_t type) = 0;
        virtual int sd_bus_message_new_method_call(sd_bus *bus, sd_bus_message **m, const char *destination, const char *path, const char *interface, const char *member) = 0;
        virtual int sd_bus_message_new_signal(sd_bus *bus, sd_bus_message **m, const char *path, const char *interface, const char *member) = 0;
        virtual int sd_bus_message_new_method_return(sd_bus_message *call, sd_bus_message **m) = 0;
        virtual int sd_bus_message_new_method_error(sd_bus_message *call, sd_bus_message **m, const sd_bus_error *e) = 0;

        virtual int sd_bus_set_method_call_timeout(sd_bus *bus, uint64_t usec) = 0;
        virtual int sd_bus_get_method_call_timeout(sd_bus *bus, uint64_t *ret) = 0;

        virtual int sd_bus_emit_properties_changed_strv(sd_bus *bus, const char *path, const char *interface, char **names) = 0;
        virtual int sd_bus_emit_object_added(sd_bus *bus, const char *path) = 0;
        virtual int sd_bus_emit_object_removed(sd_bus *bus, const char *path) = 0;
        virtual int sd_bus_emit_interfaces_added_strv(sd_bus *bus, const char *path, char **interfaces) = 0;
        virtual int sd_bus_emit_interfaces_removed_strv(sd_bus *bus, const char *path, char **interfaces) = 0;

        virtual int sd_bus_open(sd_bus **ret) = 0;
        virtual int sd_bus_open_system(sd_bus **ret) = 0;
        virtual int sd_bus_open_user(sd_bus **ret) = 0;
        virtual int sd_bus_open_user_with_address(sd_bus **ret, const char* address) = 0;
        virtual int sd_bus_open_system_remote(sd_bus **ret, const char* host) = 0;
        virtual int sd_bus_open_direct(sd_bus **ret, const char* address) = 0;
        virtual int sd_bus_open_server(sd_bus **ret, int fd) = 0;
        virtual int sd_bus_request_name(sd_bus *bus, const char *name, uint64_t flags) = 0;
        virtual int sd_bus_release_name(sd_bus *bus, const char *name) = 0;
        virtual int sd_bus_get_unique_name(sd_bus *bus, const char **name) = 0;
        virtual int sd_bus_add_object_vtable(sd_bus *bus, sd_bus_slot **slot, const char *path, const char *interface, const sd_bus_vtable *vtable, void *userdata) = 0;
        virtual int sd_bus_add_object_manager(sd_bus *bus, sd_bus_slot **slot, const char *path) = 0;
        virtual int sd_bus_add_match(sd_bus *bus, sd_bus_slot **slot, const char *match, sd_bus_message_handler_t callback, void *userdata) = 0;
        virtual sd_bus_slot* sd_bus_slot_unref(sd_bus_slot *slot) = 0;

        virtual int sd_bus_new(sd_bus **ret) = 0;
        virtual int sd_bus_start(sd_bus *bus) = 0;

        virtual int sd_bus_process(sd_bus *bus, sd_bus_message **r) = 0;
        virtual int sd_bus_get_poll_data(sd_bus *bus, PollData* data) = 0;

        virtual int sd_bus_flush(sd_bus *bus) = 0;
        virtual sd_bus *sd_bus_flush_close_unref(sd_bus *bus) = 0;
        virtual sd_bus *sd_bus_close_unref(sd_bus *bus) = 0;

        virtual int sd_bus_message_set_destination(sd_bus_message *m, const char *destination) = 0;

        virtual int sd_bus_query_sender_creds(sd_bus_message *m, uint64_t mask, sd_bus_creds **c) = 0;
        virtual sd_bus_creds* sd_bus_creds_unref(sd_bus_creds *c) = 0;

        virtual int sd_bus_creds_get_pid(sd_bus_creds *c, pid_t *pid) = 0;
        virtual int sd_bus_creds_get_uid(sd_bus_creds *c, uid_t *uid) = 0;
        virtual int sd_bus_creds_get_euid(sd_bus_creds *c, uid_t *uid) = 0;
        virtual int sd_bus_creds_get_gid(sd_bus_creds *c, gid_t *gid) = 0;
        virtual int sd_bus_creds_get_egid(sd_bus_creds *c, gid_t *egid) = 0;
        virtual int sd_bus_creds_get_supplementary_gids(sd_bus_creds *c, const gid_t **gids) = 0;
        virtual int sd_bus_creds_get_selinux_context(sd_bus_creds *c, const char **label) = 0;
    };

}

#endif //SDBUS_CXX_ISDBUS_H
