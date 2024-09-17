/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2024 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file SdBus.h
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

#ifndef SDBUS_CXX_SDBUS_H
#define SDBUS_CXX_SDBUS_H

#include "ISdBus.h"
#include <mutex>

namespace sdbus::internal {

class SdBus final : public ISdBus
{
public:
    virtual sd_bus_message* sd_bus_message_ref(sd_bus_message *m) override;
    virtual sd_bus_message* sd_bus_message_unref(sd_bus_message *m) override;

    virtual int sd_bus_send(sd_bus *bus, sd_bus_message *m, uint64_t *cookie) override;
    virtual int sd_bus_call(sd_bus *bus, sd_bus_message *m, uint64_t usec, sd_bus_error *ret_error, sd_bus_message **reply) override;
    virtual int sd_bus_call_async(sd_bus *bus, sd_bus_slot **slot, sd_bus_message *m, sd_bus_message_handler_t callback, void *userdata, uint64_t usec) override;

    virtual int sd_bus_message_new(sd_bus *bus, sd_bus_message **m, uint8_t type) override;
    virtual int sd_bus_message_new_method_call(sd_bus *bus, sd_bus_message **m, const char *destination, const char *path, const char *interface, const char *member) override;
    virtual int sd_bus_message_new_signal(sd_bus *bus, sd_bus_message **m, const char *path, const char *interface, const char *member) override;
    virtual int sd_bus_message_new_method_return(sd_bus_message *call, sd_bus_message **m) override;
    virtual int sd_bus_message_new_method_error(sd_bus_message *call, sd_bus_message **m, const sd_bus_error *e) override;

    virtual int sd_bus_set_method_call_timeout(sd_bus *bus, uint64_t usec) override;
    virtual int sd_bus_get_method_call_timeout(sd_bus *bus, uint64_t *ret) override;

    virtual int sd_bus_emit_properties_changed_strv(sd_bus *bus, const char *path, const char *interface, char **names) override;
    virtual int sd_bus_emit_object_added(sd_bus *bus, const char *path) override;
    virtual int sd_bus_emit_object_removed(sd_bus *bus, const char *path) override;
    virtual int sd_bus_emit_interfaces_added_strv(sd_bus *bus, const char *path, char **interfaces) override;
    virtual int sd_bus_emit_interfaces_removed_strv(sd_bus *bus, const char *path, char **interfaces) override;

    virtual int sd_bus_open(sd_bus **ret) override;
    virtual int sd_bus_open_system(sd_bus **ret) override;
    virtual int sd_bus_open_user(sd_bus **ret) override;
    virtual int sd_bus_open_user_with_address(sd_bus **ret, const char* address) override;
    virtual int sd_bus_open_system_remote(sd_bus **ret, const char* hsot) override;
    virtual int sd_bus_open_direct(sd_bus **ret, const char* address) override;
    virtual int sd_bus_open_direct(sd_bus **ret, int fd) override;
    virtual int sd_bus_open_server(sd_bus **ret, int fd) override;
    virtual int sd_bus_request_name(sd_bus *bus, const char *name, uint64_t flags) override;
    virtual int sd_bus_release_name(sd_bus *bus, const char *name) override;
    virtual int sd_bus_get_unique_name(sd_bus *bus, const char **name) override;
    virtual int sd_bus_add_object_vtable(sd_bus *bus, sd_bus_slot **slot, const char *path, const char *interface, const sd_bus_vtable *vtable, void *userdata) override;
    virtual int sd_bus_add_object_manager(sd_bus *bus, sd_bus_slot **slot, const char *path) override;
    virtual int sd_bus_add_match(sd_bus *bus, sd_bus_slot **slot, const char *match, sd_bus_message_handler_t callback, void *userdata) override;
    virtual int sd_bus_add_match_async(sd_bus *bus, sd_bus_slot **slot, const char *match, sd_bus_message_handler_t callback, sd_bus_message_handler_t install_callback, void *userdata) override;
    virtual int sd_bus_match_signal(sd_bus *bus, sd_bus_slot **ret, const char *sender, const char *path, const char *interface, const char *member, sd_bus_message_handler_t callback, void *userdata) override;
    virtual sd_bus_slot* sd_bus_slot_unref(sd_bus_slot *slot) override;

    virtual int sd_bus_new(sd_bus **ret) override;
    virtual int sd_bus_start(sd_bus *bus) override;

    virtual int sd_bus_process(sd_bus *bus, sd_bus_message **r) override;
    virtual sd_bus_message* sd_bus_get_current_message(sd_bus *bus) override;
    virtual int sd_bus_get_poll_data(sd_bus *bus, PollData* data) override;
    virtual int sd_bus_get_n_queued(sd_bus *bus, uint64_t *read, uint64_t* write) override;
    virtual int sd_bus_flush(sd_bus *bus) override;
    virtual sd_bus *sd_bus_flush_close_unref(sd_bus *bus) override;
    virtual sd_bus *sd_bus_close_unref(sd_bus *bus) override;

    virtual int sd_bus_message_set_destination(sd_bus_message *m, const char *destination) override;

    virtual int sd_bus_query_sender_creds(sd_bus_message *m, uint64_t mask, sd_bus_creds **c) override;
    virtual sd_bus_creds* sd_bus_creds_ref(sd_bus_creds *c) override;
    virtual sd_bus_creds* sd_bus_creds_unref(sd_bus_creds *c) override;

    virtual int sd_bus_creds_get_pid(sd_bus_creds *c, pid_t *pid) override;
    virtual int sd_bus_creds_get_uid(sd_bus_creds *c, uid_t *uid) override;
    virtual int sd_bus_creds_get_euid(sd_bus_creds *c, uid_t *euid) override;
    virtual int sd_bus_creds_get_gid(sd_bus_creds *c, gid_t *gid) override;
    virtual int sd_bus_creds_get_egid(sd_bus_creds *c, gid_t *egid) override;
    virtual int sd_bus_creds_get_supplementary_gids(sd_bus_creds *c, const gid_t **gids) override;
    virtual int sd_bus_creds_get_selinux_context(sd_bus_creds *c, const char **label) override;

private:
    std::recursive_mutex sdbusMutex_;
};

}

#endif //SDBUS_C_SDBUS_H
