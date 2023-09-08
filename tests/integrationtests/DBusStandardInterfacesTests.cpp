/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2022 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file DBusStandardInterfacesTests.cpp
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
using namespace std::chrono_literals;
using namespace sdbus::test;

using SdbusTestObject = TestFixture;

/*-------------------------------------*/
/* --          TEST CASES           -- */
/*-------------------------------------*/

TEST_F(SdbusTestObject, PingsViaPeerInterface)
{
    ASSERT_NO_THROW(m_proxy->Ping());
}

TEST_F(SdbusTestObject, AnswersMachineUuidViaPeerInterface)
{
    if (::access("/etc/machine-id", F_OK) == -1 &&
        ::access("/var/lib/dbus/machine-id", F_OK) == -1)
        GTEST_SKIP() << "/etc/machine-id and /var/lib/dbus/machine-id files do not exist, GetMachineId() will not work";

    ASSERT_NO_THROW(m_proxy->GetMachineId());
}

// TODO: Adjust expected xml and uncomment this test
//TEST_F(SdbusTestObject, AnswersXmlApiDescriptionViaIntrospectableInterface)
//{
//    ASSERT_THAT(m_proxy->Introspect(), Eq(m_adaptor->getExpectedXmlApiDescription()));
//}

TEST_F(SdbusTestObject, GetsPropertyViaPropertiesInterface)
{
    ASSERT_THAT(m_proxy->Get(INTERFACE_NAME, "state").get<std::string>(), Eq(DEFAULT_STATE_VALUE));
}

TEST_F(SdbusTestObject, SetsPropertyViaPropertiesInterface)
{
    uint32_t newActionValue = 2345;

    m_proxy->Set(INTERFACE_NAME, "action", sdbus::Variant{newActionValue});

    ASSERT_THAT(m_proxy->action(), Eq(newActionValue));
}

TEST_F(SdbusTestObject, GetsAllPropertiesViaPropertiesInterface)
{
    const auto properties = m_proxy->GetAll(INTERFACE_NAME);

    ASSERT_THAT(properties, SizeIs(3));
    EXPECT_THAT(properties.at("state").get<std::string>(), Eq(DEFAULT_STATE_VALUE));
    EXPECT_THAT(properties.at("action").get<uint32_t>(), Eq(DEFAULT_ACTION_VALUE));
    EXPECT_THAT(properties.at("blocking").get<bool>(), Eq(DEFAULT_BLOCKING_VALUE));
}

TEST_F(SdbusTestObject, EmitsPropertyChangedSignalForSelectedProperties)
{
    std::atomic<bool> signalReceived{false};
    m_proxy->m_onPropertiesChangedHandler = [&signalReceived]( const std::string& interfaceName
                                                             , const std::map<std::string, sdbus::Variant>& changedProperties
                                                             , const std::vector<std::string>& /*invalidatedProperties*/ )
    {
        EXPECT_THAT(interfaceName, Eq(INTERFACE_NAME));
        EXPECT_THAT(changedProperties, SizeIs(1));
        EXPECT_THAT(changedProperties.at("blocking").get<bool>(), Eq(!DEFAULT_BLOCKING_VALUE));
        signalReceived = true;
    };

    m_proxy->blocking(!DEFAULT_BLOCKING_VALUE);
    m_proxy->action(DEFAULT_ACTION_VALUE*2);
    m_adaptor->emitPropertiesChangedSignal(INTERFACE_NAME, {"blocking"});

    ASSERT_TRUE(waitUntil(signalReceived));
}

TEST_F(SdbusTestObject, EmitsPropertyChangedSignalForAllProperties)
{
    std::atomic<bool> signalReceived{false};
    m_proxy->m_onPropertiesChangedHandler = [&signalReceived]( const std::string& interfaceName
                                                             , const std::map<std::string, sdbus::Variant>& changedProperties
                                                             , const std::vector<std::string>& invalidatedProperties )
    {
        EXPECT_THAT(interfaceName, Eq(INTERFACE_NAME));
        EXPECT_THAT(changedProperties, SizeIs(1));
        EXPECT_THAT(changedProperties.at("blocking").get<bool>(), Eq(DEFAULT_BLOCKING_VALUE));
        ASSERT_THAT(invalidatedProperties, SizeIs(1));
        EXPECT_THAT(invalidatedProperties[0], Eq("action"));
        signalReceived = true;
    };

    m_adaptor->emitPropertiesChangedSignal(INTERFACE_NAME);

    ASSERT_TRUE(waitUntil(signalReceived));
}

TEST_F(SdbusTestObject, GetsZeroManagedObjectsIfHasNoSubPathObjects)
{
    m_adaptor.reset();
    const auto objectsInterfacesAndProperties = m_objectManagerProxy->GetManagedObjects();

    ASSERT_THAT(objectsInterfacesAndProperties, SizeIs(0));
}

TEST_F(SdbusTestObject, GetsManagedObjectsSuccessfully)
{
    auto adaptor2 = std::make_unique<TestAdaptor>(*s_adaptorConnection, OBJECT_PATH_2);
    const auto objectsInterfacesAndProperties = m_objectManagerProxy->GetManagedObjects();

    ASSERT_THAT(objectsInterfacesAndProperties, SizeIs(2));
    EXPECT_THAT(objectsInterfacesAndProperties.at(OBJECT_PATH)
        .at(org::sdbuscpp::integrationtests_adaptor::INTERFACE_NAME)
        .at("action").get<uint32_t>(), Eq(DEFAULT_ACTION_VALUE));
    EXPECT_THAT(objectsInterfacesAndProperties.at(OBJECT_PATH_2)
        .at(org::sdbuscpp::integrationtests_adaptor::INTERFACE_NAME)
        .at("action").get<uint32_t>(), Eq(DEFAULT_ACTION_VALUE));
}

TEST_F(SdbusTestObject, EmitsInterfacesAddedSignalForSelectedObjectInterfaces)
{
    std::atomic<bool> signalReceived{false};
    m_objectManagerProxy->m_onInterfacesAddedHandler = [&signalReceived]( const sdbus::ObjectPath& objectPath
            , const std::map<std::string, std::map<std::string, sdbus::Variant>>& interfacesAndProperties )
    {
        EXPECT_THAT(objectPath, Eq(OBJECT_PATH));
        EXPECT_THAT(interfacesAndProperties, SizeIs(1));
        EXPECT_THAT(interfacesAndProperties.count(INTERFACE_NAME), Eq(1));
#if LIBSYSTEMD_VERSION<=244
        // Up to sd-bus v244, all properties are added to the list, i.e. `state', `action', and `blocking' in this case.
        EXPECT_THAT(interfacesAndProperties.at(INTERFACE_NAME), SizeIs(3));
        EXPECT_TRUE(interfacesAndProperties.at(INTERFACE_NAME).count("state"));
        EXPECT_TRUE(interfacesAndProperties.at(INTERFACE_NAME).count("action"));
        EXPECT_TRUE(interfacesAndProperties.at(INTERFACE_NAME).count("blocking"));
#else
        // Since v245 sd-bus does not add to the InterfacesAdded signal message the values of properties marked only
        // for invalidation on change, which makes the behavior consistent with the PropertiesChangedSignal.
        // So in this specific instance, `action' property is no more added to the list.
        EXPECT_THAT(interfacesAndProperties.at(INTERFACE_NAME), SizeIs(2));
        EXPECT_TRUE(interfacesAndProperties.at(INTERFACE_NAME).count("state"));
        EXPECT_TRUE(interfacesAndProperties.at(INTERFACE_NAME).count("blocking"));
#endif
        signalReceived = true;
    };

    m_adaptor->emitInterfacesAddedSignal({INTERFACE_NAME});

    ASSERT_TRUE(waitUntil(signalReceived));
}

TEST_F(SdbusTestObject, EmitsInterfacesAddedSignalForAllObjectInterfaces)
{
    std::atomic<bool> signalReceived{false};
    m_objectManagerProxy->m_onInterfacesAddedHandler = [&signalReceived]( const sdbus::ObjectPath& objectPath
            , const std::map<std::string, std::map<std::string, sdbus::Variant>>& interfacesAndProperties )
    {
        EXPECT_THAT(objectPath, Eq(OBJECT_PATH));
#if LIBSYSTEMD_VERSION<=250
        EXPECT_THAT(interfacesAndProperties, SizeIs(5)); // INTERFACE_NAME + 4 standard interfaces
#else
        // Since systemd v251, ObjectManager standard interface is not listed among the interfaces
        // if the object does not have object manager functionality explicitly enabled.
        EXPECT_THAT(interfacesAndProperties, SizeIs(4)); // INTERFACE_NAME + 3 standard interfaces
#endif
#if LIBSYSTEMD_VERSION<=244
        // Up to sd-bus v244, all properties are added to the list, i.e. `state', `action', and `blocking' in this case.
        EXPECT_THAT(interfacesAndProperties.at(INTERFACE_NAME), SizeIs(3));
        EXPECT_TRUE(interfacesAndProperties.at(INTERFACE_NAME).count("state"));
        EXPECT_TRUE(interfacesAndProperties.at(INTERFACE_NAME).count("action"));
        EXPECT_TRUE(interfacesAndProperties.at(INTERFACE_NAME).count("blocking"));
#else
        // Since v245 sd-bus does not add to the InterfacesAdded signal message the values of properties marked only
        // for invalidation on change, which makes the behavior consistent with the PropertiesChangedSignal.
        // So in this specific instance, `action' property is no more added to the list.
        EXPECT_THAT(interfacesAndProperties.at(INTERFACE_NAME), SizeIs(2));
        EXPECT_TRUE(interfacesAndProperties.at(INTERFACE_NAME).count("state"));
        EXPECT_TRUE(interfacesAndProperties.at(INTERFACE_NAME).count("blocking"));
#endif
        signalReceived = true;
    };

    m_adaptor->emitInterfacesAddedSignal();

    ASSERT_TRUE(waitUntil(signalReceived));
}

TEST_F(SdbusTestObject, EmitsInterfacesRemovedSignalForSelectedObjectInterfaces)
{
    std::atomic<bool> signalReceived{false};
    m_objectManagerProxy->m_onInterfacesRemovedHandler = [&signalReceived]( const sdbus::ObjectPath& objectPath
                                                                          , const std::vector<std::string>& interfaces )
    {
        EXPECT_THAT(objectPath, Eq(OBJECT_PATH));
        ASSERT_THAT(interfaces, SizeIs(1));
        EXPECT_THAT(interfaces[0], Eq(INTERFACE_NAME));
        signalReceived = true;
    };

    m_adaptor->emitInterfacesRemovedSignal({INTERFACE_NAME});

    ASSERT_TRUE(waitUntil(signalReceived));
}

TEST_F(SdbusTestObject, EmitsInterfacesRemovedSignalForAllObjectInterfaces)
{
    std::atomic<bool> signalReceived{false};
    m_objectManagerProxy->m_onInterfacesRemovedHandler = [&signalReceived]( const sdbus::ObjectPath& objectPath
                                                                          , const std::vector<std::string>& interfaces )
    {
        EXPECT_THAT(objectPath, Eq(OBJECT_PATH));
#if LIBSYSTEMD_VERSION<=250
        ASSERT_THAT(interfaces, SizeIs(5)); // INTERFACE_NAME + 4 standard interfaces
#else
        // Since systemd v251, ObjectManager standard interface is not listed among the interfaces
        // if the object does not have object manager functionality explicitly enabled.
        ASSERT_THAT(interfaces, SizeIs(4)); // INTERFACE_NAME + 3 standard interfaces
#endif
        signalReceived = true;
    };

    m_adaptor->emitInterfacesRemovedSignal();

    ASSERT_TRUE(waitUntil(signalReceived));
}
