
/*
 * This file was automatically generated by sdbus-c++-xml2cpp; DO NOT EDIT!
 */

#ifndef __sdbuscpp__perftests_adaptor_h__adaptor__H__
#define __sdbuscpp__perftests_adaptor_h__adaptor__H__

#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <tuple>

namespace org {
namespace sdbuscpp {

class perftests_adaptor
{
public:
    static constexpr const char* INTERFACE_NAME = "org.sdbuscpp.perftests";

protected:
    perftests_adaptor(sdbus::IObject& object)
        : m_object(object)
    {
    }

    perftests_adaptor(const perftests_adaptor&) = delete;
    perftests_adaptor& operator=(const perftests_adaptor&) = delete;
    perftests_adaptor(perftests_adaptor&&) = delete;
    perftests_adaptor& operator=(perftests_adaptor&&) = delete;

    ~perftests_adaptor() = default;

    void registerAdaptor()
    {
        m_object.addVTable( sdbus::registerMethod("sendDataSignals").withInputParamNames("numberOfSignals", "signalMsgSize").implementedAs([this](const uint32_t& numberOfSignals, const uint32_t& signalMsgSize){ return this->sendDataSignals(numberOfSignals, signalMsgSize); })
                          , sdbus::registerMethod("concatenateTwoStrings").withInputParamNames("string1", "string2").withOutputParamNames("result").implementedAs([this](const std::string& string1, const std::string& string2){ return this->concatenateTwoStrings(string1, string2); })
                          , sdbus::registerSignal("dataSignal").withParameters<std::string>("data")
                          ).forInterface(INTERFACE_NAME);
    }

public:
    void emitDataSignal(const std::string& data)
    {
        m_object.emitSignal("dataSignal").onInterface(INTERFACE_NAME).withArguments(data);
    }

private:
    virtual void sendDataSignals(const uint32_t& numberOfSignals, const uint32_t& signalMsgSize) = 0;
    virtual std::string concatenateTwoStrings(const std::string& string1, const std::string& string2) = 0;

private:
    sdbus::IObject& m_object;
};

}} // namespaces

#endif
