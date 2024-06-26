
/*
 * This file was automatically generated by sdbus-c++-xml2cpp; DO NOT EDIT!
 */

#ifndef __sdbuscpp__fahrenheit_thermometer_adaptor_h__adaptor__H__
#define __sdbuscpp__fahrenheit_thermometer_adaptor_h__adaptor__H__

#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <tuple>

namespace org {
namespace sdbuscpp {
namespace stresstests {
namespace fahrenheit {

class thermometer_adaptor
{
public:
    static constexpr const char* INTERFACE_NAME = "org.sdbuscpp.stresstests.fahrenheit.thermometer";

protected:
    thermometer_adaptor(sdbus::IObject& object)
        : m_object(object)
    {
    }

    thermometer_adaptor(const thermometer_adaptor&) = delete;
    thermometer_adaptor& operator=(const thermometer_adaptor&) = delete;
    thermometer_adaptor(thermometer_adaptor&&) = delete;
    thermometer_adaptor& operator=(thermometer_adaptor&&) = delete;

    ~thermometer_adaptor() = default;

    void registerAdaptor()
    {
        m_object.addVTable(sdbus::registerMethod("getCurrentTemperature").withOutputParamNames("result").implementedAs([this](){ return this->getCurrentTemperature(); })).forInterface(INTERFACE_NAME);
    }

private:
    virtual uint32_t getCurrentTemperature() = 0;

private:
    sdbus::IObject& m_object;
};

}}}} // namespaces

namespace org {
namespace sdbuscpp {
namespace stresstests {
namespace fahrenheit {
namespace thermometer {

class factory_adaptor
{
public:
    static constexpr const char* INTERFACE_NAME = "org.sdbuscpp.stresstests.fahrenheit.thermometer.factory";

protected:
    factory_adaptor(sdbus::IObject& object)
        : m_object(object)
    {
    }

    factory_adaptor(const factory_adaptor&) = delete;
    factory_adaptor& operator=(const factory_adaptor&) = delete;
    factory_adaptor(factory_adaptor&&) = delete;
    factory_adaptor& operator=(factory_adaptor&&) = delete;

    ~factory_adaptor() = default;

    void registerAdaptor()
    {
        m_object.addVTable( sdbus::registerMethod("createDelegateObject").withOutputParamNames("delegate").implementedAs([this](sdbus::Result<sdbus::ObjectPath>&& result){ this->createDelegateObject(std::move(result)); })
                          , sdbus::registerMethod("destroyDelegateObject").withInputParamNames("delegate").implementedAs([this](sdbus::Result<>&& result, sdbus::ObjectPath delegate){ this->destroyDelegateObject(std::move(result), std::move(delegate)); }).withNoReply()
                          ).forInterface(INTERFACE_NAME);
    }

private:
    virtual void createDelegateObject(sdbus::Result<sdbus::ObjectPath>&& result) = 0;
    virtual void destroyDelegateObject(sdbus::Result<>&& result, sdbus::ObjectPath delegate) = 0;

private:
    sdbus::IObject& m_object;
};

}}}}} // namespaces

#endif
