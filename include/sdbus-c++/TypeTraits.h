/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
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

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <functional>
#include <tuple>

// Forward declarations
namespace sdbus {
    class Variant;
    template <typename... _ValueTypes> class Struct;
    class ObjectPath;
    class Signature;
    class Message;
}

namespace sdbus {

    using method_callback = std::function<void(Message& msg, Message& reply)>;
    using signal_handler = std::function<void(Message& signal)>;
    using property_set_callback = std::function<void(Message& msg)>;
    using property_get_callback = std::function<void(Message& reply)>;

    // Primary template
    template <typename _T>
    struct signature_of
    {
        static constexpr bool is_valid = false;

        static const std::string str()
        {
            // sizeof(_T) < 0 is here to make compiler not being able to figure out
            // the assertion expression before the template instantiation takes place.
            static_assert(sizeof(_T) < 0, "Unknown DBus type");
            return "";
        }
    };

    template <>
    struct signature_of<void>
    {
        static constexpr bool is_valid = true;

        static const std::string str()
        {
            return "";
        }
    };

    template <>
    struct signature_of<bool>
    {
        static constexpr bool is_valid = true;

        static const std::string str()
        {
            return "b";
        }
    };

    template <>
    struct signature_of<uint8_t>
    {
        static constexpr bool is_valid = true;

        static const std::string str()
        {
            return "y";
        }
    };

    template <>
    struct signature_of<int16_t>
    {
        static constexpr bool is_valid = true;

        static const std::string str()
        {
            return "n";
        }
    };

    template <>
    struct signature_of<uint16_t>
    {
        static constexpr bool is_valid = true;

        static const std::string str()
        {
            return "q";
        }
    };

    template <>
    struct signature_of<int32_t>
    {
        static constexpr bool is_valid = true;

        static const std::string str()
        {
            return "i";
        }
    };

    template <>
    struct signature_of<uint32_t>
    {
        static constexpr bool is_valid = true;

        static const std::string str()
        {
            return "u";
        }
    };

    template <>
    struct signature_of<int64_t>
    {
        static constexpr bool is_valid = true;

        static const std::string str()
        {
            return "x";
        }
    };

    template <>
    struct signature_of<uint64_t>
    {
        static constexpr bool is_valid = true;

        static const std::string str()
        {
            return "t";
        }
    };

    template <>
    struct signature_of<double>
    {
        static constexpr bool is_valid = true;

        static const std::string str()
        {
            return "d";
        }
    };

    template <>
    struct signature_of<char*>
    {
        static constexpr bool is_valid = true;

        static const std::string str()
        {
            return "s";
        }
    };

    template <>
    struct signature_of<const char*>
    {
        static constexpr bool is_valid = true;

        static const std::string str()
        {
            return "s";
        }
    };

    template <std::size_t _N>
    struct signature_of<char[_N]>
    {
        static constexpr bool is_valid = true;

        static const std::string str()
        {
            return "s";
        }
    };

    template <std::size_t _N>
    struct signature_of<const char[_N]>
    {
        static constexpr bool is_valid = true;

        static const std::string str()
        {
            return "s";
        }
    };

    template <>
    struct signature_of<std::string>
    {
        static constexpr bool is_valid = true;

        static const std::string str()
        {
            return "s";
        }
    };

    template <typename... _ValueTypes>
    struct signature_of<Struct<_ValueTypes...>>
    {
        static constexpr bool is_valid = true;

        static const std::string str()
        {
            std::initializer_list<std::string> signatures{signature_of<_ValueTypes>::str()...};
            std::string signature;
            signature += "(";
            for (const auto& item : signatures)
                signature += item;
            signature += ")";
            return signature;
        }
    };

    template <>
    struct signature_of<Variant>
    {
        static constexpr bool is_valid = true;

        static const std::string str()
        {
            return "v";
        }
    };

    template <>
    struct signature_of<ObjectPath>
    {
        static constexpr bool is_valid = true;

        static const std::string str()
        {
            return "o";
        }
    };

    template <>
    struct signature_of<Signature>
    {
        static constexpr bool is_valid = true;

        static const std::string str()
        {
            return "g";
        }
    };

    template <typename _Element>
    struct signature_of<std::vector<_Element>>
    {
        static constexpr bool is_valid = true;

        static const std::string str()
        {
            return "a" + signature_of<_Element>::str();
        }
    };

    template <typename _Key, typename _Value>
    struct signature_of<std::map<_Key, _Value>>
    {
        static constexpr bool is_valid = true;

        static const std::string str()
        {
            return "a{" + signature_of<_Key>::str() + signature_of<_Value>::str() + "}";
        }
    };

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

    // Function traits implementation inspired by (c) kennytm,
    // https://github.com/kennytm/utils/blob/master/traits.hpp
    template <typename _ReturnType, typename... _Args>
    struct function_traits<_ReturnType(_Args...)>
    {
        typedef _ReturnType result_type;
        typedef std::tuple<_Args...> arguments_type;

        typedef _ReturnType function_type(_Args...);

        static constexpr std::size_t arity = sizeof...(_Args);

        template <size_t _Idx>
        struct arg
        {
            typedef std::tuple_element_t<_Idx, std::tuple<_Args...>> type;
        };

        template <size_t _Idx>
        using arg_t = typename arg<_Idx>::type;
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

    template <typename _FunctionType, size_t _Idx>
    using function_argument_t = typename function_traits<_FunctionType>::template arg_t<_Idx>;

    template <typename _FunctionType>
    using function_result_t = typename function_traits<_FunctionType>::result_type;

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
            std::initializer_list<std::string> signatures{signature_of<std::decay_t<_Types>>::str()...};
            std::string signature;
            for (const auto& item : signatures)
                signature += item;
            return signature;
        }
    };

    // Get a tuple of function input argument types from function signature.
    // But first, convert provided function signature to the standardized form `out(in...)'.
    template <typename _Function>
    struct tuple_of_function_input_arg_types
        : public tuple_of_function_input_arg_types<typename function_traits<_Function>::function_type>
    {};

    // Get a tuple of function input argument types from function signature.
    // Function signature is expected in the standardized form `out(in...)'.
    template <typename _ReturnType, typename... _Args>
    struct tuple_of_function_input_arg_types<_ReturnType(_Args...)>
    {
        // Arguments may be cv-qualified and may be references, so we have to strip cv and references
        // with decay_t in order to get real 'naked' types.
        // Example: for a function with signature void(const int i, const std::vector<float> v, double d)
        // the `type' will be `std::tuple<int, std::vector<float>, double>'.
        typedef std::tuple<std::decay_t<_Args>...> type;
    };

    template <typename _Function>
    using tuple_of_function_input_arg_types_t = typename tuple_of_function_input_arg_types<_Function>::type;

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
            return aggregate_signature<function_result_t<_Function>>::str();
        }
    };

    namespace detail
    {
        // Version of apply_impl for functions returning non-void values.
        // In this case just forward function return value.
        template <class _Function, class _Tuple, std::size_t... _I>
        constexpr decltype(auto) apply_impl( _Function&& f
                                           , _Tuple&& t
                                           , std::index_sequence<_I...>
                                           , std::enable_if_t<!std::is_void<function_result_t<_Function>>::value>* = nullptr)
        {
            return std::forward<_Function>(f)(std::get<_I>(std::forward<_Tuple>(t))...);
        }

        // Version of apply_impl for functions returning void.
        // In this case, to have uniform code on the caller side, return empty tuple, our synonym for `void'.
        template <class _Function, class _Tuple, std::size_t... _I>
        constexpr decltype(auto) apply_impl( _Function&& f
                                           , _Tuple&& t
                                           , std::index_sequence<_I...>
                                           , std::enable_if_t<std::is_void<function_result_t<_Function>>::value>* = nullptr)
        {
            std::forward<_Function>(f)(std::get<_I>(std::forward<_Tuple>(t))...);
            return std::tuple<>{};
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

}

#endif /* SDBUS_CXX_TYPETRAITS_H_ */
