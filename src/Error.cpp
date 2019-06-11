/**
 * (C) 2016 - 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2019 Stanislav Angelovic <angelovic.s@gmail.com>
 *
 * @file Error.cpp
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

#include <sdbus-c++/Error.h>
#include <systemd/sd-bus.h>
#include "ScopeGuard.h"

namespace sdbus
{
    sdbus::Error createError(int errNo, const std::string& customMsg)
    {
        sd_bus_error sdbusError = SD_BUS_ERROR_NULL;
        sd_bus_error_set_errno(&sdbusError, errNo);
        SCOPE_EXIT{ sd_bus_error_free(&sdbusError); };

        std::string name(sdbusError.name);
        std::string message(customMsg + " (" + sdbusError.message + ")");
        return sdbus::Error(name, message);
    }
}
