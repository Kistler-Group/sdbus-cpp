/**
 * (C) 2016 - 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2019 Stanislav Angelovic <angelovic.s@gmail.com>
 *
 * @file Message_test.cpp
 *
 * Created on: Dec 3, 2016
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

#include <sdbus-c++/Types.h>
#include "MessageUtils.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstdint>

using ::testing::Eq;
using ::testing::Gt;
using ::testing::DoubleEq;
using namespace std::string_literals;

namespace
{
    std::string deserializeString(sdbus::PlainMessage& msg)
    {
        std::string str;
        msg >> str;
        return str;
    }
}

/*-------------------------------------*/
/* --          TEST CASES           -- */
/*-------------------------------------*/

TEST(AMessage, CanBeDefaultConstructed)
{
    ASSERT_NO_THROW(sdbus::PlainMessage());
}

TEST(AMessage, IsInvalidAfterDefaultConstructed)
{
    sdbus::PlainMessage msg;

    ASSERT_FALSE(msg.isValid());
}

TEST(AMessage, IsValidWhenConstructedAsRealMessage)
{
    auto msg = sdbus::createPlainMessage();
    ASSERT_TRUE(msg.isValid());
}

TEST(AMessage, CreatesShallowCopyWhenCopyConstructed)
{
    auto msg = sdbus::createPlainMessage();
    msg << "I am a string"s;
    msg.seal();

    sdbus::PlainMessage msgCopy = msg;

    std::string str;
    msgCopy >> str;

    ASSERT_THAT(str, Eq("I am a string"));
    ASSERT_THROW(msgCopy >> str, sdbus::Error);
}

TEST(AMessage, CreatesDeepCopyWhenEplicitlyCopied)
{
    auto msg = sdbus::createPlainMessage();
    msg << "I am a string"s;
    msg.seal();

    auto msgCopy = sdbus::createPlainMessage();
    msg.copyTo(msgCopy, true);
    msgCopy.seal(); // Seal to be able to read from it subsequently
    msg.rewind(true); // Rewind to the beginning after copying

    ASSERT_THAT(deserializeString(msg), Eq("I am a string"));
    ASSERT_THAT(deserializeString(msgCopy), Eq("I am a string"));
}

TEST(AMessage, IsEmptyWhenContainsNoValue)
{
    auto msg = sdbus::createPlainMessage();

    ASSERT_TRUE(msg.isEmpty());
}

TEST(AMessage, IsNotEmptyWhenContainsAValue)
{
    auto msg = sdbus::createPlainMessage();
    msg << "I am a string"s;

    ASSERT_FALSE(msg.isEmpty());
}

TEST(AMessage, CanCarryASimpleInteger)
{
    auto msg = sdbus::createPlainMessage();

    int dataWritten = 5;

    msg << dataWritten;
    msg.seal();

    int dataRead;
    msg >> dataRead;

    ASSERT_THAT(dataRead, Eq(dataWritten));
}

TEST(AMessage, CanCarryAUnixFd)
{
    auto msg = sdbus::createPlainMessage();

    sdbus::UnixFd dataWritten = 0;
    msg << dataWritten;

    msg.seal();

    sdbus::UnixFd dataRead;
    msg >> dataRead;

    ASSERT_THAT(dataRead, Gt(dataWritten));
}

TEST(AMessage, CanCarryAVariant)
{
    auto msg = sdbus::createPlainMessage();

    auto dataWritten = sdbus::Variant((double)3.14);

    msg << dataWritten;
    msg.seal();

    sdbus::Variant dataRead;
    msg >> dataRead;

    ASSERT_THAT(dataRead.get<double>(), Eq(dataWritten.get<double>()));
}

TEST(AMessage, CanCarryACollectionOfEmbeddedVariants)
{
    auto msg = sdbus::createPlainMessage();

    auto value = std::vector<sdbus::Variant>{"hello"s, (double)3.14};
    auto dataWritten = sdbus::Variant{value};

    msg << dataWritten;
    msg.seal();

    sdbus::Variant dataRead;
    msg >> dataRead;

    ASSERT_THAT(dataRead.get<decltype(value)>()[0].get<std::string>(), Eq(value[0].get<std::string>()));
    ASSERT_THAT(dataRead.get<decltype(value)>()[1].get<double>(), Eq(value[1].get<double>()));
}

TEST(AMessage, CanCarryAnArray)
{
    auto msg = sdbus::createPlainMessage();

    std::vector<int64_t> dataWritten{3545342, 43643532, 324325};

    msg << dataWritten;
    msg.seal();

    std::vector<int64_t> dataRead;
    msg >> dataRead;

    ASSERT_THAT(dataRead, Eq(dataWritten));
}

TEST(AMessage, CanCarryADictionary)
{
    auto msg = sdbus::createPlainMessage();

    std::map<int, std::string> dataWritten{{1, "one"}, {2, "two"}};

    msg << dataWritten;
    msg.seal();

    std::map<int, std::string> dataRead;
    msg >> dataRead;

    ASSERT_THAT(dataRead, Eq(dataWritten));
}

TEST(AMessage, CanCarryAComplexType)
{
    auto msg = sdbus::createPlainMessage();

    using ComplexType = std::map<
                            uint64_t,
                            sdbus::Struct<
                                std::map<
                                    uint8_t,
                                    std::vector<
                                        sdbus::Struct<
                                            sdbus::ObjectPath,
                                            bool,
                                            int16_t,
                                            /*sdbus::Variant,*/
                                            std::map<int, std::string>
                                        >
                                    >
                                >,
                                sdbus::Signature,
                                double
                            >
                        >;

    ComplexType dataWritten = { {1, {{{5, {{"/some/object", true, 45, {{6, "hello"}, {7, "world"}}}}}}, "av", 3.14}}};

    msg << dataWritten;
    msg.seal();

    ComplexType dataRead;
    msg >> dataRead;

    ASSERT_THAT(dataRead, Eq(dataWritten));
}
