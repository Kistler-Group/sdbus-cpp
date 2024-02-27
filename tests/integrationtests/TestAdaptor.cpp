/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2022 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file TestAdaptor.cpp
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

#include "TestAdaptor.h"
#include <thread>
#include <chrono>
#include <atomic>

namespace sdbus { namespace test {

TestAdaptor::TestAdaptor(sdbus::IConnection& connection, sdbus::ObjectPath path) :
    AdaptorInterfaces(connection, std::move(path))
{
    registerAdaptor();
}

TestAdaptor::~TestAdaptor()
{
    unregisterAdaptor();
}

void TestAdaptor::noArgNoReturn()
{
}

int32_t TestAdaptor::getInt()
{
    return INT32_VALUE;
}

std::tuple<uint32_t, std::string> TestAdaptor::getTuple()
{
    return std::make_tuple(UINT32_VALUE, STRING_VALUE);
}

double TestAdaptor::multiply(const int64_t& a, const double& b)
{
    return a * b;
}

void TestAdaptor::multiplyWithNoReply(const int64_t& a, const double& b)
{
    m_multiplyResult = a * b;
    m_wasMultiplyCalled = true;
}

std::vector<int16_t> TestAdaptor::getInts16FromStruct(const sdbus::Struct<uint8_t, int16_t, double, std::string, std::vector<int16_t>>& x)
{
    std::vector<int16_t> res{x.get<1>()};
    auto y = std::get<std::vector<int16_t>>(x);
    res.insert(res.end(), y.begin(), y.end());
    return res;
}

sdbus::Variant TestAdaptor::processVariant(const sdbus::Variant& v)
{
    sdbus::Variant res{static_cast<int32_t>(v.get<double>())};
    return res;
}

std::map<int32_t, sdbus::Variant> TestAdaptor::getMapOfVariants(const std::vector<int32_t>& x, const sdbus::Struct<sdbus::Variant, sdbus::Variant>& y)
{
    std::map<int32_t, sdbus::Variant> res;
    for (auto item : x)
    {
        res[item] = (item <= 0) ? std::get<0>(y) : std::get<1>(y);
    }
    return res;
}

sdbus::Struct<std::string, sdbus::Struct<std::map<int32_t, int32_t>>> TestAdaptor::getStructInStruct()
{
    return sdbus::Struct{STRING_VALUE, sdbus::Struct{std::map<int32_t, int32_t>{{INT32_VALUE, INT32_VALUE}}}};
}

int32_t TestAdaptor::sumStructItems(const sdbus::Struct<uint8_t, uint16_t>& a, const sdbus::Struct<int32_t, int64_t>& b)
{
    int32_t res{0};
    res += std::get<0>(a) + std::get<1>(a);
    res += std::get<0>(b) + std::get<1>(b);
    return res;
}

uint32_t TestAdaptor::sumArrayItems(const std::vector<uint16_t>& a, const std::array<uint64_t, 3>& b)
{
    uint32_t res{0};
    for (auto x : a)
    {
        res += x;
    }
    for (auto x : b)
    {
        res += x;
    }
    return res;
}

uint32_t TestAdaptor::doOperation(const uint32_t& param)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(param));

    m_methodCallMsg = std::make_unique<const Message>(getObject().getCurrentlyProcessedMessage());
    m_methodCallMemberName = m_methodCallMsg->getMemberName();

    return param;
}

void TestAdaptor::doOperationAsync(sdbus::Result<uint32_t>&& result, uint32_t param)
{
    m_methodCallMsg = std::make_unique<const Message>(getObject().getCurrentlyProcessedMessage());
    m_methodCallMemberName = m_methodCallMsg->getMemberName();

    if (param == 0)
    {
        // Don't sleep and return the result from this thread
        result.returnResults(param);
    }
    else
    {
        // Process asynchronously in another thread and return the result from there
        std::thread([param, result = std::move(result)]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(param));
            result.returnResults(param);
        }).detach();
    }
}

sdbus::Signature TestAdaptor::getSignature()
{
    return SIGNATURE_VALUE;
}
sdbus::ObjectPath TestAdaptor::getObjPath()
{
    return OBJECT_PATH_VALUE;
}
sdbus::UnixFd TestAdaptor::getUnixFd()
{
    return sdbus::UnixFd{UNIX_FD_VALUE};
}

std::unordered_map<uint64_t, sdbus::Struct<std::map<uint8_t, std::vector<sdbus::Struct<sdbus::ObjectPath, bool, sdbus::Variant, std::map<int32_t, std::string>>>>, sdbus::Signature, std::string>> TestAdaptor::getComplex()
{
    return { // unordered_map
        {
            0,  // uint_64_t
            {   // struct
                {   // map
                    {
                        23,  // uint8_t
                        {   // vector
                            {   // struct
                                    sdbus::ObjectPath{"/object/path"}, // object path
                                    false,
                                    Variant{3.14},
                                    {   // map
                                        {0, "zero"}
                                    }
                            }
                        }
                    }
                },
                "a{t(a{ya(obva{is})}gs)}", // signature
                std::string{}
            }
        }
    };
}

void TestAdaptor::throwError()
{
    m_wasThrowErrorCalled = true;
    throw sdbus::createError(1, "A test error occurred");
}

void TestAdaptor::throwErrorWithNoReply()
{
    TestAdaptor::throwError();
}

void TestAdaptor::doPrivilegedStuff()
{
    // Intentionally left blank
}

void TestAdaptor::emitTwoSimpleSignals()
{
    emitSimpleSignal();
    emitSignalWithMap({});
}

std::string TestAdaptor::state()
{
    return m_state;
}

uint32_t TestAdaptor::action()
{
    return m_action;
}

void TestAdaptor::action(const uint32_t& value)
{
    m_action = value;
}

bool TestAdaptor::blocking()
{
    return m_blocking;
}

void TestAdaptor::blocking(const bool& value)
{
    m_propertySetMsg = std::make_unique<const Message>(getObject().getCurrentlyProcessedMessage());
    m_propertySetSender = m_propertySetMsg->getSender();

    m_blocking = value;
}

void TestAdaptor::emitSignalWithoutRegistration(const sdbus::Struct<std::string, sdbus::Struct<sdbus::Signature>>& s)
{
    getObject().emitSignal("signalWithoutRegistration").onInterface(sdbus::test::INTERFACE_NAME).withArguments(s);
}

std::string TestAdaptor::getExpectedXmlApiDescription() const
{
    return
R"delimiter(<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"
"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">
<node>
<interface name="org.freedesktop.DBus.Peer">
<method name="Ping"/>
<method name="GetMachineId">
<arg type="s" name="machine_uuid" direction="out"/>
</method>
</interface>
<interface name="org.freedesktop.DBus.Introspectable">
<method name="Introspect">
<arg name="data" type="s" direction="out"/>
</method>
</interface>
<interface name="org.freedesktop.DBus.Properties">
<method name="Get">
<arg name="interface" direction="in" type="s"/>
<arg name="property" direction="in" type="s"/>
<arg name="value" direction="out" type="v"/>
</method>
<method name="GetAll">
<arg name="interface" direction="in" type="s"/>
<arg name="properties" direction="out" type="a{sv}"/>
</method>
<method name="Set">
<arg name="interface" direction="in" type="s"/>
<arg name="property" direction="in" type="s"/>
<arg name="value" direction="in" type="v"/>
</method>
<signal name="PropertiesChanged">
<arg type="s" name="interface"/>
<arg type="a{sv}" name="changed_properties"/>
<arg type="as" name="invalidated_properties"/>
</signal>
</interface>
<interface name="org.freedesktop.DBus.ObjectManager">
<method name="GetManagedObjects">
<arg type="a{oa{sa{sv}}}" name="object_paths_interfaces_and_properties" direction="out"/>
</method>
<signal name="InterfacesAdded">
<arg type="o" name="object_path"/>
<arg type="a{sa{sv}}" name="interfaces_and_properties"/>
</signal>
<signal name="InterfacesRemoved">
<arg type="o" name="object_path"/>
<arg type="as" name="interfaces"/>
</signal>
</interface>
<interface name="org.sdbuscpp.integrationtests">
<annotation name="org.freedesktop.DBus.Deprecated" value="true"/>
<method name="doOperation">
<arg type="u" direction="in"/>
<arg type="u" direction="out"/>
</method>
<method name="doOperationAsync">
<arg type="u" direction="in"/>
<arg type="u" direction="out"/>
</method>
<method name="doPrivilegedStuff">
<annotation name="org.freedesktop.systemd1.Privileged" value="true"/>
</method>
<method name="emitTwoSimpleSignals">
</method>
<method name="getComplex">
<arg type="a{t(a{ya(obva{is})}gs)}" direction="out"/>
<annotation name="org.freedesktop.DBus.Deprecated" value="true"/>
</method>
<method name="getInt">)delimiter"
#if LIBSYSTEMD_VERSION>=242
R"delimiter(
<arg type="i" name="anInt" direction="out"/>)delimiter"
#else
R"delimiter(
<arg type="i" direction="out"/>)delimiter"
#endif
R"delimiter(
</method>
<method name="getInts16FromStruct">
<arg type="(yndsan)" direction="in"/>
<arg type="an" direction="out"/>
</method>
<method name="getMapOfVariants">)delimiter"
#if LIBSYSTEMD_VERSION>=242
R"delimiter(
<arg type="ai" name="x" direction="in"/>
<arg type="(vv)" name="y" direction="in"/>
<arg type="a{iv}" name="aMapOfVariants" direction="out"/>)delimiter"
#else
R"delimiter(
<arg type="ai" direction="in"/>
<arg type="(vv)" direction="in"/>
<arg type="a{iv}" direction="out"/>)delimiter"
#endif
R"delimiter(
</method>
<method name="getObjPath">
<arg type="o" direction="out"/>
</method>
<method name="getSignature">
<arg type="g" direction="out"/>
</method>
<method name="getStructInStruct">
<arg type="(s(a{ii}))" direction="out"/>
</method>
<method name="getTuple">
<arg type="u" direction="out"/>
<arg type="s" direction="out"/>
</method>
<method name="getUnixFd">
<arg type="h" direction="out"/>
</method>
<method name="multiply">)delimiter"
#if LIBSYSTEMD_VERSION>=242
R"delimiter(
<arg type="x" name="a" direction="in"/>
<arg type="d" name="b" direction="in"/>
<arg type="d" name="result" direction="out"/>)delimiter"
#else
R"delimiter(
<arg type="x" direction="in"/>
<arg type="d" direction="in"/>
<arg type="d" direction="out"/>)delimiter"
#endif
R"delimiter(
</method>
<method name="multiplyWithNoReply">
<arg type="x" direction="in"/>
<arg type="d" direction="in"/>
<annotation name="org.freedesktop.DBus.Deprecated" value="true"/>
<annotation name="org.freedesktop.DBus.Method.NoReply" value="true"/>
</method>
<method name="noArgNoReturn">
</method>
<method name="processVariant">
<arg type="v" direction="in"/>
<arg type="v" direction="out"/>
</method>
<method name="sumStructItems">
<arg type="(yq)" direction="in"/>
<arg type="(ix)" direction="in"/>
<arg type="i" direction="out"/>
</method>
<method name="sumArrayItems">
<arg type="aq" direction="in"/>
<arg type="at" direction="in"/>
<arg type="u" direction="out"/>
</method>
<method name="throwError">
</method>
<method name="throwErrorWithNoReply">
<annotation name="org.freedesktop.DBus.Method.NoReply" value="true"/>
</method>
<signal name="signalWithMap">
<arg type="a{is}"/>
</signal>
<signal name="signalWithVariant">
<arg type="v"/>
</signal>
<signal name="simpleSignal">
<annotation name="org.freedesktop.DBus.Deprecated" value="true"/>
</signal>
<property name="action" type="u" access="readwrite">
<annotation name="org.freedesktop.DBus.Property.EmitsChangedSignal" value="invalidates"/>
</property>
<property name="blocking" type="b" access="readwrite">
</property>
<property name="state" type="s" access="read">
<annotation name="org.freedesktop.DBus.Deprecated" value="true"/>
<annotation name="org.freedesktop.DBus.Property.EmitsChangedSignal" value="const"/>
</property>
</interface>
</node>
)delimiter";
}

}}
