/**
 * (C) 2016 - 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2020 Stanislav Angelovic <angelovic.s@gmail.com>
 *
 * @file DBusPropertiesTests.cpp
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

#include "TestFixture.h"
#include "TestAdaptor.h"
#include "TestProxy.h"
#include "sdbus-c++/sdbus-c++.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <thread>
#include <tuple>
#include <chrono>
#include <fstream>
#include <future>
#include <unistd.h>

using ::testing::Eq;
using ::testing::DoubleEq;
using ::testing::Gt;
using ::testing::AnyOf;
using ::testing::ElementsAre;
using ::testing::SizeIs;
using ::testing::NotNull;
using ::testing::Not;
using ::testing::IsEmpty;
using namespace std::chrono_literals;
using namespace sdbus::test;

using SdbusTestObject = TestFixture;

/*-------------------------------------*/
/* --          TEST CASES           -- */
/*-------------------------------------*/

TEST_F(SdbusTestObject, ReadsReadOnlyPropertySuccesfully)
{
    ASSERT_THAT(m_proxy->state(), Eq(DEFAULT_STATE_VALUE));
}

TEST_F(SdbusTestObject, FailsWritingToReadOnlyProperty)
{
    ASSERT_THROW(m_proxy->setStateProperty("new_value"), sdbus::Error);
}

TEST_F(SdbusTestObject, WritesAndReadsReadWritePropertySuccesfully)
{
    uint32_t newActionValue = 5678;

    m_proxy->action(newActionValue);

    ASSERT_THAT(m_proxy->action(), Eq(newActionValue));
}

TEST_F(SdbusTestObject, CanAccessAssociatedPropertySetMessageInPropertySetHandler)
{
    m_proxy->blocking(true); // This will save pointer to property get message on server side

    ASSERT_THAT(m_adaptor->m_propertySetMsg, NotNull());
    ASSERT_THAT(m_adaptor->m_propertySetSender, Not(IsEmpty()));
}
