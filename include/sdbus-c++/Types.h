/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
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
#include <string>
#include <type_traits>
#include <typeinfo>
#include <memory>
#include <tuple>

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
        Variant(const _ValueType& value)
            : Variant()
        {
            msg_.openVariant(signature_of<_ValueType>::str());
            msg_ << value;
            msg_.closeVariant();
            msg_.seal();
        }

        template <typename _ValueType>
        _ValueType get() const
        {
            _ValueType val;
            msg_.rewind(false);
            msg_.enterVariant(signature_of<_ValueType>::str());
            msg_ >> val;
            msg_.exitVariant();
            return val;
        }

        // Only allow conversion operator for true D-Bus type representations in C++
        template <typename _ValueType, typename = std::enable_if_t<signature_of<_ValueType>::is_valid>>
        operator _ValueType() const
        {
            return get<_ValueType>();
        }

        template <typename _Type>
        bool containsValueOfType() const
        {
            return signature_of<_Type>::str() == peekValueType();
        }

        bool isEmpty() const;

        void serializeTo(Message& msg) const;
        void deserializeFrom(Message& msg);
        std::string peekValueType() const;

    private:
        mutable Message msg_{};
    };

    template <typename... _ValueTypes>
    class Struct
        : public std::tuple<_ValueTypes...>
    {
    public:
        using std::tuple<_ValueTypes...>::tuple;

        // Disable constructor if an older then 7.1.0 version of GCC is used 
#if !((defined(__GNUC__) || defined(__GNUG__)) && !defined(__clang__) && !(__GNUC__ > 7 || (__GNUC__ == 7 && (__GNUC_MINOR__ > 1 || (__GNUC_MINOR__ == 1 && __GNUC_PATCHLEVEL__ > 0)))))
        Struct() = default;

        explicit Struct(const std::tuple<_ValueTypes...>& t)
            : std::tuple<_ValueTypes...>(t)
        {
        }
#endif

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

    template<typename... _Elements>
    constexpr Struct<std::decay_t<_Elements>...>
    make_struct(_Elements&&... args)
    {
        typedef Struct<std::decay_t<_Elements>...> result_type;
        return result_type(std::forward<_Elements>(args)...);
    }

    class ObjectPath : public std::string
    {
    public:
        using std::string::string;
        using std::string::operator=;
    };

    class Signature : public std::string
    {
    public:
        using std::string::string;
        using std::string::operator=;
    };

}

#endif /* SDBUS_CXX_TYPES_H_ */
