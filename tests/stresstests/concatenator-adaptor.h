
/*
 * This file was automatically generated by sdbus-c++-xml2cpp; DO NOT EDIT!
 */

#ifndef __sdbuscpp__concatenator_adaptor_h__adaptor__H__
#define __sdbuscpp__concatenator_adaptor_h__adaptor__H__

#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <tuple>

namespace org {
namespace sdbuscpp {
namespace stresstests {

class concatenator_adaptor
{
public:
    static constexpr const char* INTERFACE_NAME = "org.sdbuscpp.stresstests.concatenator";

protected:
    concatenator_adaptor(sdbus::IObject& object)
        : object_(&object)
    {
    }

    concatenator_adaptor(const concatenator_adaptor&) = delete;
    concatenator_adaptor& operator=(const concatenator_adaptor&) = delete;
    concatenator_adaptor(concatenator_adaptor&&) = default;
    concatenator_adaptor& operator=(concatenator_adaptor&&) = default;

    ~concatenator_adaptor() = default;

    void registerAdaptor()
    {
        object_->addVTable( sdbus::registerMethod("concatenate").withInputParamNames("params").withOutputParamNames("result").implementedAs([this](sdbus::Result<std::string>&& result, std::map<std::string, sdbus::Variant> params){ this->concatenate(std::move(result), std::move(params)); })
                          , sdbus::registerSignal("concatenatedSignal").withParameters<std::string>("concatenatedString")
                          ).forInterface(INTERFACE_NAME);
    }

public:
    void emitConcatenatedSignal(const std::string& concatenatedString)
    {
        object_->emitSignal("concatenatedSignal").onInterface(INTERFACE_NAME).withArguments(concatenatedString);
    }

private:
    virtual void concatenate(sdbus::Result<std::string>&& result, std::map<std::string, sdbus::Variant> params) = 0;

private:
    sdbus::IObject* object_;
};

}}} // namespaces

#endif
