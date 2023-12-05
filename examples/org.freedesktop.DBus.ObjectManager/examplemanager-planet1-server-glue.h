
/*
 * This file was automatically generated by sdbus-c++-xml2cpp; DO NOT EDIT!
 */

#ifndef __sdbuscpp__examplemanager_planet1_server_glue_h__adaptor__H__
#define __sdbuscpp__examplemanager_planet1_server_glue_h__adaptor__H__

#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <tuple>

namespace org {
namespace sdbuscpp {
namespace ExampleManager {

class Planet1_adaptor
{
public:
    static constexpr const char* INTERFACE_NAME = "org.sdbuscpp.ExampleManager.Planet1";

protected:
    Planet1_adaptor(sdbus::IObject& object) : object_(object)
    {
    }

    ~Planet1_adaptor() = default;

    void registerAdaptor()
    {
        object_.addVTable( sdbus::registerMethod("GetPopulation").withOutputParamNames("population").implementedAs([this](){ return this->GetPopulation(); })
                         , sdbus::registerProperty("Name").withGetter([this](){ return this->Name(); })
                         ).forInterface(INTERFACE_NAME);
    }

private:
    virtual uint64_t GetPopulation() = 0;

private:
    virtual std::string Name() = 0;

private:
    sdbus::IObject& object_;
};

}}} // namespaces

#endif
