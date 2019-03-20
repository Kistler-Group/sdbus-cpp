/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
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

namespace sdbus { namespace internal {

class SdBus : public ISdBus
{
public:
    int sd_bus_request_name(sd_bus *bus, const char *name, uint64_t flags) override;
    int sd_bus_release_name(sd_bus *bus, const char *name) override;
    int sd_bus_add_object_vtable(sd_bus *bus, sd_bus_slot **slot, const char *path, const char *interface, const sd_bus_vtable *vtable, void *userdata) override;
    sd_bus_slot* sd_bus_slot_unref(sd_bus_slot *slot) override;
    int sd_bus_message_new_method_call(sd_bus *bus, sd_bus_message **m, const char *destination, const char *path, const char *interface, const char *member) override;
    sd_bus_message* sd_bus_message_ref(sd_bus_message *m) override;
    sd_bus_message* sd_bus_message_unref(sd_bus_message *m) override;
    int sd_bus_message_new_signal(sd_bus *bus, sd_bus_message **m, const char *path, const char *interface, const char *member) override;
    int sd_bus_add_match(sd_bus *bus, sd_bus_slot **slot, const char *match, sd_bus_message_handler_t callback, void *userdata) override;
    int sd_bus_open_user(sd_bus **ret) override;
    int sd_bus_open_system(sd_bus **ret) override;
    int sd_bus_flush(sd_bus *bus) override;
    int sd_bus_process(sd_bus *bus, sd_bus_message **r) override;
    int sd_bus_get_fd(sd_bus *bus) override;
    int sd_bus_get_events(sd_bus *bus) override;
    int sd_bus_get_timeout(sd_bus *bus, uint64_t *timeout_usec) override;
    sd_bus *sd_bus_flush_close_unref(sd_bus *bus) override;

private:
    std::recursive_mutex sdbusMutex_;
};

}}

#endif //SDBUS_C_SDBUS_H
