/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 *
 * @file Message.h
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

#ifndef SDBUS_CXX_MESSAGE_H_
#define SDBUS_CXX_MESSAGE_H_

#include <sdbus-c++/TypeTraits.h>
#include <sdbus-c++/Error.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <utility>
#include <cstdint>
#include <cassert>

#include <iostream>

// Forward declarations
namespace sdbus {
    class Variant;
    class ObjectPath;
    class Signature;
    template <typename... _ValueTypes> class Struct;

    class Message;
    class MethodCall;
    class MethodReply;
    class Signal;
    template <typename... _Results> class Result;
}

namespace sdbus {

    /********************************************//**
     * @class Message
     *
     * Message represents a D-Bus message, which can be either method call message,
     * method reply message, signal message, or a plain message serving as a storage
     * for serialized data.
     *
     * Serialization and deserialization functions are provided for types supported
     * by D-Bus.
     *
     * You don't need to work with this class directly if you use high-level APIs
     * of @c IObject and @c IObjectProxy.
     *
     ***********************************************/
    class Message
    {
    public:
        Message() = default;
        // TODO Consider creating adopt_message_t{} c-tor overload and remove
        // all SCOPE_EXIT{ sd_bus_message_unref(sdbusMsg); }; from when Message takes over msg
        Message(void *msg) noexcept;
        Message(const Message&) noexcept;
        Message& operator=(const Message&) noexcept;
        Message(Message&& other) noexcept;
        Message& operator=(Message&& other) noexcept;
        ~Message();

        Message& operator<<(bool item);
        Message& operator<<(int16_t item);
        Message& operator<<(int32_t item);
        Message& operator<<(int64_t item);
        Message& operator<<(uint8_t item);
        Message& operator<<(uint16_t item);
        Message& operator<<(uint32_t item);
        Message& operator<<(uint64_t item);
        Message& operator<<(double item);
        Message& operator<<(const char *item);
        Message& operator<<(const std::string &item);
        Message& operator<<(const Variant &item);
        Message& operator<<(const ObjectPath &item);
        Message& operator<<(const Signature &item);

        Message& operator>>(bool& item);
        Message& operator>>(int16_t& item);
        Message& operator>>(int32_t& item);
        Message& operator>>(int64_t& item);
        Message& operator>>(uint8_t& item);
        Message& operator>>(uint16_t& item);
        Message& operator>>(uint32_t& item);
        Message& operator>>(uint64_t& item);
        Message& operator>>(double& item);
        Message& operator>>(char*& item);
        Message& operator>>(std::string &item);
        Message& operator>>(Variant &item);
        Message& operator>>(ObjectPath &item);
        Message& operator>>(Signature &item);

        Message& openContainer(const std::string& signature);
        Message& closeContainer();
        Message& openDictEntry(const std::string& signature);
        Message& closeDictEntry();
        Message& openVariant(const std::string& signature);
        Message& closeVariant();
        Message& openStruct(const std::string& signature);
        Message& closeStruct();

        Message& enterContainer(const std::string& signature);
        Message& exitContainer();
        Message& enterDictEntry(const std::string& signature);
        Message& exitDictEntry();
        Message& enterVariant(const std::string& signature);
        Message& exitVariant();
        Message& enterStruct(const std::string& signature);
        Message& exitStruct();

        operator bool() const;
        void clearFlags();

        std::string getInterfaceName() const;
        std::string getMemberName() const;
        void peekType(std::string& type, std::string& contents) const;
        bool isValid() const;
        bool isEmpty() const;

        void copyTo(Message& destination, bool complete) const;
        void seal();
        void rewind(bool complete);

    protected:
        void* getMsg() const;

    private:
        void* msg_{};
        mutable bool ok_{true};
    };

    class MethodCall : public Message
    {
    public:
        using Message::Message;
        MethodReply send() const;
        MethodReply createReply() const;
        MethodReply createErrorReply(const sdbus::Error& error) const;
        void dontExpectReply();
        bool doesntExpectReply() const;

    private:
        MethodReply sendWithReply() const;
        MethodReply sendWithNoReply() const;
    };

    class AsyncMethodCall : public Message
    {
    public:
        using Message::Message;
        AsyncMethodCall(MethodCall&& call) noexcept;
        void send(void* callback, void* userData) const;
    };

    class MethodReply : public Message
    {
    public:
        using Message::Message;
        void send() const;
    };

    class Signal : public Message
    {
    public:
        using Message::Message;
        void send() const;
    };

    template <typename _Element>
    inline Message& operator<<(Message& msg, const std::vector<_Element>& items)
    {
        msg.openContainer(signature_of<_Element>::str());

        for (const auto& item : items)
            msg << item;

        msg.closeContainer();

        return msg;
    }

    template <typename _Key, typename _Value>
    inline Message& operator<<(Message& msg, const std::map<_Key, _Value>& items)
    {
        const std::string dictEntrySignature = signature_of<_Key>::str() + signature_of<_Value>::str();
        const std::string arraySignature = "{" + dictEntrySignature + "}";

        msg.openContainer(arraySignature);

        for (const auto& item : items)
        {
            msg.openDictEntry(dictEntrySignature);
            msg << item.first;
            msg << item.second;
            msg.closeDictEntry();
        }

        msg.closeContainer();

        return msg;
    }

    namespace detail
    {
        template <typename... _Args>
        void serialize_pack(Message& msg, _Args&&... args)
        {
            // Use initializer_list because it guarantees left to right order, and can be empty
            using _ = std::initializer_list<int>;
            // We are not interested in the list itself, but in the side effects
            (void)_{(void(msg << std::forward<_Args>(args)), 0)...};
        }

        template <class _Tuple, std::size_t... _Is>
        void serialize_tuple( Message& msg
                            , const _Tuple& t
                            , std::index_sequence<_Is...>)
        {
            serialize_pack(msg, std::get<_Is>(t)...);
        }
    }

    template <typename... _ValueTypes>
    inline Message& operator<<(Message& msg, const Struct<_ValueTypes...>& item)
    {
        auto structSignature = signature_of<Struct<_ValueTypes...>>::str();
        assert(structSignature.size() > 2);
        // Remove opening and closing parenthesis from the struct signature to get contents signature
        auto structContentSignature = structSignature.substr(1, structSignature.size()-2);

        msg.openStruct(structContentSignature);
        detail::serialize_tuple(msg, item, std::index_sequence_for<_ValueTypes...>{});
        msg.closeStruct();

        return msg;
    }

    template <typename... _ValueTypes>
    inline Message& operator<<(Message& msg, const std::tuple<_ValueTypes...>& item)
    {
        detail::serialize_tuple(msg, item, std::index_sequence_for<_ValueTypes...>{});
        return msg;
    }


    template <typename _Element>
    inline Message& operator>>(Message& msg, std::vector<_Element>& items)
    {
        if(!msg.enterContainer(signature_of<_Element>::str()))
            return msg;

        while (true)
        {
            _Element elem;
            if (msg >> elem)
                items.emplace_back(std::move(elem));
            else
                break;
        }

        msg.clearFlags();

        msg.exitContainer();

        return msg;
    }

    template <typename _Key, typename _Value>
    inline Message& operator>>(Message& msg, std::map<_Key, _Value>& items)
    {
        const std::string dictEntrySignature = signature_of<_Key>::str() + signature_of<_Value>::str();
        const std::string arraySignature = "{" + dictEntrySignature + "}";

        if (!msg.enterContainer(arraySignature))
            return msg;

        while (true)
        {
            if (!msg.enterDictEntry(dictEntrySignature))
                break;

            _Key key;
            _Value value;
            msg >> key >> value;

            items.emplace(std::move(key), std::move(value));

            msg.exitDictEntry();
        }

        msg.clearFlags();

        msg.exitContainer();

        return msg;
    }

    namespace detail
    {
        template <typename... _Args>
        void deserialize_pack(Message& msg, _Args&... args)
        {
            // Use initializer_list because it guarantees left to right order, and can be empty
            using _ = std::initializer_list<int>;
            // We are not interested in the list itself, but in the side effects
            (void)_{(void(msg >> args), 0)...};
        }

        template <class _Tuple, std::size_t... _Is>
        void deserialize_tuple( Message& msg
                              , _Tuple& t
                              , std::index_sequence<_Is...> )
        {
            deserialize_pack(msg, std::get<_Is>(t)...);
        }
    }

    template <typename... _ValueTypes>
    inline Message& operator>>(Message& msg, Struct<_ValueTypes...>& item)
    {
        auto structSignature = signature_of<Struct<_ValueTypes...>>::str();
        // Remove opening and closing parenthesis from the struct signature to get contents signature
        auto structContentSignature = structSignature.substr(1, structSignature.size()-2);

        if (!msg.enterStruct(structContentSignature))
            return msg;

        detail::deserialize_tuple(msg, item, std::index_sequence_for<_ValueTypes...>{});

        msg.exitStruct();

        return msg;
    }

    template <typename... _ValueTypes>
    inline Message& operator>>(Message& msg, std::tuple<_ValueTypes...>& item)
    {
        detail::deserialize_tuple(msg, item, std::index_sequence_for<_ValueTypes...>{});
        return msg;
    }

}

#endif /* SDBUS_CXX_MESSAGE_H_ */
