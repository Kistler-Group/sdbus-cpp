/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2022 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
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
#include <utility>
#include <cstdint>
#include <cassert>
#include <functional>
#include <sys/types.h>

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
        class IConnection;
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
     * You don't need to work with this class directly if you use high-level APIs
     * of @c IObject and @c IProxy.
     *
     ***********************************************/
    class [[nodiscard]] Message
    {
    public:
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
        Message& operator<<(const UnixFd &item);
        template <typename _Element>
        Message& appendArray(const std::vector<_Element>& items);
        
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
        Message& operator>>(UnixFd &item);        
        template <typename _Element>
        Message& readArray(std::vector<_Element>& items);

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

    protected:
        Message() = default;
        explicit Message(internal::ISdBus* sdbus) noexcept;
        Message(void *msg, internal::ISdBus* sdbus) noexcept;
        Message(void *msg, internal::ISdBus* sdbus, adopt_message_t) noexcept;

        Message(const Message&) noexcept;
        Message& operator=(const Message&) noexcept;
        Message(Message&& other) noexcept;
        Message& operator=(Message&& other) noexcept;

        ~Message();

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
        [[deprecated("Use send overload with floating_slot instead")]] void send(void* callback, void* userData, uint64_t timeout, dont_request_slot_t) const;
        void send(void* callback, void* userData, uint64_t timeout, floating_slot_t) const;
        [[nodiscard]] Slot send(void* callback, void* userData, uint64_t timeout) const;

        MethodReply createReply() const;
        MethodReply createErrorReply(const sdbus::Error& error) const;

        void dontExpectReply();
        bool doesntExpectReply() const;

    protected:
        MethodCall(void *msg, internal::ISdBus* sdbus, const internal::IConnection* connection, adopt_message_t) noexcept;

    private:
        MethodReply sendWithReply(uint64_t timeout = 0) const;
        MethodReply sendWithNoReply() const;
        const internal::IConnection* connection_{};
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

    // Below template is used for any _Element type except the ones explicitly
    // specialized below
    template <typename _Element>
    inline Message& operator<<(Message& msg, const std::vector<_Element>& items)
    {
        msg.openContainer(signature_of<_Element>::str());

        for (const auto& item : items)
            msg << item;

        msg.closeContainer();

        return msg;
    }
    
    // Specialize std::vector<int16_t>
    template <> inline Message& operator<<(Message& msg, const std::vector<int16_t>& items) 
    { return msg.appendArray(items); }
    // Specialize std::vector<int32_t>
    template <> inline Message& operator<<(Message& msg, const std::vector<int32_t>& items) 
    { return msg.appendArray(items); }
    // Specialize std::vector<int64_t>
    template <> inline Message& operator<<(Message& msg, const std::vector<int64_t>& items) 
    { return msg.appendArray(items); }
    // Specialize std::vector<uint8_t>
    template <> inline Message& operator<<(Message& msg, const std::vector<uint8_t>& items) 
    { return msg.appendArray(items); }
    // Specialize std::vector<uint16_t>
    template <> inline Message& operator<<(Message& msg, const std::vector<uint16_t>& items) 
    { return msg.appendArray(items); }
    // Specialize std::vector<uint32_t>
    template <> inline Message& operator<<(Message& msg, const std::vector<uint32_t>& items) 
    { return msg.appendArray(items); }
    // Specialize std::vector<uint64_t>
    template <> inline Message& operator<<(Message& msg, const std::vector<uint64_t>& items) 
    { return msg.appendArray(items); }
    // Specialize std::vector<double>
    template <> inline Message& operator<<(Message& msg, const std::vector<double>& items) 
    { return msg.appendArray(items); }

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


    // Below template is used for any _Element type except the ones explicitly
    // specialized below
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
    
    
    // Specialize std::vector<int16_t>
    template <> inline Message& operator>>(Message& msg, std::vector<int16_t>& items) 
    { return msg.readArray(items); }
    // Specialize std::vector<int32_t>
    template <> inline Message& operator>>(Message& msg, std::vector<int32_t>& items) 
    { return msg.readArray(items); }
    // Specialize std::vector<int64_t>
    template <> inline Message& operator>>(Message& msg, std::vector<int64_t>& items) 
    { return msg.readArray(items); }
    // Specialize std::vector<uint8_t>
    template <> inline Message& operator>>(Message& msg, std::vector<uint8_t>& items) 
    { return msg.readArray(items); }
    // Specialize std::vector<uint16_t>
    template <> inline Message& operator>>(Message& msg, std::vector<uint16_t>& items) 
    { return msg.readArray(items); }
    // Specialize std::vector<uint32_t>
    template <> inline Message& operator>>(Message& msg, std::vector<uint32_t>& items) 
    { return msg.readArray(items); }
    // Specialize std::vector<uint64_t>
    template <> inline Message& operator>>(Message& msg, std::vector<uint64_t>& items) 
    { return msg.readArray(items); }
    // Specialize std::vector<double>
    template <> inline Message& operator>>(Message& msg, std::vector<double>& items) 
    { return msg.readArray(items); }

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
