/**
 * (C) 2016 - 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2019 Stanislav Angelovic <angelovic.s@gmail.com>
 *
 * @file TestingProxy.h
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

#ifndef SDBUS_CPP_INTEGRATIONTESTS_TESTINGPROXY_H_
#define SDBUS_CPP_INTEGRATIONTESTS_TESTINGPROXY_H_

#include "proxy-glue.h"
#include <atomic>

class TestingProxy : public sdbus::ProxyInterfaces< ::testing_proxy
                                                  , sdbus::Peer_proxy
                                                  , sdbus::Introspectable_proxy
                                                  , sdbus::Properties_proxy
                                                  , sdbus::ObjectManager_proxy >
{
public:
    TestingProxy(std::string destination, std::string objectPath)
        : ProxyInterfaces(std::move(destination), std::move(objectPath))
    {
        registerProxy();
    }

    ~TestingProxy()
    {
        unregisterProxy();
    }

    void installDoOperationClientSideAsyncReplyHandler(std::function<void(uint32_t res, const sdbus::Error* err)> handler)
    {
        m_DoOperationClientSideAsyncReplyHandler = handler;
    }

protected:
    void onSimpleSignal() override
    {
        m_gotSimpleSignal = true;
    }

    void onSignalWithMap(const std::map<int32_t, std::string>& m) override
    {
        m_mapFromSignal = m;
        m_gotSignalWithMap = true;
    }

    void onSignalWithVariant(const sdbus::Variant& v) override
    {
        m_variantFromSignal = v.get<double>();
        m_gotSignalWithVariant = true;
    }

    void onSignalWithoutRegistration(const sdbus::Struct<std::string, sdbus::Struct<sdbus::Signature>>& s) override
    {
        m_signatureFromSignal[std::get<0>(s)] = static_cast<std::string>(std::get<0>(std::get<1>(s)));
        m_gotSignalWithSignature = true;
    }

    void onDoOperationReply(uint32_t returnValue, const sdbus::Error* error) override
    {
        if (m_DoOperationClientSideAsyncReplyHandler)
            m_DoOperationClientSideAsyncReplyHandler(returnValue, error);
    }

    // Signals of standard D-Bus interfaces

    void onPropertiesChanged( const std::string& interfaceName
                            , const std::map<std::string, sdbus::Variant>& changedProperties
                            , const std::vector<std::string>& invalidatedProperties ) override
    {
        if (m_onPropertiesChangedHandler)
            m_onPropertiesChangedHandler(interfaceName, changedProperties, invalidatedProperties);
    }

    void onInterfacesAdded( const sdbus::ObjectPath& objectPath
                          , const std::map<std::string, std::map<std::string, sdbus::Variant>>& interfacesAndProperties) override
    {
        if (m_onInterfacesAddedHandler)
            m_onInterfacesAddedHandler(objectPath, interfacesAndProperties);
    }
    void onInterfacesRemoved( const sdbus::ObjectPath& objectPath
                            , const std::vector<std::string>& interfaces) override
    {
        if (m_onInterfacesRemovedHandler)
            m_onInterfacesRemovedHandler(objectPath, interfaces);
    }

//private:
public: // for tests
    std::atomic<bool> m_gotSimpleSignal{false};
    std::atomic<bool> m_gotSignalWithMap{false};
    std::map<int32_t, std::string> m_mapFromSignal;
    std::atomic<bool> m_gotSignalWithVariant{false};
    double m_variantFromSignal;
    std::atomic<bool> m_gotSignalWithSignature{false};
    std::map<std::string, std::string> m_signatureFromSignal;

    std::function<void(uint32_t res, const sdbus::Error* err)> m_DoOperationClientSideAsyncReplyHandler;
    std::function<void(const std::string&, const std::map<std::string, sdbus::Variant>&, const std::vector<std::string>&)> m_onPropertiesChangedHandler;
    std::function<void(const sdbus::ObjectPath&, const std::map<std::string, std::map<std::string, sdbus::Variant>>&)> m_onInterfacesAddedHandler;
    std::function<void(const sdbus::ObjectPath&, const std::vector<std::string>&)> m_onInterfacesRemovedHandler;
};

#endif /* SDBUS_CPP_INTEGRATIONTESTS_TESTINGPROXY_H_ */
