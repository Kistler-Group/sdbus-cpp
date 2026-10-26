/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2026 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 * (C) 2026 - Roman Khromenok <roma55592@yandex.ru>
 *
 * @file Track_test.cpp
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

#include "Connection.h"
#include "sdbus-c++/Error.h"
#include "sdbus-c++/Message.h"
#include "sdbus-c++/Track.h"
#include "sdbus-c++/Types.h"
#include "mocks/SdBusMock.h"

#include <gtest/gtest.h> // IWYU pragma: export
#include <cerrno>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// NOLINTBEGIN(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)

using ::testing::_;
using ::testing::DoAll;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::IsNull;
using ::testing::NiceMock;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::SetArgPointee;
using ::testing::SetArgReferee;
using ::testing::StrEq;
using ::sdbus::internal::Connection;

namespace
{
class ATrack : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ON_CALL(*sdBusIntfMock_, sd_bus_new(_)).WillByDefault(DoAll(SetArgPointee<0>(fakeBusPtr_), Return(1)));
        ON_CALL(*sdBusIntfMock_, sd_bus_start(_)).WillByDefault(Return(-EINVAL));
        ON_CALL(*sdBusIntfMock_, sd_bus_track_new(_, _, _, _))
            .WillByDefault(DoAll(SetArgPointee<1>(fakeTrackPtr_), SaveArg<2>(&trackHandler_), SaveArg<3>(&trackUserData_), Return(0)));
        ON_CALL(*sdBusIntfMock_, sd_bus_track_set_destroy_callback(_, _)).WillByDefault(DoAll(SaveArg<1>(&destroyCallback_), Return(0)));
        con_ = std::make_unique<Connection>(std::unique_ptr<NiceMock<SdBusMock>>(sdBusIntfMock_), Connection::pseudo_bus);
    }

    void TearDown() override
    {
        // Simulates the release of the last reference to the sd-bus track, as sd-bus would do
        if (destroyCallback_ != nullptr)
            destroyCallback_(trackUserData_);
    }

    NiceMock<SdBusMock>* sdBusIntfMock_ = new NiceMock<SdBusMock>(); // con_ below will assume ownership
    sd_bus* fakeBusPtr_ = reinterpret_cast<sd_bus*>(1); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
    sd_bus_track* fakeTrackPtr_ = reinterpret_cast<sd_bus_track*>(2); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
    sd_bus_track_handler_t trackHandler_{};
    void* trackUserData_{};
    sd_bus_destroy_t destroyCallback_{};
    std::unique_ptr<Connection> con_;
};
} // namespace

TEST_F(ATrack, IsCreatedOnTheConnectionBus)
{
    EXPECT_CALL(*sdBusIntfMock_, sd_bus_track_new(fakeBusPtr_, NotNull(), NotNull(), NotNull()));

    auto track = con_->createTrack([](const sdbus::Track&){});
}

TEST_F(ATrack, IsCreatedWithoutSdBusHandlerWhenNoHandlerIsProvided)
{
    EXPECT_CALL(*sdBusIntfMock_, sd_bus_track_new(fakeBusPtr_, NotNull(), IsNull(), IsNull()));
    EXPECT_CALL(*sdBusIntfMock_, sd_bus_track_set_destroy_callback(_, _)).Times(0);

    auto track = con_->createTrack({});
}

TEST_F(ATrack, ThrowsWhenCreationFails)
{
    EXPECT_CALL(*sdBusIntfMock_, sd_bus_track_new(_, _, _, _)).WillOnce(Return(-ENOMEM));

    ASSERT_THROW((void)con_->createTrack([](const sdbus::Track&){}), sdbus::Error);
}

TEST_F(ATrack, ThrowsAndReleasesTrackWhenSettingDestroyCallbackFails)
{
    EXPECT_CALL(*sdBusIntfMock_, sd_bus_track_set_destroy_callback(_, _)).WillOnce(Return(-EINVAL));
    EXPECT_CALL(*sdBusIntfMock_, sd_bus_track_unref(fakeTrackPtr_)).Times(1);

    ASSERT_THROW((void)con_->createTrack([](const sdbus::Track&){}), sdbus::Error);
}

TEST_F(ATrack, UnrefsUnderlyingTrackWhenDestructed)
{
    EXPECT_CALL(*sdBusIntfMock_, sd_bus_track_unref(fakeTrackPtr_)).Times(1);

    auto track = con_->createTrack([](const sdbus::Track&){});
}

TEST_F(ATrack, RefsUnderlyingTrackWhenCopied)
{
    auto track = con_->createTrack([](const sdbus::Track&){});

    EXPECT_CALL(*sdBusIntfMock_, sd_bus_track_ref(fakeTrackPtr_)).Times(1);
    EXPECT_CALL(*sdBusIntfMock_, sd_bus_track_unref(fakeTrackPtr_)).Times(2);

    auto copy = track; // NOLINT(performance-unnecessary-copy-initialization)
}

TEST_F(ATrack, DoesNotRefUnderlyingTrackWhenMoved)
{
    auto track = con_->createTrack([](const sdbus::Track&){});

    EXPECT_CALL(*sdBusIntfMock_, sd_bus_track_ref(_)).Times(0);
    EXPECT_CALL(*sdBusIntfMock_, sd_bus_track_unref(fakeTrackPtr_)).Times(1);

    auto moved = std::move(track);
}

TEST_F(ATrack, ForwardsAddedNameToSdBus)
{
    auto track = con_->createTrack([](const sdbus::Track&){});

    EXPECT_CALL(*sdBusIntfMock_, sd_bus_track_add_name(fakeTrackPtr_, StrEq(":1.42"))).WillOnce(Return(1));

    track.addName(sdbus::BusName{":1.42"});
}

TEST_F(ATrack, ThrowsWhenAddingNameFails)
{
    auto track = con_->createTrack([](const sdbus::Track&){});

    EXPECT_CALL(*sdBusIntfMock_, sd_bus_track_add_name(_, _)).WillOnce(Return(-EINVAL));

    ASSERT_THROW(track.addName(sdbus::BusName{"invalid name"}), sdbus::Error);
}

TEST_F(ATrack, ForwardsRemovedNameToSdBus)
{
    auto track = con_->createTrack([](const sdbus::Track&){});

    EXPECT_CALL(*sdBusIntfMock_, sd_bus_track_remove_name(fakeTrackPtr_, StrEq(":1.42"))).WillOnce(Return(1));

    track.removeName(sdbus::BusName{":1.42"});
}

TEST_F(ATrack, ThrowsWhenRemovingNameFails)
{
    auto track = con_->createTrack([](const sdbus::Track&){});

    EXPECT_CALL(*sdBusIntfMock_, sd_bus_track_remove_name(_, _)).WillOnce(Return(-EINVAL));

    ASSERT_THROW(track.removeName(sdbus::BusName{":1.42"}), sdbus::Error);
}

TEST_F(ATrack, ForwardsMessageToSdBusWhenAddingAndRemovingSender)
{
    auto track = con_->createTrack([](const sdbus::Track&){});
    auto message = sdbus::createPlainMessage();

    EXPECT_CALL(*sdBusIntfMock_, sd_bus_track_add_sender(fakeTrackPtr_, NotNull())).WillOnce(Return(1));
    EXPECT_CALL(*sdBusIntfMock_, sd_bus_track_remove_sender(fakeTrackPtr_, NotNull())).WillOnce(Return(1));

    track.addSender(message);
    track.removeSender(message);
}

TEST_F(ATrack, ThrowsWhenAddingSenderFails)
{
    auto track = con_->createTrack([](const sdbus::Track&){});
    auto message = sdbus::createPlainMessage();

    EXPECT_CALL(*sdBusIntfMock_, sd_bus_track_add_sender(_, _)).WillOnce(Return(-EINVAL));

    ASSERT_THROW(track.addSender(message), sdbus::Error);
}

TEST_F(ATrack, ReportsWhetherNameIsContained)
{
    auto track = con_->createTrack([](const sdbus::Track&){});
    const std::string name{":1.42"};

    EXPECT_CALL(*sdBusIntfMock_, sd_bus_track_contains(fakeTrackPtr_, StrEq(":1.42"))).WillOnce(Return(name.c_str()));
    EXPECT_CALL(*sdBusIntfMock_, sd_bus_track_contains(fakeTrackPtr_, StrEq(":1.43"))).WillOnce(Return(nullptr));

    EXPECT_TRUE(track.contains(sdbus::BusName{":1.42"}));
    EXPECT_FALSE(track.contains(sdbus::BusName{":1.43"}));
}

TEST_F(ATrack, ReturnsNumberOfTrackedNames)
{
    auto track = con_->createTrack([](const sdbus::Track&){});

    EXPECT_CALL(*sdBusIntfMock_, sd_bus_track_count(fakeTrackPtr_)).WillOnce(Return(3));

    EXPECT_THAT(track.count(), Eq(3));
}

TEST_F(ATrack, ReturnsTrackedNames)
{
    auto track = con_->createTrack([](const sdbus::Track&){});

    EXPECT_CALL(*sdBusIntfMock_, sd_bus_track_get_names(fakeTrackPtr_, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::vector<std::string>{":1.42", "org.sdbuscpp.name"}), Return(0)));

    EXPECT_THAT(track.getNames(), ElementsAre(sdbus::BusName{":1.42"}, sdbus::BusName{"org.sdbuscpp.name"}));
}

TEST_F(ATrack, ForwardsRecursiveModeToSdBus)
{
    auto track = con_->createTrack([](const sdbus::Track&){});

    EXPECT_CALL(*sdBusIntfMock_, sd_bus_track_set_recursive(fakeTrackPtr_, 1)).WillOnce(Return(0));
    EXPECT_CALL(*sdBusIntfMock_, sd_bus_track_get_recursive(fakeTrackPtr_)).WillOnce(Return(1));

    track.setRecursive(true);
    EXPECT_TRUE(track.isRecursive());
}

TEST_F(ATrack, ThrowsWhenSettingRecursiveModeOnNonEmptyTrack)
{
    auto track = con_->createTrack([](const sdbus::Track&){});

    EXPECT_CALL(*sdBusIntfMock_, sd_bus_track_set_recursive(_, _)).WillOnce(Return(-EBUSY));

    ASSERT_THROW(track.setRecursive(true), sdbus::Error);
}

TEST_F(ATrack, InvokesHandlerWithTheSameTrackWhenSdBusDispatchesIt)
{
    bool handlerCalled{};
    auto track = con_->createTrack([&](const sdbus::Track& dispatchedTrack)
    {
        handlerCalled = true;
        EXPECT_THAT(dispatchedTrack.count(), Eq(0));
    });
    ASSERT_THAT(trackHandler_, NotNull());

    EXPECT_CALL(*sdBusIntfMock_, sd_bus_track_ref(fakeTrackPtr_)).WillOnce(Return(fakeTrackPtr_));
    EXPECT_CALL(*sdBusIntfMock_, sd_bus_track_count(fakeTrackPtr_)).WillOnce(Return(0));

    auto r = trackHandler_(fakeTrackPtr_, trackUserData_);

    EXPECT_TRUE(handlerCalled);
    EXPECT_THAT(r, Eq(1)) << "A positive return value is needed, otherwise sd-bus re-queues the empty track";
}

TEST_F(ATrack, ReturnsErrorToSdBusWhenHandlerThrows)
{
    auto track = con_->createTrack([](const sdbus::Track&){ throw std::runtime_error("handler failure"); });
    ASSERT_THAT(trackHandler_, NotNull());

    ON_CALL(*sdBusIntfMock_, sd_bus_track_ref(fakeTrackPtr_)).WillByDefault(Return(fakeTrackPtr_));

    EXPECT_THAT(trackHandler_(fakeTrackPtr_, trackUserData_), Eq(-1));
}

// NOLINTEND(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
