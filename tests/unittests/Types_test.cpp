/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2026 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
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

#include <sdbus-c++/Error.h>
#include <sdbus-c++/Message.h>
#include <sdbus-c++/Types.h>
#include <sdbus-c++/TypeTraits.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstdint>
#include <cerrno>
#include <map>
#include <string>
#include <variant>
#include <vector>
#include <sys/eventfd.h>
#include <tuple>
#include <type_traits>
#include <unistd.h>
#include <utility>

using ::testing::Eq;
using ::testing::Gt;
using ::testing::HasSubstr;
using namespace std::string_literals;

namespace
{
    constexpr const uint64_t ANY_UINT64 = 84578348354;
    constexpr const double ANY_DOUBLE = 3.14;
} // namespace

/*-------------------------------------*/
/* --          TEST CASES           -- */
/*-------------------------------------*/

TEST(AVariant, CanBeDefaultConstructed)
{
    ASSERT_NO_THROW(sdbus::Variant());
}

TEST(AVariant, ContainsNoValueAfterDefaultConstructed)
{
    const sdbus::Variant var;

    ASSERT_TRUE(var.isEmpty());
}

TEST(AVariant, CanBeConstructedFromASimpleValue)
{
    const int value = 5;

    ASSERT_NO_THROW(sdbus::Variant{value});
}

TEST(AVariant, CanBeConstructedFromAComplexValue)
{
    using ComplexType = std::map<uint64_t, std::vector<sdbus::Struct<std::string, double>>>;
    const ComplexType value{ {ANY_UINT64, ComplexType::mapped_type{{"hello"s, ANY_DOUBLE}, {"world"s, ANY_DOUBLE}}} };

    ASSERT_NO_THROW(sdbus::Variant{value});
}

TEST(AVariant, CanBeConstructedFromAnStdVariant)
{
    using ComplexType = std::vector<sdbus::Struct<std::string, double>>;
    using StdVariantType = std::variant<std::string, uint64_t, ComplexType>;
    ComplexType value{{"hello"s, ANY_DOUBLE}, {"world"s, ANY_DOUBLE}};
    const StdVariantType stdVariant{value};

    const sdbus::Variant sdbusVariant{stdVariant};

    ASSERT_TRUE(sdbusVariant.containsValueOfType<ComplexType>());
    ASSERT_THAT(sdbusVariant.get<ComplexType>(), Eq(value));
}

TEST(AVariant, CanBeCopied)
{
    auto value = "hello"s;
    const sdbus::Variant variant(value);

    const auto variantCopy1{variant}; // NOLINT(performance-unnecessary-copy-initialization)
    const auto variantCopy2 = variantCopy1; // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_THAT(variantCopy1.get<std::string>(), Eq(value));
    ASSERT_THAT(variantCopy2.get<std::string>(), Eq(value));
}

TEST(AVariant, CanBeMoved)
{
    auto value = "hello"s;
    sdbus::Variant variant(value);

    auto movedVariant{std::move(variant)};

    ASSERT_THAT(movedVariant.get<std::string>(), Eq(value));
    ASSERT_TRUE(variant.isEmpty()); // NOLINT(bugprone-use-after-move,hicpp-invalid-access-moved)
}

TEST(AVariant, CanBeMovedIntoAMap)
{
    const auto value = "hello"s;
    sdbus::Variant variant(value); // NOLINT(misc-const-correctness)

    std::map<std::string, sdbus::Variant> mymap;
    mymap.try_emplace("payload", std::move(variant));

    ASSERT_THAT(mymap["payload"].get<std::string>(), Eq(value));
    ASSERT_TRUE(variant.isEmpty()); // NOLINT(bugprone-use-after-move,hicpp-invalid-access-moved)
}

TEST(AVariant, IsNotEmptyWhenContainsAValue)
{
    const sdbus::Variant var("hello");

    ASSERT_FALSE(var.isEmpty());
}

TEST(ASimpleVariant, ReturnsTheSimpleValueWhenAsked)
{
    const int value = 5;

    const sdbus::Variant variant(value);

    ASSERT_THAT(variant.get<int>(), Eq(value));
}

#ifndef SDBUS_basu // Dumping message or variant to a string is not supported on basu backend
TEST(ASimpleVariant, CanBeDumpedToAString)
{
    const int value = 5;
    const sdbus::Variant variant(value);

    // This should produce something like:
    // VARIANT "i" {
    //     INT32 5;
    // };
    const auto str = variant.dumpToString();

    EXPECT_THAT(str, ::HasSubstr("VARIANT \"i\""));
    EXPECT_THAT(str, ::HasSubstr("INT32"));
    EXPECT_THAT(str, ::HasSubstr("5"));
}
#endif // SDBUS_basu

TEST(AComplexVariant, ReturnsTheComplexValueWhenAsked)
{
    using ComplexType = std::map<uint64_t, std::vector<sdbus::Struct<std::string, double>>>;
    const ComplexType value{ {ANY_UINT64, ComplexType::mapped_type{{"hello"s, ANY_DOUBLE}, {"world"s, ANY_DOUBLE}}} };

    const sdbus::Variant variant(value);

    ASSERT_THAT(variant.get<ComplexType>(), Eq(value));
}

#ifndef SDBUS_basu // Dumping message or variant to a string is not supported on basu backend
TEST(AComplexVariant, CanBeDumpedToAString)
{
    using ComplexType = std::map<uint64_t, std::vector<sdbus::Struct<std::string, double>>>;
    const ComplexType value{ {ANY_UINT64, ComplexType::mapped_type{{"hello"s, ANY_DOUBLE}, {"world"s, ANY_DOUBLE}}} };
    const sdbus::Variant variant(value);

    // This should produce something like:
    // VARIANT "a{ta(sd)}" {
    //     ARRAY "{ta(sd)}" {
    //         DICT_ENTRY "ta(sd)" {
    //             UINT64 84578348354;
    //             ARRAY "(sd)" {
    //                 STRUCT "sd" {
    //                     STRING "hello";
    //                     DOUBLE 3.14;
    //                 };
    //                 STRUCT "sd" {
    //                     STRING "world";
    //                     DOUBLE 3.14;
    //                 };
    //             };
    //         };
    //     };
    // };
    const auto str = variant.dumpToString();

    EXPECT_THAT(str, ::HasSubstr("VARIANT \"a{ta(sd)}\""));
    EXPECT_THAT(str, ::HasSubstr("hello"));
    EXPECT_THAT(str, ::HasSubstr("world"));
}
#endif // SDBUS_basu

TEST(AVariant, HasConceptuallyNonmutableGetMethodWhichCanBeCalledXTimes)
{
    const std::string value{"I am a string"};
    const sdbus::Variant variant(value);

    ASSERT_THAT(variant.get<std::string>(), Eq(value));
    ASSERT_THAT(variant.get<std::string>(), Eq(value));
    ASSERT_THAT(variant.get<std::string>(), Eq(value));
}

TEST(AVariant, ReturnsTrueWhenAskedIfItContainsTheTypeItReallyContains)
{
    using ComplexType = std::map<uint64_t, std::vector<sdbus::Struct<std::string, double>>>;
    const ComplexType value{ {ANY_UINT64, ComplexType::mapped_type{{"hello"s, ANY_DOUBLE}, {"world"s, ANY_DOUBLE}}} };

    const sdbus::Variant variant(value);

    ASSERT_TRUE(variant.containsValueOfType<ComplexType>());
}

TEST(AVariant, CanBeConvertedIntoAnStdVariant)
{
    using ComplexType = std::vector<sdbus::Struct<std::string, double>>;
    using StdVariantType = std::variant<std::string, uint64_t, ComplexType>;
    const ComplexType value{{"hello"s, ANY_DOUBLE}, {"world"s, ANY_DOUBLE}};
    const sdbus::Variant sdbusVariant{value};
    StdVariantType stdVariant{sdbusVariant};

    ASSERT_TRUE(std::holds_alternative<ComplexType>(stdVariant));
    ASSERT_THAT(std::get<ComplexType>(stdVariant), Eq(value));
}

TEST(AVariant, IsImplicitlyInterchangeableWithStdVariant)
{
    using ComplexType = std::vector<sdbus::Struct<std::string, double>>;
    using StdVariantType = std::variant<std::string, uint64_t, ComplexType>;
    ComplexType value{{"hello"s, ANY_DOUBLE}, {"world"s, ANY_DOUBLE}};
    const StdVariantType stdVariant{value};

    auto stdVariantCopy = [](const sdbus::Variant &var) -> StdVariantType { return var; }(stdVariant);

    ASSERT_THAT(stdVariantCopy, Eq(stdVariant));
}

TEST(ASimpleVariant, ReturnsFalseWhenAskedIfItContainsTypeItDoesntReallyContain)
{
    const int value = 5;

    const sdbus::Variant variant(value);

    ASSERT_FALSE(variant.containsValueOfType<double>());
}

TEST(AVariant, CanContainOtherEmbeddedVariants)
{
    using TypeWithVariants = std::vector<sdbus::Struct<sdbus::Variant, double>>;
    TypeWithVariants value;
    value.emplace_back(sdbus::Variant("a string"), ANY_DOUBLE);
    value.emplace_back(sdbus::Variant(ANY_UINT64), ANY_DOUBLE);

    const sdbus::Variant variant(value);

    ASSERT_TRUE(variant.containsValueOfType<TypeWithVariants>());
}

TEST(ANonEmptyVariant, SerializesSuccessfullyToAMessage)
{
    const sdbus::Variant variant("a string");

    auto msg = sdbus::createPlainMessage();

    ASSERT_NO_THROW(variant.serializeTo(msg));
}

TEST(AnEmptyVariant, ThrowsWhenBeingSerializedToAMessage)
{
    const sdbus::Variant variant;

    auto msg = sdbus::createPlainMessage();

    ASSERT_THROW(variant.serializeTo(msg), sdbus::Error);
}

TEST(ANonEmptyVariant, SerializesToAndDeserializesFromAMessageSuccessfully)
{
    using ComplexType = std::map<uint64_t, std::vector<sdbus::Struct<std::string, double>>>;
    const ComplexType value{ {ANY_UINT64, ComplexType::mapped_type{{"hello"s, ANY_DOUBLE}, {"world"s, ANY_DOUBLE}}} };
    const sdbus::Variant variant(value);

    auto msg = sdbus::createPlainMessage();
    variant.serializeTo(msg);
    msg.seal();
    sdbus::Variant variant2;
    variant2.deserializeFrom(msg);

    ASSERT_THAT(variant2.get<ComplexType>(), Eq(value));
}

TEST(CopiesOfVariant, SerializeToAndDeserializeFromMessageSuccessfully)
{
    using ComplexType = std::map<uint64_t, std::vector<sdbus::Struct<std::string, double>>>;
    const ComplexType value{ {ANY_UINT64, ComplexType::mapped_type{{"hello"s, ANY_DOUBLE}, {"world"s, ANY_DOUBLE}}} };
    const sdbus::Variant variant(value);
    auto variantCopy1{variant}; // NOLINT(performance-unnecessary-copy-initialization)
    auto variantCopy2 = variant; // NOLINT(performance-unnecessary-copy-initialization)

    auto msg = sdbus::createPlainMessage();
    variant.serializeTo(msg);
    variantCopy1.serializeTo(msg);
    variantCopy2.serializeTo(msg);
    msg.seal();
    sdbus::Variant receivedVariant1, receivedVariant2, receivedVariant3; // NOLINT(readability-isolate-declaration)
    receivedVariant1.deserializeFrom(msg);
    receivedVariant2.deserializeFrom(msg);
    receivedVariant3.deserializeFrom(msg);

    ASSERT_THAT(receivedVariant1.get<ComplexType>(), Eq(value));
    ASSERT_THAT(receivedVariant2.get<ComplexType>(), Eq(value));
    ASSERT_THAT(receivedVariant3.get<ComplexType>(), Eq(value));
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
    const sdbus::Struct valueStruct(1234, "abcd", true);

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
    const std::string aPath{"/some/path"};

    ASSERT_THAT(sdbus::ObjectPath{aPath}, Eq(aPath));
}

TEST(AnObjectPath, CanBeMovedLikeAStdString)
{
    std::string aPath{"/some/very/long/path/longer/than/sso"};
    sdbus::ObjectPath oPath{aPath};

    ASSERT_THAT(sdbus::ObjectPath{std::move(oPath)}, Eq(sdbus::ObjectPath(std::move(aPath))));
}

TEST(ASignature, CanBeConstructedFromCString)
{
    const char* aSignature = "us";

    ASSERT_THAT(sdbus::Signature{aSignature}, Eq(aSignature));
}

TEST(ASignature, CanBeConstructedFromStdString)
{
    const std::string aSignature{"us"};

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
    const sdbus::UnixFd unixFd(::eventfd(0, EFD_SEMAPHORE | EFD_NONBLOCK));

    const sdbus::UnixFd unixFdCopy{unixFd}; // NOLINT(performance-unnecessary-copy-initialization)

    EXPECT_THAT(unixFdCopy.get(), Gt(unixFd.get()));
}

TEST(AUnixFd, TakesOverFdUponMoveConstruction)
{
    auto fd = ::eventfd(0, EFD_SEMAPHORE | EFD_NONBLOCK);
    sdbus::UnixFd unixFd(fd, sdbus::adopt_fd);

    const sdbus::UnixFd unixFdNew{std::move(unixFd)};

    EXPECT_FALSE(unixFd.isValid()); // NOLINT(bugprone-use-after-move,hicpp-invalid-access-moved,clang-analyzer-cplusplus.Move)
    EXPECT_THAT(unixFdNew.get(), Eq(fd));
}

TEST(AUnixFd, ClosesFdProperlyUponDestruction)
{
    int fd{}, fdCopy{}; // NOLINT(readability-isolate-declaration)
    {
        fd = ::eventfd(0, EFD_SEMAPHORE | EFD_NONBLOCK);
        sdbus::UnixFd unixFd(fd, sdbus::adopt_fd);
        auto unixFdNew = std::move(unixFd);
        auto unixFdCopy = unixFdNew; // NOLINT(performance-unnecessary-copy-initialization)
        fdCopy = unixFdCopy.get();
    }

    EXPECT_THAT(::close(fd), Eq(-1));
    EXPECT_THAT(::close(fdCopy), Eq(-1));
}

TEST(AUnixFd, DoesNotCloseReleasedFd)
{
    auto fd = ::eventfd(0, EFD_SEMAPHORE | EFD_NONBLOCK);
    int fdReleased{};
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
    EXPECT_TRUE(error.isValid());
}

TEST(AnError, CanBeConstructedFromANameOnly)
{
    auto error1 = sdbus::Error(sdbus::Error::Name{"org.sdbuscpp.error"});
    auto error2 = sdbus::Error(sdbus::Error::Name{"org.sdbuscpp.error"}, nullptr);
    EXPECT_THAT(error1.getName(), Eq<std::string>("org.sdbuscpp.error"));
    EXPECT_THAT(error2.getName(), Eq<std::string>("org.sdbuscpp.error"));

    EXPECT_TRUE(error1.getMessage().empty());
    EXPECT_TRUE(error2.getMessage().empty());

    EXPECT_TRUE(error1.isValid());
    EXPECT_TRUE(error2.isValid());
}

TEST(AnError, IsInvalidWhenConstructedWithAnEmptyName)
{
    auto error = sdbus::Error({});

    EXPECT_TRUE(error.getName().empty());
    EXPECT_TRUE(error.getMessage().empty());
    EXPECT_FALSE(error.isValid());
}

TEST(AnErrorFactory, CanCreateAnErrorFromErrno)
{
    auto error = sdbus::createError(ENOENT, "custom message");

    EXPECT_THAT(error.getName(), Eq<std::string>("org.freedesktop.DBus.Error.FileNotFound"));
    EXPECT_THAT(error.getMessage(), Eq<std::string>("custom message (No such file or directory)"));
    EXPECT_TRUE(error.isValid());
}

#ifndef SDBUS_basu // Creating error from invalid errno is not supported on basu backend
TEST(AnErrorFactory, CreatesGenericErrorWhenErrnoIsUnknown)
{
    auto error = sdbus::createError(123456, "custom message");

    EXPECT_THAT(error.getName(), Eq<std::string>("org.freedesktop.DBus.Error.Failed"));
    EXPECT_THAT(error.getMessage(), Eq<std::string>("custom message (Unknown error 123456)"));
    EXPECT_TRUE(error.isValid());
}
#endif // SDBUS_basu

TEST(AnErrorFactory, CreatesEmptyInvalidErrorWhenErrnoIsZero)
{
    auto error = sdbus::createError(0, "custom message");

    EXPECT_TRUE(error.getName().empty());
    EXPECT_THAT(error.getMessage(), Eq<std::string>("custom message"));
    EXPECT_FALSE(error.isValid());
}
