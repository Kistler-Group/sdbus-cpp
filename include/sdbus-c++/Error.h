/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 *
 * @file ConvenienceClasses.h
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

#ifndef SDBUS_CXX_ERROR_H_
#define SDBUS_CXX_ERROR_H_

#include <stdexcept>

namespace sdbus {

    /********************************************//**
     * @class Error
     *
     * Represents a common sdbus-c++ exception.
     *
     ***********************************************/
    class Error
        : public std::runtime_error
    {
    public:
        Error(const std::string& name, const std::string& message)
            : std::runtime_error("[" + name + "] " + message)
            , name_(name)
            , message_(message)
        {
        }

        const std::string& getName() const
        {
            return name_;
        }

        const std::string& getMessage() const
        {
            return message_;
        }

    private:
        std::string name_;
        std::string message_;
    };

    sdbus::Error createError(int errNo, const std::string& customMsg);
}

#define SDBUS_THROW_ERROR(_MSG, _ERRNO)            \
    throw sdbus::createError((_ERRNO), (_MSG))     \
    /**/

#define SDBUS_THROW_ERROR_IF(_COND, _MSG, _ERRNO)  \
    if (_COND) SDBUS_THROW_ERROR((_MSG), (_ERRNO)) \
    /**/

#endif /* SDBUS_CXX_ERROR_H_ */
