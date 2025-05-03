/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2024 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file Message.cpp
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

#include "sdbus-c++/Message.h"

#include "sdbus-c++/Error.h"
#include "sdbus-c++/Types.h"
#include "sdbus-c++/TypeTraits.h"

#include "IConnection.h"
#include "MessageUtils.h"
#include "ScopeGuard.h"

#include <cassert>
#include <cstdint> // int16_t, uint64_t, ...
#include <cstdlib> // atexit
#include <cstring>
#include <string>
#include <string_view>
#include <sys/types.h> // pid_t, gid_t, ...
#include <tuple> // std::ignore
#include <memory> // std::unique_ptr
#include <utility> // std::move
#include <vector>
#include SDBUS_HEADER

namespace sdbus {

Message::Message(internal::IConnection* connection) noexcept
    : connection_(connection)
{
    assert(connection_ != nullptr);
}

Message::Message(void *msg, internal::IConnection* connection) noexcept
    : msg_(msg)
    , connection_(connection)
{
    assert(msg_ != nullptr);
    assert(connection_ != nullptr);
    connection_->incrementMessageRefCount(static_cast<sd_bus_message*>(msg_));
}

Message::Message(void *msg, internal::IConnection* connection, adopt_message_t) noexcept
    : msg_(msg)
    , connection_(connection)
{
    assert(msg_ != nullptr);
    assert(connection_ != nullptr);
}

Message::Message(const Message& other) noexcept
{
    *this = other;
}

Message& Message::operator=(const Message& other) noexcept
{
    if (this == &other)
        return *this;

    if (msg_)
        connection_->decrementMessageRefCount(static_cast<sd_bus_message*>(msg_));

    msg_ = other.msg_;
    connection_ = other.connection_;
    ok_ = other.ok_;

    connection_->incrementMessageRefCount(static_cast<sd_bus_message*>(msg_));

    return *this;
}

Message::Message(Message&& other) noexcept
{
    *this = std::move(other);
}

Message& Message::operator=(Message&& other) noexcept
{
    if (msg_)
        connection_->decrementMessageRefCount(static_cast<sd_bus_message*>(msg_));

    msg_ = other.msg_;
    other.msg_ = nullptr;
    connection_ = other.connection_;
    other.connection_ = nullptr;
    ok_ = other.ok_;
    other.ok_ = true;

    return *this;
}

Message::~Message()
{
    if (msg_)
        connection_->decrementMessageRefCount(static_cast<sd_bus_message*>(msg_));
}

Message& Message::operator<<(bool item)
{
    int itemAsInt = static_cast<int>(item);

    // Direct sd-bus method, bypassing SdBus mutex, are called here, since Message serialization/deserialization,
    // as well as getter/setter methods are not thread safe by design. Additionally, they are called frequently,
    // so some overhead is spared. What is thread-safe in Message class is Message constructors, copy/move operations
    // and the destructor, so the Message instance can be passed from one thread to another safely.
    auto r = sd_bus_message_append_basic(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_BOOLEAN, &itemAsInt);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize a bool value", -r);

    return *this;
}

Message& Message::operator<<(int16_t item)
{
    auto r = sd_bus_message_append_basic(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_INT16, &item);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize a int16_t value", -r);

    return *this;
}

Message& Message::operator<<(int32_t item)
{
    auto r = sd_bus_message_append_basic(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_INT32, &item);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize a int32_t value", -r);

    return *this;
}

Message& Message::operator<<(int64_t item)
{
    auto r = sd_bus_message_append_basic(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_INT64, &item);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize a int64_t value", -r);

    return *this;
}

Message& Message::operator<<(uint8_t item)
{
    auto r = sd_bus_message_append_basic(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_BYTE, &item);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize a byte value", -r);

    return *this;
}

Message& Message::operator<<(uint16_t item)
{
    auto r = sd_bus_message_append_basic(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_UINT16, &item);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize a uint16_t value", -r);

    return *this;
}

Message& Message::operator<<(uint32_t item)
{
    auto r = sd_bus_message_append_basic(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_UINT32, &item);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize a uint32_t value", -r);

    return *this;
}

Message& Message::operator<<(uint64_t item)
{
    auto r = sd_bus_message_append_basic(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_UINT64, &item);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize a uint64_t value", -r);

    return *this;
}

Message& Message::operator<<(double item)
{
    auto r = sd_bus_message_append_basic(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_DOUBLE, &item);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize a double value", -r);

    return *this;
}

Message& Message::operator<<(const char* item)
{
    auto r = sd_bus_message_append_basic(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_STRING, item);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize a C-string value", -r);

    return *this;
}

Message& Message::operator<<(const std::string& item)
{
    auto r = sd_bus_message_append_basic(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_STRING, item.c_str());
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize a string value", -r);

    return *this;
}

Message& Message::operator<<(std::string_view item)
{
    char* destPtr{};
    auto r = sd_bus_message_append_string_space(static_cast<sd_bus_message*>(msg_), item.length(), &destPtr);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize a string_view value", -r);

    std::memcpy(destPtr, item.data(), item.length());

    return *this;
}

Message& Message::operator<<(const Variant &item)
{
    item.serializeTo(*this);

    return *this;
}

Message& Message::operator<<(const ObjectPath &item)
{
    auto r = sd_bus_message_append_basic(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_OBJECT_PATH, item.c_str());
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize an ObjectPath value", -r);

    return *this;
}

Message& Message::operator<<(const Signature &item)
{
    auto r = sd_bus_message_append_basic(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_SIGNATURE, item.c_str());
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize a Signature value", -r);

    return *this;
}

Message& Message::operator<<(const UnixFd &item)
{
    auto fd = item.get();
    auto r = sd_bus_message_append_basic(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_UNIX_FD, &fd);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize a UnixFd value", -r);

    return *this;
}

Message& Message::appendArray(char type, const void *ptr, size_t size)
{
    auto r = sd_bus_message_append_array(static_cast<sd_bus_message*>(msg_), type, ptr, size);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize an array", -r);

    return *this;
}

Message& Message::operator>>(bool& item)
{
    int intItem{};
    auto r = sd_bus_message_read_basic(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_BOOLEAN, &intItem);
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to deserialize a bool value", -r);

    item = static_cast<bool>(intItem);

    return *this;
}

Message& Message::operator>>(int16_t& item)
{
    auto r = sd_bus_message_read_basic(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_INT16, &item);
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to deserialize a int16_t value", -r);

    return *this;
}

Message& Message::operator>>(int32_t& item)
{
    auto r = sd_bus_message_read_basic(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_INT32, &item);
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to deserialize a int32_t value", -r);

    return *this;
}

Message& Message::operator>>(int64_t& item)
{
    auto r = sd_bus_message_read_basic(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_INT64, &item);
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to deserialize a bool value", -r);

    return *this;
}

Message& Message::operator>>(uint8_t& item)
{
    auto r = sd_bus_message_read_basic(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_BYTE, &item);
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to deserialize a byte value", -r);

    return *this;
}

Message& Message::operator>>(uint16_t& item)
{
    auto r = sd_bus_message_read_basic(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_UINT16, &item);
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to deserialize a uint16_t value", -r);

    return *this;
}

Message& Message::operator>>(uint32_t& item)
{
    auto r = sd_bus_message_read_basic(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_UINT32, &item);
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to deserialize a uint32_t value", -r);

    return *this;
}

Message& Message::operator>>(uint64_t& item)
{
    auto r = sd_bus_message_read_basic(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_UINT64, &item);
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to deserialize a uint64_t value", -r);

    return *this;
}

Message& Message::readArray(char type, const void **ptr, size_t *size)
{
    auto r = sd_bus_message_read_array(static_cast<sd_bus_message*>(msg_), type, ptr, size);
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to deserialize an array", -r);

    return *this;
}

Message& Message::operator>>(double& item)
{
    auto r = sd_bus_message_read_basic(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_DOUBLE, &item);
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to deserialize a double value", -r);

    return *this;
}

Message& Message::operator>>(char*& item)
{
    auto r = sd_bus_message_read_basic(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_STRING, reinterpret_cast<void*>(&item));
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to deserialize a string value", -r);

    return *this;
}

Message& Message::operator>>(std::string& item)
{
    char* str{};
    (*this) >> str;

    if (str != nullptr)
        item = str;

    return *this;
}

Message& Message::operator>>(Variant &item)
{
    item.deserializeFrom(*this);

    // Empty variant is normally prohibited. Users cannot send empty variants.
    // Therefore in this context an empty variant means that we are at the end
    // of deserializing a container, and thus we shall set ok_ flag to false.
    if (item.isEmpty())
        ok_ = false;

    return *this;
}

Message& Message::operator>>(ObjectPath &item)
{
    char* str{};
    auto r = sd_bus_message_read_basic(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_OBJECT_PATH, reinterpret_cast<void*>(&str));
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to deserialize an ObjectPath value", -r);

    if (str != nullptr)
        item = str;

    return *this;
}

Message& Message::operator>>(Signature &item)
{
    char* str{};
    auto r = sd_bus_message_read_basic(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_SIGNATURE, reinterpret_cast<void*>(&str));
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to deserialize a Signature value", -r);

    if (str != nullptr)
        item = str;

    return *this;
}

Message& Message::operator>>(UnixFd &item)
{
    int fd = -1;
    auto r = sd_bus_message_read_basic(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_UNIX_FD, &fd);
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to deserialize a UnixFd value", -r);

    item.reset(fd);

    return *this;
}

Message& Message::openContainer(const char* signature)
{
    auto r = sd_bus_message_open_container(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_ARRAY, signature);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to open a container", -r);

    return *this;
}

Message& Message::closeContainer()
{
    auto r = sd_bus_message_close_container(static_cast<sd_bus_message*>(msg_));
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to close a container", -r);

    return *this;
}

Message& Message::openDictEntry(const char* signature)
{
    auto r = sd_bus_message_open_container(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_DICT_ENTRY, signature);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to open a dictionary entry", -r);

    return *this;
}

Message& Message::closeDictEntry()
{
    auto r = sd_bus_message_close_container(static_cast<sd_bus_message*>(msg_));
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to close a dictionary entry", -r);

    return *this;
}

Message& Message::openVariant(const char* signature)
{
    auto r = sd_bus_message_open_container(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_VARIANT, signature);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to open a variant", -r);

    return *this;
}

Message& Message::closeVariant()
{
    auto r = sd_bus_message_close_container(static_cast<sd_bus_message*>(msg_));
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to close a variant", -r);

    return *this;
}

Message& Message::openStruct(const char* signature)
{
    auto r = sd_bus_message_open_container(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_STRUCT, signature);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to open a struct", -r);

    return *this;
}

Message& Message::closeStruct()
{
    auto r = sd_bus_message_close_container(static_cast<sd_bus_message*>(msg_));
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to close a struct", -r);

    return *this;
}

Message& Message::enterContainer(const char* signature)
{
    auto r = sd_bus_message_enter_container(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_ARRAY, signature);
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to enter a container", -r);

    return *this;
}

Message& Message::exitContainer()
{
    auto r = sd_bus_message_exit_container(static_cast<sd_bus_message*>(msg_));
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to exit a container", -r);

    return *this;
}

Message& Message::enterDictEntry(const char* signature)
{
    auto r = sd_bus_message_enter_container(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_DICT_ENTRY, signature);
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to enter a dictionary entry", -r);

    return *this;
}

Message& Message::exitDictEntry()
{
    auto r = sd_bus_message_exit_container(static_cast<sd_bus_message*>(msg_));
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to exit a dictionary entry", -r);

    return *this;
}

Message& Message::enterVariant(const char* signature)
{
    auto r = sd_bus_message_enter_container(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_VARIANT, signature);
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to enter a variant", -r);

    return *this;
}

Message& Message::exitVariant()
{
    auto r = sd_bus_message_exit_container(static_cast<sd_bus_message*>(msg_));
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to exit a variant", -r);

    return *this;
}

Message& Message::enterStruct(const char* signature)
{
    auto r = sd_bus_message_enter_container(static_cast<sd_bus_message*>(msg_), SD_BUS_TYPE_STRUCT, signature);
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to enter a struct", -r);

    return *this;
}

Message& Message::exitStruct()
{
    auto r = sd_bus_message_exit_container(static_cast<sd_bus_message*>(msg_));
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to exit a struct", -r);

    return *this;
}


Message::operator bool() const
{
    return ok_;
}

void Message::clearFlags()
{
    ok_ = true;
}

void Message::copyTo(Message& destination, bool complete) const
{
    auto r = sd_bus_message_copy(static_cast<sd_bus_message*>(destination.msg_), static_cast<sd_bus_message*>(msg_), complete); // NOLINT(readability-implicit-bool-conversion)
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to copy the message", -r);
}

void Message::seal()
{
    const auto messageCookie = 1;
    const auto sealTimeout = 0;
    auto r = sd_bus_message_seal(static_cast<sd_bus_message*>(msg_), messageCookie, sealTimeout);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to seal the message", -r);
}

void Message::rewind(bool complete)
{
    auto r = sd_bus_message_rewind(static_cast<sd_bus_message*>(msg_), complete); // NOLINT(readability-implicit-bool-conversion)
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to rewind the message", -r);
}

const char* Message::getInterfaceName() const
{
    return sd_bus_message_get_interface(static_cast<sd_bus_message*>(msg_));
}

const char* Message::getMemberName() const
{
    return sd_bus_message_get_member(static_cast<sd_bus_message*>(msg_));
}

const char* Message::getSender() const
{
    return sd_bus_message_get_sender(static_cast<sd_bus_message*>(msg_));
}

const char* Message::getPath() const
{
    return sd_bus_message_get_path(static_cast<sd_bus_message*>(msg_));
}

const char* Message::getDestination() const
{
    return sd_bus_message_get_destination(static_cast<sd_bus_message*>(msg_));
}

uint64_t Message::getCookie() const
{
    uint64_t cookie{};
    auto r =  sd_bus_message_get_cookie(static_cast<sd_bus_message*>(msg_), &cookie);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get cookie", -r);
    return cookie;
}

std::pair<char, const char*> Message::peekType() const
{
    char typeSignature{};
    const char* contentsSignature{};
    auto r = sd_bus_message_peek_type(static_cast<sd_bus_message*>(msg_), &typeSignature, &contentsSignature);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to peek message type", -r);
    return {typeSignature, contentsSignature};
}

bool Message::isValid() const
{
    return msg_ != nullptr && connection_ != nullptr;
}

bool Message::isEmpty() const
{
    return sd_bus_message_is_empty(static_cast<sd_bus_message*>(msg_)) != 0;
}

bool Message::isAtEnd(bool complete) const
{
    return sd_bus_message_at_end(static_cast<sd_bus_message*>(msg_), complete) > 0; // NOLINT(readability-implicit-bool-conversion)
}

// TODO: Create a RAII ownership class for creds with copy&move semantics, doing ref()/unref() under the hood.
//   Create a method Message::querySenderCreds() that will return an object of this class by value, through IConnection and SdBus mutex.
//   The class will expose methods like getPid(), getUid(), etc. that will directly call sd_bus_creds_* functions, no need for mutex here.
pid_t Message::getCredsPid() const
{
    const uint64_t mask = SD_BUS_CREDS_PID | SD_BUS_CREDS_AUGMENT;
    sd_bus_creds *creds = nullptr;
    SCOPE_EXIT{ connection_->decrementCredsRefCount(creds); };

    int r = connection_->querySenderCredentials(static_cast<sd_bus_message*>(msg_), mask, &creds);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus creds", -r);

    pid_t pid = 0;
    r = sd_bus_creds_get_pid(creds, &pid);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus cred pid", -r);
    return pid;
}

uid_t Message::getCredsUid() const
{
    const uint64_t mask = SD_BUS_CREDS_UID | SD_BUS_CREDS_AUGMENT;
    sd_bus_creds *creds = nullptr;
    SCOPE_EXIT{ connection_->decrementCredsRefCount(creds); };
    int r = connection_->querySenderCredentials(static_cast<sd_bus_message*>(msg_), mask, &creds);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus creds", -r);

    auto uid = static_cast<uid_t>(-1);
    r = sd_bus_creds_get_uid(creds, &uid);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus cred uid", -r);
    return uid;
}

uid_t Message::getCredsEuid() const
{
    const uint64_t mask = SD_BUS_CREDS_EUID | SD_BUS_CREDS_AUGMENT;
    sd_bus_creds *creds = nullptr;
    SCOPE_EXIT{ connection_->decrementCredsRefCount(creds); };
    int r = connection_->querySenderCredentials(static_cast<sd_bus_message*>(msg_), mask, &creds);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus creds", -r);

    auto euid = static_cast<uid_t>(-1);
    r = sd_bus_creds_get_euid(creds, &euid);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus cred euid", -r);
    return euid;
}

gid_t Message::getCredsGid() const
{
    const uint64_t mask = SD_BUS_CREDS_GID | SD_BUS_CREDS_AUGMENT;
    sd_bus_creds *creds = nullptr;
    SCOPE_EXIT{ connection_->decrementCredsRefCount(creds); };
    int r = connection_->querySenderCredentials(static_cast<sd_bus_message*>(msg_), mask, &creds);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus creds", -r);

    auto gid = static_cast<gid_t>(-1);
    r = sd_bus_creds_get_gid(creds, &gid);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus cred gid", -r);
    return gid;
}

gid_t Message::getCredsEgid() const
{
    const uint64_t mask = SD_BUS_CREDS_EGID | SD_BUS_CREDS_AUGMENT;
    sd_bus_creds *creds = nullptr;
    SCOPE_EXIT{ connection_->decrementCredsRefCount(creds); };
    int r = connection_->querySenderCredentials(static_cast<sd_bus_message*>(msg_), mask, &creds);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus creds", -r);

    auto egid = static_cast<gid_t>(-1);
    r = sd_bus_creds_get_egid(creds, &egid);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus cred egid", -r);
    return egid;
}

std::vector<gid_t> Message::getCredsSupplementaryGids() const
{
    const uint64_t mask = SD_BUS_CREDS_SUPPLEMENTARY_GIDS | SD_BUS_CREDS_AUGMENT;
    sd_bus_creds *creds = nullptr;
    SCOPE_EXIT{ connection_->decrementCredsRefCount(creds); };
    int r = connection_->querySenderCredentials(static_cast<sd_bus_message*>(msg_), mask, &creds);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus creds", -r);

    const gid_t *cGids = nullptr;
    r = sd_bus_creds_get_supplementary_gids(creds, &cGids);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus cred supplementary gids", -r);

    std::vector<gid_t> gids{};
    if (cGids != nullptr)
    {
        for (int i = 0; i < r; i++)
            gids.push_back(cGids[i]); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    return gids;
}

std::string Message::getSELinuxContext() const
{
    const uint64_t mask = SD_BUS_CREDS_AUGMENT | SD_BUS_CREDS_SELINUX_CONTEXT;
    sd_bus_creds *creds = nullptr;
    SCOPE_EXIT{ connection_->decrementCredsRefCount(creds); };
    int r = connection_->querySenderCredentials(static_cast<sd_bus_message*>(msg_), mask, &creds);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus creds", -r);

    const char *cLabel = nullptr;
    r = sd_bus_creds_get_selinux_context(creds, &cLabel);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus cred selinux context", -r);
    return cLabel;
}


MethodCall::MethodCall( void *msg
                      , internal::IConnection *connection
                      , adopt_message_t) noexcept
   : Message(msg, connection, adopt_message)
{
}

void MethodCall::dontExpectReply()
{
    auto r = sd_bus_message_set_expect_reply(static_cast<sd_bus_message*>(msg_), 0);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to set the dont-expect-reply flag", -r);
}

bool MethodCall::doesntExpectReply() const
{
    auto r = sd_bus_message_get_expect_reply(static_cast<sd_bus_message*>(msg_));
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get the dont-expect-reply flag", -r);
    return r == 0;
}

MethodReply MethodCall::send(uint64_t timeout) const
{
    if (!doesntExpectReply())
        return sendWithReply(timeout);

    return sendWithNoReply();
}

MethodReply MethodCall::sendWithReply(uint64_t timeout) const
{
    auto* sdbusReply = connection_->callMethod(static_cast<sd_bus_message*>(msg_), timeout);

    return Factory::create<MethodReply>(sdbusReply, connection_, adopt_message);
}

MethodReply MethodCall::sendWithNoReply() const
{
    connection_->sendMessage(static_cast<sd_bus_message*>(msg_));

    return Factory::create<MethodReply>(); // No reply
}

Slot MethodCall::send(void* callback, void* userData, uint64_t timeout, return_slot_t) const // NOLINT(bugprone-easily-swappable-parameters)
{
    return connection_->callMethodAsync(static_cast<sd_bus_message*>(msg_), reinterpret_cast<sd_bus_message_handler_t>(callback), userData, timeout, return_slot);
}

MethodReply MethodCall::createReply() const
{
    auto* sdbusReply = connection_->createMethodReply(static_cast<sd_bus_message*>(msg_));

    return Factory::create<MethodReply>(sdbusReply, connection_, adopt_message);
}

MethodReply MethodCall::createErrorReply(const Error& error) const
{
    sd_bus_message* sdbusErrorReply = connection_->createErrorReplyMessage(static_cast<sd_bus_message*>(msg_), error);

    return Factory::create<MethodReply>(sdbusErrorReply, connection_, adopt_message);
}

void MethodReply::send() const
{
    connection_->sendMessage(static_cast<sd_bus_message*>(msg_));
}

uint64_t MethodReply::getReplyCookie() const
{
    uint64_t cookie{};
    auto r =  sd_bus_message_get_reply_cookie(static_cast<sd_bus_message*>(msg_), &cookie);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get cookie", -r);
    return cookie;
}

void Signal::send() const
{
    connection_->sendMessage(static_cast<sd_bus_message*>(msg_));
}

void Signal::setDestination(const std::string& destination)
{
    setDestination(destination.c_str());
}

void Signal::setDestination(const char* destination)
{
    auto r = sd_bus_message_set_destination(static_cast<sd_bus_message*>(msg_), destination);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to set signal destination", -r);
}

namespace {

// Pseudo-connection lifetime handling. In standard cases, we could do with simply function-local static pseudo
// connection instance below. However, it may happen that client's sdbus-c++ objects outlive this static connection
// instance (because they are used in global application objects that were created before this connection, and thus
// are destroyed later). This by itself sounds like a smell in client's application design, but it is downright bad
// in sdbus-c++ because it has no control over when client's dependent statics get destroyed. A "Phoenix" pattern
// (see Modern C++ Design - Generic Programming and Design Patterns Applied, by Andrei Alexandrescu) is applied to fix
// this by re-creating the connection again in such cases and keeping it alive until the next exit handler is invoked.
// Please note that the solution is NOT thread-safe.
// Another common solution is global sdbus-c++ startup/shutdown functions, but that would be an intrusive change.

#ifdef __cpp_constinit
constinit bool pseudoConnectionDestroyed{}; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
#else
bool pseudoConnectionDestroyed{}; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
#endif

std::unique_ptr<sdbus::internal::IConnection, void(*)(sdbus::internal::IConnection*)> createPseudoConnection()
{
    auto deleter = [](sdbus::internal::IConnection* con)
    {
        delete con; // NOLINT(cppcoreguidelines-owning-memory)
        pseudoConnectionDestroyed = true;
    };

    return {internal::createPseudoConnection().release(), std::move(deleter)};
}

sdbus::internal::IConnection& getPseudoConnectionInstance()
{
    static auto connection = createPseudoConnection();

    if (pseudoConnectionDestroyed)
    {
        connection = createPseudoConnection(); // Phoenix rising from the ashes
        std::ignore = atexit([](){ connection.~unique_ptr(); }); // We have to manually take care of deleting the phoenix
        pseudoConnectionDestroyed = false;
    }

    assert(connection != nullptr);

    return *connection;
}

} // namespace

PlainMessage createPlainMessage()
{
    // Let's create a pseudo connection -- one that does not really connect to the real bus.
    // This is a bit of a hack, but it enables use to work with D-Bus message locally without
    // the need of D-Bus daemon. This is especially useful in unit tests of both sdbus-c++ and client code.
    // Additionally, it's light-weight and fast solution.
    const auto& connection = getPseudoConnectionInstance();
    return connection.createPlainMessage();
}

} // namespace sdbus
