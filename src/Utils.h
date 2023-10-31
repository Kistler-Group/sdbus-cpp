/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2022 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file Utils.h
 *
 * Created on: Feb 9, 2022
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

#ifndef SDBUS_CXX_INTERNAL_UTILS_H_
#define SDBUS_CXX_INTERNAL_UTILS_H_

#include <sdbus-c++/Error.h>
#include SDBUS_HEADER

#if LIBSYSTEMD_VERSION>=246
#define SDBUS_CHECK_OBJECT_PATH(_PATH)                                                                                                            \
    SDBUS_THROW_ERROR_IF(!sd_bus_object_path_is_valid(_PATH.c_str()), "Invalid object path '" + _PATH + "' provided", EINVAL)                     \
    /**/
#define SDBUS_CHECK_INTERFACE_NAME(_NAME)                                                                                                         \
    SDBUS_THROW_ERROR_IF(!sd_bus_interface_name_is_valid(_NAME.c_str()), "Invalid interface name '" + _NAME + "' provided", EINVAL)               \
    /**/
#define SDBUS_CHECK_SERVICE_NAME(_NAME)                                                                                                           \
    SDBUS_THROW_ERROR_IF(!_NAME.empty() && !sd_bus_service_name_is_valid(_NAME.c_str()), "Invalid service name '" + _NAME + "' provided", EINVAL) \
    /**/
#define SDBUS_CHECK_MEMBER_NAME(_NAME)                                                                                                            \
    SDBUS_THROW_ERROR_IF(!sd_bus_member_name_is_valid(_NAME.c_str()), "Invalid member name '" + _NAME + "' provided", EINVAL)                     \
    /**/
#else
#define SDBUS_CHECK_OBJECT_PATH(_PATH)
#define SDBUS_CHECK_INTERFACE_NAME(_NAME)
#define SDBUS_CHECK_SERVICE_NAME(_NAME)
#define SDBUS_CHECK_MEMBER_NAME(_NAME)
#endif

namespace sdbus::internal {

    template <typename _Callable>
    bool invokeHandlerAndCatchErrors(_Callable callable, sd_bus_error *retError)
    {
        try
        {
            callable();
        }
        catch (const Error& e)
        {
            sd_bus_error_set(retError, e.getName().c_str(), e.getMessage().c_str());
            return false;
        }
        catch (const std::exception& e)
        {
            sd_bus_error_set(retError, SDBUSCPP_ERROR_NAME, e.what());
            return false;
        }
        catch (...)
        {
            sd_bus_error_set(retError, SDBUSCPP_ERROR_NAME, "Unknown error occurred");
            return false;
        }

        return true;
    }

}

#endif /* SDBUS_CXX_INTERNAL_UTILS_H_ */
