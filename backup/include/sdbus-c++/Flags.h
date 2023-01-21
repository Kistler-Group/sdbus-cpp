/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2022 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file Flags.h
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

#ifndef SDBUS_CXX_FLAGS_H_
#define SDBUS_CXX_FLAGS_H_

#include <bitset>
#include <cstdint>

namespace sdbus {

    // D-Bus interface, method, signal or property flags
    class Flags
    {
    public:
        enum GeneralFlags : uint8_t
        {    DEPRECATED = 0
        ,    METHOD_NO_REPLY = 1
        ,    PRIVILEGED = 2
        };

        enum PropertyUpdateBehaviorFlags : uint8_t
        {   EMITS_CHANGE_SIGNAL = 3
        ,   EMITS_INVALIDATION_SIGNAL = 4
        ,   EMITS_NO_SIGNAL = 5
        ,   CONST_PROPERTY_VALUE = 6
        };

        enum : uint8_t
        {   FLAG_COUNT = 7
        };

        Flags()
        {
            // EMITS_CHANGE_SIGNAL is on by default
            flags_.set(EMITS_CHANGE_SIGNAL, true);
        }

        void set(GeneralFlags flag, bool value = true)
        {
            flags_.set(flag, value);
        }

        void set(PropertyUpdateBehaviorFlags flag, bool value = true)
        {
            flags_.set(EMITS_CHANGE_SIGNAL, false);
            flags_.set(EMITS_INVALIDATION_SIGNAL, false);
            flags_.set(EMITS_NO_SIGNAL, false);
            flags_.set(CONST_PROPERTY_VALUE, false);

            flags_.set(flag, value);
        }

        bool test(GeneralFlags flag) const
        {
            return flags_.test(flag);
        }

        bool test(PropertyUpdateBehaviorFlags flag) const
        {
            return flags_.test(flag);
        }

        uint64_t toSdBusInterfaceFlags() const;
        uint64_t toSdBusMethodFlags() const;
        uint64_t toSdBusSignalFlags() const;
        uint64_t toSdBusPropertyFlags() const;
        uint64_t toSdBusWritablePropertyFlags() const;

    private:
        std::bitset<FLAG_COUNT> flags_;
    };
    
}

#endif /* SDBUS_CXX_FLAGS_H_ */
