/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2022 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file TestProxy.cpp
 *
 * Created on: May 23, 2020
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

#include "TestProxy.h"
#include <thread>
#include <chrono>
#include <atomic>

namespace sdbus { namespace test {

TestProxy::TestProxy(ServiceName destination, ObjectPath objectPath)
    : ProxyInterfaces(std::move(destination), std::move(objectPath))
{
    getProxy().uponSignal("signalWithoutRegistration").onInterface(sdbus::test::INTERFACE_NAME).call([this](const sdbus::Struct<std::string, sdbus::Struct<sdbus::Signature>>& s){ this->onSignalWithoutRegistration(s); });

    registerProxy();
}

TestProxy::TestProxy(ServiceName destination, ObjectPath objectPath, dont_run_event_loop_thread_t)
    : ProxyInterfaces(std::move(destination), std::move(objectPath), dont_run_event_loop_thread)
{
    // It doesn't make sense to register any signals here since proxy upon a D-Bus connection with no event loop thread
    // will not receive any incoming messages except replies to synchronous D-Bus calls.
}

TestProxy::TestProxy(sdbus::IConnection& connection, ServiceName destination, ObjectPath objectPath)
    : ProxyInterfaces(connection, std::move(destination), std::move(objectPath))
{
    getProxy().uponSignal("signalWithoutRegistration").onInterface(sdbus::test::INTERFACE_NAME).call([this](const sdbus::Struct<std::string, sdbus::Struct<sdbus::Signature>>& s){ this->onSignalWithoutRegistration(s); });

    registerProxy();
}

TestProxy::~TestProxy()
{
    unregisterProxy();
}

void TestProxy::onSimpleSignal()
{
    m_signalMsg = std::make_unique<sdbus::Message>(getProxy().getCurrentlyProcessedMessage());
    m_signalName = m_signalMsg->getMemberName();

    m_gotSimpleSignal = true;
}

void TestProxy::onSignalWithMap(const std::map<int32_t, std::string>& aMap)
{
    m_mapFromSignal = aMap;
    m_gotSignalWithMap = true;
}

void TestProxy::onSignalWithVariant(const sdbus::Variant& aVariant)
{
    m_variantFromSignal = aVariant.get<double>();
    m_gotSignalWithVariant = true;
}

void TestProxy::onSignalWithoutRegistration(const sdbus::Struct<std::string, sdbus::Struct<sdbus::Signature>>& s)
{
    m_signatureFromSignal[std::get<0>(s)] = static_cast<std::string>(std::get<0>(std::get<1>(s)));
    m_gotSignalWithSignature = true;
}

void TestProxy::onDoOperationReply(uint32_t returnValue, std::optional<sdbus::Error> error)
{
    if (m_DoOperationClientSideAsyncReplyHandler)
        m_DoOperationClientSideAsyncReplyHandler(returnValue, error);
}

void TestProxy::onPropertiesChanged( const sdbus::InterfaceName& interfaceName
                                   , const std::map<std::string, sdbus::Variant>& changedProperties
                                   , const std::vector<std::string>& invalidatedProperties )
{
    if (m_onPropertiesChangedHandler)
        m_onPropertiesChangedHandler(interfaceName, changedProperties, invalidatedProperties);
}

void TestProxy::installDoOperationClientSideAsyncReplyHandler(std::function<void(uint32_t res, std::optional<sdbus::Error> err)> handler)
{
    m_DoOperationClientSideAsyncReplyHandler = std::move(handler);
}

uint32_t TestProxy::doOperationWithTimeout(const std::chrono::microseconds &timeout, uint32_t param)
{
    using namespace std::chrono_literals;
    uint32_t result;
    getProxy().callMethod("doOperation").onInterface(sdbus::test::INTERFACE_NAME).withTimeout(timeout).withArguments(param).storeResultsTo(result);
    return result;
}

sdbus::PendingAsyncCall TestProxy::doOperationClientSideAsync(uint32_t param)
{
    return getProxy().callMethodAsync("doOperation")
                     .onInterface(sdbus::test::INTERFACE_NAME)
                     .withArguments(param)
                     .uponReplyInvoke([this](std::optional<sdbus::Error> error, uint32_t returnValue)
                                      {
                                          this->onDoOperationReply(returnValue, std::move(error));
                                      });
}

std::future<uint32_t> TestProxy::doOperationClientSideAsync(uint32_t param, with_future_t)
{
    return getProxy().callMethodAsync("doOperation")
                     .onInterface(sdbus::test::INTERFACE_NAME)
                     .withArguments(param)
                     .getResultAsFuture<uint32_t>();
}

std::future<MethodReply> TestProxy::doOperationClientSideAsyncOnBasicAPILevel(uint32_t param)
{
    auto methodCall = getProxy().createMethodCall(sdbus::test::INTERFACE_NAME, sdbus::MethodName{"doOperation"});
    methodCall << param;

    return getProxy().callMethodAsync(methodCall, sdbus::with_future);
}

void TestProxy::doErroneousOperationClientSideAsync()
{
    getProxy().callMethodAsync("throwError")
              .onInterface(sdbus::test::INTERFACE_NAME)
              .uponReplyInvoke([this](std::optional<sdbus::Error> error)
                               {
                                   this->onDoOperationReply(0, std::move(error));
                               });
}

std::future<void> TestProxy::doErroneousOperationClientSideAsync(with_future_t)
{
    return getProxy().callMethodAsync("throwError")
                     .onInterface(sdbus::test::INTERFACE_NAME)
                     .getResultAsFuture<>();
}

void TestProxy::doOperationClientSideAsyncWithTimeout(const std::chrono::microseconds &timeout, uint32_t param)
{
    using namespace std::chrono_literals;
    getProxy().callMethodAsync("doOperation")
              .onInterface(sdbus::test::INTERFACE_NAME)
              .withTimeout(timeout)
              .withArguments(param)
              .uponReplyInvoke([this](std::optional<sdbus::Error> error, uint32_t returnValue)
                               {
                                   this->onDoOperationReply(returnValue, std::move(error));
                               });
}

int32_t TestProxy::callNonexistentMethod()
{
    int32_t result;
    getProxy().callMethod("callNonexistentMethod").onInterface(sdbus::test::INTERFACE_NAME).storeResultsTo(result);
    return result;
}

int32_t TestProxy::callMethodOnNonexistentInterface()
{
    sdbus::InterfaceName nonexistentInterfaceName{"sdbuscpp.interface.that.does.not.exist"};
    int32_t result;
    getProxy().callMethod("someMethod").onInterface(nonexistentInterfaceName).storeResultsTo(result);
    return result;
}

void TestProxy::setStateProperty(const std::string& value)
{
    getProxy().setProperty("state").onInterface(sdbus::test::INTERFACE_NAME).toValue(value);
}

}}
