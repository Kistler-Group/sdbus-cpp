/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2022 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file TestAdaptor.h
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

#ifndef SDBUS_CPP_INTEGRATIONTESTS_TESTADAPTOR_H_
#define SDBUS_CPP_INTEGRATIONTESTS_TESTADAPTOR_H_

#include "integrationtests-adaptor.h"
#include "Defs.h"
#include <thread>
#include <chrono>
#include <atomic>
#include <utility>
#include <memory>

namespace sdbus { namespace test {

class ObjectManagerTestAdaptor final : public sdbus::AdaptorInterfaces< sdbus::ObjectManager_adaptor >
{
public:
    ObjectManagerTestAdaptor(sdbus::IConnection& connection, std::string path) :
        AdaptorInterfaces(connection, std::move(path))
    {
        registerAdaptor();
    }

    ~ObjectManagerTestAdaptor()
    {
        unregisterAdaptor();
    }
};

class TestAdaptor final : public sdbus::AdaptorInterfaces< org::sdbuscpp::integrationtests_adaptor
                                                         , sdbus::Properties_adaptor
                                                         , sdbus::ManagedObject_adaptor >
{
public:
    TestAdaptor(sdbus::IConnection& connection, const std::string& path);
    ~TestAdaptor();

protected:
    void noArgNoReturn() override;
    int32_t getInt() override;
    std::tuple<uint32_t, std::string> getTuple() override;
    double multiply(const int64_t& a, const double& b) override;
    void multiplyWithNoReply(const int64_t& a, const double& b) override;
    std::vector<int16_t> getInts16FromStruct(const sdbus::Struct<uint8_t, int16_t, double, std::string, std::vector<int16_t>>& arg0) override;
    sdbus::Variant processVariant(const sdbus::Variant& variant) override;
    std::map<int32_t, sdbus::Variant> getMapOfVariants(const std::vector<int32_t>& x, const sdbus::Struct<sdbus::Variant, sdbus::Variant>& y) override;
    sdbus::Struct<std::string, sdbus::Struct<std::map<int32_t, int32_t>>> getStructInStruct() override;
    int32_t sumStructItems(const sdbus::Struct<uint8_t, uint16_t>& arg0, const sdbus::Struct<int32_t, int64_t>& arg1) override;
    uint32_t sumArrayItems(const std::vector<uint16_t>& arg0, const std::array<uint64_t, 3>& arg1) override;
    uint32_t doOperation(const uint32_t& arg0) override;
    void doOperationAsync(sdbus::Result<uint32_t>&& result, uint32_t arg0) override;
    sdbus::Signature getSignature() override;
    sdbus::ObjectPath getObjPath() override;
    sdbus::UnixFd getUnixFd() override;
    std::unordered_map<uint64_t, sdbus::Struct<std::map<uint8_t, std::vector<sdbus::Struct<sdbus::ObjectPath, bool, sdbus::Variant, std::map<int32_t, std::string>>>>, sdbus::Signature, std::string>> getComplex() override;
    void throwError() override;
    void throwErrorWithNoReply() override;
    void doPrivilegedStuff() override;
    void emitTwoSimpleSignals() override;

    uint32_t action() override;
    void action(const uint32_t& value) override;
    bool blocking() override;
    void blocking(const bool& value) override;
    std::string state() override;

public:
    void emitSignalWithoutRegistration(const sdbus::Struct<std::string, sdbus::Struct<sdbus::Signature>>& s);
    std::string getExpectedXmlApiDescription() const;

private:
    const std::string m_state{DEFAULT_STATE_VALUE};
    uint32_t m_action{DEFAULT_ACTION_VALUE};
    bool m_blocking{DEFAULT_BLOCKING_VALUE};

public: // for tests
    // For dont-expect-reply method call verifications
    mutable std::atomic<bool> m_wasMultiplyCalled{false};
    mutable double m_multiplyResult{};
    mutable std::atomic<bool> m_wasThrowErrorCalled{false};

    std::unique_ptr<const Message> m_methodCallMsg;
    std::string m_methodCallMemberName;
    std::unique_ptr<const Message> m_propertySetMsg;
    std::string m_propertySetSender;
};

class DummyTestAdaptor final : public sdbus::AdaptorInterfaces< org::sdbuscpp::integrationtests_adaptor
                                                              , sdbus::Properties_adaptor
                                                              , sdbus::ManagedObject_adaptor >
{
public:
    DummyTestAdaptor(sdbus::IConnection& connection, const std::string& path) : AdaptorInterfaces(connection, path) {}

protected:
    void noArgNoReturn() override {}
    int32_t getInt() override { return {}; }
    std::tuple<uint32_t, std::string> getTuple() override { return {}; }
    double multiply(const int64_t&, const double&) override { return {}; }
    void multiplyWithNoReply(const int64_t&, const double&) override {}
    std::vector<int16_t> getInts16FromStruct(const sdbus::Struct<uint8_t, int16_t, double, std::string, std::vector<int16_t>>&) override { return {}; }
    sdbus::Variant processVariant(const sdbus::Variant&) override { return {}; }
    std::map<int32_t, sdbus::Variant> getMapOfVariants(const std::vector<int32_t>&, const sdbus::Struct<sdbus::Variant, sdbus::Variant>&) override { return {}; }
    sdbus::Struct<std::string, sdbus::Struct<std::map<int32_t, int32_t>>> getStructInStruct() override { return {}; }
    int32_t sumStructItems(const sdbus::Struct<uint8_t, uint16_t>&, const sdbus::Struct<int32_t, int64_t>&) override { return {}; }
    uint32_t sumArrayItems(const std::vector<uint16_t>&, const std::array<uint64_t, 3>&) override { return {}; }
    uint32_t doOperation(const uint32_t&) override { return {}; }
    void doOperationAsync(sdbus::Result<uint32_t>&&, uint32_t) override {}
    sdbus::Signature getSignature() override { return {}; }
    sdbus::ObjectPath getObjPath() override { return {}; }
    sdbus::UnixFd getUnixFd() override { return {}; }
    std::unordered_map<uint64_t, sdbus::Struct<std::map<uint8_t, std::vector<sdbus::Struct<sdbus::ObjectPath, bool, sdbus::Variant, std::map<int32_t, std::string>>>>, sdbus::Signature, std::string>> getComplex() override { return {}; }
    void throwError() override {}
    void throwErrorWithNoReply() override {}
    void doPrivilegedStuff() override {}
    void emitTwoSimpleSignals() override {}

    uint32_t action() override { return {}; }
    void action(const uint32_t&) override {}
    bool blocking() override { return {}; }
    void blocking(const bool&) override {}
    std::string state() override { return {}; }
};

}}

#endif /* INTEGRATIONTESTS_TESTADAPTOR_H_ */
