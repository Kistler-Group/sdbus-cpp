/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2022 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
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

#include <type_traits>
#include <string>
#include <vector>
#include <array>
#if __cplusplus >= 202002L
#include <span>
#endif
#include <map>
#include <unordered_map>
#include <cstdint>
#include <functional>
#include <memory>
#include <tuple>

// Forward declarations
namespace sdbus {
    class Variant;
    template <typename... _ValueTypes> class Struct;
    class ObjectPath;
    class Signature;
    class UnixFd;
    class MethodCall;
    class MethodReply;
    class Signal;
    class Message;
    class PropertySetCall;
    class PropertyGetReply;
    template <typename... _Results> class Result;
    class Error;
}

namespace sdbus {

    // Callbacks from sdbus-c++
    using method_callback = std::function<void(MethodCall msg)>;
    using async_reply_handler = std::function<void(MethodReply& reply, const Error* error)>;
    using signal_handler = std::function<void(Signal& signal)>;
    using message_handler = std::function<void(Message& msg)>;
    using property_set_callback = std::function<void(PropertySetCall& msg)>;
    using property_get_callback = std::function<void(PropertyGetReply& reply)>;

    // Type-erased RAII-style handle to callbacks/subscriptions registered to sdbus-c++
    using Slot = std::unique_ptr<void, std::function<void(void*)>>;

    // Tag specifying that an owning slot handle shall be returned from the function
    struct request_slot_t { explicit request_slot_t() = default; };
    inline constexpr request_slot_t request_slot{};
    // Tag specifying that the library shall own the slot resulting from the call of the function (so-called floating slot)
    struct floating_slot_t { explicit floating_slot_t() = default; };
    inline constexpr floating_slot_t floating_slot{};
    // Deprecated name for the above -- a floating slot
    struct dont_request_slot_t { explicit dont_request_slot_t() = default; };
    [[deprecated("Replaced by floating_slot")]] inline constexpr dont_request_slot_t dont_request_slot{};
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

    // Template specializations for getting D-Bus signatures from C++ types
    template <typename _T>
    struct signature_of
    {
        static constexpr bool is_valid = false;
        static constexpr bool is_trivial_dbus_type = false;

        static const std::string str()
        {
            // sizeof(_T) < 0 is here to make compiler not being able to figure out
            // the assertion expression before the template instantiation takes place.
            static_assert(sizeof(_T) < 0, "Unknown DBus type");
            return "";
        }
    };

    template <typename _T>
    struct signature_of<const _T>
        : public signature_of<_T>
    {};

    template <typename _T>
    struct signature_of<_T&>
            : public signature_of<_T>
    {};

    template <>
    struct signature_of<void>
    {
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;

        static const std::string str()
        {
            return "";
        }
    };

    template <>
    struct signature_of<bool>
    {
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = true;

        static const std::string str()
        {
            return "b";
        }
    };

    template <>
    struct signature_of<uint8_t>
    {
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = true;

        static const std::string str()
        {
            return "y";
        }
    };

    template <>
    struct signature_of<int16_t>
    {
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = true;

        static const std::string str()
        {
            return "n";
        }
    };

    template <>
    struct signature_of<uint16_t>
    {
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = true;

        static const std::string str()
        {
            return "q";
        }
    };

    template <>
    struct signature_of<int32_t>
    {
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = true;

        static const std::string str()
        {
            return "i";
        }
    };

    template <>
    struct signature_of<uint32_t>
    {
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = true;

        static const std::string str()
        {
            return "u";
        }
    };

    template <>
    struct signature_of<int64_t>
    {
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = true;

        static const std::string str()
        {
            return "x";
        }
    };

    template <>
    struct signature_of<uint64_t>
    {
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = true;

        static const std::string str()
        {
            return "t";
        }
    };

    template <>
    struct signature_of<double>
    {
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = true;

        static const std::string str()
        {
            return "d";
        }
    };

    template <>
    struct signature_of<char*>
    {
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;

        static const std::string str()
        {
            return "s";
        }
    };

    template <>
    struct signature_of<const char*>
    {
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;

        static const std::string str()
        {
            return "s";
        }
    };

    template <std::size_t _N>
    struct signature_of<char[_N]>
    {
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;

        static const std::string str()
        {
            return "s";
        }
    };

    template <std::size_t _N>
    struct signature_of<const char[_N]>
    {
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;

        static const std::string str()
        {
            return "s";
        }
    };

    template <>
    struct signature_of<std::string>
    {
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;

        static const std::string str()
        {
            return "s";
        }
    };

    template <typename... _ValueTypes>
    struct signature_of<Struct<_ValueTypes...>>
    {
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;

        static const std::string str()
        {
            std::string signature;
            signature += "(";
            (signature += ... += signature_of<_ValueTypes>::str());
            signature += ")";
            return signature;
        }
    };

    template <>
    struct signature_of<Variant>
    {
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;

        static const std::string str()
        {
            return "v";
        }
    };

    template <>
    struct signature_of<ObjectPath>
    {
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;

        static const std::string str()
        {
            return "o";
        }
    };

    template <>
    struct signature_of<Signature>
    {
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;

        static const std::string str()
        {
            return "g";
        }
    };

    template <>
    struct signature_of<UnixFd>
    {
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;

        static const std::string str()
        {
            return "h";
        }
    };

    template <typename _Element, typename _Allocator>
    struct signature_of<std::vector<_Element, _Allocator>>
    {
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;

        static const std::string str()
        {
            return "a" + signature_of<_Element>::str();
        }
    };

    template <typename _Element, std::size_t _Size>
    struct signature_of<std::array<_Element, _Size>>
    {
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;

        static const std::string str()
        {
            return "a" + signature_of<_Element>::str();
        }
    };

#if __cplusplus >= 202002L
    template <typename _Element, std::size_t _Extent>
    struct signature_of<std::span<_Element, _Extent>>
    {
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;

        static const std::string str()
        {
            return "a" + signature_of<_Element>::str();
        }
    };

    template <typename Enum> requires std::is_enum_v<Enum>
    struct signature_of<Enum>
        : public signature_of<std::underlying_type_t<Enum>>
    {};
#endif

    template <typename _Key, typename _Value, typename _Compare, typename _Allocator>
    struct signature_of<std::map<_Key, _Value, _Compare, _Allocator>>
    {
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;

        static const std::string str()
        {
            return "a{" + signature_of<_Key>::str() + signature_of<_Value>::str() + "}";
        }
    };

    template <typename _Key, typename _Value, typename _Hash, typename _KeyEqual, typename _Allocator>
    struct signature_of<std::unordered_map<_Key, _Value, _Hash, _KeyEqual, _Allocator>>
    {
        static constexpr bool is_valid = true;
        static constexpr bool is_trivial_dbus_type = false;

        static const std::string str()
        {
            return "a{" + signature_of<_Key>::str() + signature_of<_Value>::str() + "}";
        }
    };

    // Function traits implementation inspired by (c) kennytm,
    // https://github.com/kennytm/utils/blob/master/traits.hpp
    template <typename _Type>
    struct function_traits
        : public function_traits<decltype(&_Type::operator())>
    {};

    template <typename _Type>
    struct function_traits<const _Type>
        : public function_traits<_Type>
    {};

    template <typename _Type>
    struct function_traits<_Type&>
        : public function_traits<_Type>
    {};

    template <typename _ReturnType, typename... _Args>
    struct function_traits_base
    {
        typedef _ReturnType result_type;
        typedef std::tuple<_Args...> arguments_type;
        typedef std::tuple<std::decay_t<_Args>...> decayed_arguments_type;

        typedef _ReturnType function_type(_Args...);

        static constexpr std::size_t arity = sizeof...(_Args);

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

        template <size_t _Idx>
        struct arg
        {
            typedef std::tuple_element_t<_Idx, std::tuple<_Args...>> type;
        };

        template <size_t _Idx>
        using arg_t = typename arg<_Idx>::type;
    };

    template <typename _ReturnType, typename... _Args>
    struct function_traits<_ReturnType(_Args...)>
        : public function_traits_base<_ReturnType, _Args...>
    {
        static constexpr bool is_async = false;
        static constexpr bool has_error_param = false;
    };

    template <typename... _Args>
    struct function_traits<void(const Error*, _Args...)>
        : public function_traits_base<void, _Args...>
    {
        static constexpr bool has_error_param = true;
    };

    template <typename... _Args, typename... _Results>
    struct function_traits<void(Result<_Results...>, _Args...)>
        : public function_traits_base<std::tuple<_Results...>, _Args...>
    {
        static constexpr bool is_async = true;
        using async_result_t = Result<_Results...>;
    };

    template <typename... _Args, typename... _Results>
    struct function_traits<void(Result<_Results...>&&, _Args...)>
        : public function_traits_base<std::tuple<_Results...>, _Args...>
    {
        static constexpr bool is_async = true;
        using async_result_t = Result<_Results...>;
    };

    template <typename _ReturnType, typename... _Args>
    struct function_traits<_ReturnType(*)(_Args...)>
        : public function_traits<_ReturnType(_Args...)>
    {};

    template <typename _ClassType, typename _ReturnType, typename... _Args>
    struct function_traits<_ReturnType(_ClassType::*)(_Args...)>
        : public function_traits<_ReturnType(_Args...)>
    {
        typedef _ClassType& owner_type;
    };

    template <typename _ClassType, typename _ReturnType, typename... _Args>
    struct function_traits<_ReturnType(_ClassType::*)(_Args...) const>
        : public function_traits<_ReturnType(_Args...)>
    {
        typedef const _ClassType& owner_type;
    };

    template <typename _ClassType, typename _ReturnType, typename... _Args>
    struct function_traits<_ReturnType(_ClassType::*)(_Args...) volatile>
        : public function_traits<_ReturnType(_Args...)>
    {
        typedef volatile _ClassType& owner_type;
    };

    template <typename _ClassType, typename _ReturnType, typename... _Args>
    struct function_traits<_ReturnType(_ClassType::*)(_Args...) const volatile>
        : public function_traits<_ReturnType(_Args...)>
    {
        typedef const volatile _ClassType& owner_type;
    };

    template <typename FunctionType>
    struct function_traits<std::function<FunctionType>>
        : public function_traits<FunctionType>
    {};

    template <class _Function>
    constexpr auto is_async_method_v = function_traits<_Function>::is_async;

    template <class _Function>
    constexpr auto has_error_param_v = function_traits<_Function>::has_error_param;

    template <typename _FunctionType>
    using function_arguments_t = typename function_traits<_FunctionType>::arguments_type;

    template <typename _FunctionType, size_t _Idx>
    using function_argument_t = typename function_traits<_FunctionType>::template arg_t<_Idx>;

    template <typename _FunctionType>
    constexpr auto function_argument_count_v = function_traits<_FunctionType>::arity;

    template <typename _FunctionType>
    using function_result_t = typename function_traits<_FunctionType>::result_type;

    template <typename _Function>
    struct tuple_of_function_input_arg_types
    {
        typedef typename function_traits<_Function>::decayed_arguments_type type;
    };

    template <typename _Function>
    using tuple_of_function_input_arg_types_t = typename tuple_of_function_input_arg_types<_Function>::type;

    template <typename _Function>
    struct tuple_of_function_output_arg_types
    {
        typedef typename function_traits<_Function>::result_type type;
    };

    template <typename _Function>
    using tuple_of_function_output_arg_types_t = typename tuple_of_function_output_arg_types<_Function>::type;

    template <typename _Type>
    struct aggregate_signature
    {
        static const std::string str()
        {
            return signature_of<std::decay_t<_Type>>::str();
        }
    };

    template <typename... _Types>
    struct aggregate_signature<std::tuple<_Types...>>
    {
        static const std::string str()
        {
            std::string signature;
            (void)(signature += ... += signature_of<std::decay_t<_Types>>::str());
            return signature;
        }
    };

    template <typename _Function>
    struct signature_of_function_input_arguments
    {
        static const std::string str()
        {
            return aggregate_signature<tuple_of_function_input_arg_types_t<_Function>>::str();
        }
    };

    template <typename _Function>
    struct signature_of_function_output_arguments
    {
        static const std::string str()
        {
            return aggregate_signature<tuple_of_function_output_arg_types_t<_Function>>::str();
        }
    };


    template <typename... _Args> struct future_return
    {
        typedef std::tuple<_Args...> type;
    };

    template <> struct future_return<>
    {
        typedef void type;
    };

    template <typename _Type> struct future_return<_Type>
    {
        typedef _Type type;
    };

    template <typename... _Args>
    using future_return_t = typename future_return<_Args...>::type;


    namespace detail
    {
        template <class _Function, class _Tuple, typename... _Args, std::size_t... _I>
        constexpr decltype(auto) apply_impl( _Function&& f
                                           , Result<_Args...>&& r
                                           , _Tuple&& t
                                           , std::index_sequence<_I...> )
        {
            return std::forward<_Function>(f)(std::move(r), std::get<_I>(std::forward<_Tuple>(t))...);
        }

        template <class _Function, class _Tuple, std::size_t... _I>
        constexpr decltype(auto) apply_impl( _Function&& f
                                           , const Error* e
                                           , _Tuple&& t
                                           , std::index_sequence<_I...> )
        {
            return std::forward<_Function>(f)(e, std::get<_I>(std::forward<_Tuple>(t))...);
        }

        // For non-void returning functions, apply_impl simply returns function return value (a tuple of values).
        // For void-returning functions, apply_impl returns an empty tuple.
        template <class _Function, class _Tuple, std::size_t... _I>
        constexpr decltype(auto) apply_impl( _Function&& f
                                           , _Tuple&& t
                                           , std::index_sequence<_I...> )
        {
            if constexpr (!std::is_void_v<function_result_t<_Function>>)
                return std::forward<_Function>(f)(std::get<_I>(std::forward<_Tuple>(t))...);
            else
                return std::forward<_Function>(f)(std::get<_I>(std::forward<_Tuple>(t))...), std::tuple<>{};
        }
    }

    // Convert tuple `t' of values into a list of arguments
    // and invoke function `f' with those arguments.
    template <class _Function, class _Tuple>
    constexpr decltype(auto) apply(_Function&& f, _Tuple&& t)
    {
        return detail::apply_impl( std::forward<_Function>(f)
                                 , std::forward<_Tuple>(t)
                                 , std::make_index_sequence<std::tuple_size<std::decay_t<_Tuple>>::value>{} );
    }

    // Convert tuple `t' of values into a list of arguments
    // and invoke function `f' with those arguments.
    template <class _Function, class _Tuple, typename... _Args>
    constexpr decltype(auto) apply(_Function&& f, Result<_Args...>&& r, _Tuple&& t)
    {
        return detail::apply_impl( std::forward<_Function>(f)
                                 , std::move(r)
                                 , std::forward<_Tuple>(t)
                                 , std::make_index_sequence<std::tuple_size<std::decay_t<_Tuple>>::value>{} );
    }

    // Convert tuple `t' of values into a list of arguments
    // and invoke function `f' with those arguments.
    template <class _Function, class _Tuple>
    constexpr decltype(auto) apply(_Function&& f, const Error* e, _Tuple&& t)
    {
        return detail::apply_impl( std::forward<_Function>(f)
                                 , e
                                 , std::forward<_Tuple>(t)
                                 , std::make_index_sequence<std::tuple_size<std::decay_t<_Tuple>>::value>{} );
    }
}

#endif /* SDBUS_CXX_TYPETRAITS_H_ */
