/**
 * (C) 2016 - 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2019 Stanislav Angelovic <angelovic.s@gmail.com>
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

namespace sdbus { namespace test {

class TestAdaptor final : public sdbus::AdaptorInterfaces< org::sdbuscpp::integrationtests_adaptor
                                                         , sdbus::Properties_adaptor
                                                         , sdbus::ObjectManager_adaptor >
{
public:
    TestAdaptor(sdbus::IConnection& connection);
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
    uint32_t sumVectorItems(const std::vector<uint16_t>& arg0, const std::vector<uint64_t>& arg1) override;
    uint32_t doOperation(const uint32_t& arg0) override;
    void doOperationAsync(sdbus::Result<uint32_t>&& result, uint32_t arg0) override;
    sdbus::Signature getSignature() override;
    sdbus::ObjectPath getObjectPath() override;
    sdbus::UnixFd getUnixFd() override;
    std::map<uint64_t, sdbus::Struct<std::map<uint8_t, std::vector<sdbus::Struct<sdbus::ObjectPath, bool, sdbus::Variant, std::map<int32_t, std::string>>>>, sdbus::Signature, std::string>> getComplex() override;
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
};

}}

#endif /* INTEGRATIONTESTS_TESTADAPTOR_H_ */
