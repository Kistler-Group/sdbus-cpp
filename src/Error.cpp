/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2024 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
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

#include "sdbus-c++/Error.h"

#include "ScopeGuard.h"

#include SDBUS_HEADER

namespace sdbus
{
    Error createError(int errNo, std::string customMsg)
    {
        sd_bus_error sdbusError = SD_BUS_ERROR_NULL;
        sd_bus_error_set_errno(&sdbusError, errNo);
        SCOPE_EXIT{ sd_bus_error_free(&sdbusError); };

        Error::Name name(sd_bus_error_is_set(&sdbusError) ? sdbusError.name : "");
        std::string message(std::move(customMsg));
        if (!message.empty() && sdbusError.message != nullptr)
        {
            message.append(" (");
            message.append(sdbusError.message);
            message.append(")");
        }
        else if (sdbusError.message != nullptr)
        {
            message = sdbusError.message;
        }

        return Error(std::move(name), std::move(message));
    }
} // namespace sdbus
