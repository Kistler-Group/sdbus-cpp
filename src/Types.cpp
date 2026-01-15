/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2026 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file Types.cpp
 *
 * Created on: Nov 30, 2016
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
#include "sdbus-c++/Message.h"
#include "sdbus-c++/Types.h"

#include <cerrno>
#include <system_error>
#include SDBUS_HEADER
#include <unistd.h>

namespace sdbus {

Variant::Variant()
    : msg_(createPlainMessage())
{
}

void Variant::serializeTo(Message& msg) const
{
    SDBUS_THROW_ERROR_IF(isEmpty(), "Empty variant is not allowed", EINVAL);
    msg_.rewind(true);
    msg_.copyTo(msg, true);
}

void Variant::deserializeFrom(Message& msg)
{
    msg.copyTo(msg_, false);
    msg_.seal();
}

const char* Variant::peekValueType() const
{
    msg_.rewind(false);
    auto [type, contents] = msg_.peekType();
    return contents;
}

bool Variant::isEmpty() const
{
    return msg_.isEmpty();
}

void UnixFd::close() // NOLINT(readability-make-member-function-const)
{
    if (fd_ >= 0)
    {
        ::close(fd_);
    }
}

int UnixFd::checkedDup(int fd)
{
    if (fd < 0)
    {
        return fd;
    }

    const int ret = ::dup(fd); // NOLINT(android-cloexec-dup) // TODO: verify
    if (ret < 0)
    {
        throw std::system_error(errno, std::generic_category(), "dup failed");
    }
    return ret;
}

} // namespace sdbus
