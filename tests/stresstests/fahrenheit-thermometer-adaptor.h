
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
        : object_(&object)
    {
        object_->registerMethod("getCurrentTemperature").onInterface(INTERFACE_NAME).withOutputParamNames("result").implementedAs([this](){ return this->getCurrentTemperature(); });
    }

    thermometer_adaptor(const thermometer_adaptor&) = delete;
    thermometer_adaptor& operator=(const thermometer_adaptor&) = delete;
    thermometer_adaptor(thermometer_adaptor&&) = default;
    thermometer_adaptor& operator=(thermometer_adaptor&&) = default;

    ~thermometer_adaptor() = default;

private:
    virtual uint32_t getCurrentTemperature() = 0;

private:
    sdbus::IObject* object_;
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
        : object_(&object)
    {
        object_->registerMethod("createDelegateObject").onInterface(INTERFACE_NAME).withOutputParamNames("delegate").implementedAs([this](sdbus::Result<sdbus::ObjectPath>&& result){ this->createDelegateObject(std::move(result)); });
        object_->registerMethod("destroyDelegateObject").onInterface(INTERFACE_NAME).withInputParamNames("delegate").implementedAs([this](sdbus::Result<>&& result, sdbus::ObjectPath delegate){ this->destroyDelegateObject(std::move(result), std::move(delegate)); }).withNoReply();
    }

    factory_adaptor(const factory_adaptor&) = delete;
    factory_adaptor& operator=(const factory_adaptor&) = delete;
    factory_adaptor(factory_adaptor&&) = default;
    factory_adaptor& operator=(factory_adaptor&&) = default;

    ~factory_adaptor() = default;

private:
    virtual void createDelegateObject(sdbus::Result<sdbus::ObjectPath>&& result) = 0;
    virtual void destroyDelegateObject(sdbus::Result<>&& result, sdbus::ObjectPath delegate) = 0;

private:
    sdbus::IObject* object_;
};

}}}}} // namespaces

#endif
