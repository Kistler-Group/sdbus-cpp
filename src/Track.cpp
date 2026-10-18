/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2026 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 * (C) 2026 - Roman Khromenok <roma55592@yandex.ru>
 *
 * @file Track.cpp
 *
 * Created on: Oct 18, 2026
 * Project: sdbus-c++
 * Description: High-level D-Bus IPC C++ library based on sd-bus
 *
 * This file is part of sdbus-c++.
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

#include "sdbus-c++/Track.h"

#include "sdbus-c++/Error.h"
#include "sdbus-c++/Message.h"
#include "sdbus-c++/Types.h"

#include "ISdBus.h"

#include <cassert>
#include <cstddef>
#include <string>
#include SDBUS_HEADER
#include <utility>
#include <vector>

namespace sdbus {

Track::Track(void *track, internal::ISdBus* sdbus) noexcept
    : track_(track)
    , sdbus_(sdbus)
{
    assert(track_ != nullptr);
    assert(sdbus_ != nullptr);
}

Track::Track(const Track& other) noexcept
{
    *this = other;
}

Track& Track::operator=(const Track& other) noexcept
{
    if (this == &other)
        return *this;

    if (track_)
        sdbus_->sd_bus_track_unref(static_cast<sd_bus_track*>(track_));

    track_ = other.track_;
    sdbus_ = other.sdbus_;

    if (track_)
        sdbus_->sd_bus_track_ref(static_cast<sd_bus_track*>(track_));

    return *this;
}

Track::Track(Track&& other) noexcept
{
    *this = std::move(other);
}

Track& Track::operator=(Track&& other) noexcept
{
    if (this == &other)
        return *this;

    if (track_)
        sdbus_->sd_bus_track_unref(static_cast<sd_bus_track*>(track_));

    track_ = std::exchange(other.track_, nullptr);
    sdbus_ = std::exchange(other.sdbus_, nullptr);

    return *this;
}

Track::~Track()
{
    if (track_)
        sdbus_->sd_bus_track_unref(static_cast<sd_bus_track*>(track_));
}

void Track::addName(const BusName& name)
{
    auto r = sdbus_->sd_bus_track_add_name(static_cast<sd_bus_track*>(track_), name.c_str());

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to add name to track", -r);
}

void Track::addSender(const Message& message)
{
    auto r = sdbus_->sd_bus_track_add_sender(static_cast<sd_bus_track*>(track_), static_cast<sd_bus_message*>(message.msg_));

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to add message sender to track", -r);
}

void Track::removeName(const BusName& name)
{
    auto r = sdbus_->sd_bus_track_remove_name(static_cast<sd_bus_track*>(track_), name.c_str());

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to remove name from track", -r);
}

void Track::removeSender(const Message& message)
{
    auto r = sdbus_->sd_bus_track_remove_sender(static_cast<sd_bus_track*>(track_), static_cast<sd_bus_message*>(message.msg_));

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to remove message sender from track", -r);
}

bool Track::contains(const BusName& name) const
{
    return sdbus_->sd_bus_track_contains(static_cast<sd_bus_track*>(track_), name.c_str()) != nullptr;
}

std::size_t Track::count() const
{
    return sdbus_->sd_bus_track_count(static_cast<sd_bus_track*>(track_));
}

std::vector<BusName> Track::getNames() const
{
    std::vector<std::string> names;

    auto r = sdbus_->sd_bus_track_get_names(static_cast<sd_bus_track*>(track_), names);

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get track names", -r);

    std::vector<BusName> result;
    result.reserve(names.size());
    for (auto& name : names)
        result.emplace_back(std::move(name));

    return result;
}

void Track::setRecursive(bool recursive)
{
    auto r = sdbus_->sd_bus_track_set_recursive(static_cast<sd_bus_track*>(track_), recursive ? 1 : 0);

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to set track recursive mode", -r);
}

bool Track::isRecursive() const
{
    auto r = sdbus_->sd_bus_track_get_recursive(static_cast<sd_bus_track*>(track_));

    SDBUS_THROW_ERROR_IF(r < 0, "Failed to get track recursive mode", -r);

    return r > 0;
}

} // namespace sdbus
