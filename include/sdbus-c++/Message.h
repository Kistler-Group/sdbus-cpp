/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2024 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
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
#include <array>
#include <string>
#include <utility>
#include <vector>
#if __cplusplus >= 202002L
#include <span>
#endif
#include <map>
#include <unordered_map>
#include <utility>
#include <cstdint>
#include <cassert>
#include <functional>
#include <sys/types.h>
#include <algorithm>
#include <variant>

// Forward declarations
namespace sdbus {
    class Variant;
    class ObjectPath;
    class Signature;
    template <typename... _ValueTypes> class Struct;
    class UnixFd;
    class MethodReply;
    namespace internal {
        class ISdBus;
    }
}

namespace sdbus {

    /********************************************//**
     * @class Message
     *
     * Message represents a D-Bus message, which can be either method call message,
     * method reply message, signal message, or a plain message.
     *
     * Serialization and deserialization functions are provided for types supported
     * by D-Bus.
     *
     * You mostly don't need to work with this class directly if you use high-level
     * APIs of @c IObject and @c IProxy.
     *
     ***********************************************/
    class [[nodiscard]] Message
    {
    public:
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
        template <typename ...Elements>
        Message& operator<<(const std::variant<Elements...>& value);
        Message& operator<<(const ObjectPath &item);
        Message& operator<<(const Signature &item);
        Message& operator<<(const UnixFd &item);
        template <typename _Element, typename _Allocator>
        Message& operator<<(const std::vector<_Element, _Allocator>& items);
        template <typename _Element, std::size_t _Size>
        Message& operator<<(const std::array<_Element, _Size>& items);
#if __cplusplus >= 202002L
        template <typename _Element, std::size_t _Extent>
        Message& operator<<(const std::span<_Element, _Extent>& items);
#endif
        template <typename _Enum, typename = std::enable_if_t<std::is_enum_v<_Enum>>>
        Message& operator<<(const _Enum& item);
        template <typename _Key, typename _Value, typename _Compare, typename _Allocator>
        Message& operator<<(const std::map<_Key, _Value, _Compare, _Allocator>& items);
        template <typename _Key, typename _Value, typename _Hash, typename _KeyEqual, typename _Allocator>
        Message& operator<<(const std::unordered_map<_Key, _Value, _Hash, _KeyEqual, _Allocator>& items);
        template <typename... _ValueTypes>
        Message& operator<<(const Struct<_ValueTypes...>& item);
        template <typename... _ValueTypes>
        Message& operator<<(const std::tuple<_ValueTypes...>& item);

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
        template <typename ...Elements>
        Message& operator>>(std::variant<Elements...>& value);
        Message& operator>>(ObjectPath &item);
        Message& operator>>(Signature &item);
        Message& operator>>(UnixFd &item);
        template <typename _Element, typename _Allocator>
        Message& operator>>(std::vector<_Element, _Allocator>& items);
        template <typename _Element, std::size_t _Size>
        Message& operator>>(std::array<_Element, _Size>& items);
#if __cplusplus >= 202002L
        template <typename _Element, std::size_t _Extent>
        Message& operator>>(std::span<_Element, _Extent>& items);
#endif
        template <typename _Enum, typename = std::enable_if_t<std::is_enum_v<_Enum>>>
        Message& operator>>(_Enum& item);
        template <typename _Key, typename _Value, typename _Compare, typename _Allocator>
        Message& operator>>(std::map<_Key, _Value, _Compare, _Allocator>& items);
        template <typename _Key, typename _Value, typename _Hash, typename _KeyEqual, typename _Allocator>
        Message& operator>>(std::unordered_map<_Key, _Value, _Hash, _KeyEqual, _Allocator>& items);
        template <typename... _ValueTypes>
        Message& operator>>(Struct<_ValueTypes...>& item);
        template <typename... _ValueTypes>
        Message& operator>>(std::tuple<_ValueTypes...>& item);

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

        Message& appendArray(char type, const void *ptr, size_t size);
        Message& readArray(char type, const void **ptr, size_t *size);

        explicit operator bool() const;
        void clearFlags();

        std::string getInterfaceName() const;
        std::string getMemberName() const;
        std::string getSender() const;
        std::string getPath() const;
        std::string getDestination() const;
        void peekType(std::string& type, std::string& contents) const;
        bool isValid() const;
        bool isEmpty() const;
        bool isAtEnd(bool complete) const;

        void copyTo(Message& destination, bool complete) const;
        void seal();
        void rewind(bool complete);

        pid_t getCredsPid() const;
        uid_t getCredsUid() const;
        uid_t getCredsEuid() const;
        gid_t getCredsGid() const;
        gid_t getCredsEgid() const;
        std::vector<gid_t> getCredsSupplementaryGids() const;
        std::string getSELinuxContext() const;

        class Factory;

    private:
        template <typename _Array>
        void serializeArray(const _Array& items);
        template <typename _Array>
        void deserializeArray(_Array& items);
        template <typename _Array>
        void deserializeArrayFast(_Array& items);
        template <typename _Element, typename _Allocator>
        void deserializeArrayFast(std::vector<_Element, _Allocator>& items);
        template <typename _Array>
        void deserializeArraySlow(_Array& items);
        template <typename _Element, typename _Allocator>
        void deserializeArraySlow(std::vector<_Element, _Allocator>& items);

        template <typename _Dictionary>
        void serializeDictionary(const _Dictionary& items);
        template <typename _Dictionary>
        void deserializeDictionary(_Dictionary& items);

    protected:
        Message() = default;
        explicit Message(internal::ISdBus* sdbus) noexcept;
        Message(void *msg, internal::ISdBus* sdbus) noexcept;
        Message(void *msg, internal::ISdBus* sdbus, adopt_message_t) noexcept;

        friend Factory;

    protected:
        void* msg_{};
        internal::ISdBus* sdbus_{};
        mutable bool ok_{true};
    };

    class MethodCall : public Message
    {
        using Message::Message;
        friend Factory;

    public:
        MethodCall() = default;

        MethodReply send(uint64_t timeout) const;
        void send(void* callback, void* userData, uint64_t timeout, floating_slot_t) const;
        [[nodiscard]] Slot send(void* callback, void* userData, uint64_t timeout) const;

        MethodReply createReply() const;
        MethodReply createErrorReply(const sdbus::Error& error) const;

        void dontExpectReply();
        bool doesntExpectReply() const;

    protected:
        MethodCall(void *msg, internal::ISdBus* sdbus, adopt_message_t) noexcept;

    private:
        MethodReply sendWithReply(uint64_t timeout = 0) const;
        MethodReply sendWithNoReply() const;
    };

    class MethodReply : public Message
    {
        using Message::Message;
        friend Factory;

    public:
        MethodReply() = default;
        void send() const;
    };

    class Signal : public Message
    {
        using Message::Message;
        friend Factory;

    public:
        Signal() = default;
        void setDestination(const std::string& destination);
        void send() const;
    };

    class PropertySetCall : public Message
    {
        using Message::Message;
        friend Factory;

    public:
        PropertySetCall() = default;
    };

    class PropertyGetReply : public Message
    {
        using Message::Message;
        friend Factory;

    public:
        PropertyGetReply() = default;
    };

    // Represents any of the above message types, or just a message that serves as a container for data
    class PlainMessage : public Message
    {
        using Message::Message;
        friend Factory;

    public:
        PlainMessage() = default;
    };

    template <typename ...Elements>
    inline Message& Message::operator<<(const std::variant<Elements...>& value)
    {
        std::visit([this](const auto& inner){
            openVariant(signature_of<decltype(inner)>::str());
            *this << inner;
            closeVariant();
        }, value);

        return *this;
    }

    template <typename _Element, typename _Allocator>
    inline Message& Message::operator<<(const std::vector<_Element, _Allocator>& items)
    {
        serializeArray(items);

        return *this;
    }

    template <typename _Element, std::size_t _Size>
    inline Message& Message::operator<<(const std::array<_Element, _Size>& items)
    {
        serializeArray(items);

        return *this;
    }

#if __cplusplus >= 202002L
    template <typename _Element, std::size_t _Extent>
    inline Message& Message::operator<<(const std::span<_Element, _Extent>& items)
    {
        serializeArray(items);

        return *this;
    }
#endif

    template <typename _Enum, typename>
    inline Message& Message::operator<<(const _Enum &item)
    {
        return operator<<(static_cast<std::underlying_type_t<_Enum>>(item));
    }

    template <typename _Array>
    inline void Message::serializeArray(const _Array& items)
    {
        using ElementType = typename _Array::value_type;

        // Use faster, one-step serialization of contiguous array of elements of trivial D-Bus types except bool,
        // otherwise use step-by-step serialization of individual elements.
        if constexpr (signature_of<ElementType>::is_trivial_dbus_type && !std::is_same_v<ElementType, bool>)
        {
            appendArray(*signature_of<ElementType>::str().c_str(), items.data(), items.size() * sizeof(ElementType));
        }
        else
        {
            openContainer(signature_of<ElementType>::str());

            for (const auto& item : items)
                *this << item;

            closeContainer();
        }
    }

    template <typename _Key, typename _Value, typename _Compare, typename _Allocator>
    inline Message& Message::operator<<(const std::map<_Key, _Value, _Compare, _Allocator>& items)
    {
        serializeDictionary(items);

        return *this;
    }

    template <typename _Key, typename _Value, typename _Hash, typename _KeyEqual, typename _Allocator>
    inline Message& Message::operator<<(const std::unordered_map<_Key, _Value, _Hash, _KeyEqual, _Allocator>& items)
    {
        serializeDictionary(items);

        return *this;
    }

    template <typename _Dictionary>
    inline void Message::serializeDictionary(const _Dictionary& items)
    {
        using KeyType = typename _Dictionary::key_type;
        using ValueType = typename _Dictionary::mapped_type;

        const std::string dictEntrySignature = signature_of<KeyType>::str() + signature_of<ValueType>::str();
        const std::string arraySignature = "{" + dictEntrySignature + "}";

        openContainer(arraySignature);

        for (const auto& item : items)
        {
            openDictEntry(dictEntrySignature);
            *this << item.first;
            *this << item.second;
            closeDictEntry();
        }

        closeContainer();
    }

    namespace detail
    {
        template <typename... _Args>
        void serialize_pack(Message& msg, _Args&&... args)
        {
            (void)(msg << ... << args);
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
    inline Message& Message::operator<<(const Struct<_ValueTypes...>& item)
    {
        auto structSignature = signature_of<Struct<_ValueTypes...>>::str();
        assert(structSignature.size() > 2);
        // Remove opening and closing parenthesis from the struct signature to get contents signature
        auto structContentSignature = structSignature.substr(1, structSignature.size() - 2);

        openStruct(structContentSignature);
        detail::serialize_tuple(*this, item, std::index_sequence_for<_ValueTypes...>{});
        closeStruct();

        return *this;
    }

    template <typename... _ValueTypes>
    inline Message& Message::operator<<(const std::tuple<_ValueTypes...>& item)
    {
        detail::serialize_tuple(*this, item, std::index_sequence_for<_ValueTypes...>{});
        return *this;
    }

    namespace detail
    {
        template <typename _Element, typename... _Elements>
        bool deserialize_variant(Message& msg, std::variant<_Elements...>& value, const std::string& signature)
        {
            if (signature != signature_of<_Element>::str())
                return false;

            _Element temp;
            msg.enterVariant(signature);
            msg >> temp;
            msg.exitVariant();
            value = std::move(temp);
            return true;
        }
    }

    template <typename... Elements>
    inline Message& Message::operator>>(std::variant<Elements...>& value)
    {
        std::string type;
        std::string contentType;
        peekType(type, contentType);
        bool result = (detail::deserialize_variant<Elements>(*this, value, contentType) || ...);
        SDBUS_THROW_ERROR_IF(!result, "Failed to deserialize variant: signature did not match any of the variant types", EINVAL);
        return *this;
    }

    template <typename _Element, typename _Allocator>
    inline Message& Message::operator>>(std::vector<_Element, _Allocator>& items)
    {
        deserializeArray(items);

        return *this;
    }

    template <typename _Element, std::size_t _Size>
    inline Message& Message::operator>>(std::array<_Element, _Size>& items)
    {
        deserializeArray(items);

        return *this;
    }

#if __cplusplus >= 202002L
    template <typename _Element, std::size_t _Extent>
    inline Message& Message::operator>>(std::span<_Element, _Extent>& items)
    {
        deserializeArray(items);

        return *this;
    }
#endif

    template <typename _Enum, typename>
    inline Message& Message::operator>>(_Enum& item)
    {
        std::underlying_type_t<_Enum> val;
        *this >> val;
        item = static_cast<_Enum>(val);
        return *this;
    }

    template <typename _Array>
    inline void Message::deserializeArray(_Array& items)
    {
        using ElementType = typename _Array::value_type;

        // Use faster, one-step deserialization of contiguous array of elements of trivial D-Bus types except bool,
        // otherwise use step-by-step deserialization of individual elements.
        if constexpr (signature_of<ElementType>::is_trivial_dbus_type && !std::is_same_v<ElementType, bool>)
        {
            deserializeArrayFast(items);
        }
        else
        {
            deserializeArraySlow(items);
        }
    }

    template <typename _Array>
    inline void Message::deserializeArrayFast(_Array& items)
    {
        using ElementType = typename _Array::value_type;

        size_t arraySize{};
        const ElementType* arrayPtr{};

        readArray(*signature_of<ElementType>::str().c_str(), (const void**)&arrayPtr, &arraySize);

        size_t elementsInMsg = arraySize / sizeof(ElementType);
        bool notEnoughSpace = items.size() < elementsInMsg;
        SDBUS_THROW_ERROR_IF(notEnoughSpace, "Failed to deserialize array: not enough space in destination sequence", EINVAL);

        std::copy_n(arrayPtr, elementsInMsg, items.begin());
    }

    template <typename _Element, typename _Allocator>
    void Message::deserializeArrayFast(std::vector<_Element, _Allocator>& items)
    {
        size_t arraySize{};
        const _Element* arrayPtr{};

        readArray(*signature_of<_Element>::str().c_str(), (const void**)&arrayPtr, &arraySize);

        items.insert(items.end(), arrayPtr, arrayPtr + (arraySize / sizeof(_Element)));
    }

    template <typename _Array>
    inline void Message::deserializeArraySlow(_Array& items)
    {
        using ElementType = typename _Array::value_type;

        if(!enterContainer(signature_of<ElementType>::str()))
            return;

        for (auto& elem : items)
            if (!(*this >> elem))
                break; // Keep the rest in the destination sequence untouched

        SDBUS_THROW_ERROR_IF(!isAtEnd(false), "Failed to deserialize array: not enough space in destination sequence", EINVAL);

        clearFlags();

        exitContainer();
    }

    template <typename _Element, typename _Allocator>
    void Message::deserializeArraySlow(std::vector<_Element, _Allocator>& items)
    {
        if(!enterContainer(signature_of<_Element>::str()))
            return;

        while (true)
        {
            _Element elem;
            if (*this >> elem)
                items.emplace_back(std::move(elem));
            else
                break;
        }

        clearFlags();

        exitContainer();
    }

    template <typename _Key, typename _Value, typename _Compare, typename _Allocator>
    inline Message& Message::operator>>(std::map<_Key, _Value, _Compare, _Allocator>& items)
    {
        deserializeDictionary(items);

        return *this;
    }

    template <typename _Key, typename _Value, typename _Hash, typename _KeyEqual, typename _Allocator>
    inline Message& Message::operator>>(std::unordered_map<_Key, _Value, _Hash, _KeyEqual, _Allocator>& items)
    {
        deserializeDictionary(items);

        return *this;
    }

    template <typename _Dictionary>
    inline void Message::deserializeDictionary(_Dictionary& items)
    {
        using KeyType = typename _Dictionary::key_type;
        using ValueType = typename _Dictionary::mapped_type;

        const std::string dictEntrySignature = signature_of<KeyType>::str() + signature_of<ValueType>::str();
        const std::string arraySignature = "{" + dictEntrySignature + "}";

        if (!enterContainer(arraySignature))
            return;

        while (true)
        {
            if (!enterDictEntry(dictEntrySignature))
                break;

            KeyType key;
            ValueType value;
            *this >> key >> value;

            items.emplace(std::move(key), std::move(value));

            exitDictEntry();
        }

        clearFlags();

        exitContainer();
    }

    namespace detail
    {
        template <typename... _Args>
        void deserialize_pack(Message& msg, _Args&... args)
        {
            (void)(msg >> ... >> args);
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
    inline Message& Message::operator>>(Struct<_ValueTypes...>& item)
    {
        auto structSignature = signature_of<Struct<_ValueTypes...>>::str();
        // Remove opening and closing parenthesis from the struct signature to get contents signature
        auto structContentSignature = structSignature.substr(1, structSignature.size()-2);

        if (!enterStruct(structContentSignature))
            return *this;

        detail::deserialize_tuple(*this, item, std::index_sequence_for<_ValueTypes...>{});

        exitStruct();

        return *this;
    }

    template <typename... _ValueTypes>
    inline Message& Message::operator>>(std::tuple<_ValueTypes...>& item)
    {
        detail::deserialize_tuple(*this, item, std::index_sequence_for<_ValueTypes...>{});
        return *this;
    }

}

#endif /* SDBUS_CXX_MESSAGE_H_ */
