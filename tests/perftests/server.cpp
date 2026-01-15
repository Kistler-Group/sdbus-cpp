/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2026 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file server.cpp
 *
 * Created on: Jan 25, 2019
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

#include "perftests-adaptor.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <sdbus-c++/sdbus-c++.h>
#include <utility>
#include <string>
#include <chrono>
#include <algorithm>
#include <iostream>

using namespace std::chrono_literals;

std::string createRandomString(size_t length);

class PerftestAdaptor final : public sdbus::AdaptorInterfaces<org::sdbuscpp::perftests_adaptor>
{
public:
    PerftestAdaptor(sdbus::IConnection& connection, sdbus::ObjectPath objectPath)
        : AdaptorInterfaces(connection, std::move(objectPath))
    {
        registerAdaptor();
    }

    PerftestAdaptor(const PerftestAdaptor&) = delete;
    PerftestAdaptor& operator=(const PerftestAdaptor&) = delete;
    PerftestAdaptor(PerftestAdaptor&&) = delete;
    PerftestAdaptor& operator=(PerftestAdaptor&&) = delete;

    ~PerftestAdaptor()
    {
        unregisterAdaptor();
    }

protected:
    void sendDataSignals(const uint32_t& numberOfSignals, const uint32_t& signalMsgSize) override // NOLINT(bugprone-easily-swappable-parameters)
    {
        auto data = createRandomString(signalMsgSize);

        auto start_time = std::chrono::steady_clock::now();
        for (uint32_t i = 0; i < numberOfSignals; ++i)
        {
            // Emit signal
            emitDataSignal(data);
        }
        auto stop_time = std::chrono::steady_clock::now();
        std::cout << "Server sent " << numberOfSignals << " signals in: " << std::chrono::duration_cast<std::chrono::milliseconds>(stop_time - start_time).count() << " ms" << '\n';
    }

    std::string concatenateTwoStrings(const std::string& string1, const std::string& string2) override
    {
        return string1 + string2;
    }
};

std::string createRandomString(size_t length)
{
    auto randchar = []() -> char
    {
        const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[ random() % max_index ];
    };

    struct timespec ts{};
    (void)timespec_get(&ts, TIME_UTC);
    srandom(ts.tv_nsec ^ ts.tv_sec);  /* Seed the PRNG */

    std::string str(length, 0);
    std::generate_n(str.begin(), length, randchar);
    return str;
}


//-----------------------------------------
int main(int /*argc*/, char */*argv*/[])
{
    sdbus::ServiceName const serviceName{"org.sdbuscpp.perftests"};
    auto connection = sdbus::createSystemBusConnection(serviceName);

    sdbus::ObjectPath objectPath{"/org/sdbuscpp/perftests"};
    PerftestAdaptor server(*connection, std::move(objectPath)); // NOLINT(misc-const-correctness)

    connection->enterEventLoop();
}
