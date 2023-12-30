
/*
 * This file was automatically generated by sdbus-c++-xml2cpp; DO NOT EDIT!
 */

#ifndef __sdbuscpp__concatenator_proxy_h__proxy__H__
#define __sdbuscpp__concatenator_proxy_h__proxy__H__

#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <tuple>

namespace org {
namespace sdbuscpp {
namespace stresstests {

class concatenator_proxy
{
public:
    static constexpr const char* INTERFACE_NAME = "org.sdbuscpp.stresstests.concatenator";

protected:
    concatenator_proxy(sdbus::IProxy& proxy)
        : proxy_(&proxy)
    {
    }

    concatenator_proxy(const concatenator_proxy&) = delete;
    concatenator_proxy& operator=(const concatenator_proxy&) = delete;
    concatenator_proxy(concatenator_proxy&&) = default;
    concatenator_proxy& operator=(concatenator_proxy&&) = default;

    ~concatenator_proxy() = default;

    void registerProxy()
    {
        proxy_->uponSignal("concatenatedSignal").onInterface(INTERFACE_NAME).call([this](const std::string& concatenatedString){ this->onConcatenatedSignal(concatenatedString); });
    }

    virtual void onConcatenatedSignal(const std::string& concatenatedString) = 0;

    virtual void onConcatenateReply(const std::string& result, const sdbus::Error* error) = 0;

public:
    sdbus::PendingAsyncCall concatenate(const std::map<std::string, sdbus::Variant>& params)
    {
        return proxy_->callMethodAsync("concatenate").onInterface(INTERFACE_NAME).withArguments(params).uponReplyInvoke([this](const sdbus::Error* error, const std::string& result){ this->onConcatenateReply(result, error); });
    }

private:
    sdbus::IProxy* proxy_;
};

}}} // namespaces

#endif
