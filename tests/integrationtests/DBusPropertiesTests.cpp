/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2024 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
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

/*-------------------------------------*/
/* --          TEST CASES           -- */
/*-------------------------------------*/

TYPED_TEST(SdbusTestObject, ReadsReadOnlyPropertySuccessfully)
{
    ASSERT_THAT(this->m_proxy->state(), Eq(DEFAULT_STATE_VALUE));
}

TYPED_TEST(SdbusTestObject, FailsWritingToReadOnlyProperty)
{
    ASSERT_THROW(this->m_proxy->setStateProperty("new_value"), sdbus::Error);
}

TYPED_TEST(SdbusTestObject, WritesAndReadsReadWritePropertySuccessfully)
{
    uint32_t newActionValue = 5678;

    this->m_proxy->action(newActionValue);

    ASSERT_THAT(this->m_proxy->action(), Eq(newActionValue));
}

TYPED_TEST(SdbusTestObject, CanAccessAssociatedPropertySetMessageInPropertySetHandler)
{
    this->m_proxy->blocking(true); // This will save pointer to property get message on server side

    ASSERT_THAT(this->m_adaptor->m_propertySetMsg, NotNull());
    ASSERT_THAT(this->m_adaptor->m_propertySetSender, Not(IsEmpty()));
}

TYPED_TEST(SdbusTestObject, WritesAndReadsReadWriteVariantPropertySuccessfully)
{
    sdbus::Variant newActionValue{5678};

    this->m_proxy->actionVariant(newActionValue);

    ASSERT_THAT(this->m_proxy->actionVariant().template get<int>(), Eq(5678));
}
