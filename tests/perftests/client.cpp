/**
 * (C) 2019 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 *
 * @file client.cpp
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

#include "perftests-proxy.h"
#include <sdbus-c++/sdbus-c++.h>
#include <vector>
#include <string>
#include <iostream>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <cassert>
#include <algorithm>
#include <iostream>
//#include "SdBus.h"

using namespace std::chrono_literals;

//class MySdBus : public sdbus::internal::SdBus
//{
//public:
//    virtual int sd_bus_call_async(sd_bus *bus, sd_bus_slot **slot, sd_bus_message *m, sd_bus_message_handler_t callback, void *userdata, uint64_t usec) override
//    {
//        sd_bus_message* sdbusReply{};
//        auto r = this->sd_bus_message_new_method_return(m, &sdbusReply);
//        SDBUS_THROW_ERROR_IF(r < 0, "Failed to create method reply", -r);

//        callback(sdbusReply, userdata, nullptr);

//        return 1;
//    }
//};

namespace sdbus
{
    PlainMessage createPlainMessage();
}

uint64_t totalDuration = 0;

class PerftestProxy : public sdbus::ProxyInterfaces<org::sdbuscpp::perftests_proxy>
{
public:
    PerftestProxy(std::string destination, std::string objectPath)
        : ProxyInterfaces(std::move(destination), std::move(objectPath))
    {
        registerProxy();
    }

    ~PerftestProxy()
    {
        unregisterProxy();
    }

protected:
    virtual void onDataSignal(const std::string& data) override
    {
        static unsigned int counter = 0;
        static std::chrono::time_point<std::chrono::steady_clock> startTime;

        assert(data.size() == m_msgSize);

        ++counter;

        if (counter == 1)
            startTime = std::chrono::steady_clock::now();
        else if (counter == m_msgCount)
        {
            auto stopTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTime).count();
            totalDuration += duration;
            std::cout << "Received " << m_msgCount << " signals in: " << duration << " ms" << std::endl;
            counter = 0;
        }
    }

public:
    unsigned int m_msgSize{};
    unsigned int m_msgCount{};
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
        return charset[ rand() % max_index ];
    };
    std::string str(length, 0);
    std::generate_n(str.begin(), length, randchar);
    return str;
}


//-----------------------------------------
int main(int /*argc*/, char */*argv*/[])
{
    const char* destinationName = "org.sdbuscpp.perftests";
    const char* objectPath = "/org/sdbuscpp/perftests";
    //PerftestProxy client(destinationName, objectPath);

    const unsigned int repetitions{20};
    unsigned int msgCount = 100000;
    unsigned int msgSize{};

    /*
    msgSize = 20;
    std::cout << "** Measuring signals of size " << msgSize << " bytes (" << repetitions << " repetitions)..." << std::endl << std::endl;
    client.m_msgCount = msgCount; client.m_msgSize = msgSize;
    for (unsigned int r = 0; r < repetitions; ++r)
    {
        client.sendDataSignals(msgCount, msgSize);

        std::this_thread::sleep_for(1000ms);
    }

    std::cout << "AVERAGE: " << (totalDuration/repetitions) << " ms" << std::endl;
    totalDuration = 0;

    msgSize = 1000;
    std::cout << std::endl << "** Measuring signals of size " << msgSize << " bytes (" << repetitions << " repetitions)..." << std::endl << std::endl;
    client.m_msgCount = msgCount; client.m_msgSize = msgSize;
    for (unsigned int r = 0; r < repetitions; ++r)
    {
        client.sendDataSignals(msgCount, msgSize);

        std::this_thread::sleep_for(1000ms);
    }

    std::cout << "AVERAGE: " << (totalDuration/repetitions) << " ms" << std::endl;
    totalDuration = 0;
    */

//    msgSize = 20;
//    std::cout << std::endl << "** Measuring method calls of size " << msgSize << " bytes (" << repetitions << " repetitions)..." << std::endl << std::endl;
//    for (unsigned int r = 0; r < repetitions; ++r)
//    {
//        auto str1 = createRandomString(msgSize/2);
//        auto str2 = createRandomString(msgSize/2);

//        auto startTime = std::chrono::steady_clock::now();
//        for (unsigned int i = 0; i < msgCount; i++)
//        {
//            auto result = client.concatenateTwoStrings(str1, str2);

//            assert(result.size() == str1.size() + str2.size());
//            assert(result.size() == msgSize);
//        }
//        auto stopTime = std::chrono::steady_clock::now();
//        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTime).count();
//        totalDuration += duration;
//        std::cout << "Called " << msgCount << " methods in: " << duration << " ms" << std::endl;

//        std::this_thread::sleep_for(1000ms);
//    }

//    std::cout << "AVERAGE: " << (totalDuration/repetitions) << " ms" << std::endl;
//    totalDuration = 0;

//    msgSize = 1000;
//    std::cout << std::endl << "** Measuring method calls of size " << msgSize << " bytes (" << repetitions << " repetitions)..." << std::endl << std::endl;
//    for (unsigned int r = 0; r < repetitions; ++r)
//    {
//        auto str1 = createRandomString(msgSize/2);
//        auto str2 = createRandomString(msgSize/2);

//        auto startTime = std::chrono::steady_clock::now();
//        for (unsigned int i = 0; i < msgCount; i++)
//        {
//            auto result = client.concatenateTwoStrings(str1, str2);

//            assert(result.size() == str1.size() + str2.size());
//            assert(result.size() == msgSize);
//        }
//        auto stopTime = std::chrono::steady_clock::now();
//        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTime).count();
//        totalDuration += duration;
//        std::cout << "Called " << msgCount << " methods in: " << duration << " ms" << std::endl;

//        std::this_thread::sleep_for(1000ms);
//    }

//    std::cout << "AVERAGE: " << (totalDuration/repetitions) << " ms" << std::endl;
//    totalDuration = 0;

    auto proxy = sdbus::createProxy(destinationName, objectPath);
    auto msg = proxy->createMethodCall("org.sdbuscpp.perftests", "concatenateTwoStrings");
    //auto msg = sdbus::createPlainMessage();
    msg.seal();

    msgSize = 20;
    std::cout << std::endl << "** Measuring method calls of size " << msgSize << " bytes (" << repetitions << " repetitions)..." << std::endl << std::endl;
    for (unsigned int r = 0; r < repetitions; ++r)
    {
        auto startTime = std::chrono::steady_clock::now();
        for (unsigned int i = 0; i < msgCount; i++)
        {
            //auto result = client.concatenateTwoStrings(str1, str2);
            proxy->callMethod(msg);

            //assert(result.size() == str1.size() + str2.size());
            //assert(result.size() == msgSize);
        }
        auto stopTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTime).count();
        totalDuration += duration;
        std::cout << "Called " << msgCount << " methods in: " << duration << " ms" << std::endl;

        std::this_thread::sleep_for(100ms);
    }

    std::cout << "AVERAGE: " << (totalDuration/repetitions) << " ms" << std::endl;
    totalDuration = 0;

//    msgSize = 1000;
//    std::cout << std::endl << "** Measuring method calls of size " << msgSize << " bytes (" << repetitions << " repetitions)..." << std::endl << std::endl;
//    for (unsigned int r = 0; r < repetitions; ++r)
//    {
//        auto str1 = createRandomString(msgSize/2);
//        auto str2 = createRandomString(msgSize/2);

//        auto startTime = std::chrono::steady_clock::now();
//        for (unsigned int i = 0; i < msgCount; i++)
//        {
//            auto result = client.concatenateTwoStrings(str1, str2);

//            assert(result.size() == str1.size() + str2.size());
//            assert(result.size() == msgSize);
//        }
//        auto stopTime = std::chrono::steady_clock::now();
//        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTime).count();
//        totalDuration += duration;
//        std::cout << "Called " << msgCount << " methods in: " << duration << " ms" << std::endl;

//        std::this_thread::sleep_for(1000ms);
//    }

    std::cout << "AVERAGE: " << (totalDuration/repetitions) << " ms" << std::endl;
    totalDuration = 0;

    return 0;
}
