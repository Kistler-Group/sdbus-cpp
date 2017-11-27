/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
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

#include <sdbus-c++/Types.h>
#include <sdbus-c++/Error.h>
#include "MessageUtils.h"
#include <systemd/sd-bus.h>
#include <cassert>

namespace sdbus { /*namespace internal {*/

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

std::string Variant::peekValueType() const
{
    std::string type;
    std::string contents;
    msg_.peekType(type, contents);
    return contents;
}

bool Variant::isEmpty() const
{
    return msg_.isEmpty();
}

}
