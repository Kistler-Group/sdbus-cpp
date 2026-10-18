/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2026 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 * (C) 2026 - Roman Khromenok <roma55592@yandex.ru>
 *
 * @file Track.h
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

#ifndef SDBUS_CXX_TRACK_H_
#define SDBUS_CXX_TRACK_H_

#include <cstddef>
#include <vector>

// Forward declarations
namespace sdbus {
    class BusName;
    class Message;
    namespace internal {
        class ISdBus;
        class Connection;
    } // namespace internal
} // namespace sdbus

namespace sdbus {

    /********************************************//**
     * @class Track
     *
     * Track represents a set of D-Bus peers (identified by their bus names)
     * that are tracked on a bus connection. Once all tracked peers disappear
     * from the bus, the handler provided upon track creation is invoked.
     * A typical use case is releasing resources held on behalf of a client
     * (e.g. a lock) when the client disconnects from the bus.
     *
     * The handler is invoked when the whole track becomes empty, not for each
     * individual name. To be notified of each peer separately, create a separate
     * track for each peer.
     *
     * Track objects are created via IConnection::createTrack(). Copies of a track
     * object refer to the same underlying sd-bus track. A track object must not
     * outlive the connection it was created on.
     *
     * For more information, consult man sd_bus_track_new and man sd_bus_track_add_name.
     *
     ***********************************************/
    class [[nodiscard]] Track
    {
    public:
        Track(const Track&) noexcept;
        Track& operator=(const Track&) noexcept;
        Track(Track&& other) noexcept;
        Track& operator=(Track&& other) noexcept;
        ~Track();

        /*!
         * @brief Adds a bus name to the track
         *
         * @param[in] name Unique or well-known bus name to track
         *
         * If the track is recursive, adding an already tracked name increases its reference counter.
         * Otherwise, adding an already tracked name has no effect.
         *
         * For more information, consult man sd_bus_track_add_name.
         *
         * @throws sdbus::Error in case of failure
         */
        void addName(const BusName& name);

        /*!
         * @brief Adds the sender of a D-Bus message to the track
         *
         * @param[in] message D-Bus message whose sender shall be tracked
         *
         * A typical use is tracking the caller of a D-Bus method, within a method callback handler.
         * The message must have been received on the same bus connection the track was created on.
         *
         * For more information, consult man sd_bus_track_add_sender.
         *
         * @throws sdbus::Error in case of failure
         */
        void addSender(const Message& message);

        /*!
         * @brief Removes a bus name from the track
         *
         * @param[in] name Bus name to remove
         *
         * If the track is recursive, the reference counter of the name is decreased,
         * and the name is removed once the counter drops to zero. Otherwise, the name
         * is removed regardless of how many times it was added.
         *
         * For more information, consult man sd_bus_track_remove_name.
         *
         * @throws sdbus::Error in case of failure
         */
        void removeName(const BusName& name);

        /*!
         * @brief Removes the sender of a D-Bus message from the track
         *
         * @param[in] message D-Bus message whose sender shall be removed
         *
         * For more information, consult man sd_bus_track_remove_sender.
         *
         * @throws sdbus::Error in case of failure
         */
        void removeSender(const Message& message);

        /*!
         * @brief Checks whether a bus name is tracked
         *
         * @param[in] name Bus name to look up
         * @return True if the name is tracked, false otherwise
         *
         * For more information, consult man sd_bus_track_contains.
         */
        [[nodiscard]] bool contains(const BusName& name) const;

        /*!
         * @brief Returns the number of tracked bus names
         *
         * Each name is counted once, regardless of its reference counter in a recursive track.
         *
         * For more information, consult man sd_bus_track_count.
         */
        [[nodiscard]] std::size_t count() const;

        /*!
         * @brief Returns all tracked bus names
         *
         * For more information, consult man sd_bus_track_first.
         *
         * @throws sdbus::Error in case of failure
         */
        [[nodiscard]] std::vector<BusName> getNames() const;

        /*!
         * @brief Enables or disables recursive mode of the track
         *
         * @param[in] recursive True to enable recursive mode
         *
         * In recursive mode, each name has a reference counter, and it is only removed
         * from the track once removed as many times as it was added. The mode can only
         * be changed while the track is empty.
         *
         * For more information, consult man sd_bus_track_set_recursive.
         *
         * @throws sdbus::Error in case of failure
         */
        void setRecursive(bool recursive);

        /*!
         * @brief Checks whether the track is in recursive mode
         *
         * For more information, consult man sd_bus_track_get_recursive.
         *
         * @throws sdbus::Error in case of failure
         */
        [[nodiscard]] bool isRecursive() const;

    private:
        friend internal::Connection;

        // Takes over the reference held by the caller
        Track(void *track, internal::ISdBus* sdbus) noexcept;

        void* track_{};
        internal::ISdBus* sdbus_{};
    };

} // namespace sdbus

#endif /* SDBUS_CXX_TRACK_H_ */
