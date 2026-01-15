/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2026 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file Types.h
 *
 * Created on: Nov 23, 2016
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

#ifndef SDBUS_CXX_TYPES_H_
#define SDBUS_CXX_TYPES_H_

#include <sdbus-c++/Message.h>
#include <sdbus-c++/TypeTraits.h>

#include <cstring>
#include <cstddef>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace sdbus {

    /********************************************//**
     * @class Variant
     *
     * Variant can hold value of any D-Bus-supported type.
     *
     * Note: Even though thread-aware, Variant objects are not thread-safe.
     * Some const methods are conceptually const, but not physically const,
     * thus are not thread-safe. This is by design: normally, clients
     * should process a single Variant object in a single thread at a time.
     * Otherwise, they need to take care of synchronization by themselves.
     *
     ***********************************************/
    class Variant
    {
    public:
        Variant();

        template <typename ValueType>
        explicit Variant(const ValueType& value) : Variant()
        {
            msg_.openVariant<ValueType>();
            msg_ << value;
            msg_.closeVariant();
            msg_.seal();
        }

        Variant(const Variant& value, embed_variant_t) : Variant()
        {
            msg_.openVariant<Variant>();
            msg_ << value;
            msg_.closeVariant();
            msg_.seal();
        }

        template <typename Struct>
        explicit Variant(const as_dictionary<Struct>& value) : Variant()
        {
            msg_.openVariant<std::map<std::string, Variant>>();
            msg_ << as_dictionary(value.m_struct);
            msg_.closeVariant();
            msg_.seal();
        }

        template <typename... Elements>
        Variant(const std::variant<Elements...>& value) // NOLINT(google-explicit-constructor,hicpp-explicit-conversions): implicit conversion intentional
            : Variant()
        {
            msg_ << value;
            msg_.seal();
        }

        template <typename ValueType>
        ValueType get() const
        {
            msg_.rewind(false);

            msg_.enterVariant<ValueType>();
            ValueType val;
            msg_ >> val;
            msg_.exitVariant();
            return val;
        }

        // Only allow conversion operator for true D-Bus type representations in C++
        // NOLINTNEXTLINE(modernize-use-constraints): TODO for future: Use `requires signature_of<_ValueType>::is_valid` (when we stop supporting C++17 in public API)
        template <typename ValueType, typename = std::enable_if_t<signature_of<ValueType>::is_valid>>
        explicit operator ValueType() const
        {
            return get<ValueType>();
        }

        template <typename... Elements>
        operator std::variant<Elements...>() const // NOLINT(google-explicit-constructor,hicpp-explicit-conversions): implicit conversion intentional
        {
            std::variant<Elements...> result;
            msg_.rewind(false);
            msg_ >> result;
            return result;
        }

        template <typename Type>
        bool containsValueOfType() const
        {
            constexpr auto signature = as_null_terminated(signature_of_v<Type>);
            return std::strcmp(signature.data(), peekValueType()) == 0;
        }

        bool isEmpty() const;

        void serializeTo(Message& msg) const;
        void deserializeFrom(Message& msg);
        const char* peekValueType() const;

    private:
        mutable PlainMessage msg_;
    };

    /********************************************//**
     * @class Struct
     *
     * Representation of struct D-Bus type
     *
     * Struct implements tuple protocol, i.e. it's a tuple-like class.
     * It can be used with std::get<>(), std::tuple_element,
     * std::tuple_size and in structured bindings.
     *
     ***********************************************/
    template <typename... ValueTypes>
    class Struct
        : public std::tuple<ValueTypes...>
    {
    public:
        using std::tuple<ValueTypes...>::tuple;

        Struct() = default;

        explicit Struct(const std::tuple<ValueTypes...>& tuple)
            : std::tuple<ValueTypes...>(tuple)
        {
        }

        template <std::size_t I>
        [[nodiscard]] auto& get()
        {
            return std::get<I>(*this);
        }

        template <std::size_t I>
        [[nodiscard]] const auto& get() const
        {
            return std::get<I>(*this);
        }
    };

    template <typename... Elements>
    Struct(Elements...) -> Struct<Elements...>;

    template <typename... Elements>
    Struct(const std::tuple<Elements...>&) -> Struct<Elements...>;

    template <typename... Elements>
    Struct(std::tuple<Elements...>&&) -> Struct<Elements...>;

    template<typename... Elements>
    constexpr Struct<std::decay_t<Elements>...>
    make_struct(Elements&&... args)
    {
        using result_type = Struct<std::decay_t<Elements>...>;
        return result_type(std::forward<Elements>(args)...);
    }

    /********************************************//**
     * @class ObjectPath
     *
     * Strong type representing the D-Bus object path
     *
     ***********************************************/
    class ObjectPath : public std::string
    {
    public:
        ObjectPath() = default;
        explicit ObjectPath(std::string value)
            : std::string(std::move(value))
        {}
        explicit ObjectPath(const char* value)
            : std::string(value)
        {}

        using std::string::operator=;
    };

    /********************************************//**
     * @class BusName
     *
     * Strong type representing the D-Bus bus/service/connection name
     *
     ***********************************************/
    class BusName : public std::string
    {
    public:
        BusName() = default;
        explicit BusName(std::string value)
            : std::string(std::move(value))
        {}
        explicit BusName(const char* value)
            : std::string(value)
        {}

        using std::string::operator=;
    };

    using ServiceName = BusName;
    using ConnectionName = BusName;

    /********************************************//**
     * @class InterfaceName
     *
     * Strong type representing the D-Bus interface name
     *
     ***********************************************/
    class InterfaceName : public std::string
    {
    public:
        InterfaceName() = default;
        explicit InterfaceName(std::string value)
            : std::string(std::move(value))
        {}
        explicit InterfaceName(const char* value)
            : std::string(value)
        {}

        using std::string::operator=;
    };

    /********************************************//**
     * @class MemberName
     *
     * Strong type representing the D-Bus member name
     *
     ***********************************************/
    class MemberName : public std::string
    {
    public:
        MemberName() = default;
        explicit MemberName(std::string value)
                : std::string(std::move(value))
        {}
        explicit MemberName(const char* value)
                : std::string(value)
        {}

        using std::string::operator=;
    };

    using MethodName = MemberName;
    using SignalName = MemberName;
    using PropertyName = MemberName;

    /********************************************//**
     * @class Signature
     *
     * Strong type representing the D-Bus object path
     *
     ***********************************************/
    class Signature : public std::string
    {
    public:
        Signature() = default;
        explicit Signature(std::string value)
            : std::string(std::move(value))
        {}
        explicit Signature(const char* value)
            : std::string(value)
        {}

        using std::string::operator=;
    };

    /********************************************//**
     * @struct UnixFd
     *
     * UnixFd is a representation of file descriptor D-Bus type that owns
     * the underlying fd, provides access to it, and closes the fd when
     * the UnixFd goes out of scope.
     *
     * UnixFd can be default constructed (owning invalid fd), or constructed from
     * an explicitly provided fd by either duplicating or adopting that fd as-is.
     *
     ***********************************************/
    class UnixFd
    {
    public:
        UnixFd() = default;

        explicit UnixFd(int fd)
            : fd_(checkedDup(fd))
        {
        }

        UnixFd(int fd, adopt_fd_t)
            : fd_(fd)
        {
        }

        UnixFd(const UnixFd& other)
        {
            *this = other;
        }

        UnixFd& operator=(const UnixFd& other)
        {
            if (this == &other)
            {
                return *this;
            }
            close();
            fd_ = checkedDup(other.fd_);
            return *this;
        }

        UnixFd(UnixFd&& other) noexcept
        {
            *this = std::move(other);
        }

        UnixFd& operator=(UnixFd&& other) noexcept
        {
            if (this == &other)
            {
                return *this;
            }
            close();
            fd_ = std::exchange(other.fd_, -1);
            return *this;
        }

        ~UnixFd()
        {
            close();
        }

        [[nodiscard]] int get() const
        {
            return fd_;
        }

        void reset(int fd = -1)
        {
            *this = UnixFd{fd};
        }

        void reset(int fd, adopt_fd_t)
        {
            *this = UnixFd{fd, adopt_fd};
        }

        int release()
        {
            return std::exchange(fd_, -1);
        }

        [[nodiscard]] bool isValid() const
        {
            return fd_ >= 0;
        }

    private:
        /// Closes file descriptor, but does not set it to -1.
        void close();

        /// Returns negative argument unchanged.
        /// Otherwise, call ::dup and throw on failure.
        static int checkedDup(int fd);

        int fd_ = -1;
    };

    /********************************************//**
     * @typedef DictEntry
     *
     * DictEntry is implemented as std::pair, a standard
     * value_type in STL(-like) associative containers.
     *
     ***********************************************/
    template<typename T1, typename T2>
    using DictEntry = std::pair<T1, T2>;

} // namespace sdbus

// Making sdbus::Struct implement the tuple-protocol, i.e. be a tuple-like type
template <size_t I, typename... ValueTypes>
struct std::tuple_element<I, sdbus::Struct<ValueTypes...>> // NOLINT(cert-dcl58-cpp): specialization in std namespace allowed in this case
    : std::tuple_element<I, std::tuple<ValueTypes...>>
{};
template <typename... ValueTypes>
struct std::tuple_size<sdbus::Struct<ValueTypes...>> // NOLINT(cert-dcl58-cpp): specialization in std namespace allowed in this case
    : std::tuple_size<std::tuple<ValueTypes...>>
{};

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

/********************************************//**
 * @name SDBUSCPP_REGISTER_STRUCT
 *
 * A convenient way to extend sdbus-c++ type system with user-defined structs.
 *
 * The macro teaches sdbus-c++ to recognize the user-defined struct
 * as a valid C++ representation of a D-Bus Struct type, and enables
 * clients to use their struct conveniently instead of the (too
 * generic and less expressive) `sdbus::Struct<...>` in sdbus-c++ API.
 *
 * It also enables to serialize a user-defined struct as an a{sv} dictionary,
 * and to deserialize an a{sv} dictionary into the user-defined struct.
 *
 * The first argument is the struct type name and the remaining arguments
 * are names of struct members. Members must be of types supported by
 * sdbus-c++ (or of user-defined types that sdbus-c++ was taught to support).
 * Members can be other structs (nesting is supported).
 * The macro must be placed in the global namespace.
 *
 * For example, given the user-defined struct `ABC`:
 *
 * namespace foo {
 *     struct ABC
 *     {
 *         int number;
 *         std::string name;
 *         std::vector<double> data;
 *     };
 * }
 *
 * one can teach sdbus-c++ about the contents of this struct simply with:
 *
 * SDBUSCPP_REGISTER_STRUCT(foo::ABC, number, name, data);
 *
 * Up to 16 struct members are supported by the macro.
 *
 ***********************************************/
#define SDBUSCPP_REGISTER_STRUCT(STRUCT, ...)                                                                                                           \
    namespace sdbus {                                                                                                                                   \
        static_assert(SDBUSCPP_PP_NARG(__VA_ARGS__) <= 16,                                                                                              \
                     "Not more than 16 struct members are supported, please open an issue if you need more");                                           \
                                                                                                                                                        \
        template <>                                                                                                                                     \
        struct signature_of<STRUCT>                                                                                                                     \
            : signature_of<Struct<SDBUSCPP_STRUCT_MEMBER_TYPES(STRUCT, __VA_ARGS__)>>                                                                   \
        {};                                                                                                                                             \
                                                                                                                                                        \
        inline auto as_dictionary_if_struct(const STRUCT& object)                                                                                       \
        {                                                                                                                                               \
            return as_dictionary<STRUCT>(object);                                                                                                       \
        }                                                                                                                                               \
                                                                                                                                                        \
        inline Message& operator<<(Message& msg, const STRUCT& items)                                                                                   \
        {                                                                                                                                               \
            return msg << Struct{std::forward_as_tuple(SDBUSCPP_STRUCT_MEMBERS(items, __VA_ARGS__))};                                                   \
        }                                                                                                                                               \
                                                                                                                                                        \
        inline Message& operator<<(Message& msg, const as_dictionary<STRUCT>& s)                                                                        \
        {                                                                                                                                               \
            if constexpr (!nested_struct_as_dict_serialization_v<STRUCT>)                                                                               \
                return msg.serializeDictionary<std::string, Variant>({SDBUSCPP_STRUCT_MEMBERS_AS_DICT_ENTRIES(s.m_struct, __VA_ARGS__)});               \
            else                                                                                                                                        \
                return msg.serializeDictionary<std::string, Variant>({SDBUSCPP_STRUCT_MEMBERS_AS_NESTED_DICT_ENTRIES(s.m_struct, __VA_ARGS__)});        \
        }                                                                                                                                               \
                                                                                                                                                        \
        inline Message& operator>>(Message& msg, STRUCT& s)                                                                                             \
        {                                                                                                                                               \
            /* First, try to deserialize as a struct */                                                                                                 \
            if (msg.peekType().first == signature_of<STRUCT>::type_value)                                                                               \
            {                                                                                                                                           \
                Struct sdbusStruct{std::forward_as_tuple(SDBUSCPP_STRUCT_MEMBERS(s, __VA_ARGS__))};                                                     \
                return msg >> sdbusStruct;                                                                                                              \
            }                                                                                                                                           \
                                                                                                                                                        \
            /* Otherwise try to deserialize as a dictionary of strings to variants */                                                                   \
                                                                                                                                                        \
            return msg.deserializeDictionary<std::string, Variant>([&s](const auto& dictEntry)                                                          \
            {                                                                                                                                           \
                const std::string& key = dictEntry.first; /* Intentionally not using structured bindings */                                             \
                const Variant& value = dictEntry.second;                                                                                                \
                                                                                                                                                        \
                using namespace std::string_literals;                                                                                                   \
                /* This also handles members which are structs serialized as dict of strings to variants, recursively */                                \
                SDBUSCPP_FIND_AND_DESERIALIZE_STRUCT_MEMBERS(s, __VA_ARGS__)                                                                            \
                SDBUS_THROW_ERROR_IF( strict_dict_as_struct_deserialization_v<STRUCT>                                                                   \
                                    , ("Failed to deserialize struct from a dictionary: could not find field '"s += key) += "' in struct 'my::Struct'"  \
                                    , EINVAL );                                                                                                         \
            });                                                                                                                                         \
        }                                                                                                                                               \
    }                                                                                                                                                   \
    /**/

/********************************************//**
 * @name SDBUSCPP_ENABLE_RELAXED_DICT2STRUCT_DESERIALIZATION
 *
 * Enables relaxed deserialization of an a{sv} dictionary into a user-defined struct STRUCT.
 *
 * The default (strict) deserialization mode is that if there are entries in the dictionary
 * which do not have a corresponding field in the struct, an exception is thrown.
 * In the relaxed mode, such entries are silently skipped.
 *
 * The macro can only be used in combination with SDBUSCPP_REGISTER_STRUCT macro,
 * and must be placed before SDBUSCPP_REGISTER_STRUCT macro.
 ***********************************************/
#define SDBUSCPP_ENABLE_RELAXED_DICT2STRUCT_DESERIALIZATION(STRUCT)                                             \
    template <>                                                                                                 \
    constexpr auto sdbus::strict_dict_as_struct_deserialization_v<STRUCT> = false;                              \
    /**/

/********************************************//**
 * @name SDBUSCPP_ENABLE_NESTED_STRUCT2DICT_SERIALIZATION
 *
 * Enables nested serialization of user-defined struct STRUCT as an a{sv} dictionary.
 *
 * By default, STRUCT fields which are structs themselves are serialized as D-Bus structs.
 * This macro tells sdbus-c++ to also serialize nested structs, in a recursive fashion,
 * as a{sv} dictionaries.
 *
 * The macro can only be used in combination with SDBUSCPP_REGISTER_STRUCT macro,
 * and must be placed before SDBUSCPP_REGISTER_STRUCT macro.
 ***********************************************/
#define SDBUSCPP_ENABLE_NESTED_STRUCT2DICT_SERIALIZATION(STRUCT)                                                \
    template <>                                                                                                 \
    constexpr auto sdbus::nested_struct_as_dict_serialization_v<STRUCT> = true                                  \
    /**/

/*!
 * @cond SDBUSCPP_INTERNAL
 *
 * Internal helper preprocessor facilities
 */
#define SDBUSCPP_STRUCT_MEMBERS(STRUCT, ...)                                                                                                                    \
    SDBUSCPP_PP_CAT(SDBUSCPP_FOR_EACH_, SDBUSCPP_PP_NARG(__VA_ARGS__))(SDBUSCPP_STRUCT_MEMBER, SDBUSCPP_PP_COMMA, STRUCT, __VA_ARGS__)                          \
    /**/
#define SDBUSCPP_STRUCT_MEMBER(STRUCT, MEMBER) STRUCT.MEMBER

#define SDBUSCPP_STRUCT_MEMBER_TYPES(STRUCT, ...)                                                                                                               \
    SDBUSCPP_PP_CAT(SDBUSCPP_FOR_EACH_, SDBUSCPP_PP_NARG(__VA_ARGS__))(SDBUSCPP_STRUCT_MEMBER_TYPE, SDBUSCPP_PP_COMMA, STRUCT, __VA_ARGS__)                     \
    /**/
#define SDBUSCPP_STRUCT_MEMBER_TYPE(STRUCT, MEMBER) decltype(STRUCT::MEMBER)

#define SDBUSCPP_STRUCT_MEMBERS_AS_DICT_ENTRIES(STRUCT, ...)                                                                                                    \
    SDBUSCPP_PP_CAT(SDBUSCPP_FOR_EACH_, SDBUSCPP_PP_NARG(__VA_ARGS__))(SDBUSCPP_STRUCT_MEMBER_AS_DICT_ENTRY, SDBUSCPP_PP_COMMA, STRUCT, __VA_ARGS__)            \
    /**/
#define SDBUSCPP_STRUCT_MEMBER_AS_DICT_ENTRY(STRUCT, MEMBER) {#MEMBER, Variant{STRUCT.MEMBER}}

#define SDBUSCPP_STRUCT_MEMBERS_AS_NESTED_DICT_ENTRIES(STRUCT, ...)                                                                                             \
    SDBUSCPP_PP_CAT(SDBUSCPP_FOR_EACH_, SDBUSCPP_PP_NARG(__VA_ARGS__))(SDBUSCPP_STRUCT_MEMBER_AS_NESTED_DICT_ENTRY, SDBUSCPP_PP_COMMA, STRUCT, __VA_ARGS__)     \
    /**/
#define SDBUSCPP_STRUCT_MEMBER_AS_NESTED_DICT_ENTRY(STRUCT, MEMBER) {#MEMBER, Variant{as_dictionary_if_struct(STRUCT.MEMBER)}}

#define SDBUSCPP_FIND_AND_DESERIALIZE_STRUCT_MEMBERS(STRUCT, ...)                                                                                               \
    SDBUSCPP_PP_CAT(SDBUSCPP_FOR_EACH_, SDBUSCPP_PP_NARG(__VA_ARGS__))(SDBUSCPP_FIND_AND_DESERIALIZE_STRUCT_MEMBER, SDBUSCPP_PP_SPACE, STRUCT, __VA_ARGS__)     \
    /**/
#define SDBUSCPP_FIND_AND_DESERIALIZE_STRUCT_MEMBER(STRUCT, MEMBER) if (key == #MEMBER) STRUCT.MEMBER = value.get<decltype(STRUCT.MEMBER)>(); else

#define SDBUSCPP_FOR_EACH_1(M, D, S, M1) M(S, M1)
#define SDBUSCPP_FOR_EACH_2(M, D, S, M1, M2) M(S, M1) D M(S, M2)
#define SDBUSCPP_FOR_EACH_3(M, D, S, M1, M2, M3) M(S, M1) D M(S, M2) D M(S, M3)
#define SDBUSCPP_FOR_EACH_4(M, D, S, M1, M2, M3, M4) M(S, M1) D M(S, M2) D M(S, M3) D M(S, M4)
#define SDBUSCPP_FOR_EACH_5(M, D, S, M1, M2, M3, M4, M5) M(S, M1) D M(S, M2) D M(S, M3) D M(S, M4) D M(S, M5)
#define SDBUSCPP_FOR_EACH_6(M, D, S, M1, M2, M3, M4, M5, M6) M(S, M1) D M(S, M2) D M(S, M3) D M(S, M4) D M(S, M5) D M(S, M6)
#define SDBUSCPP_FOR_EACH_7(M, D, S, M1, M2, M3, M4, M5, M6, M7) M(S, M1) D M(S, M2) D M(S, M3) D M(S, M4) D M(S, M5) D M(S, M6) D M(S, M7)
#define SDBUSCPP_FOR_EACH_8(M, D, S, M1, M2, M3, M4, M5, M6, M7, M8) M(S, M1) D M(S, M2) D M(S, M3) D M(S, M4) D M(S, M5) D M(S, M6) D M(S, M7) D M(S, M8)
#define SDBUSCPP_FOR_EACH_9(M, D, S, M1, M2, M3, M4, M5, M6, M7, M8, M9) M(S, M1) D M(S, M2) D M(S, M3) D M(S, M4) D M(S, M5) D M(S, M6) D M(S, M7) D M(S, M8) D M(S, M9)
#define SDBUSCPP_FOR_EACH_10(M, D, S, M1, M2, M3, M4, M5, M6, M7, M8, M9, M10) M(S, M1) D M(S, M2) D M(S, M3) D M(S, M4) D M(S, M5) D M(S, M6) D M(S, M7) D M(S, M8) D M(S, M9) D M(S, M10)
#define SDBUSCPP_FOR_EACH_11(M, D, S, M1, M2, M3, M4, M5, M6, M7, M8, M9, M10, M11) M(S, M1) D M(S, M2) D M(S, M3) D M(S, M4) D M(S, M5) D M(S, M6) D M(S, M7) D M(S, M8) D M(S, M9) D M(S, M10) D M(S, M11)
#define SDBUSCPP_FOR_EACH_12(M, D, S, M1, M2, M3, M4, M5, M6, M7, M8, M9, M10, M11, M12) M(S, M1) D M(S, M2) D M(S, M3) D M(S, M4) D M(S, M5) D M(S, M6) D M(S, M7) D M(S, M8) D M(S, M9) D M(S, M10) D M(S, M11) D M(S, M12)
#define SDBUSCPP_FOR_EACH_13(M, D, S, M1, M2, M3, M4, M5, M6, M7, M8, M9, M10, M11, M12, M13) M(S, M1) D M(S, M2) D M(S, M3) D M(S, M4) D M(S, M5) D M(S, M6) D M(S, M7) D M(S, M8) D M(S, M9) D M(S, M10) D M(S, M11) D M(S, M12) D M(S, M13)
#define SDBUSCPP_FOR_EACH_14(M, D, S, M1, M2, M3, M4, M5, M6, M7, M8, M9, M10, M11, M12, M13, M14) M(S, M1) D M(S, M2) D M(S, M3) D M(S, M4) D M(S, M5) D M(S, M6) D M(S, M7) D M(S, M8) D M(S, M9) D M(S, M10) D M(S, M11) D M(S, M12) D M(S, M13) D M(S, M14)
#define SDBUSCPP_FOR_EACH_15(M, D, S, M1, M2, M3, M4, M5, M6, M7, M8, M9, M10, M11, M12, M13, M14, M15) M(S, M1) D M(S, M2) D M(S, M3) D M(S, M4) D M(S, M5) D M(S, M6) D M(S, M7) D M(S, M8) D M(S, M9) D M(S, M10) D M(S, M11) D M(S, M12) D M(S, M13) D M(S, M14) D M(S, M15)
#define SDBUSCPP_FOR_EACH_16(M, D, S, M1, M2, M3, M4, M5, M6, M7, M8, M9, M10, M11, M12, M13, M14, M15, M16) M(S, M1) D M(S, M2) D M(S, M3) D M(S, M4) D M(S, M5) D M(S, M6) D M(S, M7) D M(S, M8) D M(S, M9) D M(S, M10) D M(S, M11) D M(S, M12) D M(S, M13) D M(S, M14) D M(S, M15) D M(S, M16)

#define SDBUSCPP_PP_CAT(X, Y) SDBUSCPP_PP_CAT_IMPL(X, Y)
#define SDBUSCPP_PP_CAT_IMPL(X, Y) X##Y
#define SDBUSCPP_PP_NARG(...) SDBUSCPP_PP_NARG_IMPL(__VA_ARGS__, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define SDBUSCPP_PP_NARG_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _N, ...) _N

#define SDBUSCPP_PP_COMMA ,
#define SDBUSCPP_PP_SPACE

// NOLINTEND(cppcoreguidelines-macro-usage)

#endif /* SDBUS_CXX_TYPES_H_ */
