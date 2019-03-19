/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
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

class TestingAdaptor : public sdbus::Interfaces<testing_adaptor>
{
public:
    TestingAdaptor(sdbus::IConnection& connection) :
        sdbus::Interfaces<::testing_adaptor>(connection, OBJECT_PATH) { }

    virtual ~TestingAdaptor() { }

    bool wasMultiplyCalled() const { return m_multiplyCalled; }
    double getMultiplyResult() const { return m_multiplyResult; }
    bool wasThrowErrorCalled() const { return m_throwErrorCalled; }

protected:

    void noArgNoReturn() const {}

    int32_t getInt() const { return INT32_VALUE; }

    std::tuple<uint32_t, std::string> getTuple() const { }

    double multiply(const int64_t& a, const double& b) const { return a * b; }

    void multiplyWithNoReply(const int64_t& a, const double& b) const
    {
        m_multiplyResult = a * b;
        m_multiplyCalled = true;
    }

    std::vector<int16_t> getInts16FromStruct(const sdbus::Struct<uint8_t, int16_t, double, std::string, std::vector<int16_t>>& x) const
    {
        std::vector<int16_t> res{x.get<1>()};
        auto y = std::get<std::vector<int16_t>>(x);
        res.insert(res.end(), y.begin(), y.end());
        return res;
    }

    sdbus::Variant processVariant(sdbus::Variant& v)
    {
        sdbus::Variant res = static_cast<int32_t>(v.get<double>());
        return res;
    }

    std::map<int32_t, sdbus::Variant> getMapOfVariants(const std::vector<int32_t>& x, const sdbus::Struct<sdbus::Variant, sdbus::Variant>& y) const
    {
        std::map<int32_t, sdbus::Variant> res;
        for (auto item : x)
        {
            res[item] = (item <= 0) ? std::get<0>(y) : std::get<1>(y);
        }
        return res;
    }

    sdbus::Struct<std::string, sdbus::Struct<std::map<int32_t, int32_t>>> getStructInStruct() const
    {
        return sdbus::make_struct(STRING_VALUE, sdbus::make_struct(std::map<int32_t, int32_t>{{INT32_VALUE, INT32_VALUE}}));
    }

    int32_t sumStructItems(const sdbus::Struct<uint8_t, uint16_t>& a, const sdbus::Struct<int32_t, int64_t>& b)
    {
        int32_t res{0};
        res += std::get<0>(a) + std::get<1>(a);
        res += std::get<0>(b) + std::get<1>(b);
        return res;
    }

    uint32_t sumVectorItems(const std::vector<uint16_t>& a, const std::vector<uint64_t>& b)
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

    uint32_t doOperation(uint32_t param)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(param));
        return param;
    }

    void doOperationAsync(uint32_t param, sdbus::Result<uint32_t> result)
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

    sdbus::Signature getSignature() const { return SIGNATURE_VALUE; }
    sdbus::ObjectPath getObjectPath() const { return OBJECT_PATH_VALUE; }

    ComplexType getComplex() const
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

    void throwError() const
    {
        m_throwErrorCalled = true;
        throw sdbus::createError(1, "A test error occurred");
    }

    std::string state() { return STRING_VALUE; }
    uint32_t action() { return m_action; }
    void action(const uint32_t& value) { m_action = value; }
    bool blocking() { return m_blocking; }
    void blocking(const bool& value) { m_blocking = value; }

private:
    uint32_t m_action;
    bool m_blocking;

    // For dont-expect-reply method call verifications
    mutable std::atomic<bool> m_multiplyCalled{};
    mutable double m_multiplyResult{};
    mutable std::atomic<bool> m_throwErrorCalled{};
};



#endif /* INTEGRATIONTESTS_TESTINGADAPTOR_H_ */
