/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2022 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file TestFixture.cpp
 *
 * Created on: May 23, 2020
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

#include "TestFixture.h"

namespace sdbus { namespace test {

std::unique_ptr<sdbus::IConnection> BaseTestFixture::s_adaptorConnection = sdbus::createSystemBusConnection();
std::unique_ptr<sdbus::IConnection> BaseTestFixture::s_proxyConnection = sdbus::createSystemBusConnection();

#ifndef SDBUS_basu // sd_event integration is not supported in basu-based sdbus-c++

std::thread TestFixture<SdEventLoop>::s_adaptorEventLoopThread{};
std::thread TestFixture<SdEventLoop>::s_proxyEventLoopThread{};
sd_event *TestFixture<SdEventLoop>::s_adaptorSdEvent{};
sd_event *TestFixture<SdEventLoop>::s_proxySdEvent{};
int TestFixture<SdEventLoop>::s_eventExitFd{-1};

#endif // SDBUS_basu

}}
