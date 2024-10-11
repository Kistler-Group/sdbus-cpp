/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2024 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
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
#include <list>

using ::testing::Eq;
using ::testing::StrEq;
using ::testing::Gt;
using ::testing::DoubleEq;
using ::testing::IsNull;
using ::testing::SizeIs;
using ::testing::ElementsAre;
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

namespace sdbus {

    template <typename _ElementType>
    sdbus::Message& operator<<(sdbus::Message& msg, const std::list<_ElementType>& items)
    {
        // TODO: This can also be simplified on the basis of a callback (see dictionary...)
        msg.openContainer<_ElementType>();

        for (const auto& item : items)
            msg << item;

        msg.closeContainer();

        return msg;
    }

    template <typename _ElementType>
    sdbus::Message& operator>>(sdbus::Message& msg, std::list<_ElementType>& items)
    {
        if(!msg.enterContainer<_ElementType>())
            return msg;

        while (true)
        {
            _ElementType elem;
            if (msg >> elem)
                items.emplace_back(std::move(elem));
            else
                break;
        }

        msg.clearFlags();

        msg.exitContainer();

        return msg;
    }

}

template <typename _Element, typename _Allocator>
struct sdbus::signature_of<std::list<_Element, _Allocator>>
    : sdbus::signature_of<std::vector<_Element, _Allocator>>
{};

namespace my {
    enum class Enum
    {
        Value1,
        Value2,
        Value3
    };

    struct Struct
    {
        int i;
        std::string s;
        std::list<double> l;
        Enum e;

        friend bool operator==(const Struct& lhs, const Struct& rhs) = default;
    };

    struct RelaxedStruct
    {
        int i;
        std::string s;
        std::list<double> l;
        Enum e;

        friend bool operator==(const RelaxedStruct& lhs, const RelaxedStruct& rhs) = default;
    };

    struct NestedStruct
    {
        int i;
        std::string s;
        Enum e;
        Struct x;

        friend bool operator==(const NestedStruct& lhs, const NestedStruct& rhs) = default;
    };
}

SDBUSCPP_REGISTER_STRUCT(my::Struct, i, s, l, e);

SDBUSCPP_ENABLE_RELAXED_DICT2STRUCT_DESERIALIZATION(my::RelaxedStruct);
SDBUSCPP_REGISTER_STRUCT(my::RelaxedStruct, i, s, l, e);

SDBUSCPP_ENABLE_NESTED_STRUCT2DICT_SERIALIZATION(my::NestedStruct);
SDBUSCPP_REGISTER_STRUCT(my::NestedStruct, i, s, e, x);

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

    const int dataWritten = 5;

    msg << dataWritten;
    msg.seal();

    int dataRead;
    msg >> dataRead;

    ASSERT_THAT(dataRead, Eq(dataWritten));
}

TEST(AMessage, CanCarryAStringAsAStringView)
{
    auto msg = sdbus::createPlainMessage();

    const std::string_view dataWritten = "Hello";

    msg << dataWritten;
    msg.seal();

    std::string dataRead;
    msg >> dataRead;

    ASSERT_THAT(dataRead, Eq(dataWritten));
}

TEST(AMessage, CanCarryAUnixFd)
{
    auto msg = sdbus::createPlainMessage();

    const sdbus::UnixFd dataWritten{0};
    msg << dataWritten;

    msg.seal();

    sdbus::UnixFd dataRead;
    msg >> dataRead;

    ASSERT_THAT(dataRead.get(), Gt(dataWritten.get()));
}

TEST(AMessage, CanCarryAVariant)
{
    auto msg = sdbus::createPlainMessage();

    const auto dataWritten = sdbus::Variant((double)3.14);

    msg << dataWritten;
    msg.seal();

    sdbus::Variant dataRead;
    msg >> dataRead;

    ASSERT_THAT(dataRead.get<double>(), Eq(dataWritten.get<double>()));
}

TEST(AMessage, CanCarryACollectionOfEmbeddedVariants)
{
    auto msg = sdbus::createPlainMessage();

    std::vector<sdbus::Variant> value{sdbus::Variant{"hello"s}, sdbus::Variant{(double)3.14}};
    const auto dataWritten = sdbus::Variant{value};

    msg << dataWritten;
    msg.seal();

    sdbus::Variant dataRead;
    msg >> dataRead;

    ASSERT_THAT(dataRead.get<decltype(value)>()[0].get<std::string>(), Eq(value[0].get<std::string>()));
    ASSERT_THAT(dataRead.get<decltype(value)>()[1].get<double>(), Eq(value[1].get<double>()));
}

TEST(AMessage, CanCarryDBusArrayOfTrivialTypesGivenAsStdVector)
{
    auto msg = sdbus::createPlainMessage();

    const std::vector<int64_t> dataWritten{3545342, 43643532, 324325};

    msg << dataWritten;
    msg.seal();

    std::vector<int64_t> dataRead;
    msg >> dataRead;

    ASSERT_THAT(dataRead, Eq(dataWritten));
}

TEST(AMessage, CanCarryDBusArrayOfNontrivialTypesGivenAsStdVector)
{
    auto msg = sdbus::createPlainMessage();

    const std::vector dataWritten{sdbus::Signature{"s"}, sdbus::Signature{"u"}, sdbus::Signature{"b"}};

    msg << dataWritten;
    msg.seal();

    std::vector<sdbus::Signature> dataRead;
    msg >> dataRead;

    ASSERT_THAT(dataRead, Eq(dataWritten));
}

TEST(AMessage, CanCarryDBusArrayOfTrivialTypesGivenAsStdArray)
{
    auto msg = sdbus::createPlainMessage();

    const std::array<int, 3> dataWritten{3545342, 43643532, 324325};

    msg << dataWritten;
    msg.seal();

    std::array<int, 3> dataRead;
    msg >> dataRead;

    ASSERT_THAT(dataRead, Eq(dataWritten));
}

TEST(AMessage, CanCarryDBusArrayOfNontrivialTypesGivenAsStdArray)
{
    auto msg = sdbus::createPlainMessage();

    const std::array dataWritten{sdbus::Signature{"s"}, sdbus::Signature{"u"}, sdbus::Signature{"b"}};

    msg << dataWritten;
    msg.seal();

    std::array<sdbus::Signature, 3> dataRead;
    msg >> dataRead;

    ASSERT_THAT(dataRead, Eq(dataWritten));
}

#ifdef __cpp_lib_span
TEST(AMessage, CanCarryDBusArrayOfTrivialTypesGivenAsStdSpan)
{
    auto msg = sdbus::createPlainMessage();

    const std::array<int, 3> sourceArray{3545342, 43643532, 324325};
    const std::span dataWritten{sourceArray};

    msg << dataWritten;
    msg.seal();

    std::array<int, 3> destinationArray;
    std::span dataRead{destinationArray};
    msg >> dataRead;

    ASSERT_THAT(std::vector(dataRead.begin(), dataRead.end()), Eq(std::vector(dataWritten.begin(), dataWritten.end())));
}

TEST(AMessage, CanCarryDBusArrayOfNontrivialTypesGivenAsStdSpan)
{
    auto msg = sdbus::createPlainMessage();

    const std::array sourceArray{sdbus::Signature{"s"}, sdbus::Signature{"u"}, sdbus::Signature{"b"}};
    const std::span dataWritten{sourceArray};

    msg << dataWritten;
    msg.seal();

    std::array<sdbus::Signature, 3> destinationArray;
    std::span dataRead{destinationArray};
    msg >> dataRead;

    ASSERT_THAT(std::vector(dataRead.begin(), dataRead.end()), Eq(std::vector(dataWritten.begin(), dataWritten.end())));
}
#endif

TEST(AMessage, CanCarryAnEnumValue)
{
    auto msg = sdbus::createPlainMessage();

    enum class EnumA : int16_t {X = 5} aWritten{EnumA::X};
    enum EnumB {Y = 11} bWritten{EnumB::Y};

    msg << aWritten << bWritten;
    msg.seal();

    EnumA aRead{};
    EnumB bRead{};
    msg >> aRead >> bRead;

    ASSERT_THAT(aRead, Eq(aWritten));
    ASSERT_THAT(bRead, Eq(bWritten));
}

TEST(AMessage, ThrowsWhenDestinationStdArrayIsTooSmallDuringDeserialization)
{
    auto msg = sdbus::createPlainMessage();

    const std::vector<int> dataWritten{3545342, 43643532, 324325, 89789, 15343};

    msg << dataWritten;
    msg.seal();

    std::array<int, 3> dataRead;
    ASSERT_THROW(msg >> dataRead, sdbus::Error);
}

#ifdef __cpp_lib_span
TEST(AMessage, ThrowsWhenDestinationStdSpanIsTooSmallDuringDeserialization)
{
    auto msg = sdbus::createPlainMessage();

    const std::array<int, 3> dataWritten{3545342, 43643532, 324325};

    msg << dataWritten;
    msg.seal();

    std::array<int, 2> destinationArray;
    std::span dataRead{destinationArray};
    ASSERT_THROW(msg >> dataRead, sdbus::Error);
}
#endif

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

    ComplexType dataWritten = { {1, {{{5, {{sdbus::ObjectPath{"/some/object"}, true, 45, {{6, "hello"}, {7, "world"}}}}}}, sdbus::Signature{"av"}, 3.14}}};

    msg << dataWritten;
    msg.seal();

    ComplexType dataRead;
    msg >> dataRead;

    ASSERT_THAT(dataRead, Eq(dataWritten));
}

TEST(AMessage, CanPeekASimpleType)
{
    auto msg = sdbus::createPlainMessage();
    msg << 123;
    msg.seal();

    auto [type, contents] = msg.peekType();

    ASSERT_THAT(type, Eq('i'));
    ASSERT_THAT(contents, IsNull());
}

TEST(AMessage, CanPeekContainerContents)
{
    auto msg = sdbus::createPlainMessage();
    msg << std::map<int, std::string>{{1, "one"}, {2, "two"}};
    msg.seal();

    auto [type, contents] = msg.peekType();

    ASSERT_THAT(type, Eq('a'));
    ASSERT_THAT(contents, StrEq("{is}"));
}

TEST(AMessage, CanCarryDBusArrayGivenAsCustomType)
{
    auto msg = sdbus::createPlainMessage();

    const std::list<int64_t> dataWritten{3545342, 43643532, 324325};
    //custom::MyType t;

    msg << dataWritten;
    // msg << t;
    msg.seal();

    std::list<int64_t> dataRead;
    msg >> dataRead;

    ASSERT_THAT(dataRead, Eq(dataWritten));
}

TEST(AMessage, CanCarryUserDefinedStruct)
{
    auto msg = sdbus::createPlainMessage();

    const my::Struct dataWritten{3545342, "hello"s, {3.14, 2.4568546}, my::Enum::Value2};

    msg << dataWritten;
    msg.seal();

    my::Struct dataRead;
    msg >> dataRead;

    ASSERT_THAT(dataRead, Eq(dataWritten));
}

TEST(AMessage, CanCarryNestedUserDefinedStruct)
{
    auto msg = sdbus::createPlainMessage();

    const my::NestedStruct dataWritten{3545342, "hello"s, my::Enum::Value2, {12, "world"s, {3.14, 2.4568546}, my::Enum::Value3}};

    msg << dataWritten;
    msg.seal();

    my::NestedStruct dataRead;
    msg >> dataRead;

    ASSERT_THAT(dataRead, Eq(dataWritten));
}

TEST(AMessage, CanSerializeUserDefinedStructAsDictionaryOfStringsToVariants)
{
    auto msg = sdbus::createPlainMessage();

    const my::Struct dataWritten{3545342, "hello"s, {3.14, 2.4568546}, my::Enum::Value2};

    msg << sdbus::as_dictionary{dataWritten};
    msg.seal();

    std::map<std::string, sdbus::Variant> dataRead;
    msg >> dataRead;

    ASSERT_THAT(dataRead, SizeIs(4));
    ASSERT_THAT(dataRead.at("i").get<int>(), Eq(3545342));
    ASSERT_THAT(dataRead.at("s").get<std::string>(), Eq("hello"));
    ASSERT_THAT(dataRead.at("l").get<std::list<double>>(), ElementsAre(3.14, 2.4568546));
    ASSERT_THAT(dataRead.at("e").get<my::Enum>(), Eq(my::Enum::Value2));
}

TEST(AMessage, CanRecursivelySerializeUserDefinedStructAsDictionaryOfStringsToVariants)
{
    auto msg = sdbus::createPlainMessage();

    const my::NestedStruct dataWritten{3545342, "hello"s, my::Enum::Value2, {12, "world"s, {3.14, 2.4568546}, my::Enum::Value3}};

    msg << sdbus::as_dictionary{dataWritten};
    msg.seal();

    std::map<std::string, sdbus::Variant> dataRead;
    msg >> dataRead;

    ASSERT_THAT(dataRead, SizeIs(4));
    ASSERT_THAT(dataRead.at("i").get<int>(), Eq(3545342));
    ASSERT_THAT(dataRead.at("s").get<std::string>(), Eq("hello"));
    ASSERT_THAT(dataRead.at("e").get<my::Enum>(), Eq(my::Enum::Value2));
    auto nestedStructRead = dataRead.at("x").get<std::map<std::string, sdbus::Variant>>(); // Nested struct serialized as dict
    ASSERT_THAT(nestedStructRead.at("i").get<int>(), Eq(12));
    ASSERT_THAT(nestedStructRead.at("s").get<std::string>(), Eq("world"));
    ASSERT_THAT(nestedStructRead.at("l").get<std::list<double>>(), ElementsAre(3.14, 2.4568546));
    ASSERT_THAT(nestedStructRead.at("e").get<my::Enum>(), Eq(my::Enum::Value3));
}

TEST(AMessage, CanDeserializeDictionaryOfStringsToVariantsIntoUserDefinedStruct)
{
    auto msg = sdbus::createPlainMessage();

    std::map<std::string, sdbus::Variant> dataWritten{ {"i", sdbus::Variant{3545342}}
                                                     , {"s", sdbus::Variant{"hello"s}}
                                                     , {"l", sdbus::Variant{std::list<double>{3.14, 2.4568546}}}
                                                     , {"e", sdbus::Variant{my::Enum::Value2}} };

    msg << dataWritten;
    msg.seal();

    my::Struct dataRead;
    msg >> dataRead;

    ASSERT_THAT(dataRead, Eq(my::Struct{3545342, "hello"s, {3.14, 2.4568546}, my::Enum::Value2}));
}

TEST(AMessage, FailsDeserializingDictionaryIntoUserDefinedStructIfStructMemberIsNotFound)
{
    auto msg = sdbus::createPlainMessage();

    std::map<std::string, sdbus::Variant> dataWritten{ {"i", sdbus::Variant{3545342}}
                                                     , {"nonexistent", sdbus::Variant{"hello"s}}
                                                     , {"l", sdbus::Variant{std::list<double>{3.14, 2.4568546}}}
                                                     , {"e", sdbus::Variant{my::Enum::Value2}} };

    msg << dataWritten;
    msg.seal();

    my::Struct dataRead;

    ASSERT_THROW(msg >> dataRead, sdbus::Error);
}

TEST(AMessage, DeserializesDictionaryIntoStructWithMissingMembersSuccessfullyIfRelaxedOptionIsSet)
{
    auto msg = sdbus::createPlainMessage();

    std::map<std::string, sdbus::Variant> dataWritten{ {"some_nonexistent_struct_member", sdbus::Variant{3545342}}
                                                     , {"another_nonexistent_struct_member", sdbus::Variant{"hello"s}}
                                                     , {"l", sdbus::Variant{std::list<double>{3.14, 2.4568546}}}
                                                     , {"e", sdbus::Variant{my::Enum::Value2}} };

    msg << dataWritten;
    msg.seal();

    my::RelaxedStruct dataRead{};
    msg >> dataRead;

    ASSERT_THAT(dataRead, Eq(my::RelaxedStruct{{}, {}, {3.14, 2.4568546}, my::Enum::Value2}));
}

class AMessage : public ::testing::TestWithParam<std::variant<int32_t, std::string, my::Struct>>
{
};

TEST_P(AMessage, CanCarryDBusVariantGivenAsStdVariant)
{
    auto msg = sdbus::createPlainMessage();

    const std::variant<int32_t, std::string, my::Struct> dataWritten{GetParam()};

    msg << dataWritten;
    msg.seal();

    std::variant<int32_t, std::string, my::Struct> dataRead;
    msg >> dataRead;

    ASSERT_THAT(dataRead, Eq(dataWritten));
}

TEST_P(AMessage, ThrowsWhenDestinationStdVariantHasWrongTypeDuringDeserialization)
{
    auto msg = sdbus::createPlainMessage();

    const std::variant<int32_t, std::string, my::Struct> dataWritten{GetParam()};

    msg << dataWritten;
    msg.seal();

    std::variant<std::vector<bool>> dataRead;
    ASSERT_THROW(msg >> dataRead, sdbus::Error);
}

INSTANTIATE_TEST_SUITE_P( StringIntStruct
                        , AMessage
                        , ::testing::Values("hello"s, 1, my::Struct{}));
