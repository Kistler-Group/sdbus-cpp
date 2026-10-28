/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2026 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
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
#include "Defs.h"
#include <sdbus-c++/sdbus-c++.h>

#include <atomic>
#include <cstdint>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using ::testing::Eq;
using ::testing::NotNull;
using ::testing::Not;
using ::testing::IsEmpty;
using ::testing::Optional;
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
    uint32_t const newActionValue = 5678;

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
    sdbus::Variant const newActionValue{5678};

    this->m_proxy->actionVariant(newActionValue);

    ASSERT_THAT(this->m_proxy->actionVariant().template get<int>(), Eq(5678));
}

TYPED_TEST(SdbusTestObject, NotifiesAboutPropertyChangeWithNewPropertyValue)
{
    std::atomic notified{false};
    std::optional<bool> blockingValue;
    this->m_proxy->getProxy().uponPropertyChange(BLOCKING_PROPERTY).onInterface(INTERFACE_NAME).call([&](std::optional<bool> value)
    {
        blockingValue = value;
        notified = true;
    });

    this->m_proxy->blocking(!DEFAULT_BLOCKING_VALUE);
    this->m_adaptor->emitPropertiesChangedSignal(INTERFACE_NAME, {BLOCKING_PROPERTY});

    ASSERT_TRUE(waitUntil(notified));
    EXPECT_THAT(blockingValue, Optional(Eq(!DEFAULT_BLOCKING_VALUE))) << "Changed property shall be passed with its new value";
}

TYPED_TEST(SdbusTestObject, NotifiesAboutInvalidatedPropertyWithEmptyPropertyValue)
{
    std::atomic notified{false};
    std::optional<uint32_t> actionValue{0};
    // The `action` property is annotated with EmitsChangedSignal=invalidates
    this->m_proxy->getProxy().uponPropertyChange(ACTION_PROPERTY).onInterface(INTERFACE_NAME).call([&](std::optional<uint32_t> value)
    {
        actionValue = value;
        notified = true;
    });

    this->m_adaptor->emitPropertiesChangedSignal(INTERFACE_NAME, {ACTION_PROPERTY});

    ASSERT_TRUE(waitUntil(notified));
    EXPECT_FALSE(actionValue.has_value()) << "Invalidated property shall be passed without value, but got " << actionValue.value_or(0);
}

TYPED_TEST(SdbusTestObject, DoesNotNotifyAboutChangeOfOtherProperty)
{
    std::atomic notificationCount{0};
    this->m_proxy->getProxy().uponPropertyChange(BLOCKING_PROPERTY).onInterface(INTERFACE_NAME).call([&](std::optional<bool> /*value*/)
    {
        ++notificationCount;
    });

    this->m_adaptor->emitPropertiesChangedSignal(INTERFACE_NAME, {ACTION_PROPERTY});
    // Signals are delivered in order, so the notification about `blocking` comes after the one about `action`
    this->m_adaptor->emitPropertiesChangedSignal(INTERFACE_NAME, {BLOCKING_PROPERTY});

    ASSERT_TRUE(waitUntil([&]{ return notificationCount > 0; }));
    EXPECT_THAT(notificationCount.load(), Eq(1)) << "Change of `action` property shall not invoke handler of `blocking` property";
}

TYPED_TEST(SdbusTestObject, DoesNotNotifyAboutPropertyChangeOnOtherInterface)
{
    std::atomic notificationCount{0};
    std::optional<bool> blockingValue;
    this->m_proxy->getProxy().uponPropertyChange(BLOCKING_PROPERTY).onInterface(INTERFACE_NAME).call([&](std::optional<bool> value)
    {
        blockingValue = value;
        ++notificationCount;
    });

    std::map<sdbus::PropertyName, sdbus::Variant> const changedProperties{{BLOCKING_PROPERTY, sdbus::Variant{!DEFAULT_BLOCKING_VALUE}}};
    this->m_adaptor->getObject().emitSignal("PropertiesChanged")
                                .onInterface("org.freedesktop.DBus.Properties")
                                .withArguments(sdbus::InterfaceName{"org.sdbuscpp.integrationtests.other"}, changedProperties, std::vector<sdbus::PropertyName>{});
    this->m_adaptor->emitPropertiesChangedSignal(INTERFACE_NAME, {BLOCKING_PROPERTY});

    ASSERT_TRUE(waitUntil([&]{ return notificationCount > 0; }));
    EXPECT_THAT(notificationCount.load(), Eq(1)) << "Change of the property on other interface shall not invoke the handler";
    EXPECT_THAT(blockingValue, Eq(DEFAULT_BLOCKING_VALUE));
}

TYPED_TEST(SdbusTestObject, PassesErrorToPropertyChangeHandlerOnPropertyTypeMismatch)
{
    std::atomic notified{false};
    std::optional<sdbus::Error> receivedError;
    std::optional<std::string> blockingValue{"not reset"};
    // The `blocking` property is of bool type, not string
    this->m_proxy->getProxy().uponPropertyChange(BLOCKING_PROPERTY).onInterface(INTERFACE_NAME).call([&](std::optional<sdbus::Error> error, std::optional<std::string> value)
    {
        receivedError = std::move(error);
        blockingValue = std::move(value);
        notified = true;
    });

    this->m_adaptor->emitPropertiesChangedSignal(INTERFACE_NAME, {BLOCKING_PROPERTY});

    ASSERT_TRUE(waitUntil(notified));
    EXPECT_TRUE(receivedError.has_value()) << "Reading bool property as string shall be reported as error";
    EXPECT_FALSE(blockingValue.has_value()) << "No value shall be passed along with error";
}

TYPED_TEST(SdbusTestObject, StopsNotifyingAboutPropertyChangeAfterSlotIsDestroyed)
{
    std::atomic slotNotificationCount{0};
    std::atomic floatingNotificationCount{0};
    auto slot = this->m_proxy->getProxy().uponPropertyChange(BLOCKING_PROPERTY).onInterface(INTERFACE_NAME).call([&](std::optional<bool> /*value*/)
    {
        ++slotNotificationCount;
    }, sdbus::return_slot);

    this->m_adaptor->emitPropertiesChangedSignal(INTERFACE_NAME, {BLOCKING_PROPERTY});
    ASSERT_TRUE(waitUntil([&]{ return slotNotificationCount > 0; }));

    slot.reset();
    // The floating handler is used to know when the second notification has been processed
    this->m_proxy->getProxy().uponPropertyChange(BLOCKING_PROPERTY).onInterface(INTERFACE_NAME).call([&](std::optional<bool> /*value*/)
    {
        ++floatingNotificationCount;
    });
    this->m_adaptor->emitPropertiesChangedSignal(INTERFACE_NAME, {BLOCKING_PROPERTY});

    ASSERT_TRUE(waitUntil([&]{ return floatingNotificationCount > 0; }));
    EXPECT_THAT(slotNotificationCount.load(), Eq(1)) << "Handler shall not be invoked after its slot has been destroyed";
}
