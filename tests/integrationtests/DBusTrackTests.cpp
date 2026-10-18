/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2026 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 * (C) 2026 - Roman Khromenok <roma55592@yandex.ru>
 *
 * @file DBusTrackTests.cpp
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

#include "TestFixture.h"
#include "Defs.h"
#include <sdbus-c++/sdbus-c++.h>

#include <atomic>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <optional>

using ::testing::ElementsAre;
using ::testing::Eq;
using namespace sdbus::test;

namespace {

const sdbus::ObjectPath TRACK_OBJECT_PATH{"/org/sdbuscpp/integrationtests/track"};

// A service whose `lock` method starts tracking the caller
class LockService
{
public:
    LockService()
    {
        object_->addVTable(sdbus::registerMethod("lock").implementedAs([this]()
        {
            track_.emplace(connection_->createTrack([this](sdbus::Track&){ released_ = true; }));
            track_->addSender(object_->getCurrentlyProcessedMessage());
        })).forInterface(INTERFACE_NAME);
        connection_->enterEventLoopAsync();
    }

    LockService(const LockService&) = delete;
    LockService& operator=(const LockService&) = delete;
    LockService(LockService&&) = delete;
    LockService& operator=(LockService&&) = delete;

    ~LockService()
    {
        // Stop the event loop first, so that the track handler cannot run on members being destroyed
        connection_->leaveEventLoop();
    }

    std::unique_ptr<sdbus::IConnection> connection_{sdbus::createBusConnection()};
    std::unique_ptr<sdbus::IObject> object_{sdbus::createObject(*connection_, TRACK_OBJECT_PATH)};
    std::optional<sdbus::Track> track_;
    std::atomic<bool> released_{false};
};

void callLock(sdbus::IConnection& client, const sdbus::ServiceName& destination)
{
    auto proxy = sdbus::createProxy(client, destination, TRACK_OBJECT_PATH);
    proxy->callMethod("lock").onInterface(INTERFACE_NAME);
}

} // namespace

/*-------------------------------------*/
/* --          TEST CASES           -- */
/*-------------------------------------*/

TEST(ATrack, ContainsSenderOfTrackedMethodCall)
{
    LockService service;
    auto client = sdbus::createBusConnection();

    callLock(*client, service.connection_->getUniqueName());

    ASSERT_TRUE(service.track_.has_value());
    const auto& track = service.track_.value(); // NOLINT(bugprone-unchecked-optional-access): checked above
    EXPECT_TRUE(track.contains(client->getUniqueName()));
    EXPECT_THAT(track.count(), Eq(1));
    EXPECT_THAT(track.getNames(), ElementsAre(client->getUniqueName()));
    EXPECT_FALSE(service.released_);
}

TEST(ATrack, InvokesHandlerWhenTrackedClientDisconnects)
{
    LockService service;
    auto client = sdbus::createBusConnection();
    callLock(*client, service.connection_->getUniqueName());
    ASSERT_FALSE(service.released_);

    client.reset();

    ASSERT_TRUE(waitUntil(service.released_));
}

TEST(ATrack, KeepsNameInRecursiveModeUntilRemovedAsManyTimesAsAdded)
{
    auto connection = sdbus::createBusConnection();
    auto peer = sdbus::createBusConnection();
    const auto name = peer->getUniqueName();
    auto track = connection->createTrack({});
    track.setRecursive(true);

    track.addName(name);
    track.addName(name);
    track.removeName(name);

    EXPECT_TRUE(track.isRecursive());
    EXPECT_TRUE(track.contains(name));

    track.removeName(name);

    EXPECT_FALSE(track.contains(name));
    EXPECT_THAT(track.count(), Eq(0));
}

TEST(ATrack, ThrowsWhenAddingNameThatIsNotOnTheBus)
{
    auto connection = sdbus::createBusConnection();
    auto track = connection->createTrack({});

    ASSERT_THROW(track.addName(sdbus::BusName{"org.sdbuscpp.nonexistent.name"}), sdbus::Error);
    EXPECT_THAT(track.count(), Eq(0));
}

TEST(ATrack, SharesUnderlyingTrackAmongCopies)
{
    auto connection = sdbus::createBusConnection();
    auto peer = sdbus::createBusConnection();
    auto track = connection->createTrack({});
    auto copy = track;

    track.addName(peer->getUniqueName());

    EXPECT_TRUE(copy.contains(peer->getUniqueName()));
}
