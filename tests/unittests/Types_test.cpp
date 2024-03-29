/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2022 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
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
#include <sys/eventfd.h>

using ::testing::Eq;
using ::testing::Gt;
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
    ComplexType value{ {ANY_UINT64, ComplexType::mapped_type{{"hello"s, ANY_DOUBLE}, {"world"s, ANY_DOUBLE}}} };

    ASSERT_NO_THROW(sdbus::Variant{value});
}

TEST(AVariant, CanBeConstructedFromAnStdVariant)
{
    using ComplexType = std::vector<sdbus::Struct<std::string, double>>;
    using StdVariantType = std::variant<std::string, uint64_t, ComplexType>;
    ComplexType value{{"hello"s, ANY_DOUBLE}, {"world"s, ANY_DOUBLE}};
    StdVariantType stdVariant{value};

    sdbus::Variant sdbusVariant{stdVariant};

    ASSERT_TRUE(sdbusVariant.containsValueOfType<ComplexType>());
    ASSERT_THAT(sdbusVariant.get<ComplexType>(), Eq(value));
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
    ComplexType value{ {ANY_UINT64, ComplexType::mapped_type{{"hello"s, ANY_DOUBLE}, {"world"s, ANY_DOUBLE}}} };

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
    ComplexType value{ {ANY_UINT64, ComplexType::mapped_type{{"hello"s, ANY_DOUBLE}, {"world"s, ANY_DOUBLE}}} };

    sdbus::Variant variant(value);

    ASSERT_TRUE(variant.containsValueOfType<ComplexType>());
}

TEST(AVariant, CanBeConvertedIntoAnStdVariant)
{
    using ComplexType = std::vector<sdbus::Struct<std::string, double>>;
    using StdVariantType = std::variant<std::string, uint64_t, ComplexType>;
    ComplexType value{{"hello"s, ANY_DOUBLE}, {"world"s, ANY_DOUBLE}};
    sdbus::Variant sdbusVariant{value};
    StdVariantType stdVariant{sdbusVariant};

    ASSERT_TRUE(std::holds_alternative<ComplexType>(stdVariant));
    ASSERT_THAT(std::get<ComplexType>(stdVariant), Eq(value));
}

TEST(AVariant, IsImplicitlyInterchangeableWithStdVariant)
{
    using ComplexType = std::vector<sdbus::Struct<std::string, double>>;
    using StdVariantType = std::variant<std::string, uint64_t, ComplexType>;
    ComplexType value{{"hello"s, ANY_DOUBLE}, {"world"s, ANY_DOUBLE}};
    StdVariantType stdVariant{value};

    auto stdVariantCopy = [](const sdbus::Variant &v) -> StdVariantType { return v; }(stdVariant);

    ASSERT_THAT(stdVariantCopy, Eq(stdVariant));
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
    value.push_back({sdbus::Variant("a string"), ANY_DOUBLE});
    value.push_back({sdbus::Variant(ANY_UINT64), ANY_DOUBLE});

    sdbus::Variant variant(value);

    ASSERT_TRUE(variant.containsValueOfType<TypeWithVariants>());
}

TEST(ANonEmptyVariant, SerializesSuccessfullyToAMessage)
{
    sdbus::Variant variant("a string");

    auto msg = sdbus::createPlainMessage();

    ASSERT_NO_THROW(variant.serializeTo(msg));
}

TEST(AnEmptyVariant, ThrowsWhenBeingSerializedToAMessage)
{
    sdbus::Variant variant;

    auto msg = sdbus::createPlainMessage();

    ASSERT_THROW(variant.serializeTo(msg), sdbus::Error);
}

TEST(ANonEmptyVariant, SerializesToAndDeserializesFromAMessageSuccessfully)
{
    using ComplexType = std::map<uint64_t, std::vector<sdbus::Struct<std::string, double>>>;
    ComplexType value{ {ANY_UINT64, ComplexType::mapped_type{{"hello"s, ANY_DOUBLE}, {"world"s, ANY_DOUBLE}}} };
    sdbus::Variant variant(value);

    auto msg = sdbus::createPlainMessage();
    variant.serializeTo(msg);
    msg.seal();
    sdbus::Variant variant2;
    variant2.deserializeFrom(msg);

    ASSERT_THAT(variant2.get<decltype(value)>(), Eq(value));
}

TEST(CopiesOfVariant, SerializeToAndDeserializeFromMessageSuccessfully)
{
    using ComplexType = std::map<uint64_t, std::vector<sdbus::Struct<std::string, double>>>;
    ComplexType value{ {ANY_UINT64, ComplexType::mapped_type{{"hello"s, ANY_DOUBLE}, {"world"s, ANY_DOUBLE}}} };
    sdbus::Variant variant(value);
    auto variantCopy1{variant};
    auto variantCopy2 = variant;

    auto msg = sdbus::createPlainMessage();
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

TEST(AStruct, CanBeCreatedFromStdTuple)
{
    std::tuple<int32_t, std::string> value{1234, "abcd"};
    sdbus::Struct valueStruct{value};

    ASSERT_THAT(valueStruct.get<0>(), Eq(std::get<0>(value)));
    ASSERT_THAT(valueStruct.get<1>(), Eq(std::get<1>(value)));
}

TEST(AStruct, CanProvideItsDataThroughStdGet)
{
    std::tuple<int32_t, std::string> value{1234, "abcd"};
    sdbus::Struct valueStruct{value};

    ASSERT_THAT(std::get<0>(valueStruct), Eq(std::get<0>(value)));
    ASSERT_THAT(std::get<1>(valueStruct), Eq(std::get<1>(value)));
}

TEST(AStruct, CanBeUsedLikeStdTupleType)
{
    using StructType = sdbus::Struct<int, std::string, bool>;

    static_assert(std::tuple_size_v<StructType> == 3);
    static_assert(std::is_same_v<std::tuple_element_t<1, StructType>, std::string>);
}

TEST(AStruct, CanBeUsedInStructuredBinding)
{
    sdbus::Struct valueStruct(1234, "abcd", true);

    auto [first, second, third] = valueStruct;

    ASSERT_THAT(std::tie(first, second, third), Eq(std::tuple{1234, "abcd", true}));
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

TEST(AnObjectPath, CanBeMovedLikeAStdString)
{
    std::string aPath{"/some/very/long/path/longer/than/sso"};
    sdbus::ObjectPath oPath{aPath};

    ASSERT_THAT(sdbus::ObjectPath{std::move(oPath)}, Eq(sdbus::ObjectPath(std::move(aPath))));
    ASSERT_THAT(std::string(oPath), Eq(aPath));
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

TEST(ASignature, CanBeMovedLikeAStdString)
{
    std::string aSignature{"us"};
    sdbus::Signature oSignature{aSignature};

    ASSERT_THAT(sdbus::Signature{std::move(oSignature)}, Eq(sdbus::Signature(std::move(aSignature))));
}

TEST(AUnixFd, DuplicatesAndOwnsFdUponStandardConstruction)
{
    auto fd = ::eventfd(0, EFD_SEMAPHORE | EFD_NONBLOCK);

    EXPECT_THAT(sdbus::UnixFd{fd}.get(), Gt(fd));
    EXPECT_THAT(::close(fd), Eq(0));
}

TEST(AUnixFd, AdoptsAndOwnsFdAsIsUponAdoptionConstruction)
{
    auto fd = ::eventfd(0, EFD_SEMAPHORE | EFD_NONBLOCK);

    EXPECT_THAT(sdbus::UnixFd(fd, sdbus::adopt_fd).get(), Eq(fd));
    EXPECT_THAT(::close(fd), Eq(-1));
}

TEST(AUnixFd, DuplicatesFdUponCopyConstruction)
{
    sdbus::UnixFd unixFd(::eventfd(0, EFD_SEMAPHORE | EFD_NONBLOCK));

    sdbus::UnixFd unixFdCopy{unixFd};

    EXPECT_THAT(unixFdCopy.get(), Gt(unixFd.get()));
}

TEST(AUnixFd, TakesOverFdUponMoveConstruction)
{
    auto fd = ::eventfd(0, EFD_SEMAPHORE | EFD_NONBLOCK);
    sdbus::UnixFd unixFd(fd, sdbus::adopt_fd);

    sdbus::UnixFd unixFdNew{std::move(unixFd)};

    EXPECT_FALSE(unixFd.isValid());
    EXPECT_THAT(unixFdNew.get(), Eq(fd));
}

TEST(AUnixFd, ClosesFdProperlyUponDestruction)
{
    int fd, fdCopy;
    {
        fd = ::eventfd(0, EFD_SEMAPHORE | EFD_NONBLOCK);
        sdbus::UnixFd unixFd(fd, sdbus::adopt_fd);
        auto unixFdNew = std::move(unixFd);
        auto unixFdCopy = unixFdNew;
        fdCopy = unixFdCopy.get();
    }

    EXPECT_THAT(::close(fd), Eq(-1));
    EXPECT_THAT(::close(fdCopy), Eq(-1));
}

TEST(AUnixFd, DoesNotCloseReleasedFd)
{
    auto fd = ::eventfd(0, EFD_SEMAPHORE | EFD_NONBLOCK);
    int fdReleased;
    {
        sdbus::UnixFd unixFd(fd, sdbus::adopt_fd);
        fdReleased = unixFd.release();
        EXPECT_FALSE(unixFd.isValid());
    }

    EXPECT_THAT(fd, Eq(fdReleased));
    EXPECT_THAT(::close(fd), Eq(0));
}

TEST(AUnixFd, ClosesFdOnReset)
{
    auto fd = ::eventfd(0, EFD_SEMAPHORE | EFD_NONBLOCK);
    sdbus::UnixFd unixFd(fd, sdbus::adopt_fd);

    unixFd.reset();

    EXPECT_FALSE(unixFd.isValid());
    EXPECT_THAT(::close(fd), Eq(-1));
}

TEST(AUnixFd, DuplicatesNewFdAndClosesOriginalFdOnReset)
{
    auto fd = ::eventfd(0, EFD_SEMAPHORE | EFD_NONBLOCK);
    sdbus::UnixFd unixFd(fd, sdbus::adopt_fd);
    auto newFd = ::eventfd(0, EFD_SEMAPHORE | EFD_NONBLOCK);

    unixFd.reset(newFd);

    EXPECT_THAT(unixFd.get(), Gt(newFd));
    EXPECT_THAT(::close(fd), Eq(-1));
    EXPECT_THAT(::close(newFd), Eq(0));
}

TEST(AUnixFd, TakesOverNewFdAndClosesOriginalFdOnAdoptingReset)
{
    auto fd = ::eventfd(0, EFD_SEMAPHORE | EFD_NONBLOCK);
    sdbus::UnixFd unixFd(fd, sdbus::adopt_fd);
    auto newFd = ::eventfd(0, EFD_SEMAPHORE | EFD_NONBLOCK);

    unixFd.reset(newFd, sdbus::adopt_fd);

    EXPECT_THAT(unixFd.get(), Eq(newFd));
    EXPECT_THAT(::close(fd), Eq(-1));
}

TEST(AnError, CanBeConstructedFromANameAndAMessage)
{
    auto error = sdbus::Error(sdbus::Error::Name{"org.sdbuscpp.error"}, "message");
    EXPECT_THAT(error.getName(), Eq<std::string>("org.sdbuscpp.error"));
    EXPECT_THAT(error.getMessage(), Eq<std::string>("message"));
}

TEST(AnError, CanBeConstructedFromANameOnly)
{
    auto error1 = sdbus::Error(sdbus::Error::Name{"org.sdbuscpp.error"});
    auto error2 = sdbus::Error(sdbus::Error::Name{"org.sdbuscpp.error"}, nullptr);
    EXPECT_THAT(error1.getName(), Eq<std::string>("org.sdbuscpp.error"));
    EXPECT_THAT(error2.getName(), Eq<std::string>("org.sdbuscpp.error"));

    EXPECT_THAT(error1.getMessage(), Eq<std::string>(""));
    EXPECT_THAT(error2.getMessage(), Eq<std::string>(""));
}
