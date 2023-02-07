/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2022 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
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
 * under the terms of the GNU Lesser General Public License as published byyyy
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
        template<typename _Msg>
        static _Msg create()
        {
            return _Msg{};
        }

        template<typename _Msg>
        static _Msg create(void *msg)
        {
            return _Msg{msg};
        }

        template<typename _Msg>
        static _Msg create(void *msg, internal::ISdBus* sdbus)
        {
            return _Msg{msg, sdbus};
        }

        template<typename _Msg>
        static _Msg create(void *msg, internal::ISdBus* sdbus, adopt_message_t)
        {
            return _Msg{msg, sdbus, adopt_message};
        }

        template<typename _Msg>
        static _Msg create(void *msg, internal::ISdBus* sdbus, const internal::IConnection* connection, adopt_message_t)
        {
            return _Msg{msg, sdbus, connection, adopt_message};
        }
    };

    PlainMessage createPlainMessage();
}

#endif /* SDBUS_CXX_INTERNAL_MESSAGEUTILS_H_ */
