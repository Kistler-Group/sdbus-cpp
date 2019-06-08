/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 *
 * @file Types_test.cpp
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
using namespace std::string_literals;

namespace
{
    constexpr const uint64_t ANY_UINT64 = 84578348354;
    constexpr const double ANY_DOUBLE = 3.14;
}

/*-------------------------------------*/
/* --          TEST CASES           -- */
/*-------------------------------------*/

TEST(AVariant, CanBeDefaultConstructed)
{
    ASSERT_NO_THROW(sdbus::Variant());
}

TEST(AVariant, ContainsNoValueAfterDefaultConstructed)
{
    sdbus::Variant v;

    ASSERT_TRUE(v.isEmpty());
}

TEST(AVariant, CanBeConstructedFromASimpleValue)
{
    int value = 5;

    ASSERT_NO_THROW(sdbus::Variant{value});
}

TEST(AVariant, CanBeConstructedFromAComplexValue)
{
    using ComplexType = std::map<uint64_t, std::vector<sdbus::Struct<std::string, double>>>;
    ComplexType value{ {ANY_UINT64, ComplexType::mapped_type{sdbus::make_struct("hello", ANY_DOUBLE), sdbus::make_struct("world", ANY_DOUBLE)}} };

    ASSERT_NO_THROW(sdbus::Variant(value));
}

TEST(AVariant, CanBeCopied)
{
    auto value = "hello"s;
    sdbus::Variant variant(value);

    auto variantCopy1{variant};
    auto variantCopy2 = variantCopy1;

    ASSERT_THAT(variantCopy1.get<std::string>(), Eq(value));
    ASSERT_THAT(variantCopy2.get<std::string>(), Eq(value));
}

TEST(AVariant, IsNotEmptyWhenContainsAValue)
{
    sdbus::Variant v("hello");

    ASSERT_FALSE(v.isEmpty());
}

TEST(ASimpleVariant, ReturnsTheSimpleValueWhenAsked)
{
    int value = 5;

    sdbus::Variant variant(value);

    ASSERT_THAT(variant.get<int>(), Eq(value));
}

TEST(AComplexVariant, ReturnsTheComplexValueWhenAsked)
{
    using ComplexType = std::map<uint64_t, std::vector<sdbus::Struct<std::string, double>>>;
    ComplexType value{ {ANY_UINT64, ComplexType::mapped_type{sdbus::make_struct("hello", ANY_DOUBLE), sdbus::make_struct("world", ANY_DOUBLE)}} };

    sdbus::Variant variant(value);

    ASSERT_THAT(variant.get<decltype(value)>(), Eq(value));
}

TEST(AVariant, HasConceptuallyNonmutableGetMethodWhichCanBeCalledXTimes)
{
    std::string value{"I am a string"};
    sdbus::Variant variant(value);

    ASSERT_THAT(variant.get<std::string>(), Eq(value));
    ASSERT_THAT(variant.get<std::string>(), Eq(value));
    ASSERT_THAT(variant.get<std::string>(), Eq(value));
}

TEST(AVariant, ReturnsTrueWhenAskedIfItContainsTheTypeItReallyContains)
{
    using ComplexType = std::map<uint64_t, std::vector<sdbus::Struct<std::string, double>>>;
    ComplexType value{ {ANY_UINT64, ComplexType::mapped_type{sdbus::make_struct("hello", ANY_DOUBLE), sdbus::make_struct("world", ANY_DOUBLE)}} };

    sdbus::Variant variant(value);

    ASSERT_TRUE(variant.containsValueOfType<ComplexType>());
}

TEST(ASimpleVariant, ReturnsFalseWhenAskedIfItContainsTypeItDoesntReallyContain)
{
    int value = 5;

    sdbus::Variant variant(value);

    ASSERT_FALSE(variant.containsValueOfType<double>());
}

TEST(AVariant, CanContainOtherEmbeddedVariants)
{
    using TypeWithVariants = std::vector<sdbus::Struct<sdbus::Variant, double>>;
    TypeWithVariants value;
    value.emplace_back(sdbus::make_struct(sdbus::Variant("a string"), ANY_DOUBLE));
    value.emplace_back(sdbus::make_struct(sdbus::Variant(ANY_UINT64), ANY_DOUBLE));

    sdbus::Variant variant(value);

    ASSERT_TRUE(variant.containsValueOfType<TypeWithVariants>());
}

TEST(ANonEmptyVariant, SerializesSuccessfullyToAMessage)
{
    sdbus::Variant variant("a string");

    sdbus::Message msg = sdbus::createPlainMessage();

    ASSERT_NO_THROW(variant.serializeTo(msg));
}

TEST(AnEmptyVariant, ThrowsWhenBeingSerializedToAMessage)
{
    sdbus::Variant variant;

    sdbus::Message msg = sdbus::createPlainMessage();

    ASSERT_THROW(variant.serializeTo(msg), sdbus::Error);
}

TEST(ANonEmptyVariant, SerializesToAndDeserializesFromAMessageSuccessfully)
{
    using ComplexType = std::map<uint64_t, std::vector<sdbus::Struct<std::string, double>>>;
    ComplexType value{ {ANY_UINT64, ComplexType::mapped_type{sdbus::make_struct("hello", ANY_DOUBLE), sdbus::make_struct("world", ANY_DOUBLE)}} };
    sdbus::Variant variant(value);

    sdbus::Message msg = sdbus::createPlainMessage();
    variant.serializeTo(msg);
    msg.seal();
    sdbus::Variant variant2;
    variant2.deserializeFrom(msg);

    ASSERT_THAT(variant2.get<decltype(value)>(), Eq(value));
}

TEST(CopiesOfVariant, SerializeToAndDeserializeFromMessageSuccessfully)
{
    using ComplexType = std::map<uint64_t, std::vector<sdbus::Struct<std::string, double>>>;
    ComplexType value{ {ANY_UINT64, ComplexType::mapped_type{sdbus::make_struct("hello", ANY_DOUBLE), sdbus::make_struct("world", ANY_DOUBLE)}} };
    sdbus::Variant variant(value);
    auto variantCopy1{variant};
    auto variantCopy2 = variant;

    sdbus::Message msg = sdbus::createPlainMessage();
    variant.serializeTo(msg);
    variantCopy1.serializeTo(msg);
    variantCopy2.serializeTo(msg);
    msg.seal();
    sdbus::Variant receivedVariant1, receivedVariant2, receivedVariant3;
    receivedVariant1.deserializeFrom(msg);
    receivedVariant2.deserializeFrom(msg);
    receivedVariant3.deserializeFrom(msg);

    ASSERT_THAT(receivedVariant1.get<decltype(value)>(), Eq(value));
    ASSERT_THAT(receivedVariant2.get<decltype(value)>(), Eq(value));
    ASSERT_THAT(receivedVariant3.get<decltype(value)>(), Eq(value));
}

TEST(AStruct, CreatesStructFromTuple)
{
    std::tuple<int32_t, std::string> value{1234, "abcd"};
    sdbus::Struct<int32_t, std::string> valueStruct{value};

    ASSERT_THAT(std::get<0>(valueStruct), Eq(std::get<0>(value)));
    ASSERT_THAT(std::get<1>(valueStruct), Eq(std::get<1>(value)));
}

TEST(AnObjectPath, CanBeConstructedFromCString)
{
    const char* aPath = "/some/path";

    ASSERT_THAT(sdbus::ObjectPath{aPath}, Eq(aPath));
}

TEST(AnObjectPath, CanBeConstructedFromStdString)
{
    std::string aPath{"/some/path"};

    ASSERT_THAT(sdbus::ObjectPath{aPath}, Eq(aPath));
}

TEST(ASignature, CanBeConstructedFromCString)
{
    const char* aSignature = "us";

    ASSERT_THAT(sdbus::Signature{aSignature}, Eq(aSignature));
}

TEST(ASignature, CanBeConstructedFromStdString)
{
    std::string aSignature{"us"};

    ASSERT_THAT(sdbus::Signature{aSignature}, Eq(aSignature));
}

TEST(AUnixFd, CanBeConstructedFromInt)
{
    int fd{2};

    ASSERT_THAT((int)sdbus::UnixFd{fd}, Eq(fd));
}
