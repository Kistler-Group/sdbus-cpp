/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2026 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file Error.h
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

#include <cerrno>
#include <stdexcept>
#include <string>

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
        // Strong type representing the D-Bus error name
        class Name : public std::string
        {
        public:
            Name() = default;
            explicit Name(std::string value)
                : std::string(std::move(value))
            {}
            explicit Name(const char* value)
                : std::string(value)
            {}

            using std::string::operator=;
        };

        explicit Error(Name name, const char* message = nullptr)
            : Error(std::move(name), std::string(message ? message : ""))
        {
        }

        Error(Name name, std::string message)
            : std::runtime_error(!message.empty() ? "[" + name + "] " + message : "[" + name + "]")
            , name_(std::move(name))
            , message_(std::move(message))
        {
        }

        [[nodiscard]] const Name& getName() const
        {
            return name_;
        }

        [[nodiscard]] const std::string& getMessage() const
        {
            return message_;
        }

        [[nodiscard]] bool isValid() const
        {
            return !getName().empty();
        }

    private:
        Name name_;
        std::string message_;
    };

    Error createError(int errNo, std::string customMsg = {});

    inline const Error::Name SDBUSCPP_ERROR_NAME{"org.sdbuscpp.Error"};
} // namespace sdbus

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SDBUS_THROW_ERROR(_MSG, _ERRNO)                         \
    throw sdbus::createError((_ERRNO), (_MSG))                  \
    /**/
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SDBUS_THROW_ERROR_IF(_COND, _MSG, _ERRNO)               \
    if (!(_COND)) ; else SDBUS_THROW_ERROR((_MSG), (_ERRNO))    \
    /**/

#endif /* SDBUS_CXX_ERROR_H_ */
