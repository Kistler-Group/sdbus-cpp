/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 *
 * @file adaptor-glue.h
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

#ifndef SDBUS_CPP_INTEGRATIONTESTS_ADAPTOR_GLUE_H_
#define SDBUS_CPP_INTEGRATIONTESTS_ADAPTOR_GLUE_H_

#include "defs.h"

// sdbus
#include "sdbus-c++/sdbus-c++.h"

using ComplexType = std::map<
                        uint64_t,
                        sdbus::Struct<
                            std::map<
                                uint8_t,
                                std::vector<
                                    sdbus::Struct<
                                        sdbus::ObjectPath,
                                        bool,
                                        sdbus::Variant,
                                        std::map<int, std::string>
                                    >
                                >
                            >,
                            sdbus::Signature,
                            std::string // char* leads to type and memory issues, std::string is best choice
                        >
                    >;

class testing_adaptor
{

protected:
    testing_adaptor(sdbus::IObject& object) :
        object_(object)
    {
        object_.setInterfaceFlags(INTERFACE_NAME).markAsDeprecated().withPropertyUpdateBehavior(sdbus::Flags::EMITS_NO_SIGNAL);

        object_.registerMethod("noArgNoReturn").onInterface(INTERFACE_NAME).implementedAs([this](){ return this->noArgNoReturn(); });
        object_.registerMethod("getInt").onInterface(INTERFACE_NAME).implementedAs([this](){ return this->getInt(); });
        object_.registerMethod("getTuple").onInterface(INTERFACE_NAME).implementedAs([this](){ return this->getTuple(); });

        object_.registerMethod("multiply").onInterface(INTERFACE_NAME).implementedAs([this](const int64_t& a, const double& b){ return this->multiply(a, b); });
        object_.registerMethod("multiplyWithNoReply").onInterface(INTERFACE_NAME).implementedAs([this](const int64_t& a, const double& b){ this->multiplyWithNoReply(a, b); }).markAsDeprecated().withNoReply();
        object_.registerMethod("getInts16FromStruct").onInterface(INTERFACE_NAME).implementedAs([this](
                const sdbus::Struct<uint8_t, int16_t, double, std::string, std::vector<int16_t>>& x){ return this->getInts16FromStruct(x); });

        object_.registerMethod("processVariant").onInterface(INTERFACE_NAME).implementedAs([this](sdbus::Variant& v){ return this->processVariant(v); });

        object_.registerMethod("getMapOfVariants").onInterface(INTERFACE_NAME).implementedAs([this](
                const std::vector<int32_t>& x, const sdbus::Struct<sdbus::Variant, sdbus::Variant>& y){ return this->getMapOfVariants(x ,y); });

        object_.registerMethod("getStructInStruct").onInterface(INTERFACE_NAME).implementedAs([this](){ return this->getStructInStruct(); });

        object_.registerMethod("sumStructItems").onInterface(INTERFACE_NAME).implementedAs([this](
                const sdbus::Struct<uint8_t, uint16_t>& a, const sdbus::Struct<int32_t, int64_t>& b){
            return this->sumStructItems(a, b);
        });

        object_.registerMethod("sumVectorItems").onInterface(INTERFACE_NAME).implementedAs([this](
                const std::vector<uint16_t>& a, const std::vector<uint64_t>& b){
            return this->sumVectorItems(a, b);
        });

        object_.registerMethod("doOperation").onInterface(INTERFACE_NAME).implementedAs([this](uint32_t param)
        {
            return this->doOperation(param);
        });

        object_.registerMethod("doOperationAsync").onInterface(INTERFACE_NAME).implementedAs([this](sdbus::Result<uint32_t> result, uint32_t param)
        {
            this->doOperationAsync(param, std::move(result));
        });

        object_.registerMethod("getSignature").onInterface(INTERFACE_NAME).implementedAs([this](){ return this->getSignature(); });
        object_.registerMethod("getObjectPath").onInterface(INTERFACE_NAME).implementedAs([this](){ return this->getObjectPath(); });

        object_.registerMethod("getComplex").onInterface(INTERFACE_NAME).implementedAs([this](){ return this->getComplex(); }).markAsDeprecated();

        object_.registerMethod("throwError").onInterface(INTERFACE_NAME).implementedAs([this](){ return this->throwError(); });
        object_.registerMethod("throwErrorWithNoReply").onInterface(INTERFACE_NAME).implementedAs([this](){ this->throwError(); }).withNoReply();

        object_.registerMethod("doPrivilegedStuff").onInterface(INTERFACE_NAME).implementedAs([](){}).markAsPrivileged();

        // registration of signals is optional, it is useful because of introspection
        object_.registerSignal("simpleSignal").onInterface(INTERFACE_NAME).markAsDeprecated();
        object_.registerSignal("signalWithMap").onInterface(INTERFACE_NAME).withParameters<std::map<int32_t, std::string>>();
        object_.registerSignal("signalWithVariant").onInterface(INTERFACE_NAME).withParameters<sdbus::Variant>();

        object_.registerProperty("state").onInterface(INTERFACE_NAME).withGetter([this](){ return this->state(); }).markAsDeprecated().withUpdateBehavior(sdbus::Flags::CONST_PROPERTY_VALUE);
        object_.registerProperty("action").onInterface(INTERFACE_NAME).withGetter([this](){ return this->action(); }).withSetter([this](const uint32_t& value){ this->action(value); }).withUpdateBehavior(sdbus::Flags::EMITS_NO_SIGNAL);
        object_.registerProperty("blocking").onInterface(INTERFACE_NAME)./*withGetter([this](){ return this->blocking(); }).*/withSetter([this](const bool& value){ this->blocking(value); });

    }

public:
    void simpleSignal()
    {
        object_.emitSignal("simpleSignal").onInterface(INTERFACE_NAME);
    }

    void signalWithMap(const std::map<int32_t, std::string>& map)
    {
        object_.emitSignal("signalWithMap").onInterface(INTERFACE_NAME).withArguments(map);
    }

    void signalWithVariant(const sdbus::Variant& v)
    {
        object_.emitSignal("signalWithVariant").onInterface(INTERFACE_NAME).withArguments(v);
    }

    void signalWithoutRegistration(const sdbus::Struct<std::string, sdbus::Struct<sdbus::Signature>>& s)
    {
        object_.emitSignal("signalWithoutRegistration").onInterface(INTERFACE_NAME).withArguments(s);
    }

    void emitSignalOnNonexistentInterface()
    {
        object_.emitSignal("simpleSignal").onInterface("sdbuscpp.interface.that.does.not.exist");
    }

private:
    sdbus::IObject& object_;

protected:

    virtual void noArgNoReturn() const  = 0;
    virtual int32_t getInt() const = 0;
    virtual std::tuple<uint32_t, std::string> getTuple() const = 0;
    virtual double multiply(const int64_t& a, const double& b) const = 0;
    virtual void multiplyWithNoReply(const int64_t& a, const double& b) const = 0;
    virtual std::vector<int16_t> getInts16FromStruct(const sdbus::Struct<uint8_t, int16_t, double, std::string, std::vector<int16_t>>& x) const = 0;
    virtual sdbus::Variant processVariant(sdbus::Variant& v) = 0;
    virtual std::map<int32_t, sdbus::Variant> getMapOfVariants(const std::vector<int32_t>& x, const sdbus::Struct<sdbus::Variant, sdbus::Variant>& y) const = 0;
    virtual sdbus::Struct<std::string, sdbus::Struct<std::map<int32_t, int32_t>>> getStructInStruct() const = 0;
    virtual int32_t sumStructItems(const sdbus::Struct<uint8_t, uint16_t>& a, const sdbus::Struct<int32_t, int64_t>& b) = 0;
    virtual uint32_t sumVectorItems(const std::vector<uint16_t>& a, const std::vector<uint64_t>& b) = 0;
    virtual uint32_t doOperation(uint32_t param) = 0;
    virtual void doOperationAsync(uint32_t param, sdbus::Result<uint32_t> result) = 0;
    virtual sdbus::Signature getSignature() const  = 0;
    virtual sdbus::ObjectPath getObjectPath() const = 0;
    virtual ComplexType getComplex() const = 0;
    virtual void throwError() const = 0;

    virtual std::string state() = 0;
    virtual uint32_t action() = 0;
    virtual void action(const uint32_t& value) = 0;
    virtual bool blocking() = 0;
    virtual void blocking(const bool& value) = 0;

public: // For testing purposes
    std::string getExpectedXmlApiDescription()
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
 <interface name="com.kistler.testsdbuscpp">
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
  <method name="getComplex">
   <arg type="a{t(a{ya(obva{is})}gs)}" direction="out"/>
   <annotation name="org.freedesktop.DBus.Deprecated" value="true"/>
  </method>
  <method name="getInt">
   <arg type="i" direction="out"/>
  </method>
  <method name="getInts16FromStruct">
   <arg type="(yndsan)" direction="in"/>
   <arg type="an" direction="out"/>
  </method>
  <method name="getMapOfVariants">
   <arg type="ai" direction="in"/>
   <arg type="(vv)" direction="in"/>
   <arg type="a{iv}" direction="out"/>
  </method>
  <method name="getObjectPath">
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
  <method name="multiply">
   <arg type="x" direction="in"/>
   <arg type="d" direction="in"/>
   <arg type="d" direction="out"/>
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
  <method name="sumVectorItems">
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
   <annotation name="org.freedesktop.DBus.Property.EmitsChangedSignal" value="false"/>
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

};



#endif /* SDBUS_CPP_INTEGRATIONTESTS_ADAPTOR_GLUE_H_ */
