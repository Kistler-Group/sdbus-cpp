/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2026 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file TypeTraits.h
 *
 * Created on: Nov 9, 2016
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

#ifndef SDBUS_CXX_TYPETRAITS_H_
#define SDBUS_CXX_TYPETRAITS_H_

#include <sdbus-c++/Error.h>

#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#ifdef __has_include
#  if __has_include(<span>)
#    include <span>
#  endif
#endif
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

// Forward declarations
namespace sdbus {
    class Variant;
    template <typename... ValueTypes> class Struct;
    class ObjectPath;
    class Signature;
    class UnixFd;
    template<typename T1, typename T2> using DictEntry = std::pair<T1, T2>;
    class BusName;
    class InterfaceName;
    class MemberName;
    class MethodCall;
    class MethodReply;
    class Signal;
    class Message;
    class PropertySetCall;
    class PropertyGetReply;
    template <typename... Results> class Result;
    class Error;
    template <typename T, typename Enable = void> struct signature_of;
} // namespace sdbus

namespace sdbus {

    // Callbacks from sdbus-c++
    using method_callback = std::function<void(MethodCall msg)>;
    using async_reply_handler = std::function<void(MethodReply reply, std::optional<Error> error)>;
    using signal_handler = std::function<void(Signal signal)>;
    using message_handler = std::function<void(Message msg)>;
    using property_set_callback = std::function<void(PropertySetCall msg)>;
    using property_get_callback = std::function<void(PropertyGetReply& reply)>;

    // Type-erased RAII-style handle to callbacks/subscriptions registered to sdbus-c++
    using Slot = std::unique_ptr<void, std::function<void(void*)>>;

    // Tag specifying that an owning handle (so-called slot) of the logical resource shall be provided to the client
    struct return_slot_t { explicit return_slot_t() = default; };
    inline constexpr return_slot_t return_slot{};
    // Tag specifying that the library shall own the slot resulting from the call of the function (so-called floating slot)
    struct floating_slot_t { explicit floating_slot_t() = default; };
    inline constexpr floating_slot_t floating_slot{};
    // Tag denoting the assumption that the caller has already obtained message ownership
    struct adopt_message_t { explicit adopt_message_t() = default; };
    inline constexpr adopt_message_t adopt_message{};
    // Tag denoting the assumption that the caller has already obtained fd ownership
    struct adopt_fd_t { explicit adopt_fd_t() = default; };
    inline constexpr adopt_fd_t adopt_fd{};
    // Tag specifying that the proxy shall not run an event loop thread on its D-Bus connection.
    // Such proxies are typically created to carry out a simple synchronous D-Bus call(s) and then are destroyed.
    struct dont_run_event_loop_thread_t { explicit dont_run_event_loop_thread_t() = default; };
    inline constexpr dont_run_event_loop_thread_t dont_run_event_loop_thread{};
    // Tag denoting an asynchronous call that returns std::future as a handle
    struct with_future_t { explicit with_future_t() = default; };
    inline constexpr with_future_t with_future{};
    // Tag denoting a call where the reply shouldn't be waited for
    struct dont_expect_reply_t { explicit dont_expect_reply_t() = default; };
    inline constexpr dont_expect_reply_t dont_expect_reply{};
    // Tag denoting that the variant shall embed the other variant as its value, instead of creating a copy
    struct embed_variant_t { explicit embed_variant_t() = default; };
    inline constexpr embed_variant_t embed_variant{};

    // Helper for static assert
    template <class... T> constexpr bool always_false = false;

    // Helper operator+ for concatenation of `std::array`s
    template <typename T, std::size_t N1, std::size_t N2>
    constexpr std::array<T, N1 + N2> operator+(std::array<T, N1> lhs, std::array<T, N2> rhs);

    // Template specializations for getting D-Bus signatures from C++ types
    template <typename T>
    constexpr auto signature_of_v = signature_of<T>::value;

    template <typename T, typename Enable>
    struct signature_of
    {
        static constexpr bool is_valid = false;
        static constexpr bool is_trivial_dbus_type = false;

        static constexpr void* value = []
        {
            // See using-sdbus-c++.md, section "Extending sdbus-c++ type system",
            // on how to teach sdbus-c++ about your custom types
            static_assert(always_false<T>, "Unsupported D-Bus type (specialize `signature_of` for your custom types)");
        };
    };

    template <typename T>
    struct signature_of<const T> : signature_of<T>
    {};

    template <typename T>
    struct signature_of<volatile T> : signature_of<T>
    {};

    template <typename T>
    struct signature_of<const volatile T> : signature_of<T>
    {};

    template <typename T>
    struct signature_of<T&> : signature_of<T>
    {};

    template <typename T>
    struct signature_of<T&&> : signature_of<T>
    {};

    template <>
    struct signature_of<void>
    {
        static constexpr std::array<char, 0> value{};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;
    };

    template <>
    struct signature_of<bool>
    {
        static constexpr std::array value{'b'};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = true;
    };

    template <>
    struct signature_of<uint8_t>
    {
        static constexpr std::array value{'y'};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = true;
    };

    template <>
    struct signature_of<int16_t>
    {
        static constexpr std::array value{'n'};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = true;
    };

    template <>
    struct signature_of<uint16_t>
    {
        static constexpr std::array value{'q'};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = true;
    };

    template <>
    struct signature_of<int32_t>
    {
        static constexpr std::array value{'i'};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = true;
    };

    template <>
    struct signature_of<uint32_t>
    {
        static constexpr std::array value{'u'};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = true;
    };

    template <>
    struct signature_of<int64_t>
    {
        static constexpr std::array value{'x'};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = true;
    };

    template <>
    struct signature_of<uint64_t>
    {
        static constexpr std::array value{'t'};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = true;
    };

    template <>
    struct signature_of<double>
    {
        static constexpr std::array value{'d'};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = true;
    };

    template <>
    struct signature_of<std::string>
    {
        static constexpr std::array value{'s'};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;
    };

    template <>
    struct signature_of<std::string_view> : signature_of<std::string>
    {};

    template <>
    struct signature_of<char*> : signature_of<std::string>
    {};

    template <>
    struct signature_of<const char*> : signature_of<std::string>
    {};

    template <std::size_t N>
    struct signature_of<char[N]> : signature_of<std::string>
    {};

    template <std::size_t N>
    struct signature_of<const char[N]> : signature_of<std::string>
    {};

    template <>
    struct signature_of<BusName> : signature_of<std::string>
    {};

    template <>
    struct signature_of<InterfaceName> : signature_of<std::string>
    {};

    template <>
    struct signature_of<MemberName> : signature_of<std::string>
    {};

    template <typename... ValueTypes>
    struct signature_of<Struct<ValueTypes...>>
    {
        static constexpr std::array contents = (signature_of_v<ValueTypes> + ...);
        static constexpr std::array value = std::array{'('} + contents + std::array{')'};
        static constexpr char type_value{'r'}; /* Not actually used in signatures on D-Bus, see specs */
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;
    };

    template <>
    struct signature_of<Variant>
    {
        static constexpr std::array value{'v'};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;
    };

    template <typename... Elements>
    struct signature_of<std::variant<Elements...>> : signature_of<Variant>
    {};

    template <>
    struct signature_of<ObjectPath>
    {
        static constexpr std::array value{'o'};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;
    };

    template <>
    struct signature_of<Signature>
    {
        static constexpr std::array value{'g'};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;
    };

    template <>
    struct signature_of<UnixFd>
    {
        static constexpr std::array value{'h'};
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;
    };

    template <typename T1, typename T2>
    struct signature_of<DictEntry<T1, T2>>
    {
        static constexpr std::array value = std::array{'{'} + signature_of_v<std::tuple<T1, T2>> + std::array{'}'};
        static constexpr char type_value{'e'}; /* Not actually used in signatures on D-Bus, see specs */
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;
    };

    template <typename Element, typename Allocator>
    struct signature_of<std::vector<Element, Allocator>>
    {
        static constexpr std::array value = std::array{'a'} + signature_of_v<Element>;
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;
    };

    template <typename Element, std::size_t Size>
    struct signature_of<std::array<Element, Size>> : signature_of<std::vector<Element>>
    {
    };

#ifdef __cpp_lib_span
    template <typename Element, std::size_t Extent>
    struct signature_of<std::span<Element, Extent>> : signature_of<std::vector<Element>>
    {
    };
#endif

    template <typename Enum> // is_const_v and is_volatile_v to avoid ambiguity conflicts with const and volatile specializations of signature_of
    struct signature_of<Enum, std::enable_if_t<std::is_enum_v<Enum> && !std::is_const_v<Enum> && !std::is_volatile_v<Enum>>>
        : signature_of<std::underlying_type_t<Enum>>
    {};

    template <typename Key, typename Value, typename Compare, typename Allocator>
    struct signature_of<std::map<Key, Value, Compare, Allocator>>
    {
        static constexpr std::array value = std::array{'a'} + signature_of_v<DictEntry<Key, Value>>;
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;
    };

    template <typename Key, typename Value, typename Hash, typename KeyEqual, typename Allocator>
    struct signature_of<std::unordered_map<Key, Value, Hash, KeyEqual, Allocator>>
        : signature_of<std::map<Key, Value>>
    {
    };

    template <typename... Types>
    struct signature_of<std::tuple<Types...>> // A simple concatenation of signatures of _Types
    {
        static constexpr std::array value = (std::array<char, 0>{} + ... + signature_of_v<Types>);
        static constexpr bool is_valid = false;
        static constexpr bool is_trivial_dbus_type = false;
    };

    // To simplify conversions of arrays to C strings
    template <typename T, std::size_t N>
    constexpr auto as_null_terminated(std::array<T, N> arr)
    {
        return arr + std::array<T, 1>{0};
    }

    // Function traits implementation inspired by (c) kennytm,
    // https://github.com/kennytm/utils/blob/master/traits.hpp
    template <typename Type>
    struct function_traits : function_traits<decltype(&Type::operator())>
    {};

    template <typename Type>
    struct function_traits<const Type> : function_traits<Type>
    {};

    template <typename Type>
    struct function_traits<Type&> : function_traits<Type>
    {};

    template <typename ReturnType, typename... Args>
    struct function_traits_base
    {
        using result_type = ReturnType;
        using arguments_type = std::tuple<Args...>;
        using decayed_arguments_type = std::tuple<std::decay_t<Args>...>;

        using function_type = ReturnType (Args...);

        static constexpr std::size_t arity = sizeof...(Args);

//        template <size_t _Idx, typename _Enabled = void>
//        struct arg;
//
//        template <size_t _Idx>
//        struct arg<_Idx, std::enable_if_t<(_Idx < arity)>>
//        {
//            typedef std::tuple_element_t<_Idx, arguments_type> type;
//        };
//
//        template <size_t _Idx>
//        struct arg<_Idx, std::enable_if_t<!(_Idx < arity)>>
//        {
//            typedef void type;
//        };

        template <size_t Idx>
        struct arg
        {
            using type = std::tuple_element_t<Idx, std::tuple<Args...>>;
        };

        template <size_t Idx>
        using arg_t = typename arg<Idx>::type;
    };

    template <typename ReturnType, typename... Args>
    struct function_traits<ReturnType(Args...)> : function_traits_base<ReturnType, Args...>
    {
        static constexpr bool is_async = false;
        static constexpr bool has_error_param = false;
    };

    template <typename... Args>
    struct function_traits<void(std::optional<Error>, Args...)> : function_traits_base<void, Args...>
    {
        static constexpr bool has_error_param = true;
    };

    template <typename... Args>
    struct function_traits<void(std::optional<Error>&&, Args...)> : function_traits_base<void, Args...>
    {
        static constexpr bool has_error_param = true;
    };

    template <typename... Args>
    struct function_traits<void(const std::optional<Error>&, Args...)> : function_traits_base<void, Args...>
    {
        static constexpr bool has_error_param = true;
    };

    template <typename... Args, typename... Results>
    struct function_traits<void(Result<Results...>, Args...)> : function_traits_base<std::tuple<Results...>, Args...>
    {
        static constexpr bool is_async = true;
        using async_result_t = Result<Results...>;
    };

    template <typename... Args, typename... Results>
    struct function_traits<void(Result<Results...>&&, Args...)> : function_traits_base<std::tuple<Results...>, Args...>
    {
        static constexpr bool is_async = true;
        using async_result_t = Result<Results...>;
    };

    template <typename ReturnType, typename... Args>
    struct function_traits<ReturnType(*)(Args...)> : function_traits<ReturnType(Args...)>
    {};

    template <typename ClassType, typename ReturnType, typename... Args>
    struct function_traits<ReturnType(ClassType::*)(Args...)> : function_traits<ReturnType(Args...)>
    {
        using owner_type = ClassType &;
    };

    template <typename ClassType, typename ReturnType, typename... Args>
    struct function_traits<ReturnType(ClassType::*)(Args...) const> : function_traits<ReturnType(Args...)>
    {
        using owner_type = const ClassType &;
    };

    template <typename ClassType, typename ReturnType, typename... Args>
    struct function_traits<ReturnType(ClassType::*)(Args...) volatile> : function_traits<ReturnType(Args...)>
    {
        using owner_type = volatile ClassType &;
    };

    template <typename ClassType, typename ReturnType, typename... Args>
    struct function_traits<ReturnType(ClassType::*)(Args...) const volatile> : function_traits<ReturnType(Args...)>
    {
        using owner_type = const volatile ClassType &;
    };

    template <typename FunctionType>
    struct function_traits<std::function<FunctionType>> : function_traits<FunctionType>
    {};

    template <class Function>
    constexpr auto is_async_method_v = function_traits<Function>::is_async;

    template <class Function>
    constexpr auto has_error_param_v = function_traits<Function>::has_error_param;

    template <typename FunctionType>
    using function_arguments_t = typename function_traits<FunctionType>::arguments_type;

    template <typename FunctionType, size_t Idx>
    using function_argument_t = typename function_traits<FunctionType>::template arg_t<Idx>;

    template <typename FunctionType>
    constexpr auto function_argument_count_v = function_traits<FunctionType>::arity;

    template <typename FunctionType>
    using function_result_t = typename function_traits<FunctionType>::result_type;

    template <typename Function>
    struct tuple_of_function_input_arg_types
    {
        using type = typename function_traits<Function>::decayed_arguments_type;
    };

    template <typename Function>
    using tuple_of_function_input_arg_types_t = typename tuple_of_function_input_arg_types<Function>::type;

    template <typename Function>
    struct tuple_of_function_output_arg_types
    {
        using type = typename function_traits<Function>::result_type;
    };

    template <typename Function>
    using tuple_of_function_output_arg_types_t = typename tuple_of_function_output_arg_types<Function>::type;

    template <typename Function>
    struct signature_of_function_input_arguments : signature_of<tuple_of_function_input_arg_types_t<Function>>
    {
        static std::string value_as_string()
        {
            constexpr auto signature = as_null_terminated(signature_of_v<tuple_of_function_input_arg_types_t<Function>>);
            return signature.data();
        }
    };

    template <typename Function>
    inline const auto signature_of_function_input_arguments_v = signature_of_function_input_arguments<Function>::value_as_string();

    template <typename Function>
    struct signature_of_function_output_arguments : signature_of<tuple_of_function_output_arg_types_t<Function>>
    {
        static std::string value_as_string()
        {
            constexpr auto signature = as_null_terminated(signature_of_v<tuple_of_function_output_arg_types_t<Function>>);
            return signature.data();
        }
    };

    template <typename Function>
    inline const auto signature_of_function_output_arguments_v = signature_of_function_output_arguments<Function>::value_as_string();

    // std::future stuff for return values of async calls
    template <typename... Args> struct future_return
    {
        using type = std::tuple<Args...>;
    };

    template <> struct future_return<>
    {
        using type = void;
    };

    template <typename Type> struct future_return<Type>
    {
        using type = Type;
    };

    template <typename... Args>
    using future_return_t = typename future_return<Args...>::type;

    // Credit: Piotr Skotnicki (https://stackoverflow.com/a/57639506)
    template <typename, typename>
    constexpr bool is_one_of_variants_types = false;

    template <typename... VariantTypes, typename QueriedType>
    constexpr bool is_one_of_variants_types<std::variant<VariantTypes...>, QueriedType>
        = (std::is_same_v<QueriedType, VariantTypes> || ...);

    // Wrapper (tag) denoting we want to serialize user-defined struct
    // into a D-Bus message as a dictionary of strings to variants.
    template <typename Struct>
    struct as_dictionary
    {
        explicit as_dictionary(const Struct& strct) : m_struct(strct) {}
        const Struct& m_struct; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
    };

    template <typename Type>
    const Type& as_dictionary_if_struct(const Type& object)
    {
        return object; // identity in case _Type is not struct (user-defined structs shall provide an overload)
    }

    // By default, the dict-as-struct deserialization strategy is strict.
    // Strict means that every key of the deserialized dictionary must have its counterpart member in the struct, otherwise an exception is thrown.
    // Relaxed means that a key that does not have a matching struct member is silently ignored.
    // The behavior can be overridden for user-defined struct by specializing this variable template.
    template <typename Struct>
    constexpr auto strict_dict_as_struct_deserialization_v = true;

    // By default, the struct-as-dict serialization strategy is single-level only (as opposed to nested).
    // Single-level means that only the specific struct is serialized as a dictionary, serializing members that are structs always as structs.
    // Nested means that the struct *and* its members that are structs are all serialized as a dictionary. If nested strategy is also
    // defined for the nested struct, then the same behavior applies for that struct, recursively.
    // The behavior can be overridden for user-defined struct by specializing this variable template.
    template <typename Struct>
    constexpr auto nested_struct_as_dict_serialization_v = false;

    namespace detail
    {
        template <class Function, class Tuple, typename... Args, std::size_t... I>
        constexpr decltype(auto) apply_impl( Function&& fun
                                           , Result<Args...>&& res
                                           , Tuple&& tuple
                                           , std::index_sequence<I...> )
        {
            return std::forward<Function>(fun)(std::move(res), std::get<I>(std::forward<Tuple>(tuple))...);
        }

        template <class Function, class Tuple, std::size_t... I>
        decltype(auto) apply_impl( Function&& fun
                                 , std::optional<Error> err
                                 , Tuple&& tuple
                                 , std::index_sequence<I...> )
        {
            return std::forward<Function>(fun)(std::move(err), std::get<I>(std::forward<Tuple>(tuple))...);
        }

        // For non-void returning functions, apply_impl simply returns function return value (a tuple of values).
        // For void-returning functions, apply_impl returns an empty tuple.
        template <class Function, class Tuple, std::size_t... I>
        constexpr decltype(auto) apply_impl( Function&& fun
                                           , Tuple&& tuple
                                           , std::index_sequence<I...> )
        {
            if constexpr (!std::is_void_v<function_result_t<Function>>)
                return std::forward<Function>(fun)(std::get<I>(std::forward<Tuple>(tuple))...);
            else
                return std::forward<Function>(fun)(std::get<I>(std::forward<Tuple>(tuple))...), std::tuple<>{};
        }
    } // namespace detail

    // Convert tuple `t' of values into a list of arguments
    // and invoke function `f' with those arguments.
    template <class Function, class Tuple>
    constexpr decltype(auto) apply(Function&& fun, Tuple&& tuple)
    {
        return detail::apply_impl( std::forward<Function>(fun)
                                 , std::forward<Tuple>(tuple)
                                 , std::make_index_sequence<std::tuple_size_v<std::decay_t<Tuple>>>{} );
    }

    // Convert tuple `t' of values into a list of arguments
    // and invoke function `f' with those arguments.
    template <class Function, class Tuple, typename... Args>
    constexpr decltype(auto) apply(Function&& fun, Result<Args...>&& res, Tuple&& tuple)
    {
        return detail::apply_impl( std::forward<Function>(fun)
                                 , std::move(res)
                                 , std::forward<Tuple>(tuple)
                                 , std::make_index_sequence<std::tuple_size_v<std::decay_t<Tuple>>>{} );
    }

    // Convert tuple `t' of values into a list of arguments
    // and invoke function `f' with those arguments.
    template <class Function, class Tuple>
    decltype(auto) apply(Function&& fun, std::optional<Error> err, Tuple&& tuple)
    {
        return detail::apply_impl( std::forward<Function>(fun)
                                 , std::move(err)
                                 , std::forward<Tuple>(tuple)
                                 , std::make_index_sequence<std::tuple_size_v<std::decay_t<Tuple>>>{} );
    }

    // Convenient concatenation of arrays
    template <typename T, std::size_t N1, std::size_t N2>
    constexpr std::array<T, N1 + N2> operator+(std::array<T, N1> lhs, std::array<T, N2> rhs)
    {
        std::array<T, N1 + N2> result{};

        std::move(lhs.begin(), lhs.end(), result.begin());
        std::move(rhs.begin(), rhs.end(), result.begin() + N1);

        // std::size_t index = 0;
        // for (auto& item : lhs) {
        //     result[index] = std::move(item);
        //     ++index;
        // }
        // for (auto& item : rhs) {
        //     result[index] = std::move(item);
        //     ++index;
        // }

        return result;
    }

} // namespace sdbus

#endif /* SDBUS_CXX_TYPETRAITS_H_ */
