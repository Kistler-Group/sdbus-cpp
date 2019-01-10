/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 *
 * @file Flags.cpp
 *
 * Created on: Dec 31, 2018
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

#include <sdbus-c++/Flags.h>
#include <systemd/sd-bus.h>

namespace sdbus
{
    uint64_t Flags::toSdBusInterfaceFlags() const
    {
        uint64_t sdbusFlags{};

        using namespace sdbus;
        if (flags_.test(Flags::DEPRECATED))
            sdbusFlags |= SD_BUS_VTABLE_DEPRECATED;
        if (!flags_.test(Flags::PRIVILEGED))
            sdbusFlags |= SD_BUS_VTABLE_UNPRIVILEGED;

        if (flags_.test(Flags::EMITS_CHANGE_SIGNAL))
            sdbusFlags |= SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE;
        else if (flags_.test(Flags::EMITS_INVALIDATION_SIGNAL))
            sdbusFlags |= SD_BUS_VTABLE_PROPERTY_EMITS_INVALIDATION;
        else if (flags_.test(Flags::CONST_PROPERTY_VALUE))
            sdbusFlags |= SD_BUS_VTABLE_PROPERTY_CONST;
        else if (flags_.test(Flags::EMITS_NO_SIGNAL))
            sdbusFlags |= 0;
        
        return sdbusFlags;
    }

    uint64_t Flags::toSdBusMethodFlags() const
    {
        uint64_t sdbusFlags{};

        using namespace sdbus;
        if (flags_.test(Flags::DEPRECATED))
            sdbusFlags |= SD_BUS_VTABLE_DEPRECATED;
        if (!flags_.test(Flags::PRIVILEGED))
            sdbusFlags |= SD_BUS_VTABLE_UNPRIVILEGED;
        if (flags_.test(Flags::METHOD_NO_REPLY))
            sdbusFlags |= SD_BUS_VTABLE_METHOD_NO_REPLY;

        return sdbusFlags;
    }

    uint64_t Flags::toSdBusSignalFlags() const
    {
        uint64_t sdbusFlags{};

        using namespace sdbus;
        if (flags_.test(Flags::DEPRECATED))
            sdbusFlags |= SD_BUS_VTABLE_DEPRECATED;

        return sdbusFlags;
    }

    uint64_t Flags::toSdBusPropertyFlags() const
    {
        uint64_t sdbusFlags{};

        using namespace sdbus;
        if (flags_.test(Flags::DEPRECATED))
            sdbusFlags |= SD_BUS_VTABLE_DEPRECATED;
        //if (!flags_.test(Flags::PRIVILEGED))
        //    sdbusFlags |= SD_BUS_VTABLE_UNPRIVILEGED;

        if (flags_.test(Flags::EMITS_CHANGE_SIGNAL))
            sdbusFlags |= SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE;
        else if (flags_.test(Flags::EMITS_INVALIDATION_SIGNAL))
            sdbusFlags |= SD_BUS_VTABLE_PROPERTY_EMITS_INVALIDATION;
        else if (flags_.test(Flags::CONST_PROPERTY_VALUE))
            sdbusFlags |= SD_BUS_VTABLE_PROPERTY_CONST;
        else if (flags_.test(Flags::EMITS_NO_SIGNAL))
            sdbusFlags |= 0;

        return sdbusFlags;
    }

    uint64_t Flags::toSdBusWritablePropertyFlags() const
    {
        auto sdbusFlags = toSdBusPropertyFlags();

        using namespace sdbus;
        if (!flags_.test(Flags::PRIVILEGED))
            sdbusFlags |= SD_BUS_VTABLE_UNPRIVILEGED;

        return sdbusFlags;
    }
}
