/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2024 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file TestProxy.h
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

#ifndef SDBUS_CPP_INTEGRATIONTESTS_TESTPROXY_H_
#define SDBUS_CPP_INTEGRATIONTESTS_TESTPROXY_H_

#include "integrationtests-proxy.h"
#include "Defs.h"
#include <thread>
#include <chrono>
#include <atomic>
#include <future>
#include <memory>

namespace sdbus { namespace test {

class ObjectManagerTestProxy final : public sdbus::ProxyInterfaces< sdbus::ObjectManager_proxy >
{
public:
    ObjectManagerTestProxy(sdbus::IConnection& connection, ServiceName destination, ObjectPath objectPath)
        : ProxyInterfaces(connection, std::move(destination), std::move(objectPath))
    {
        registerProxy();
    }

    ~ObjectManagerTestProxy()
    {
        unregisterProxy();
    }
protected:
    void onInterfacesAdded(const sdbus::ObjectPath& objectPath, const std::map<sdbus::InterfaceName, std::map<PropertyName, sdbus::Variant>>& interfacesAndProperties) override
    {
        if (m_onInterfacesAddedHandler)
            m_onInterfacesAddedHandler(objectPath, interfacesAndProperties);
    }

    void onInterfacesRemoved(const sdbus::ObjectPath& objectPath, const std::vector<sdbus::InterfaceName>& interfaces) override
    {
        if (m_onInterfacesRemovedHandler)
            m_onInterfacesRemovedHandler(objectPath, interfaces);
    }

public: // for tests
    std::function<void(const sdbus::ObjectPath&, const std::map<sdbus::InterfaceName, std::map<PropertyName, sdbus::Variant>>&)> m_onInterfacesAddedHandler;
    std::function<void(const sdbus::ObjectPath&, const std::vector<sdbus::InterfaceName>&)> m_onInterfacesRemovedHandler;
};

class TestProxy final : public sdbus::ProxyInterfaces< org::sdbuscpp::integrationtests_proxy
                                                     , sdbus::Peer_proxy
                                                     , sdbus::Introspectable_proxy
                                                     , sdbus::Properties_proxy >
{
public:
    TestProxy(ServiceName destination, ObjectPath objectPath);
    TestProxy(ServiceName destination, ObjectPath objectPath, dont_run_event_loop_thread_t);
    TestProxy(sdbus::IConnection& connection, ServiceName destination, ObjectPath objectPath);
    ~TestProxy();

protected:
    void onSimpleSignal() override;
    void onSignalWithMap(const std::map<int32_t, std::string>& aMap) override;
    void onSignalWithVariant(const sdbus::Variant& aVariant) override;

    void onSignalWithoutRegistration(const sdbus::Struct<std::string, sdbus::Struct<sdbus::Signature>>& s);
    void onDoOperationReply(uint32_t returnValue, std::optional<sdbus::Error> error);

    // Signals of standard D-Bus interfaces
    void onPropertiesChanged( const sdbus::InterfaceName& interfaceName
                            , const std::map<PropertyName, sdbus::Variant>& changedProperties
                            , const std::vector<PropertyName>& invalidatedProperties ) override;

public:
    void installDoOperationClientSideAsyncReplyHandler(std::function<void(uint32_t res, std::optional<sdbus::Error> err)> handler);
    uint32_t doOperationWithTimeout(const std::chrono::microseconds &timeout, uint32_t param);
    MethodReply doOperationOnBasicAPILevel(uint32_t param);
    sdbus::PendingAsyncCall doOperationClientSideAsync(uint32_t param);
    [[nodiscard]] sdbus::Slot doOperationClientSideAsync(uint32_t param, sdbus::return_slot_t);
    std::future<uint32_t> doOperationClientSideAsync(uint32_t param, with_future_t);
    std::future<std::map<int32_t, std::string>> doOperationWithLargeDataClientSideAsync(const std::map<int32_t, std::string>& largeParam, with_future_t);
    std::future<MethodReply> doOperationClientSideAsyncOnBasicAPILevel(uint32_t param);
    std::future<void> doErroneousOperationClientSideAsync(with_future_t);
    void doErroneousOperationClientSideAsync();
    void doOperationClientSideAsyncWithTimeout(const std::chrono::microseconds &timeout, uint32_t param);
    int32_t callNonexistentMethod();
    int32_t callMethodOnNonexistentInterface();
    void setStateProperty(const std::string& value);

//private:
public: // for tests
    int m_SimpleSignals = 0;
    std::atomic<bool> m_gotSimpleSignal{false};
    std::atomic<bool> m_gotSignalWithMap{false};
    std::map<int32_t, std::string> m_mapFromSignal;
    std::atomic<bool> m_gotSignalWithVariant{false};
    double m_variantFromSignal;
    std::atomic<bool> m_gotSignalWithSignature{false};
    std::map<std::string, Signature> m_signatureFromSignal;

    std::function<void(uint32_t res, std::optional<sdbus::Error> err)> m_DoOperationClientSideAsyncReplyHandler;
    std::function<void(const sdbus::InterfaceName&, const std::map<PropertyName, sdbus::Variant>&, const std::vector<PropertyName>&)> m_onPropertiesChangedHandler;

    std::unique_ptr<const MethodCall> m_methodCallMsg;
    std::unique_ptr<const Message> m_signalMsg;
    SignalName m_signalName;
};

class DummyTestProxy final : public sdbus::ProxyInterfaces< org::sdbuscpp::integrationtests_proxy
                                                          , sdbus::Peer_proxy
                                                          , sdbus::Introspectable_proxy
                                                          , sdbus::Properties_proxy >
{
public:
    DummyTestProxy(ServiceName destination, ObjectPath objectPath)
        : ProxyInterfaces(destination, objectPath)
    {
    }

protected:
    void onSimpleSignal() override {}
    void onSignalWithMap(const std::map<int32_t, std::string>&) override {}
    void onSignalWithVariant(const sdbus::Variant&) override {}

    void onSignalWithoutRegistration(const sdbus::Struct<std::string, sdbus::Struct<sdbus::Signature>>&) {}
    void onDoOperationReply(uint32_t, std::optional<sdbus::Error>) {}

    // Signals of standard D-Bus interfaces
    void onPropertiesChanged(const InterfaceName&, const std::map<PropertyName, sdbus::Variant>&, const std::vector<PropertyName>&) override {}
};

}}

#endif /* SDBUS_CPP_INTEGRATIONTESTS_TESTPROXY_H_ */
