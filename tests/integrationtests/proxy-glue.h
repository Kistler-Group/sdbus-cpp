/**
 * (C) 2016 - 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2019 Stanislav Angelovic <angelovic.s@gmail.com>
 *
 * @file proxy-glue.h
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

#ifndef SDBUS_CPP_INTEGRATIONTESTS_PROXY_GLUE_H_
#define SDBUS_CPP_INTEGRATIONTESTS_PROXY_GLUE_H_

#include "defs.h"

// sdbus
#include "sdbus-c++/sdbus-c++.h"

class testing_proxy
{
protected:
    testing_proxy(sdbus::IProxy& object) :
        object_(object)
    {
        object_.uponSignal("simpleSignal").onInterface(INTERFACE_NAME).call([this](){ this->onSimpleSignal(); });
        object_.uponSignal("signalWithMap").onInterface(INTERFACE_NAME).call([this](const std::map<int32_t, std::string>& map){ this->onSignalWithMap(map); });
        object_.uponSignal("signalWithVariant").onInterface(INTERFACE_NAME).call([this](const sdbus::Variant& v){ this->onSignalWithVariant(v); });

        object_.uponSignal("signalWithoutRegistration").onInterface(INTERFACE_NAME).call([this](const sdbus::Struct<std::string, sdbus::Struct<sdbus::Signature>>& s)
                { this->onSignalWithoutRegistration(s); });
    }

    ~testing_proxy() = default;

    virtual void onSimpleSignal() = 0;
    virtual void onSignalWithMap(const std::map<int32_t, std::string>& map) = 0;
    virtual void onSignalWithVariant(const sdbus::Variant& v) = 0;
    virtual void onSignalWithoutRegistration(const sdbus::Struct<std::string, sdbus::Struct<sdbus::Signature>>& s) = 0;

    virtual void onDoOperationReply(uint32_t returnValue, const sdbus::Error* error) = 0;

public:
    void emitTwoSimpleSignals()
    {
        object_.callMethod("emitTwoSimpleSignals").onInterface(INTERFACE_NAME);
    }

    void noArgNoReturn()
    {
        object_.callMethod("noArgNoReturn").onInterface(INTERFACE_NAME);
    }

    int32_t getInt()
    {
        int32_t result;
        object_.callMethod("getInt").onInterface(INTERFACE_NAME).storeResultsTo(result);
        return result;
    }

    std::tuple<uint32_t, std::string> getTuple()
    {
        std::tuple<uint32_t, std::string> result;
        object_.callMethod("getTuple").onInterface(INTERFACE_NAME).storeResultsTo(result);
        return result;
    }

    double multiply(const int64_t& a, const double& b)
    {
        double result;
        object_.callMethod("multiply").onInterface(INTERFACE_NAME).withArguments(a, b).storeResultsTo(result);
        return result;
    }

    void multiplyWithNoReply(const int64_t& a, const double& b)
    {
        object_.callMethod("multiplyWithNoReply").onInterface(INTERFACE_NAME).withArguments(a, b).dontExpectReply();
    }

    std::vector<int16_t> getInts16FromStruct(const sdbus::Struct<uint8_t, int16_t, double, std::string, std::vector<int16_t>>& x)
    {
        std::vector<int16_t> result;
        object_.callMethod("getInts16FromStruct").onInterface(INTERFACE_NAME).withArguments(x).storeResultsTo(result);
        return result;
     }

    sdbus::Variant processVariant(const sdbus::Variant& v)
    {
        sdbus::Variant result;
        object_.callMethod("processVariant").onInterface(INTERFACE_NAME).withArguments(v).storeResultsTo(result);
        return result;
    }

    std::map<int32_t, sdbus::Variant> getMapOfVariants(const std::vector<int32_t>& x, const sdbus::Struct<sdbus::Variant, sdbus::Variant>& y)
    {
        std::map<int32_t, sdbus::Variant> result;
        object_.callMethod("getMapOfVariants").onInterface(INTERFACE_NAME).withArguments(x, y).storeResultsTo(result);
        return result;
    }

    sdbus::Struct<std::string, sdbus::Struct<std::map<int32_t, int32_t>>> getStructInStruct()
    {
        sdbus::Struct<std::string, sdbus::Struct<std::map<int32_t, int32_t>>> result;
        object_.callMethod("getStructInStruct").onInterface(INTERFACE_NAME).withArguments().storeResultsTo(result);
        return result;
    }

    int32_t sumStructItems(const sdbus::Struct<uint8_t, uint16_t>& a, const sdbus::Struct<int32_t, int64_t>& b)
    {
        int32_t result;
        object_.callMethod("sumStructItems").onInterface(INTERFACE_NAME).withArguments(a, b).storeResultsTo(result);
        return result;
    }

    uint32_t sumVectorItems(const std::vector<uint16_t>& a, const std::vector<uint64_t>& b)
    {
        uint32_t result;
        object_.callMethod("sumVectorItems").onInterface(INTERFACE_NAME).withArguments(a, b).storeResultsTo(result);
        return result;
    }

    uint32_t doOperation(uint32_t param)
    {
        uint32_t result;
        object_.callMethod("doOperation").onInterface(INTERFACE_NAME).withArguments(param).storeResultsTo(result);
        return result;
    }

    uint32_t doOperationWith500msTimeout(uint32_t param)
    {
        using namespace std::chrono_literals;
        uint32_t result;
        object_.callMethod("doOperation").onInterface(INTERFACE_NAME).withTimeout(500ms).withArguments(param).storeResultsTo(result);
        return result;
    }

    uint32_t doOperationAsync(uint32_t param)
    {
        uint32_t result;
        object_.callMethod("doOperationAsync").onInterface(INTERFACE_NAME).withArguments(param).storeResultsTo(result);
        return result;
    }

    void doOperationClientSideAsync(uint32_t param)
    {
        object_.callMethodAsync("doOperation")
               .onInterface(INTERFACE_NAME)
               .withArguments(param)
               .uponReplyInvoke([this](const sdbus::Error* error, uint32_t returnValue)
                                {
                                    this->onDoOperationReply(returnValue, error);
                                });
    }

    void doErroneousOperationClientSideAsync()
    {
        object_.callMethodAsync("throwError")
               .onInterface(INTERFACE_NAME)
               .uponReplyInvoke([this](const sdbus::Error* error)
                                {
                                    this->onDoOperationReply(0, error);
                                });
    }

    void doOperationClientSideAsyncWith500msTimeout(uint32_t param)
    {
        using namespace std::chrono_literals;
        object_.callMethodAsync("doOperation")
               .onInterface(INTERFACE_NAME)
               .withTimeout(500000us)
               .withArguments(param)
               .uponReplyInvoke([this](const sdbus::Error* error, uint32_t returnValue)
                                {
                                    this->onDoOperationReply(returnValue, error);
                                });
    }

    sdbus::Signature getSignature()
    {
        sdbus::Signature result;
        object_.callMethod("getSignature").onInterface(INTERFACE_NAME).storeResultsTo(result);
        return result;
    }

    sdbus::ObjectPath getObjectPath()
    {
        sdbus::ObjectPath result;
        object_.callMethod("getObjectPath").onInterface(INTERFACE_NAME).storeResultsTo(result);
        return result;
    }

    sdbus::UnixFd getUnixFd()
    {
        sdbus::UnixFd result;
        object_.callMethod("getUnixFd").onInterface(INTERFACE_NAME).storeResultsTo(result);
        return result;
    }

    ComplexType getComplex()
    {
        ComplexType result;
        object_.callMethod("getComplex").onInterface(INTERFACE_NAME).storeResultsTo(result);
        return result;
    }

    void throwError()
    {
        object_.callMethod("throwError").onInterface(INTERFACE_NAME);
    }

    void throwErrorWithNoReply()
    {
        object_.callMethod("throwErrorWithNoReply").onInterface(INTERFACE_NAME).dontExpectReply();
    }

    int32_t callNonexistentMethod()
    {
        int32_t result;
        object_.callMethod("callNonexistentMethod").onInterface(INTERFACE_NAME).storeResultsTo(result);
        return result;
    }

    int32_t callMethodOnNonexistentInterface()
    {
        int32_t result;
        object_.callMethod("someMethod").onInterface("sdbuscpp.interface.that.does.not.exist").storeResultsTo(result);
        return result;
    }

    std::string state()
    {
        return object_.getProperty("state").onInterface(INTERFACE_NAME);
    }

    void state(const std::string& value)
    {
        object_.setProperty("state").onInterface(INTERFACE_NAME).toValue(value);
    }

    uint32_t action()
    {
        return object_.getProperty("action").onInterface(INTERFACE_NAME);
    }

    void action(const uint32_t& value)
    {
        object_.setProperty("action").onInterface(INTERFACE_NAME).toValue(value);
    }

    bool blocking()
    {
        return object_.getProperty("blocking").onInterface(INTERFACE_NAME);
    }

    void blocking(const bool& value)
    {
        object_.setProperty("blocking").onInterface(INTERFACE_NAME).toValue(value);
    }


private:
    sdbus::IProxy& object_;

};


#endif /* SDBUS_CPP_INTEGRATIONTESTS_PROXY_GLUE_H_ */
