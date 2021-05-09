/**
 * (C) 2016 - 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2019 Stanislav Angelovic <angelovic.s@gmail.com>
 *
 * @file SdBusMock.h
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

#ifndef SDBUS_CXX_SDBUS_MOCK_H
#define SDBUS_CXX_SDBUS_MOCK_H

#include "ISdBus.h"

#include <gmock/gmock.h>

class SdBusMock : public sdbus::internal::ISdBus
{
public:
    MOCK_METHOD1(sd_bus_message_ref, sd_bus_message*(sd_bus_message *m));
    MOCK_METHOD1(sd_bus_message_unref, sd_bus_message*(sd_bus_message *m));

    MOCK_METHOD3(sd_bus_send, int(sd_bus *bus, sd_bus_message *m, uint64_t *cookie));
    MOCK_METHOD5(sd_bus_call, int(sd_bus *bus, sd_bus_message *m, uint64_t usec, sd_bus_error *ret_error, sd_bus_message **reply));
    MOCK_METHOD6(sd_bus_call_async, int(sd_bus *bus, sd_bus_slot **slot, sd_bus_message *m, sd_bus_message_handler_t callback, void *userdata, uint64_t usec));

    MOCK_METHOD3(sd_bus_message_new, int(sd_bus *bus, sd_bus_message **m, uint8_t type));
    MOCK_METHOD6(sd_bus_message_new_method_call, int(sd_bus *bus, sd_bus_message **m, const char *destination, const char *path, const char *interface, const char *member));
    MOCK_METHOD5(sd_bus_message_new_signal, int(sd_bus *bus, sd_bus_message **m, const char *path, const char *interface, const char *member));
    MOCK_METHOD2(sd_bus_message_new_method_return, int(sd_bus_message *call, sd_bus_message **m));
    MOCK_METHOD3(sd_bus_message_new_method_error, int(sd_bus_message *call, sd_bus_message **m, const sd_bus_error *e));

    MOCK_METHOD2(sd_bus_set_method_call_timeout, int(sd_bus *bus, uint64_t usec));
    MOCK_METHOD2(sd_bus_get_method_call_timeout, int(sd_bus *bus, uint64_t *ret));

    MOCK_METHOD4(sd_bus_emit_properties_changed_strv, int(sd_bus *bus, const char *path, const char *interface, char **names));
    MOCK_METHOD2(sd_bus_emit_object_added, int(sd_bus *bus, const char *path));
    MOCK_METHOD2(sd_bus_emit_object_removed, int(sd_bus *bus, const char *path));
    MOCK_METHOD3(sd_bus_emit_interfaces_added_strv, int(sd_bus *bus, const char *path, char **interfaces));
    MOCK_METHOD3(sd_bus_emit_interfaces_removed_strv, int(sd_bus *bus, const char *path, char **interfaces));

    MOCK_METHOD1(sd_bus_open, int(sd_bus **ret));
    MOCK_METHOD1(sd_bus_open_user, int(sd_bus **ret));
    MOCK_METHOD1(sd_bus_open_system, int(sd_bus **ret));
    MOCK_METHOD2(sd_bus_open_system_remote, int(sd_bus **ret, const char *host));
    MOCK_METHOD3(sd_bus_request_name, int(sd_bus *bus, const char *name, uint64_t flags));
    MOCK_METHOD2(sd_bus_release_name, int(sd_bus *bus, const char *name));
    MOCK_METHOD2(sd_bus_get_unique_name, int(sd_bus *bus, const char **name));
    MOCK_METHOD6(sd_bus_add_object_vtable, int(sd_bus *bus, sd_bus_slot **slot, const char *path, const char *interface, const sd_bus_vtable *vtable, void *userdata));
    MOCK_METHOD3(sd_bus_add_object_manager, int(sd_bus *bus, sd_bus_slot **slot, const char *path));
    MOCK_METHOD5(sd_bus_add_match, int(sd_bus *bus, sd_bus_slot **slot, const char *match, sd_bus_message_handler_t callback, void *userdata));
    MOCK_METHOD1(sd_bus_slot_unref, sd_bus_slot*(sd_bus_slot *slot));

    MOCK_METHOD2(sd_bus_process, int(sd_bus *bus, sd_bus_message **r));
    MOCK_METHOD2(sd_bus_get_poll_data, int(sd_bus *bus, PollData* data));

    MOCK_METHOD1(sd_bus_flush, int(sd_bus *bus));
    MOCK_METHOD1(sd_bus_flush_close_unref, sd_bus *(sd_bus *bus));

    MOCK_METHOD2(sd_bus_message_set_destination, int(sd_bus_message *m, const char *destination));

    MOCK_METHOD3(sd_bus_query_sender_creds, int(sd_bus_message *, uint64_t, sd_bus_creds **));
    MOCK_METHOD1(sd_bus_creds_unref, sd_bus_creds*(sd_bus_creds *));

    MOCK_METHOD2(sd_bus_creds_get_pid, int(sd_bus_creds *, pid_t *));
    MOCK_METHOD2(sd_bus_creds_get_uid, int(sd_bus_creds *, uid_t *));
    MOCK_METHOD2(sd_bus_creds_get_euid, int(sd_bus_creds *, uid_t *));
    MOCK_METHOD2(sd_bus_creds_get_gid, int(sd_bus_creds *, gid_t *));
    MOCK_METHOD2(sd_bus_creds_get_egid, int(sd_bus_creds *, gid_t *));
    MOCK_METHOD2(sd_bus_creds_get_supplementary_gids, int(sd_bus_creds *, const gid_t **));
    MOCK_METHOD2(sd_bus_creds_get_selinux_context, int(sd_bus_creds *, const char **));
};

#endif //SDBUS_CXX_SDBUS_MOCK_H
