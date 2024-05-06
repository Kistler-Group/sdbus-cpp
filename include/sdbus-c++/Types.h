/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2024 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
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
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeinfo>
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
     * Otherwise they need to take care of synchronization by themselves.
     *
     ***********************************************/
    class Variant
    {
    public:
        Variant();

        template <typename _ValueType>
        explicit Variant(const _ValueType& value) : Variant()
        {
            msg_.openVariant<_ValueType>();
            msg_ << value;
            msg_.closeVariant();
            msg_.seal();
        }

        template <typename... _Elements>
        Variant(const std::variant<_Elements...>& value)
            : Variant()
        {
            msg_ << value;
            msg_.seal();
        }

        template <typename _ValueType>
        _ValueType get() const
        {
            _ValueType val;
            msg_.rewind(false);

            msg_.enterVariant<_ValueType>();
            msg_ >> val;
            msg_.exitVariant();
            return val;
        }

        // Only allow conversion operator for true D-Bus type representations in C++
        template <typename _ValueType, typename = std::enable_if_t<signature_of<_ValueType>::is_valid>>
        explicit operator _ValueType() const
        {
            return get<_ValueType>();
        }

        template <typename... _Elements>
        operator std::variant<_Elements...>() const
        {
            std::variant<_Elements...> result;
            msg_.rewind(false);
            msg_ >> result;
            return result;
        }

        template <typename _Type>
        bool containsValueOfType() const
        {
            constexpr auto signature = as_null_terminated(signature_of_v<_Type>);
            return std::strcmp(signature.data(), peekValueType()) == 0;
        }

        bool isEmpty() const;

        void serializeTo(Message& msg) const;
        void deserializeFrom(Message& msg);
        const char* peekValueType() const;

    private:
        mutable PlainMessage msg_{};
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
    template <typename... _ValueTypes>
    class Struct
        : public std::tuple<_ValueTypes...>
    {
    public:
        using std::tuple<_ValueTypes...>::tuple;

        Struct() = default;

        explicit Struct(const std::tuple<_ValueTypes...>& t)
            : std::tuple<_ValueTypes...>(t)
        {
        }

        template <std::size_t _I>
        auto& get()
        {
            return std::get<_I>(*this);
        }

        template <std::size_t _I>
        const auto& get() const
        {
            return std::get<_I>(*this);
        }
    };

    template <typename... _Elements>
    Struct(_Elements...) -> Struct<_Elements...>;

    template<typename... _Elements>
    constexpr Struct<std::decay_t<_Elements>...>
    make_struct(_Elements&&... args)
    {
        typedef Struct<std::decay_t<_Elements>...> result_type;
        return result_type(std::forward<_Elements>(args)...);
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

        UnixFd(UnixFd&& other)
        {
            *this = std::move(other);
        }

        UnixFd& operator=(UnixFd&& other)
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
    template<typename _T1, typename _T2>
    using DictEntry = std::pair<_T1, _T2>;

}

// Making sdbus::Struct implement the tuple-protocol, i.e. be a tuple-like type
template <size_t _I, typename... _ValueTypes>
struct std::tuple_element<_I, sdbus::Struct<_ValueTypes...>>
    : std::tuple_element<_I, std::tuple<_ValueTypes...>>
{};
template <typename... _ValueTypes>
struct std::tuple_size<sdbus::Struct<_ValueTypes...>>
    : std::tuple_size<std::tuple<_ValueTypes...>>
{};

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
 * The macro effectively generates the `sdbus::Message` serialization
 * and deserialization operators and the `sdbus::signature_of`
 * specialization for `foo::ABC`.
 *
 * Up to 16 struct members are supported by the macro.
 *
 ***********************************************/
#define SDBUSCPP_REGISTER_STRUCT(STRUCT, ...)                                                                   \
    namespace sdbus {                                                                                           \
        static_assert(SDBUSCPP_PP_NARG(__VA_ARGS__) <= 16,                                                      \
                     "Not more than 16 struct members are supported, please open an issue if you need more");   \
        sdbus::Message& operator<<(sdbus::Message& msg, const STRUCT& items)                                    \
        {                                                                                                       \
            return msg << sdbus::Struct{std::forward_as_tuple(SDBUSCPP_STRUCT_MEMBERS(items, __VA_ARGS__))};    \
        }                                                                                                       \
        sdbus::Message& operator>>(sdbus::Message& msg, STRUCT& items)                                          \
        {                                                                                                       \
            sdbus::Struct s{std::forward_as_tuple(SDBUSCPP_STRUCT_MEMBERS(items, __VA_ARGS__))};                \
            return msg >> s;                                                                                    \
        }                                                                                                       \
        template <>                                                                                             \
        struct signature_of<STRUCT>                                                                             \
            : signature_of<sdbus::Struct<SDBUSCPP_STRUCT_MEMBER_TYPES(STRUCT, __VA_ARGS__)>>                    \
        {};                                                                                                     \
    }                                                                                                           \
    /**/

/*!
 * @cond SDBUSCPP_INTERNAL
 *
 * Internal helper preprocessor facilities
 */
#define SDBUSCPP_STRUCT_MEMBERS(STRUCT, ...)                                                                    \
    SDBUSCPP_PP_CAT(SDBUSCPP_STRUCT_MEMBERS_, SDBUSCPP_PP_NARG(__VA_ARGS__))(STRUCT, __VA_ARGS__)               \
    /**/
#define SDBUSCPP_STRUCT_MEMBERS_1(S, M1) S.M1
#define SDBUSCPP_STRUCT_MEMBERS_2(S, M1, M2) S.M1, S.M2
#define SDBUSCPP_STRUCT_MEMBERS_3(S, M1, M2, M3) S.M1, S.M2, S.M3
#define SDBUSCPP_STRUCT_MEMBERS_4(S, M1, M2, M3, M4) S.M1, S.M2, S.M3, S.M4
#define SDBUSCPP_STRUCT_MEMBERS_5(S, M1, M2, M3, M4, M5) S.M1, S.M2, S.M3, S.M4, S.M5
#define SDBUSCPP_STRUCT_MEMBERS_6(S, M1, M2, M3, M4, M5, M6) S.M1, S.M2, S.M3, S.M4, S.M5, S.M6
#define SDBUSCPP_STRUCT_MEMBERS_7(S, M1, M2, M3, M4, M5, M6, M7) S.M1, S.M2, S.M3, S.M4, S.M5, S.M6, S.M7
#define SDBUSCPP_STRUCT_MEMBERS_8(S, M1, M2, M3, M4, M5, M6, M7, M8) S.M1, S.M2, S.M3, S.M4, S.M5, S.M6, S.M7, S.M8
#define SDBUSCPP_STRUCT_MEMBERS_9(S, M1, M2, M3, M4, M5, M6, M7, M8, M9) S.M1, S.M2, S.M3, S.M4, S.M5, S.M6, S.M7, S.M8, S.M9
#define SDBUSCPP_STRUCT_MEMBERS_10(S, M1, M2, M3, M4, M5, M6, M7, M8, M9, M10) S.M1, S.M2, S.M3, S.M4, S.M5, S.M6, S.M7, S.M8, S.M9, S.M10
#define SDBUSCPP_STRUCT_MEMBERS_11(S, M1, M2, M3, M4, M5, M6, M7, M8, M9, M10, M11) S.M1, S.M2, S.M3, S.M4, S.M5, S.M6, S.M7, S.M8, S.M9, S.M10, S.M11
#define SDBUSCPP_STRUCT_MEMBERS_12(S, M1, M2, M3, M4, M5, M6, M7, M8, M9, M10, M11, M12) S.M1, S.M2, S.M3, S.M4, S.M5, S.M6, S.M7, S.M8, S.M9, S.M10, S.M11, S.M12
#define SDBUSCPP_STRUCT_MEMBERS_13(S, M1, M2, M3, M4, M5, M6, M7, M8, M9, M10, M11, M12, M13) S.M1, S.M2, S.M3, S.M4, S.M5, S.M6, S.M7, S.M8, S.M9, S.M10, S.M11, S.M12, S.M13
#define SDBUSCPP_STRUCT_MEMBERS_14(S, M1, M2, M3, M4, M5, M6, M7, M8, M9, M10, M11, M12, M13, M14) S.M1, S.M2, S.M3, S.M4, S.M5, S.M6, S.M7, S.M8, S.M9, S.M10, S.M11, S.M12, S.M13, S.M14
#define SDBUSCPP_STRUCT_MEMBERS_15(S, M1, M2, M3, M4, M5, M6, M7, M8, M9, M10, M11, M12, M13, M14, M15) S.M1, S.M2, S.M3, S.M4, S.M5, S.M6, S.M7, S.M8, S.M9, S.M10, S.M11, S.M12, S.M13, S.M14, S.M15
#define SDBUSCPP_STRUCT_MEMBERS_16(S, M1, M2, M3, M4, M5, M6, M7, M8, M9, M10, M11, M12, M13, M14, M15, M16) S.M1, S.M2, S.M3, S.M4, S.M5, S.M6, S.M7, S.M8, S.M9, S.M10, S.M11, S.M12, S.M13, S.M14, S.M15, S.M16

#define SDBUSCPP_STRUCT_MEMBER_TYPES(STRUCT, ...)                                                               \
    SDBUSCPP_PP_CAT(SDBUSCPP_STRUCT_MEMBER_TYPES_, SDBUSCPP_PP_NARG(__VA_ARGS__))(STRUCT, __VA_ARGS__)          \
    /**/
#define SDBUSCPP_STRUCT_MEMBER_TYPES_1(S, M1) decltype(S::M1)
#define SDBUSCPP_STRUCT_MEMBER_TYPES_2(S, M1, M2) decltype(S::M1), decltype(S::M2)
#define SDBUSCPP_STRUCT_MEMBER_TYPES_3(S, M1, M2, M3) decltype(S::M1), decltype(S::M2), decltype(S::M3)
#define SDBUSCPP_STRUCT_MEMBER_TYPES_4(S, M1, M2, M3, M4) decltype(S::M1), decltype(S::M2), decltype(S::M3), decltype(S::M4)
#define SDBUSCPP_STRUCT_MEMBER_TYPES_5(S, M1, M2, M3, M4, M5) decltype(S::M1), decltype(S::M2), decltype(S::M3), decltype(S::M4), decltype(S::M5)
#define SDBUSCPP_STRUCT_MEMBER_TYPES_6(S, M1, M2, M3, M4, M5, M6) decltype(S::M1), decltype(S::M2), decltype(S::M3), decltype(S::M4), decltype(S::M5), decltype(S::M6)
#define SDBUSCPP_STRUCT_MEMBER_TYPES_7(S, M1, M2, M3, M4, M5, M6, M7) decltype(S::M1), decltype(S::M2), decltype(S::M3), decltype(S::M4), decltype(S::M5), decltype(S::M6), decltype(S::M7)
#define SDBUSCPP_STRUCT_MEMBER_TYPES_8(S, M1, M2, M3, M4, M5, M6, M7, M8) decltype(S::M1), decltype(S::M2), decltype(S::M3), decltype(S::M4), decltype(S::M5), decltype(S::M6), decltype(S::M7), decltype(S::M8)
#define SDBUSCPP_STRUCT_MEMBER_TYPES_9(S, M1, M2, M3, M4, M5, M6, M7, M8, M9) decltype(S::M1), decltype(S::M2), decltype(S::M3), decltype(S::M4), decltype(S::M5), decltype(S::M6), decltype(S::M7), decltype(S::M8), decltype(S::M9)
#define SDBUSCPP_STRUCT_MEMBER_TYPES_10(S, M1, M2, M3, M4, M5, M6, M7, M8, M9, M10) decltype(S::M1), decltype(S::M2), decltype(S::M3), decltype(S::M4), decltype(S::M5), decltype(S::M6), decltype(S::M7), decltype(S::M8), decltype(S::M9), decltype(S::M10)
#define SDBUSCPP_STRUCT_MEMBER_TYPES_11(S, M1, M2, M3, M4, M5, M6, M7, M8, M9, M10, M11) decltype(S::M1), decltype(S::M2), decltype(S::M3), decltype(S::M4), decltype(S::M5), decltype(S::M6), decltype(S::M7), decltype(S::M8), decltype(S::M9), decltype(S::M10), decltype(S::M11)
#define SDBUSCPP_STRUCT_MEMBER_TYPES_12(S, M1, M2, M3, M4, M5, M6, M7, M8, M9, M10, M11, M12) decltype(S::M1), decltype(S::M2), decltype(S::M3), decltype(S::M4), decltype(S::M5), decltype(S::M6), decltype(S::M7), decltype(S::M8), decltype(S::M9), decltype(S::M10), decltype(S::M11), decltype(S::M12)
#define SDBUSCPP_STRUCT_MEMBER_TYPES_13(S, M1, M2, M3, M4, M5, M6, M7, M8, M9, M10, M11, M12, M13) decltype(S::M1), decltype(S::M2), decltype(S::M3), decltype(S::M4), decltype(S::M5), decltype(S::M6), decltype(S::M7), decltype(S::M8), decltype(S::M9), decltype(S::M10), decltype(S::M11), decltype(S::M12), decltype(S::M13)
#define SDBUSCPP_STRUCT_MEMBER_TYPES_14(S, M1, M2, M3, M4, M5, M6, M7, M8, M9, M10, M11, M12, M13, M14) decltype(S::M1), decltype(S::M2), decltype(S::M3), decltype(S::M4), decltype(S::M5), decltype(S::M6), decltype(S::M7), decltype(S::M8), decltype(S::M9), decltype(S::M10), decltype(S::M11), decltype(S::M12), decltype(S::M13), decltype(S::M14)
#define SDBUSCPP_STRUCT_MEMBER_TYPES_15(S, M1, M2, M3, M4, M5, M6, M7, M8, M9, M10, M11, M12, M13, M14, M15) decltype(S::M1), decltype(S::M2), decltype(S::M3), decltype(S::M4), decltype(S::M5), decltype(S::M6), decltype(S::M7), decltype(S::M8), decltype(S::M9), decltype(S::M10), decltype(S::M11), decltype(S::M12), decltype(S::M13), decltype(S::M14), decltype(S::M15)
#define SDBUSCPP_STRUCT_MEMBER_TYPES_16(S, M1, M2, M3, M4, M5, M6, M7, M8, M9, M10, M11, M12, M13, M14, M15, M16) decltype(S::M1), decltype(S::M2), decltype(S::M3), decltype(S::M4), decltype(S::M5), decltype(S::M6), decltype(S::M7), decltype(S::M8), decltype(S::M9), decltype(S::M10), decltype(S::M11), decltype(S::M12), decltype(S::M13), decltype(S::M14), decltype(S::M15), decltype(S::M16)

#define SDBUSCPP_PP_CAT(X, Y) SDBUSCPP_PP_CAT_IMPL(X, Y)
#define SDBUSCPP_PP_CAT_IMPL(X, Y) X##Y
#define SDBUSCPP_PP_NARG(...) SDBUSCPP_PP_NARG_IMPL(__VA_ARGS__, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define SDBUSCPP_PP_NARG_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _N, ...) _N

#endif /* SDBUS_CXX_TYPES_H_ */
