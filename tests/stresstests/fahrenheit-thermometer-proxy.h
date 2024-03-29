
/*
 * This file was automatically generated by sdbus-c++-xml2cpp; DO NOT EDIT!
 */

#ifndef __sdbuscpp__fahrenheit_thermometer_proxy_h__proxy__H__
#define __sdbuscpp__fahrenheit_thermometer_proxy_h__proxy__H__

#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <tuple>

namespace org {
namespace sdbuscpp {
namespace stresstests {
namespace fahrenheit {

class thermometer_proxy
{
public:
    static constexpr const char* INTERFACE_NAME = "org.sdbuscpp.stresstests.fahrenheit.thermometer";

protected:
    thermometer_proxy(sdbus::IProxy& proxy)
        : proxy_(&proxy)
    {
    }

    thermometer_proxy(const thermometer_proxy&) = delete;
    thermometer_proxy& operator=(const thermometer_proxy&) = delete;
    thermometer_proxy(thermometer_proxy&&) = default;
    thermometer_proxy& operator=(thermometer_proxy&&) = default;

    ~thermometer_proxy() = default;

public:
    uint32_t getCurrentTemperature()
    {
        uint32_t result;
        proxy_->callMethod("getCurrentTemperature").onInterface(INTERFACE_NAME).storeResultsTo(result);
        return result;
    }

private:
    sdbus::IProxy* proxy_;
};

}}}} // namespaces

namespace org {
namespace sdbuscpp {
namespace stresstests {
namespace fahrenheit {
namespace thermometer {

class factory_proxy
{
public:
    static constexpr const char* INTERFACE_NAME = "org.sdbuscpp.stresstests.fahrenheit.thermometer.factory";

protected:
    factory_proxy(sdbus::IProxy& proxy)
        : proxy_(&proxy)
    {
    }

    factory_proxy(const factory_proxy&) = delete;
    factory_proxy& operator=(const factory_proxy&) = delete;
    factory_proxy(factory_proxy&&) = default;
    factory_proxy& operator=(factory_proxy&&) = default;

    ~factory_proxy() = default;

public:
    sdbus::ObjectPath createDelegateObject()
    {
        sdbus::ObjectPath result;
        proxy_->callMethod("createDelegateObject").onInterface(INTERFACE_NAME).storeResultsTo(result);
        return result;
    }

    void destroyDelegateObject(const sdbus::ObjectPath& delegate)
    {
        proxy_->callMethod("destroyDelegateObject").onInterface(INTERFACE_NAME).withArguments(delegate).dontExpectReply();
    }

private:
    sdbus::IProxy* proxy_;
};

}}}}} // namespaces

#endif
