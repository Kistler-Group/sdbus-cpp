/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2022 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file Defs.h
 *
 * Created on: Jan 2, 2017
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

#ifndef SDBUS_CPP_INTEGRATIONTESTS_DEFS_H_
#define SDBUS_CPP_INTEGRATIONTESTS_DEFS_H_

#include "sdbus-c++/Types.h"
#include <chrono>
#include <ostream>

namespace sdbus { namespace test {

const std::string INTERFACE_NAME{"org.sdbuscpp.integrationtests"};
const std::string BUS_NAME = INTERFACE_NAME;
const std::string MANAGER_PATH {"/org/sdbuscpp/integrationtests"};
const std::string OBJECT_PATH  {"/org/sdbuscpp/integrationtests/ObjectA1"};
const std::string OBJECT_PATH_2{"/org/sdbuscpp/integrationtests/ObjectB1"};

constexpr const uint8_t UINT8_VALUE{1};
constexpr const int16_t INT16_VALUE{21};
constexpr const uint32_t UINT32_VALUE{42};
constexpr const int32_t INT32_VALUE{-42};
constexpr const int32_t INT64_VALUE{-1024};

const std::string STRING_VALUE{"sdbus-c++-testing"};
const sdbus::Signature SIGNATURE_VALUE{"a{is}"};
const sdbus::ObjectPath OBJECT_PATH_VALUE{"/"};
const int UNIX_FD_VALUE = 0;

const std::string DEFAULT_STATE_VALUE{"default-state-value"};
const uint32_t DEFAULT_ACTION_VALUE{999};
const bool DEFAULT_BLOCKING_VALUE{true};

constexpr const double DOUBLE_VALUE{3.24L};

}}

namespace testing::internal {

// Printer for std::chrono::duration types.
// This is a workaround, since it's not a good thing to add this to std namespace.
template< class Rep, class Period >
void PrintTo(const ::std::chrono::duration<Rep, Period>& d, ::std::ostream* os) {
    auto seconds = std::chrono::duration_cast<std::chrono::duration<double>>(d);
    *os << seconds.count() << "s";
}

}

#endif /* SDBUS_CPP_INTEGRATIONTESTS_DEFS_H_ */
