
/*
 * This file was automatically generated by sdbus-c++-xml2cpp; DO NOT EDIT!
 */

#ifndef __sdbuscpp__celsius_thermometer_adaptor_h__adaptor__H__
#define __sdbuscpp__celsius_thermometer_adaptor_h__adaptor__H__

#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <tuple>

namespace org {
namespace sdbuscpp {
namespace stresstests {
namespace celsius {

class thermometer_adaptor
{
public:
    static inline const sdbus::InterfaceName INTERFACE_NAME{"org.sdbuscpp.stresstests.celsius.thermometer"};

protected:
    thermometer_adaptor(sdbus::IObject& object)
        : object_(&object)
    {
    }

    thermometer_adaptor(const thermometer_adaptor&) = delete;
    thermometer_adaptor& operator=(const thermometer_adaptor&) = delete;
    thermometer_adaptor(thermometer_adaptor&&) = default;
    thermometer_adaptor& operator=(thermometer_adaptor&&) = default;

    ~thermometer_adaptor() = default;

    void registerAdaptor()
    {
        object_->addVTable(sdbus::registerMethod("getCurrentTemperature").withOutputParamNames("result").implementedAs([this](){ return this->getCurrentTemperature(); })).forInterface(INTERFACE_NAME);
    }

private:
    virtual uint32_t getCurrentTemperature() = 0;

private:
    sdbus::IObject* object_;
};

}}}} // namespaces

#endif
