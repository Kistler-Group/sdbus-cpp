/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2026 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file MessageUtils.h
 *
 * Created on: Dec 5, 2016
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

#ifndef SDBUS_CXX_INTERNAL_MESSAGEUTILS_H_
#define SDBUS_CXX_INTERNAL_MESSAGEUTILS_H_

#include <sdbus-c++/Message.h>

namespace sdbus
{
    class Message::Factory
    {
    public:
        template<typename Msg>
        static Msg create()
        {
            return Msg{};
        }

        template<typename Msg>
        static Msg create(void *msg)
        {
            return Msg{msg};
        }

        template<typename Msg>
        static Msg create(void *msg, internal::IConnection* connection)
        {
            return Msg{msg, connection};
        }

        template<typename Msg>
        static Msg create(void *msg, internal::IConnection* connection, adopt_message_t)
        {
            return Msg{msg, connection, adopt_message};
        }
    };
} // namespace sdbus

#endif /* SDBUS_CXX_INTERNAL_MESSAGEUTILS_H_ */
