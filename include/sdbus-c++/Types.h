/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2022 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
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
#include <unistd.h>

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
        /*explicit*/ Variant(const _ValueType& value) // TODO: Mark explicit in new major version so we don't break client code within v1
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
     * Representation of object path D-Bus type
     *
     ***********************************************/
    class ObjectPath : public std::string
    {
    public:
        using std::string::string;
        ObjectPath() = default; // Fixes gcc 6.3 error (default c-tor is not imported in above using declaration)
        ObjectPath(const ObjectPath&) = default; // Fixes gcc 8.3 error (deleted copy constructor)
        ObjectPath(ObjectPath&&) = default; // Enable move - user-declared copy ctor prevents implicit creation
        ObjectPath& operator = (const ObjectPath&) = default; // Fixes gcc 8.3 error (deleted copy assignment)
        ObjectPath& operator = (ObjectPath&&) = default; // Enable move - user-declared copy assign prevents implicit creation
        ObjectPath(std::string path)
            : std::string(std::move(path))
        {}
        using std::string::operator=;
    };

    /********************************************//**
     * @class Signature
     *
     * Representation of Signature D-Bus type
     *
     ***********************************************/
    class Signature : public std::string
    {
    public:
        using std::string::string;
        Signature() = default; // Fixes gcc 6.3 error (default c-tor is not imported in above using declaration)
        Signature(const Signature&) = default; // Fixes gcc 8.3 error (deleted copy constructor)
        Signature(Signature&&) = default; // Enable move - user-declared copy ctor prevents implicit creation
        Signature& operator = (const Signature&) = default; // Fixes gcc 8.3 error (deleted copy assignment)
        Signature& operator = (Signature&&) = default; // Enable move - user-declared copy assign prevents implicit creation
        Signature(std::string path)
            : std::string(std::move(path))
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
            : fd_(::dup(fd))
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
            close();
            fd_ = ::dup(other.fd_);
            return *this;
        }

        UnixFd(UnixFd&& other)
        {
            *this = std::move(other);
        }

        UnixFd& operator=(UnixFd&& other)
        {
            close();
            fd_ = other.fd_;
            other.fd_ = -1;
            return *this;
        }

        ~UnixFd()
        {
            close();
        }

        int get() const
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
            auto fd = fd_;
            fd_ = -1;
            return fd;
        }

        bool isValid() const
        {
            return fd_ >= 0;
        }

    private:
        void close()
        {
            if (fd_ >= 0)
                ::close(fd_);
        }

        int fd_ = -1;
    };

}

template <size_t _I, typename... _ValueTypes>
struct std::tuple_element<_I, sdbus::Struct<_ValueTypes...>>
    : std::tuple_element<_I, std::tuple<_ValueTypes...>>
{};

template <typename... _ValueTypes>
struct std::tuple_size<sdbus::Struct<_ValueTypes...>>
    : std::tuple_size<std::tuple<_ValueTypes...>>
{};

#endif /* SDBUS_CXX_TYPES_H_ */
