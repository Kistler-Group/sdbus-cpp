#include "perftest-proxy.h"
#include <sdbus-c++/sdbus-c++.h>
#include <vector>
#include <string>
#include <iostream>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <cassert>

using namespace std::chrono_literals;

class PerftestClient : public sdbus::ProxyInterfaces<org::sdbuscpp::perftest_proxy>
{
public:
    PerftestClient(std::string destination, std::string objectPath)
        : sdbus::ProxyInterfaces<org::sdbuscpp::perftest_proxy>(std::move(destination), std::move(objectPath))
    {
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
            std::cout << "Received " << m_msgCount << " signals in: " << std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTime).count() << " ms" << std::endl;
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
    const char* destinationName = "org.sdbuscpp.perftest";
    const char* objectPath = "/org/sdbuscpp/perftest";
    PerftestClient client(destinationName, objectPath);

    const unsigned int repetitions{20};
    unsigned int msgCount = 1000;
    unsigned int msgSize{};
    
    msgSize = 20;
    std::cout << "** Measuring signals of size " << msgSize << " bytes (" << repetitions << " repetitions)..." << std::endl << std::endl;
    client.m_msgCount = msgCount; client.m_msgSize = msgSize;
    for (unsigned int r = 0; r < repetitions; ++r)
    {
        client.sendDataSignals(msgCount, msgSize);
        
        std::this_thread::sleep_for(1000ms);
    }
    
    msgSize = 1000;
    std::cout << std::endl << "** Measuring signals of size " << msgSize << " bytes (" << repetitions << " repetitions)..." << std::endl << std::endl;
    client.m_msgCount = msgCount; client.m_msgSize = msgSize;
    for (unsigned int r = 0; r < repetitions; ++r)
    {
        client.sendDataSignals(msgCount, msgSize);
        
        std::this_thread::sleep_for(1000ms);
    }
    
    msgSize = 20;
    std::cout << std::endl << "** Measuring method calls of size " << msgSize << " bytes (" << repetitions << " repetitions)..." << std::endl << std::endl;
    for (unsigned int r = 0; r < repetitions; ++r)
    {
        auto str1 = createRandomString(msgSize/2);
        auto str2 = createRandomString(msgSize/2);
        
        auto startTime = std::chrono::steady_clock::now();
        for (unsigned int i = 0; i < msgCount; i++)
        {
            auto result = client.concatenateTwoStrings(str1, str2);
            
            assert(result.size() == str1.size() + str2.size());
            assert(result.size() == msgSize);
        }
        auto stopTime = std::chrono::steady_clock::now();
        std::cout << "Called " << msgCount << " methods in: " << std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTime).count() << " ms" << std::endl;
        
        std::this_thread::sleep_for(1000ms);
    }
    
    msgSize = 1000;
    std::cout << std::endl << "** Measuring method calls of size " << msgSize << " bytes (" << repetitions << " repetitions)..." << std::endl << std::endl;
    for (unsigned int r = 0; r < repetitions; ++r)
    {
        auto str1 = createRandomString(msgSize/2);
        auto str2 = createRandomString(msgSize/2);
        
        auto startTime = std::chrono::steady_clock::now();
        for (unsigned int i = 0; i < msgCount; i++)
        {
            auto result = client.concatenateTwoStrings(str1, str2);
            
            assert(result.size() == str1.size() + str2.size());
            assert(result.size() == msgSize);
        }
        auto stopTime = std::chrono::steady_clock::now();
        std::cout << "Called " << msgCount << " methods in: " << std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTime).count() << " ms" << std::endl;
        
        std::this_thread::sleep_for(1000ms);
    }
    
    return 0;
}
