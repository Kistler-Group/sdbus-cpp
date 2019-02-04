/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 *
 * @file Connection_test.cpp
 * @author Ardazishvili Roman (ardazishvili.roman@yandex.ru)
 *
 * Created on: Feb 4, 2019
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

#include <gtest/gtest.h>

#include "Connection.h"

using BusType = sdbus::internal::Connection::BusType;
auto validService = "org.freedesktop.systemd";
auto invalidService = "some_undefined_name";

TEST(ConnectionCreation, DefaultConstructorMustThrow)
{
  ASSERT_NO_THROW(sdbus::internal::Connection(BusType::eSession));
  ASSERT_NO_THROW(sdbus::internal::Connection(BusType::eSystem));
}

TEST(ConnectionCreation, NameRequestThrow)
{
  auto conn = sdbus::internal::Connection(BusType::eSession);
  ASSERT_THROW(conn.requestName(invalidService), std::runtime_error);
}

TEST(ConnectionCreation, NameRequestNoThrow)
{
  auto conn = sdbus::internal::Connection(BusType::eSession);
  ASSERT_NO_THROW(conn.requestName(validService));
}

TEST(ConnectionCreation, NameReleaseThrow)
{
  auto conn = sdbus::internal::Connection(BusType::eSession);
  // release without request
  ASSERT_THROW(conn.releaseName(validService), std::runtime_error);
}

TEST(ConnectionCreation, NameReleaseNoThrow)
{
  auto conn = sdbus::internal::Connection(BusType::eSession);
  // request and then release
  conn.requestName(validService);
  ASSERT_NO_THROW(conn.releaseName(validService));
}

