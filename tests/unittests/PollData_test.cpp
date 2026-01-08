/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2024 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file PollData_test.cpp
 *
 * Created on: Jan 19, 2023
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

#include <sdbus-c++/IConnection.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <chrono>

using ::testing::Eq;
using ::testing::Ge;
using ::testing::Le;
using ::testing::AllOf;
using namespace std::string_literals;
using namespace std::chrono_literals;

/*-------------------------------------*/
/* --          TEST CASES           -- */
/*-------------------------------------*/

TEST(PollData, ReturnsZeroRelativeTimeoutForZeroAbsoluteTimeout)
{
    sdbus::IConnection::PollData pollData{};
    pollData.timeout = std::chrono::microseconds::zero();

    auto relativeTimeout = pollData.getRelativeTimeout();

    EXPECT_THAT(relativeTimeout, Eq(std::chrono::microseconds::zero()));
}

TEST(PollData, ReturnsZeroPollTimeoutForZeroAbsoluteTimeout)
{
    sdbus::IConnection::PollData pollData{};
    pollData.timeout = std::chrono::microseconds::zero();

    auto pollTimeout = pollData.getPollTimeout();

    EXPECT_THAT(pollTimeout, Eq(0));
}

TEST(PollData, ReturnsInfiniteRelativeTimeoutForInfiniteAbsoluteTimeout)
{
    sdbus::IConnection::PollData pollData{};
    pollData.timeout = std::chrono::microseconds::max();

    auto relativeTimeout = pollData.getRelativeTimeout();

    EXPECT_THAT(relativeTimeout, Eq(std::chrono::microseconds::max()));
}

TEST(PollData, ReturnsNegativePollTimeoutForInfiniteAbsoluteTimeout)
{
    sdbus::IConnection::PollData pollData{};
    pollData.timeout = std::chrono::microseconds::max();

    auto pollTimeout = pollData.getPollTimeout();

    EXPECT_THAT(pollTimeout, Eq(-1));
}

TEST(PollData, ReturnsZeroRelativeTimeoutForPastAbsoluteTimeout)
{
    sdbus::IConnection::PollData pollData{};
    auto past = std::chrono::steady_clock::now() - 10s;
    pollData.timeout = std::chrono::duration_cast<std::chrono::microseconds>(past.time_since_epoch());

    auto relativeTimeout = pollData.getRelativeTimeout();

    EXPECT_THAT(relativeTimeout, Eq(0us));
}

TEST(PollData, ReturnsZeroPollTimeoutForPastAbsoluteTimeout)
{
    sdbus::IConnection::PollData pollData{};
    auto past = std::chrono::steady_clock::now() - 10s;
    pollData.timeout = std::chrono::duration_cast<std::chrono::microseconds>(past.time_since_epoch());

    auto pollTimeout = pollData.getPollTimeout();

    EXPECT_THAT(pollTimeout, Eq(0));
}

TEST(PollData, ReturnsCorrectRelativeTimeoutForFutureAbsoluteTimeout)
{
    sdbus::IConnection::PollData pollData{};
    auto future = std::chrono::steady_clock::now() + 1s;
    pollData.timeout = std::chrono::duration_cast<std::chrono::microseconds>(future.time_since_epoch());

    auto relativeTimeout = pollData.getRelativeTimeout();

    EXPECT_THAT(relativeTimeout, AllOf(Ge(900ms), Le(1100ms)));
}

TEST(PollData, ReturnsCorrectPollTimeoutForFutureAbsoluteTimeout)
{
    sdbus::IConnection::PollData pollData{};
    auto future = std::chrono::steady_clock::now() + 1s;
    pollData.timeout = std::chrono::duration_cast<std::chrono::microseconds>(future.time_since_epoch());

    auto pollTimeout = pollData.getPollTimeout();

    EXPECT_THAT(pollTimeout, AllOf(Ge(900), Le(1100)));
}
