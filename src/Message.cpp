/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2022 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
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

#include "IConnection.h"
#include "ISdBus.h"
#include "MessageUtils.h"
#include "ScopeGuard.h"

#include <cassert>
#include SDBUS_HEADER

namespace sdbus {

Message::Message(internal::ISdBus* sdbus) noexcept
    : sdbus_(sdbus)
{
    assert(sdbus_ != nullptr);
}

Message::Message(void *msg, internal::ISdBus* sdbus) noexcept
    : msg_(msg)
    , sdbus_(sdbus)
{
    assert(msg_ != nullptr);
    assert(sdbus_ != nullptr);
    sdbus_->sd_bus_message_ref((sd_bus_message*)msg_);
}

Message::Message(void *msg, internal::ISdBus* sdbus, adopt_message_t) noexcept
    : msg_(msg)
    , sdbus_(sdbus)
{
    assert(msg_ != nullptr);
    assert(sdbus_ != nullptr);
}

Message::Message(const Message& other) noexcept
{
    *this = other;
}

Message& Message::operator=(const Message& other) noexcept
{
    if (msg_)
        sdbus_->sd_bus_message_unref((sd_bus_message*)msg_);

    msg_ = other.msg_;
    sdbus_ = other.sdbus_;
    ok_ = other.ok_;

    sdbus_->sd_bus_message_ref((sd_bus_message*)msg_);

    return *this;
}

Message::Message(Message&& other) noexcept
{
    *this = std::move(other);
}

Message& Message::operator=(Message&& other) noexcept
{
    if (msg_)
        sdbus_->sd_bus_message_unref((sd_bus_message*)msg_);

    msg_ = other.msg_;
    other.msg_ = nullptr;
    sdbus_ = other.sdbus_;
    other.sdbus_ = nullptr;
    ok_ = other.ok_;
    other.ok_ = true;

    return *this;
}

Message::~Message()
{
    if (msg_)
        sdbus_->sd_bus_message_unref((sd_bus_message*)msg_);
}

Message& Message::operator<<(bool item)
{
    int intItem = item;

    auto r = sd_bus_message_append_basic((sd_bus_message*)msg_, SD_BUS_TYPE_BOOLEAN, &intItem);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize a bool value", -r);

    return *this;
}

Message& Message::operator<<(int16_t item)
{
    auto r = sd_bus_message_append_basic((sd_bus_message*)msg_, SD_BUS_TYPE_INT16, &item);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize a int16_t value", -r);

    return *this;
}

Message& Message::operator<<(int32_t item)
{
    auto r = sd_bus_message_append_basic((sd_bus_message*)msg_, SD_BUS_TYPE_INT32, &item);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize a int32_t value", -r);

    return *this;
}

Message& Message::operator<<(int64_t item)
{
    auto r = sd_bus_message_append_basic((sd_bus_message*)msg_, SD_BUS_TYPE_INT64, &item);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize a int64_t value", -r);

    return *this;
}

Message& Message::operator<<(uint8_t item)
{
    auto r = sd_bus_message_append_basic((sd_bus_message*)msg_, SD_BUS_TYPE_BYTE, &item);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize a byte value", -r);

    return *this;
}

Message& Message::operator<<(uint16_t item)
{
    auto r = sd_bus_message_append_basic((sd_bus_message*)msg_, SD_BUS_TYPE_UINT16, &item);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize a uint16_t value", -r);

    return *this;
}

Message& Message::operator<<(uint32_t item)
{
    auto r = sd_bus_message_append_basic((sd_bus_message*)msg_, SD_BUS_TYPE_UINT32, &item);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize a uint32_t value", -r);

    return *this;
}

Message& Message::operator<<(uint64_t item)
{
    auto r = sd_bus_message_append_basic((sd_bus_message*)msg_, SD_BUS_TYPE_UINT64, &item);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize a uint64_t value", -r);

    return *this;
}

Message& Message::operator<<(double item)
{
    auto r = sd_bus_message_append_basic((sd_bus_message*)msg_, SD_BUS_TYPE_DOUBLE, &item);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize a double value", -r);

    return *this;
}

Message& Message::operator<<(const char* item)
{
    auto r = sd_bus_message_append_basic((sd_bus_message*)msg_, SD_BUS_TYPE_STRING, item);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize a C-string value", -r);

    return *this;
}

Message& Message::operator<<(const std::string& item)
{
    auto r = sd_bus_message_append_basic((sd_bus_message*)msg_, SD_BUS_TYPE_STRING, item.c_str());
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize a string value", -r);

    return *this;
}

Message& Message::operator<<(const Variant &item)
{
    item.serializeTo(*this);

    return *this;
}

Message& Message::operator<<(const ObjectPath &item)
{
    auto r = sd_bus_message_append_basic((sd_bus_message*)msg_, SD_BUS_TYPE_OBJECT_PATH, item.c_str());
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize an ObjectPath value", -r);

    return *this;
}

Message& Message::operator<<(const Signature &item)
{
    auto r = sd_bus_message_append_basic((sd_bus_message*)msg_, SD_BUS_TYPE_SIGNATURE, item.c_str());
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize a Signature value", -r);

    return *this;
}

Message& Message::operator<<(const UnixFd &item)
{
    auto fd = item.get();
    auto r = sd_bus_message_append_basic((sd_bus_message*)msg_, SD_BUS_TYPE_UNIX_FD, &fd);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize a UnixFd value", -r);

    return *this;
}

Message& Message::appendArray(char type, const void *ptr, size_t size)
{
    auto r = sd_bus_message_append_array((sd_bus_message*)msg_, type, ptr, size);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to serialize an array", -r);

    return *this;
}

Message& Message::operator>>(bool& item)
{
    int intItem;
    auto r = sd_bus_message_read_basic((sd_bus_message*)msg_, SD_BUS_TYPE_BOOLEAN, &intItem);
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to deserialize a bool value", -r);

    item = static_cast<bool>(intItem);

    return *this;
}

Message& Message::operator>>(int16_t& item)
{
    auto r = sd_bus_message_read_basic((sd_bus_message*)msg_, SD_BUS_TYPE_INT16, &item);
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to deserialize a int16_t value", -r);

    return *this;
}

Message& Message::operator>>(int32_t& item)
{
    auto r = sd_bus_message_read_basic((sd_bus_message*)msg_, SD_BUS_TYPE_INT32, &item);
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to deserialize a int32_t value", -r);

    return *this;
}

Message& Message::operator>>(int64_t& item)
{
    auto r = sd_bus_message_read_basic((sd_bus_message*)msg_, SD_BUS_TYPE_INT64, &item);
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to deserialize a bool value", -r);

    return *this;
}

Message& Message::operator>>(uint8_t& item)
{
    auto r = sd_bus_message_read_basic((sd_bus_message*)msg_, SD_BUS_TYPE_BYTE, &item);
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to deserialize a byte value", -r);

    return *this;
}

Message& Message::operator>>(uint16_t& item)
{
    auto r = sd_bus_message_read_basic((sd_bus_message*)msg_, SD_BUS_TYPE_UINT16, &item);
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to deserialize a uint16_t value", -r);

    return *this;
}

Message& Message::operator>>(uint32_t& item)
{
    auto r = sd_bus_message_read_basic((sd_bus_message*)msg_, SD_BUS_TYPE_UINT32, &item);
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to deserialize a uint32_t value", -r);

    return *this;
}

Message& Message::operator>>(uint64_t& item)
{
    auto r = sd_bus_message_read_basic((sd_bus_message*)msg_, SD_BUS_TYPE_UINT64, &item);
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to deserialize a uint64_t value", -r);

    return *this;
}

Message& Message::readArray(char type, const void **ptr, size_t *size)
{
    auto r = sd_bus_message_read_array((sd_bus_message*)msg_, type, ptr, size);
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to deserialize an array", -r);

    return *this;
}

Message& Message::operator>>(double& item)
{
    auto r = sd_bus_message_read_basic((sd_bus_message*)msg_, SD_BUS_TYPE_DOUBLE, &item);
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to deserialize a double value", -r);

    return *this;
}

Message& Message::operator>>(char*& item)
{
    auto r = sd_bus_message_read_basic((sd_bus_message*)msg_, SD_BUS_TYPE_STRING, &item);
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
    auto r = sd_bus_message_read_basic((sd_bus_message*)msg_, SD_BUS_TYPE_OBJECT_PATH, &str);
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
    auto r = sd_bus_message_read_basic((sd_bus_message*)msg_, SD_BUS_TYPE_SIGNATURE, &str);
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
    auto r = sd_bus_message_read_basic((sd_bus_message*)msg_, SD_BUS_TYPE_UNIX_FD, &fd);
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to deserialize a UnixFd value", -r);

    item.reset(fd);

    return *this;
}


Message& Message::openContainer(const std::string& signature)
{
    auto r = sd_bus_message_open_container((sd_bus_message*)msg_, SD_BUS_TYPE_ARRAY, signature.c_str());
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to open a container", -r);

    return *this;
}

Message& Message::closeContainer()
{
    auto r = sd_bus_message_close_container((sd_bus_message*)msg_);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to close a container", -r);

    return *this;
}

Message& Message::openDictEntry(const std::string& signature)
{
    auto r = sd_bus_message_open_container((sd_bus_message*)msg_, SD_BUS_TYPE_DICT_ENTRY, signature.c_str());
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to open a dictionary entry", -r);

    return *this;
}

Message& Message::closeDictEntry()
{
    auto r = sd_bus_message_close_container((sd_bus_message*)msg_);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to close a dictionary entry", -r);

    return *this;
}

Message& Message::openVariant(const std::string& signature)
{
    auto r = sd_bus_message_open_container((sd_bus_message*)msg_, SD_BUS_TYPE_VARIANT, signature.c_str());
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to open a variant", -r);

    return *this;
}

Message& Message::closeVariant()
{
    auto r = sd_bus_message_close_container((sd_bus_message*)msg_);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to close a variant", -r);

    return *this;
}

Message& Message::openStruct(const std::string& signature)
{
    auto r = sd_bus_message_open_container((sd_bus_message*)msg_, SD_BUS_TYPE_STRUCT, signature.c_str());
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to open a struct", -r);

    return *this;
}

Message& Message::closeStruct()
{
    auto r = sd_bus_message_close_container((sd_bus_message*)msg_);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to close a struct", -r);

    return *this;
}


Message& Message::enterContainer(const std::string& signature)
{
    auto r = sd_bus_message_enter_container((sd_bus_message*)msg_, SD_BUS_TYPE_ARRAY, signature.c_str());
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to enter a container", -r);

    return *this;
}

Message& Message::exitContainer()
{
    auto r = sd_bus_message_exit_container((sd_bus_message*)msg_);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to exit a container", -r);

    return *this;
}

Message& Message::enterDictEntry(const std::string& signature)
{
    auto r = sd_bus_message_enter_container((sd_bus_message*)msg_, SD_BUS_TYPE_DICT_ENTRY, signature.c_str());
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to enter a dictionary entry", -r);

    return *this;
}

Message& Message::exitDictEntry()
{
    auto r = sd_bus_message_exit_container((sd_bus_message*)msg_);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to exit a dictionary entry", -r);

    return *this;
}

Message& Message::enterVariant(const std::string& signature)
{
    auto r = sd_bus_message_enter_container((sd_bus_message*)msg_, SD_BUS_TYPE_VARIANT, signature.c_str());
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to enter a variant", -r);

    return *this;
}

Message& Message::exitVariant()
{
    auto r = sd_bus_message_exit_container((sd_bus_message*)msg_);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to exit a variant", -r);

    return *this;
}

Message& Message::enterStruct(const std::string& signature)
{
    auto r = sd_bus_message_enter_container((sd_bus_message*)msg_, SD_BUS_TYPE_STRUCT, signature.c_str());
    if (r == 0)
        ok_ = false;

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to enter a struct", -r);

    return *this;
}

Message& Message::exitStruct()
{
    auto r = sd_bus_message_exit_container((sd_bus_message*)msg_);
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
    auto r = sd_bus_message_copy((sd_bus_message*)destination.msg_, (sd_bus_message*)msg_, complete);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to copy the message", -r);
}

void Message::seal()
{
    const auto messageCookie = 1;
    const auto sealTimeout = 0;
    auto r = sd_bus_message_seal((sd_bus_message*)msg_, messageCookie, sealTimeout);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to seal the message", -r);
}

void Message::rewind(bool complete)
{
    auto r = sd_bus_message_rewind((sd_bus_message*)msg_, complete);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to rewind the message", -r);
}

std::string Message::getInterfaceName() const
{
    auto interface = sd_bus_message_get_interface((sd_bus_message*)msg_);
    return interface != nullptr ? interface : "";
}

std::string Message::getMemberName() const
{
    auto member = sd_bus_message_get_member((sd_bus_message*)msg_);
    return member != nullptr ? member : "";
}

std::string Message::getSender() const
{
    return sd_bus_message_get_sender((sd_bus_message*)msg_);
}

ObjectPath Message::getPath() const
{
    auto path = sd_bus_message_get_path((sd_bus_message*)msg_);
    return path != nullptr ? ObjectPath{path} : ObjectPath{};
}

std::string Message::getDestination() const
{
    auto destination = sd_bus_message_get_destination((sd_bus_message*)msg_);
    return destination != nullptr ? destination : "";
}

void Message::peekType(std::string& type, std::string& contents) const
{
    char typeSig;
    const char* contentsSig;
    auto r = sd_bus_message_peek_type((sd_bus_message*)msg_, &typeSig, &contentsSig);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to peek message type", -r);
    type = typeSig;
    contents = contentsSig ? contentsSig : "";
}

bool Message::isValid() const
{
    return msg_ != nullptr && sdbus_ != nullptr;
}

bool Message::isEmpty() const
{
    return sd_bus_message_is_empty((sd_bus_message*)msg_) != 0;
}

bool Message::isAtEnd(bool complete) const
{
    return sd_bus_message_at_end((sd_bus_message*)msg_, complete) > 0;
}

pid_t Message::getCredsPid() const
{
    uint64_t mask = SD_BUS_CREDS_PID | SD_BUS_CREDS_AUGMENT;
    sd_bus_creds *creds = nullptr;
    SCOPE_EXIT{ sdbus_->sd_bus_creds_unref(creds); };

    int r = sdbus_->sd_bus_query_sender_creds((sd_bus_message*)msg_, mask, &creds);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus creds", -r);

    pid_t pid = 0;
    r = sdbus_->sd_bus_creds_get_pid(creds, &pid);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus cred pid", -r);
    return pid;
}

uid_t Message::getCredsUid() const
{
    uint64_t mask = SD_BUS_CREDS_UID | SD_BUS_CREDS_AUGMENT;
    sd_bus_creds *creds = nullptr;
    SCOPE_EXIT{ sdbus_->sd_bus_creds_unref(creds); };
    int r = sdbus_->sd_bus_query_sender_creds((sd_bus_message*)msg_, mask, &creds);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus creds", -r);

    uid_t uid = (uid_t)-1;
    r = sdbus_->sd_bus_creds_get_uid(creds, &uid);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus cred uid", -r);
    return uid;
}

uid_t Message::getCredsEuid() const
{
    uint64_t mask = SD_BUS_CREDS_EUID | SD_BUS_CREDS_AUGMENT;
    sd_bus_creds *creds = nullptr;
    SCOPE_EXIT{ sdbus_->sd_bus_creds_unref(creds); };
    int r = sdbus_->sd_bus_query_sender_creds((sd_bus_message*)msg_, mask, &creds);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus creds", -r);

    uid_t euid = (uid_t)-1;
    r = sdbus_->sd_bus_creds_get_euid(creds, &euid);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus cred euid", -r);
    return euid;
}

gid_t Message::getCredsGid() const
{
    uint64_t mask = SD_BUS_CREDS_GID | SD_BUS_CREDS_AUGMENT;
    sd_bus_creds *creds = nullptr;
    SCOPE_EXIT{ sdbus_->sd_bus_creds_unref(creds); };
    int r = sdbus_->sd_bus_query_sender_creds((sd_bus_message*)msg_, mask, &creds);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus creds", -r);

    gid_t gid = (gid_t)-1;
    r = sdbus_->sd_bus_creds_get_gid(creds, &gid);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus cred gid", -r);
    return gid;
}

gid_t Message::getCredsEgid() const
{
    uint64_t mask = SD_BUS_CREDS_EGID | SD_BUS_CREDS_AUGMENT;
    sd_bus_creds *creds = nullptr;
    SCOPE_EXIT{ sdbus_->sd_bus_creds_unref(creds); };
    int r = sdbus_->sd_bus_query_sender_creds((sd_bus_message*)msg_, mask, &creds);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus creds", -r);

    gid_t egid = (gid_t)-1;
    r = sdbus_->sd_bus_creds_get_egid(creds, &egid);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus cred egid", -r);
    return egid;
}

std::vector<gid_t> Message::getCredsSupplementaryGids() const
{
    uint64_t mask = SD_BUS_CREDS_SUPPLEMENTARY_GIDS | SD_BUS_CREDS_AUGMENT;
    sd_bus_creds *creds = nullptr;
    SCOPE_EXIT{ sdbus_->sd_bus_creds_unref(creds); };
    int r = sdbus_->sd_bus_query_sender_creds((sd_bus_message*)msg_, mask, &creds);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus creds", -r);

    const gid_t *cGids = nullptr;
    r = sdbus_->sd_bus_creds_get_supplementary_gids(creds, &cGids);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus cred supplementary gids", -r);

    std::vector<gid_t> gids{};
    if (cGids != nullptr)
    {
        for (int i = 0; i < r; i++)
            gids.push_back(cGids[i]);
    }

    return gids;
}

std::string Message::getSELinuxContext() const
{
    uint64_t mask = SD_BUS_CREDS_AUGMENT | SD_BUS_CREDS_SELINUX_CONTEXT;
    sd_bus_creds *creds = nullptr;
    SCOPE_EXIT{ sdbus_->sd_bus_creds_unref(creds); };
    int r = sdbus_->sd_bus_query_sender_creds((sd_bus_message*)msg_, mask, &creds);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus creds", -r);

    const char *cLabel = nullptr;
    r = sdbus_->sd_bus_creds_get_selinux_context(creds, &cLabel);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get bus cred selinux context", -r);
    return cLabel;
}


MethodCall::MethodCall( void *msg
                      , internal::ISdBus *sdbus
                      , adopt_message_t) noexcept
   : Message(msg, sdbus, adopt_message)
{
}

void MethodCall::dontExpectReply()
{
    auto r = sd_bus_message_set_expect_reply((sd_bus_message*)msg_, 0);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to set the dont-expect-reply flag", -r);
}

bool MethodCall::doesntExpectReply() const
{
    auto r = sd_bus_message_get_expect_reply((sd_bus_message*)msg_);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get the dont-expect-reply flag", -r);
    return r == 0;
}

MethodReply MethodCall::send(uint64_t timeout) const
{
    if (!doesntExpectReply())
        return sendWithReply(timeout);
    else
        return sendWithNoReply();
}

MethodReply MethodCall::sendWithReply(uint64_t timeout) const
{
    sd_bus_error sdbusError = SD_BUS_ERROR_NULL;
    SCOPE_EXIT{ sd_bus_error_free(&sdbusError); };

    sd_bus_message* sdbusReply{};
    auto r = sdbus_->sd_bus_call(nullptr, (sd_bus_message*)msg_, timeout, &sdbusError, &sdbusReply);

    if (sd_bus_error_is_set(&sdbusError))
        throw sdbus::Error(sdbusError.name, sdbusError.message);

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to call method", -r);

    return Factory::create<MethodReply>(sdbusReply, sdbus_, adopt_message);
}

MethodReply MethodCall::sendWithNoReply() const
{
    auto r = sdbus_->sd_bus_send(nullptr, (sd_bus_message*)msg_, nullptr);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to call method with no reply", -r);

    return Factory::create<MethodReply>(); // No reply
}

void MethodCall::send(void* callback, void* userData, uint64_t timeout, floating_slot_t) const
{
    auto r = sdbus_->sd_bus_call_async(nullptr, nullptr, (sd_bus_message*)msg_, (sd_bus_message_handler_t)callback, userData, timeout);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to call method", -r);
}

Slot MethodCall::send(void* callback, void* userData, uint64_t timeout) const
{
    sd_bus_slot* slot;

    auto r = sdbus_->sd_bus_call_async(nullptr, &slot, (sd_bus_message*)msg_, (sd_bus_message_handler_t)callback, userData, timeout);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to call method asynchronously", -r);

    return {slot, [sdbus_ = sdbus_](void *slot){ sdbus_->sd_bus_slot_unref((sd_bus_slot*)slot); }};
}

MethodReply MethodCall::createReply() const
{
    sd_bus_message* sdbusReply{};
    auto r = sdbus_->sd_bus_message_new_method_return((sd_bus_message*)msg_, &sdbusReply);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to create method reply", -r);

    return Factory::create<MethodReply>(sdbusReply, sdbus_, adopt_message);
}

MethodReply MethodCall::createErrorReply(const Error& error) const
{
    sd_bus_error sdbusError = SD_BUS_ERROR_NULL;
    SCOPE_EXIT{ sd_bus_error_free(&sdbusError); };
    sd_bus_error_set(&sdbusError, error.getName().c_str(), error.getMessage().c_str());

    sd_bus_message* sdbusErrorReply{};
    auto r = sdbus_->sd_bus_message_new_method_error((sd_bus_message*)msg_, &sdbusErrorReply, &sdbusError);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to create method error reply", -r);

    return Factory::create<MethodReply>(sdbusErrorReply, sdbus_, adopt_message);
}

void MethodReply::send() const
{
    auto r = sdbus_->sd_bus_send(nullptr, (sd_bus_message*)msg_, nullptr);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to send reply", -r);
}

void Signal::send() const
{
    auto r = sdbus_->sd_bus_send(nullptr, (sd_bus_message*)msg_, nullptr);
    SDBUS_THROW_ERROR_IF(r < 0, "Failed to emit signal", -r);
}

void Signal::setDestination(const std::string& destination)
{
    auto r = sdbus_->sd_bus_message_set_destination((sd_bus_message*)msg_, destination.c_str());
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
constinit static bool pseudoConnectionDestroyed{};
#else
static bool pseudoConnectionDestroyed{};
#endif

std::unique_ptr<sdbus::internal::IConnection, void(*)(sdbus::internal::IConnection*)> createPseudoConnection()
{
    auto deleter = [](sdbus::internal::IConnection* con)
    {
        delete con;
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
        atexit([](){ connection.~unique_ptr(); }); // We have to manually take care of deleting the phoenix
        pseudoConnectionDestroyed = false;
    }

    assert(connection != nullptr);

    return *connection;
}

}

PlainMessage createPlainMessage()
{
    // Let's create a pseudo connection -- one that does not really connect to the real bus.
    // This is a bit of a hack, but it enables use to work with D-Bus message locally without
    // the need of D-Bus daemon. This is especially useful in unit tests of both sdbus-c++ and client code.
    // Additionally, it's light-weight and fast solution.
    auto& connection = getPseudoConnectionInstance();
    return connection.createPlainMessage();
}

}
