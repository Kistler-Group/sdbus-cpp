/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2026 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
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

#include <sdbus-c++/Error.h>
#include <sdbus-c++/TypeTraits.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <functional>
#include <map>
#ifdef __has_include
#  if __has_include(<span>)
#    include <span>
#  endif
#endif
#include <string>
#include <sys/types.h>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

// Forward declarations
namespace sdbus {
    class Variant;
    class ObjectPath;
    class Signature;
    template <typename... ValueTypes> class Struct;
    class UnixFd;
    class MethodReply;
    namespace internal {
        class IConnection;
    } // namespace internal
} // namespace sdbus

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
        Message& operator<<(std::string_view item);
        Message& operator<<(const Variant &item);
        template <typename ...Elements>
        Message& operator<<(const std::variant<Elements...>& value);
        Message& operator<<(const ObjectPath &item);
        Message& operator<<(const Signature &item);
        Message& operator<<(const UnixFd &item);
        template <typename Element, typename Allocator>
        Message& operator<<(const std::vector<Element, Allocator>& items);
        template <typename Element, std::size_t Size>
        Message& operator<<(const std::array<Element, Size>& items);
#ifdef __cpp_lib_span
        template <typename Element, std::size_t Extent>
        Message& operator<<(const std::span<Element, Extent>& items);
#endif
        // NOLINTNEXTLINE(modernize-use-constraints): Public API is C++17-compatible
        template <typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
        Message& operator<<(const Enum& item);
        template <typename Key, typename Value>
        Message& operator<<(const DictEntry<Key, Value>& value);
        template <typename Key, typename Value, typename Compare, typename Allocator>
        Message& operator<<(const std::map<Key, Value, Compare, Allocator>& items);
        template <typename Key, typename Value, typename Hash, typename KeyEqual, typename Allocator>
        Message& operator<<(const std::unordered_map<Key, Value, Hash, KeyEqual, Allocator>& items);
        template <typename... ValueTypes>
        Message& operator<<(const Struct<ValueTypes...>& item);
        template <typename... ValueTypes>
        Message& operator<<(const std::tuple<ValueTypes...>& item);

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
        template <typename Element, typename Allocator>
        Message& operator>>(std::vector<Element, Allocator>& items);
        template <typename Element, std::size_t Size>
        Message& operator>>(std::array<Element, Size>& items);
#ifdef __cpp_lib_span
        template <typename Element, std::size_t Extent>
        Message& operator>>(std::span<Element, Extent>& items);
#endif
        // NOLINTNEXTLINE(modernize-use-constraints): Public API is C++17-compatible
        template <typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
        Message& operator>>(Enum& item);
        template <typename Key, typename Value>
        Message& operator>>(DictEntry<Key, Value>& value);
        template <typename Key, typename Value, typename Compare, typename Allocator>
        Message& operator>>(std::map<Key, Value, Compare, Allocator>& items);
        template <typename Key, typename Value, typename Hash, typename KeyEqual, typename Allocator>
        Message& operator>>(std::unordered_map<Key, Value, Hash, KeyEqual, Allocator>& items);
        template <typename... ValueTypes>
        Message& operator>>(Struct<ValueTypes...>& item);
        template <typename... ValueTypes>
        Message& operator>>(std::tuple<ValueTypes...>& item);

        template <typename ElementType>
        Message& openContainer();
        Message& openContainer(const char* signature);
        Message& closeContainer();
        template <typename KeyType, typename ValueType>
        Message& openDictEntry();
        Message& openDictEntry(const char* signature);
        Message& closeDictEntry();
        template <typename ValueType>
        Message& openVariant();
        Message& openVariant(const char* signature);
        Message& closeVariant();
        template <typename... ValueTypes>
        Message& openStruct();
        Message& openStruct(const char* signature);
        Message& closeStruct();

        template <typename ElementType>
        Message& enterContainer();
        Message& enterContainer(const char* signature);
        Message& exitContainer();
        template <typename KeyType, typename ValueType>
        Message& enterDictEntry();
        Message& enterDictEntry(const char* signature);
        Message& exitDictEntry();
        template <typename ValueType>
        Message& enterVariant();
        Message& enterVariant(const char* signature);
        Message& exitVariant();
        template <typename... ValueTypes>
        Message& enterStruct();
        Message& enterStruct(const char* signature);
        Message& exitStruct();

        Message& appendArray(char type, const void *ptr, size_t size);
        Message& readArray(char type, const void **ptr, size_t *size);

        template <typename Key, typename Value, typename Callback>
        Message& serializeDictionary(const Callback& callback);
        template <typename Key, typename Value>
        Message& serializeDictionary(const std::initializer_list<DictEntry<Key, Value>>& dictEntries);
        template <typename Key, typename Value, typename Callback>
        Message& deserializeDictionary(const Callback& callback);

        explicit operator bool() const;
        void clearFlags();

        const char* getInterfaceName() const;
        const char* getMemberName() const;
        const char* getSender() const;
        const char* getPath() const;
        const char* getDestination() const;
        uint64_t getCookie() const;
        // TODO: short docs in whole Message API
        std::pair<char, const char*> peekType() const;
        bool isValid() const;
        bool isEmpty() const;
        bool isAtEnd(bool complete) const;

        void copyTo(Message& destination, bool complete) const;
        void seal();
        void rewind(bool complete);

        enum class DumpFlags : uint64_t // NOLINT(performance-enum-size): using size from sd-bus
        {
            Default               = 0ULL,
            WithHeader            = 1ULL << 0,
            SubtreeOnly           = 1ULL << 1,
            SubtreeOnlyWithHeader = WithHeader | SubtreeOnly
        };
        [[nodiscard]] std::string dumpToString(DumpFlags flags) const;

        pid_t getCredsPid() const;
        uid_t getCredsUid() const;
        uid_t getCredsEuid() const;
        gid_t getCredsGid() const;
        gid_t getCredsEgid() const;
        std::vector<gid_t> getCredsSupplementaryGids() const;
        std::string getSELinuxContext() const;

        class Factory;

    private:
        template <typename Array>
        void serializeArray(const Array& items);
        template <typename Array>
        void deserializeArray(Array& items);
        template <typename Array>
        void deserializeArrayFast(Array& items);
        template <typename Element, typename Allocator>
        void deserializeArrayFast(std::vector<Element, Allocator>& items);
        template <typename Array>
        void deserializeArraySlow(Array& items);
        template <typename Element, typename Allocator>
        void deserializeArraySlow(std::vector<Element, Allocator>& items);

    protected:
        Message() = default;
        explicit Message(internal::IConnection* connection) noexcept;
        Message(void *msg, internal::IConnection* connection) noexcept;
        Message(void *msg, internal::IConnection* connection, adopt_message_t) noexcept;

        friend Factory;

        void* msg_{};
        internal::IConnection* connection_{};
        mutable bool ok_{true};
    };

    class MethodCall : public Message
    {
        using Message::Message;
        friend Factory;

    public:
        MethodCall() = default;

        MethodReply send(uint64_t timeout) const;
        [[nodiscard]] Slot send(void* callback, void* userData, uint64_t timeout, return_slot_t) const;

        MethodReply createReply() const;
        MethodReply createErrorReply(const Error& error) const;

        void dontExpectReply();
        bool doesntExpectReply() const;

    protected:
        MethodCall(void *msg, internal::IConnection* connection, adopt_message_t) noexcept;

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
        uint64_t getReplyCookie() const;
    };

    class Signal : public Message
    {
        using Message::Message;
        friend Factory;

    public:
        Signal() = default;
        void setDestination(const std::string& destination);
        void setDestination(const char* destination);
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

    PlainMessage createPlainMessage();

    template <typename ...Elements>
    inline Message& Message::operator<<(const std::variant<Elements...>& value)
    {
        std::visit([this](const auto& inner)
        {
            openVariant<decltype(inner)>();
            *this << inner;
            closeVariant();
        }, value);

        return *this;
    }

    template <typename Element, typename Allocator>
    inline Message& Message::operator<<(const std::vector<Element, Allocator>& items)
    {
        serializeArray(items);

        return *this;
    }

    template <typename Element, std::size_t Size>
    inline Message& Message::operator<<(const std::array<Element, Size>& items)
    {
        serializeArray(items);

        return *this;
    }

#ifdef __cpp_lib_span
    template <typename Element, std::size_t Extent>
    inline Message& Message::operator<<(const std::span<Element, Extent>& items)
    {
        serializeArray(items);

        return *this;
    }
#endif

    template <typename Enum, typename>
    inline Message& Message::operator<<(const Enum &item)
    {
        return operator<<(static_cast<std::underlying_type_t<Enum>>(item));
    }

    template <typename Array>
    inline void Message::serializeArray(const Array& items)
    {
        using ElementType = typename Array::value_type;

        // Use faster, one-step serialization of contiguous array of elements of trivial D-Bus types except bool,
        // otherwise use step-by-step serialization of individual elements.
        if constexpr (signature_of<ElementType>::is_trivial_dbus_type && !std::is_same_v<ElementType, bool>)
        {
            constexpr auto signature = as_null_terminated(signature_of_v<ElementType>);
            appendArray(*signature.data(), items.data(), items.size() * sizeof(ElementType));
        }
        else
        {
            openContainer<ElementType>();

            for (const auto& item : items)
                *this << item;

            closeContainer();
        }
    }

    template <typename Key, typename Value>
    inline Message& Message::operator<<(const DictEntry<Key, Value>& value)
    {
        openDictEntry<Key, Value>();
        *this << value.first;
        *this << value.second;
        closeDictEntry();

        return *this;
    }

    template <typename Key, typename Value, typename Compare, typename Allocator>
    inline Message& Message::operator<<(const std::map<Key, Value, Compare, Allocator>& items)
    {
        serializeDictionary<Key, Value>([&items](Message& msg){ for (const auto& item : items) msg << item; });

        return *this;
    }

    template <typename Key, typename Value, typename Hash, typename KeyEqual, typename Allocator>
    inline Message& Message::operator<<(const std::unordered_map<Key, Value, Hash, KeyEqual, Allocator>& items)
    {
        serializeDictionary<Key, Value>([&items](Message& msg){ for (const auto& item : items) msg << item; });

        return *this;
    }

    template <typename Key, typename Value>
    inline Message& Message::serializeDictionary(const std::initializer_list<DictEntry<Key, Value>>& dictEntries)
    {
        serializeDictionary<Key, Value>([&](Message& msg){ for (const auto& entry : dictEntries) msg << entry; });

        return *this;
    }

    template <typename Key, typename Value, typename Callback>
    inline Message& Message::serializeDictionary(const Callback& callback)
    {
        openContainer<DictEntry<Key, Value>>();
        callback(*this);
        closeContainer();

        return *this;
    }

    namespace detail
    {
        template <typename... Args>
        void serialize_pack(Message& msg, Args&&... args)
        {
            (void)(msg << ... << std::forward<Args>(args));
        }

        template <class Tuple, std::size_t... Is>
        void serialize_tuple( Message& msg
                            , const Tuple& tuple
                            , std::index_sequence<Is...>)
        {
            serialize_pack(msg, std::get<Is>(tuple)...);
        }
    } // namespace detail

    template <typename... ValueTypes>
    inline Message& Message::operator<<(const Struct<ValueTypes...>& item)
    {
        openStruct<ValueTypes...>();
        detail::serialize_tuple(*this, item, std::index_sequence_for<ValueTypes...>{});
        closeStruct();

        return *this;
    }

    template <typename... ValueTypes>
    inline Message& Message::operator<<(const std::tuple<ValueTypes...>& item)
    {
        detail::serialize_tuple(*this, item, std::index_sequence_for<ValueTypes...>{});
        return *this;
    }

    namespace detail
    {
        template <typename Element, typename... Elements>
        bool deserialize_variant(Message& msg, std::variant<Elements...>& value, const char* signature)
        {
            constexpr auto elemSignature = as_null_terminated(signature_of_v<Element>);
            if (std::strcmp(signature, elemSignature.data()) != 0)
                return false;

            Element temp;
            msg.enterVariant(signature);
            msg >> temp;
            msg.exitVariant();
            value = std::move(temp);
            return true;
        }
    } // namespace detail

    template <typename... Elements>
    inline Message& Message::operator>>(std::variant<Elements...>& value)
    {
        auto [type, contents] = peekType();
        const bool result = (detail::deserialize_variant<Elements>(*this, value, contents) || ...);
        SDBUS_THROW_ERROR_IF(!result, "Failed to deserialize variant: signature did not match any of the variant types", EINVAL);
        return *this;
    }

    template <typename Element, typename Allocator>
    inline Message& Message::operator>>(std::vector<Element, Allocator>& items)
    {
        deserializeArray(items);

        return *this;
    }

    template <typename Element, std::size_t Size>
    inline Message& Message::operator>>(std::array<Element, Size>& items)
    {
        deserializeArray(items);

        return *this;
    }

#ifdef __cpp_lib_span
    template <typename Element, std::size_t Extent>
    inline Message& Message::operator>>(std::span<Element, Extent>& items)
    {
        deserializeArray(items);

        return *this;
    }
#endif

    template <typename Enum, typename>
    inline Message& Message::operator>>(Enum& item)
    {
        std::underlying_type_t<Enum> val;
        *this >> val;
        item = static_cast<Enum>(val);
        return *this;
    }

    template <typename Array>
    inline void Message::deserializeArray(Array& items)
    {
        using ElementType = typename Array::value_type;

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

    template <typename Array>
    inline void Message::deserializeArrayFast(Array& items)
    {
        using ElementType = typename Array::value_type;

        size_t arraySize{};
        const ElementType* arrayPtr{};

        constexpr auto signature = as_null_terminated(signature_of_v<ElementType>);
        readArray(*signature.data(), reinterpret_cast<const void**>(&arrayPtr), &arraySize);

        const size_t elementsInMsg = arraySize / sizeof(ElementType);
        const bool notEnoughSpace = items.size() < elementsInMsg;
        SDBUS_THROW_ERROR_IF(notEnoughSpace, "Failed to deserialize array: not enough space in destination sequence", EINVAL);

        std::copy_n(arrayPtr, elementsInMsg, items.begin());
    }

    template <typename Element, typename Allocator>
    void Message::deserializeArrayFast(std::vector<Element, Allocator>& items)
    {
        size_t arraySize{};
        const Element* arrayPtr{};

        constexpr auto signature = as_null_terminated(signature_of_v<Element>);
        readArray(*signature.data(), reinterpret_cast<const void**>(&arrayPtr), &arraySize);

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        items.insert(items.end(), arrayPtr, arrayPtr + (arraySize / sizeof(Element)));
    }

    template <typename Array>
    inline void Message::deserializeArraySlow(Array& items)
    {
        using ElementType = typename Array::value_type;

        if(!enterContainer<ElementType>())
            return;

        for (auto& elem : items)
            if (!(*this >> elem))
                break; // Keep the rest in the destination sequence untouched

        SDBUS_THROW_ERROR_IF(!isAtEnd(false), "Failed to deserialize array: not enough space in destination sequence", EINVAL);

        clearFlags();

        exitContainer();
    }

    template <typename Element, typename Allocator>
    void Message::deserializeArraySlow(std::vector<Element, Allocator>& items)
    {
        if(!enterContainer<Element>())
            return;

        while (true)
        {
            Element elem;
            if (*this >> elem)
                items.emplace_back(std::move(elem));
            else
                break;
        }

        clearFlags();

        exitContainer();
    }

    template <typename Key, typename Value>
    inline Message& Message::operator>>(DictEntry<Key, Value>& value)
    {
        if (!enterDictEntry<Key, Value>())
            return *this;
        *this >> value.first >> value.second;
        exitDictEntry();

        return *this;
    }

    template <typename Key, typename Value, typename Compare, typename Allocator>
    inline Message& Message::operator>>(std::map<Key, Value, Compare, Allocator>& items)
    {
        deserializeDictionary<Key, Value>([&items](auto dictEntry){ items.insert(std::move(dictEntry)); });

        return *this;
    }

    template <typename Key, typename Value, typename Hash, typename KeyEqual, typename Allocator>
    inline Message& Message::operator>>(std::unordered_map<Key, Value, Hash, KeyEqual, Allocator>& items)
    {
        deserializeDictionary<Key, Value>([&items](auto dictEntry){ items.insert(std::move(dictEntry)); });

        return *this;
    }

    template <typename Key, typename Value, typename Callback>
    inline Message& Message::deserializeDictionary(const Callback& callback)
    {
        if (!enterContainer<DictEntry<Key, Value>>())
            return *this;

        while (true)
        {
            DictEntry<Key, Value> dictEntry;
            *this >> dictEntry;
            if (!*this)
                break;
            callback(std::move(dictEntry));
        }
        clearFlags();

        exitContainer();

        return *this;
    }

    namespace detail
    {
        template <typename... Args>
        void deserialize_pack(Message& msg, Args&... args)
        {
            (void)(msg >> ... >> args);
        }

        template <class Tuple, std::size_t... Is>
        void deserialize_tuple( Message& msg
                              , Tuple& tuple
                              , std::index_sequence<Is...> )
        {
            deserialize_pack(msg, std::get<Is>(tuple)...);
        }
    } // namespace detail

    template <typename... ValueTypes>
    inline Message& Message::operator>>(Struct<ValueTypes...>& item)
    {
        if (!enterStruct<ValueTypes...>())
            return *this;

        detail::deserialize_tuple(*this, item, std::index_sequence_for<ValueTypes...>{});

        exitStruct();

        return *this;
    }

    template <typename... ValueTypes>
    inline Message& Message::operator>>(std::tuple<ValueTypes...>& item)
    {
        detail::deserialize_tuple(*this, item, std::index_sequence_for<ValueTypes...>{});
        return *this;
    }

    template <typename ElementType>
    inline Message& Message::openContainer()
    {
        constexpr auto signature = as_null_terminated(signature_of_v<ElementType>);
        return openContainer(signature.data());
    }

    template <typename KeyType, typename ValueType>
    inline Message& Message::openDictEntry()
    {
        constexpr auto signature = as_null_terminated(signature_of_v<std::tuple<KeyType, ValueType>>);
        return openDictEntry(signature.data());
    }

    template <typename ValueType>
    inline Message& Message::openVariant()
    {
        constexpr auto signature = as_null_terminated(signature_of_v<ValueType>);
        return openVariant(signature.data());
    }

    template <typename... ValueTypes>
    inline Message& Message::openStruct()
    {
        constexpr auto signature = as_null_terminated(signature_of_v<std::tuple<ValueTypes...>>);
        return openStruct(signature.data());
    }

    template <typename ElementType>
    inline Message& Message::enterContainer()
    {
        constexpr auto signature = as_null_terminated(signature_of_v<ElementType>);
        return enterContainer(signature.data());
    }

    template <typename KeyType, typename ValueType>
    inline Message& Message::enterDictEntry()
    {
        constexpr auto signature = as_null_terminated(signature_of_v<std::tuple<KeyType, ValueType>>);
        return enterDictEntry(signature.data());
    }

    template <typename ValueType>
    inline Message& Message::enterVariant()
    {
        constexpr auto signature = as_null_terminated(signature_of_v<ValueType>);
        return enterVariant(signature.data());
    }

    template <typename... ValueTypes>
    inline Message& Message::enterStruct()
    {
        constexpr auto signature = as_null_terminated(signature_of_v<std::tuple<ValueTypes...>>);
        return enterStruct(signature.data());
    }

} // namespace sdbus

#endif /* SDBUS_CXX_MESSAGE_H_ */
