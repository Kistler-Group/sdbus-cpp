/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 *
 * @file sdbus-c++.h
 *
 * Created on: Jan 19, 2017
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

#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/IObject.h>
#include <sdbus-c++/IProxy.h>
#include <sdbus-c++/AdaptorInterfaces.h>
#include <sdbus-c++/ProxyInterfaces.h>
#include <sdbus-c++/Message.h>
#include <sdbus-c++/MethodResult.h>
#include <sdbus-c++/Types.h>
#include <sdbus-c++/TypeTraits.h>
#include <sdbus-c++/Introspection.h>
#include <sdbus-c++/Error.h>
