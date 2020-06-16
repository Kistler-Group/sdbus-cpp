/**
 * (C) 2016 - 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2019 Stanislav Angelovic <angelovic.s@gmail.com>
 *
 * @file TestingAdaptor.h
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

#ifndef SDBUS_CPP_INTEGRATIONTESTS_TESTINGADAPTOR_H_
#define SDBUS_CPP_INTEGRATIONTESTS_TESTINGADAPTOR_H_

#include "adaptor-glue.h"
#include <thread>
#include <chrono>
#include <atomic>

class TestingAdaptor : public sdbus::AdaptorInterfaces< testing_adaptor
                                                      , sdbus::Properties_adaptor
                                                      , sdbus::ObjectManager_adaptor >
{
public:
    TestingAdaptor(sdbus::IConnection& connection) :
        AdaptorInterfaces(connection, OBJECT_PATH)
    {
        registerAdaptor();
    }

    ~TestingAdaptor()
    {
        unregisterAdaptor();
    }

protected:

    void noArgNoReturn() const override
    {
    }

    int32_t getInt() const override
    {
        return INT32_VALUE;
    }

    std::tuple<uint32_t, std::string> getTuple() const override
    {
        return std::make_tuple(UINT32_VALUE, STRING_VALUE);
    }

    double multiply(const int64_t& a, const double& b) const override
    {
        return a * b;
    }

    void multiplyWithNoReply(const int64_t& a, const double& b) const override
    {
        m_multiplyResult = a * b;
        m_wasMultiplyCalled = true;
    }

    std::vector<int16_t> getInts16FromStruct(const sdbus::Struct<uint8_t, int16_t, double, std::string, std::vector<int16_t>>& x) const override
    {
        std::vector<int16_t> res{x.get<1>()};
        auto y = std::get<std::vector<int16_t>>(x);
        res.insert(res.end(), y.begin(), y.end());
        return res;
    }

    sdbus::Variant processVariant(sdbus::Variant& v) override
    {
        sdbus::Variant res = static_cast<int32_t>(v.get<double>());
        return res;
    }

    std::map<int32_t, sdbus::Variant> getMapOfVariants(const std::vector<int32_t>& x, const sdbus::Struct<sdbus::Variant, sdbus::Variant>& y) const override
    {
        std::map<int32_t, sdbus::Variant> res;
        for (auto item : x)
        {
            res[item] = (item <= 0) ? std::get<0>(y) : std::get<1>(y);
        }
        return res;
    }

    sdbus::Struct<std::string, sdbus::Struct<std::map<int32_t, int32_t>>> getStructInStruct() const override
    {
        return sdbus::make_struct(STRING_VALUE, sdbus::make_struct(std::map<int32_t, int32_t>{{INT32_VALUE, INT32_VALUE}}));
    }

    int32_t sumStructItems(const sdbus::Struct<uint8_t, uint16_t>& a, const sdbus::Struct<int32_t, int64_t>& b) override
    {
        int32_t res{0};
        res += std::get<0>(a) + std::get<1>(a);
        res += std::get<0>(b) + std::get<1>(b);
        return res;
    }

    uint32_t sumVectorItems(const std::vector<uint16_t>& a, const std::vector<uint64_t>& b) override
    {
        uint32_t res{0};
        for (auto x : a)
        {
            res += x;
        }
        for (auto x : b)
        {
            res += x;
        }
        return res;
    }

    uint32_t doOperation(uint32_t param) override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(param));
        return param;
    }

    void doOperationAsync(uint32_t param, sdbus::Result<uint32_t> result) override
    {
        if (param == 0)
        {
            // Don't sleep and return the result from this thread
            result.returnResults(param);
        }
        else
        {
            // Process asynchronously in another thread and return the result from there
            std::thread([param, result = std::move(result)]()
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(param));
                result.returnResults(param);
            }).detach();
        }
    }

    sdbus::Signature getSignature() const override
    {
        return SIGNATURE_VALUE;
    }
    sdbus::ObjectPath getObjPath() const override
    {
        return OBJECT_PATH_VALUE;
    }
    sdbus::UnixFd getUnixFd() const override
    {
        return sdbus::UnixFd{UNIX_FD_VALUE};
    }

    ComplexType getComplex() const override
    {
        return { // map
            {
                0,  // uint_64_t
                {   // struct
                    {   // map
                        {
                            'a',  // uint8_t
                            {   // vector
                                {   // struct
                                        "/object/path", // object path
                                        false,
                                        3.14,
                                        {   // map
                                            {0, "zero"}
                                        }
                                }
                            }
                        }
                    },
                    "a{t(a{ya(obva{is})}gs)}", // signature
                    ""
                }
            }
        };
    }

    void throwError() const override
    {
        m_wasThrowErrorCalled = true;
        throw sdbus::createError(1, "A test error occurred");
    }


    void emitTwoSimpleSignals() override
    {
        emitSimpleSignal();
        emitSignalWithMap({});
    }

    std::string state() override
    {
        return m_state;
    }

    uint32_t action() override
    {
        return m_action;
    }

    void action(const uint32_t& value) override
    {
        m_action = value;
    }

    bool blocking() override
    {
        return m_blocking;
    }

    void blocking(const bool& value) override
    {
        m_blocking = value;
    }

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



#endif /* INTEGRATIONTESTS_TESTINGADAPTOR_H_ */
