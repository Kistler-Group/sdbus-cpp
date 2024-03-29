/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2024 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file TypeTraits_test.cpp
 *
 * Created on: Nov 27, 2016
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

#include <sdbus-c++/TypeTraits.h>
#include <sdbus-c++/Types.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstdint>
#include <type_traits>

using ::testing::Eq;

namespace
{
    // ---
    // FIXTURE DEFINITION FOR TYPED TESTS
    // ----

    template <typename _T>
    class Type2DBusTypeSignatureConversion
        : public ::testing::Test
    {
    protected:
        const std::string dbusTypeSignature_{getDBusTypeSignature()};
    private:
        static std::string getDBusTypeSignature();
    };

    enum class SomeEnumClass : uint8_t
    {
        A, B, C
    };

    enum struct SomeEnumStruct : int64_t
    {
        A, B, C
    };

    enum SomeClassicEnum
    {
        A, B, C
    };

#define TYPE(...)                                                                       \
    template <>                                                                         \
    std::string Type2DBusTypeSignatureConversion<__VA_ARGS__>::getDBusTypeSignature()   \
    /**/
#define HAS_DBUS_TYPE_SIGNATURE(_SIG)                                                   \
    {                                                                                   \
        return (_SIG);                                                                  \
    }                                                                                   \
    /**/

    TYPE(bool)HAS_DBUS_TYPE_SIGNATURE("b")
    TYPE(uint8_t)HAS_DBUS_TYPE_SIGNATURE("y")
    TYPE(int16_t)HAS_DBUS_TYPE_SIGNATURE("n")
    TYPE(uint16_t)HAS_DBUS_TYPE_SIGNATURE("q")
    TYPE(int32_t)HAS_DBUS_TYPE_SIGNATURE("i")
    TYPE(uint32_t)HAS_DBUS_TYPE_SIGNATURE("u")
    TYPE(int64_t)HAS_DBUS_TYPE_SIGNATURE("x")
    TYPE(uint64_t)HAS_DBUS_TYPE_SIGNATURE("t")
    TYPE(double)HAS_DBUS_TYPE_SIGNATURE("d")
    TYPE(const char*)HAS_DBUS_TYPE_SIGNATURE("s")
    TYPE(std::string)HAS_DBUS_TYPE_SIGNATURE("s")
    TYPE(sdbus::BusName)HAS_DBUS_TYPE_SIGNATURE("s")
    TYPE(sdbus::InterfaceName)HAS_DBUS_TYPE_SIGNATURE("s")
    TYPE(sdbus::MemberName)HAS_DBUS_TYPE_SIGNATURE("s")
    TYPE(sdbus::ObjectPath)HAS_DBUS_TYPE_SIGNATURE("o")
    TYPE(sdbus::Signature)HAS_DBUS_TYPE_SIGNATURE("g")
    TYPE(sdbus::Variant)HAS_DBUS_TYPE_SIGNATURE("v")
    TYPE(std::variant<int16_t, std::string>)HAS_DBUS_TYPE_SIGNATURE("v")
    TYPE(sdbus::UnixFd)HAS_DBUS_TYPE_SIGNATURE("h")
    TYPE(sdbus::Struct<bool>)HAS_DBUS_TYPE_SIGNATURE("(b)")
    TYPE(sdbus::Struct<uint16_t, double, std::string, sdbus::Variant>)HAS_DBUS_TYPE_SIGNATURE("(qdsv)")
    TYPE(std::vector<int16_t>)HAS_DBUS_TYPE_SIGNATURE("an")
    TYPE(std::array<int16_t, 3>)HAS_DBUS_TYPE_SIGNATURE("an")
#ifdef __cpp_lib_span
    TYPE(std::span<int16_t>)HAS_DBUS_TYPE_SIGNATURE("an")
#endif
    TYPE(SomeEnumClass)HAS_DBUS_TYPE_SIGNATURE("y")
    TYPE(SomeEnumStruct)HAS_DBUS_TYPE_SIGNATURE("x")
    TYPE(SomeClassicEnum)HAS_DBUS_TYPE_SIGNATURE("u")
    TYPE(std::map<int32_t, int64_t>)HAS_DBUS_TYPE_SIGNATURE("a{ix}")
    TYPE(std::unordered_map<int32_t, int64_t>)HAS_DBUS_TYPE_SIGNATURE("a{ix}")
    using ComplexType = std::map<
                            uint64_t,
                            sdbus::Struct<
                                std::map<
                                    uint8_t,
                                    std::vector<
                                        sdbus::Struct<
                                            sdbus::ObjectPath,
                                            std::array<int16_t, 3>,
                                            bool,
                                            sdbus::Variant,
                                            std::unordered_map<int, std::string>
                                        >
                                    >
                                >,
                                sdbus::Signature,
                                sdbus::UnixFd,
                                const char*
                            >
                        >;
    TYPE(ComplexType)HAS_DBUS_TYPE_SIGNATURE("a{t(a{ya(oanbva{is})}ghs)}")

    typedef ::testing::Types< bool
                            , uint8_t
                            , int16_t
                            , uint16_t
                            , int32_t
                            , uint32_t
                            , int64_t
                            , uint64_t
                            , double
                            , const char*
                            , std::string
                            , sdbus::BusName
                            , sdbus::InterfaceName
                            , sdbus::MemberName
                            , sdbus::ObjectPath
                            , sdbus::Signature
                            , sdbus::Variant
                            , std::variant<int16_t, std::string>
                            , sdbus::UnixFd
                            , sdbus::Struct<bool>
                            , sdbus::Struct<uint16_t, double, std::string, sdbus::Variant>
                            , std::vector<int16_t>
                            , std::array<int16_t, 3>
#ifdef __cpp_lib_span
                            , std::span<int16_t>
#endif
                            , SomeEnumClass
                            , SomeEnumStruct
                            , SomeClassicEnum
                            , std::map<int32_t, int64_t>
                            , std::unordered_map<int32_t, int64_t>
                            , ComplexType
                            > DBusSupportedTypes;

    TYPED_TEST_SUITE(Type2DBusTypeSignatureConversion, DBusSupportedTypes);
}

/*-------------------------------------*/
/* --          TEST CASES           -- */
/*-------------------------------------*/

TYPED_TEST(Type2DBusTypeSignatureConversion, ConvertsTypeToProperDBusSignature)
{
    ASSERT_THAT(sdbus::signature_of<TypeParam>::str(), Eq(this->dbusTypeSignature_));
}

TEST(FreeFunctionTypeTraits, DetectsTraitsOfTrivialSignatureFunction)
{
    void f();
    using Fnc = decltype(f);

    static_assert(!sdbus::is_async_method_v<Fnc>, "Free function incorrectly detected as async method");
    static_assert(std::is_same<sdbus::function_arguments_t<Fnc>, std::tuple<>>::value, "Incorrectly detected free function parameters");
    static_assert(std::is_same<sdbus::tuple_of_function_input_arg_types_t<Fnc>, std::tuple<>>::value, "Incorrectly detected tuple of free function parameters");
    static_assert(std::is_same<sdbus::tuple_of_function_output_arg_types_t<Fnc>, void>::value, "Incorrectly detected tuple of free function return types");
    static_assert(sdbus::function_argument_count_v<Fnc> == 0, "Incorrectly detected free function parameter count");
    static_assert(std::is_void<sdbus::function_result_t<Fnc>>::value, "Incorrectly detected free function return type");
}

TEST(FreeFunctionTypeTraits, DetectsTraitsOfNontrivialSignatureFunction)
{
    std::tuple<char, int> f(double&, const char*, int);
    using Fnc = decltype(f);

    static_assert(!sdbus::is_async_method_v<Fnc>, "Free function incorrectly detected as async method");
    static_assert(std::is_same<sdbus::function_arguments_t<Fnc>, std::tuple<double&, const char*, int>>::value, "Incorrectly detected free function parameters");
    static_assert(std::is_same<sdbus::tuple_of_function_input_arg_types_t<Fnc>, std::tuple<double, const char*, int>>::value, "Incorrectly detected tuple of free function parameters");
    static_assert(std::is_same<sdbus::tuple_of_function_output_arg_types_t<Fnc>, std::tuple<char, int>>::value, "Incorrectly detected tuple of free function return types");
    static_assert(sdbus::function_argument_count_v<Fnc> == 3, "Incorrectly detected free function parameter count");
    static_assert(std::is_same<sdbus::function_result_t<Fnc>, std::tuple<char, int>>::value, "Incorrectly detected free function return type");
}

TEST(FreeFunctionTypeTraits, DetectsTraitsOfAsyncFunction)
{
    void f(sdbus::Result<char, int>, double&, const char*, int);
    using Fnc = decltype(f);

    static_assert(sdbus::is_async_method_v<Fnc>, "Free async function incorrectly detected as sync method");
    static_assert(std::is_same<sdbus::function_arguments_t<Fnc>, std::tuple<double&, const char*, int>>::value, "Incorrectly detected free function parameters");
    static_assert(std::is_same<sdbus::tuple_of_function_input_arg_types_t<Fnc>, std::tuple<double, const char*, int>>::value, "Incorrectly detected tuple of free function parameters");
    static_assert(std::is_same<sdbus::tuple_of_function_output_arg_types_t<Fnc>, std::tuple<char, int>>::value, "Incorrectly detected tuple of free function return types");
    static_assert(sdbus::function_argument_count_v<Fnc> == 3, "Incorrectly detected free function parameter count");
    static_assert(std::is_same<sdbus::function_result_t<Fnc>, std::tuple<char, int>>::value, "Incorrectly detected free function return type");
}
