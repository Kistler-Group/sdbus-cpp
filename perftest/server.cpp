#include "perftest-adaptor.h"
#include <sdbus-c++/sdbus-c++.h>
#include <vector>
#include <string>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

std::string createRandomString(size_t length);

class PerftestServer : public sdbus::Interfaces<org::sdbuscpp::perftest_adaptor>
{
public:
    PerftestServer(sdbus::IConnection& connection, std::string objectPath)
        : sdbus::Interfaces<org::sdbuscpp::perftest_adaptor>(connection, std::move(objectPath))
    {
    }

protected:
    virtual void sendDataSignals(const uint32_t& numberOfSignals, const uint32_t& signalMsgSize) override
    {
        auto data = createRandomString(signalMsgSize);
        char digits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
        
        auto start_time = std::chrono::steady_clock::now();
        for (uint32_t i = 0; i < numberOfSignals; ++i)
        {
            // Emit signal
            dataSignal(data);
        }
        auto stop_time = std::chrono::steady_clock::now();
        std::cout << "Server sent " << numberOfSignals << " signals in: " << std::chrono::duration_cast<std::chrono::milliseconds>(stop_time - start_time).count() << " ms" << std::endl;
    }
    
    virtual std::string concatenateTwoStrings(const std::string& string1, const std::string& string2) override
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
        return charset[ rand() % max_index ];
    };
    std::string str(length, 0);
    std::generate_n(str.begin(), length, randchar);
    return str;
}


//-----------------------------------------
int main(int /*argc*/, char */*argv*/[])
{
    const char* serviceName = "org.sdbuscpp.perftest";
    auto connection = sdbus::createSystemBusConnection(serviceName);

    const char* objectPath = "/org/sdbuscpp/perftest";
    PerftestServer server(*connection, objectPath);

    connection->enterProcessingLoop();
}
