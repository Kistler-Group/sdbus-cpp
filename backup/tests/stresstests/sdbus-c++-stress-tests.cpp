/**
 * (C) 2019 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 *
 * @file sdbus-c++-stress-tests.cpp
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

#include "celsius-thermometer-adaptor.h"
#include "celsius-thermometer-proxy.h"
#include "fahrenheit-thermometer-adaptor.h"
#include "fahrenheit-thermometer-proxy.h"
#include "concatenator-adaptor.h"
#include "concatenator-proxy.h"
#include <sdbus-c++/sdbus-c++.h>
#include <vector>
#include <string>
#include <iostream>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <cassert>
#include <cstdlib>
#include <atomic>
#include <sstream>
#include <mutex>
#include <condition_variable>
#include <queue>

using namespace std::chrono_literals;
using namespace std::string_literals;

#define SERVICE_1_BUS_NAME "org.sdbuscpp.stresstests.service1"s
#define SERVICE_2_BUS_NAME "org.sdbuscpp.stresstests.service2"s
#define CELSIUS_THERMOMETER_OBJECT_PATH "/org/sdbuscpp/stresstests/celsius/thermometer"s
#define FAHRENHEIT_THERMOMETER_OBJECT_PATH "/org/sdbuscpp/stresstests/fahrenheit/thermometer"s
#define CONCATENATOR_OBJECT_PATH "/org/sdbuscpp/stresstests/concatenator"s

class CelsiusThermometerAdaptor final : public sdbus::AdaptorInterfaces<org::sdbuscpp::stresstests::celsius::thermometer_adaptor>
{
public:
    CelsiusThermometerAdaptor(sdbus::IConnection& connection, std::string objectPath)
        : AdaptorInterfaces(connection, std::move(objectPath))
    {
        registerAdaptor();
    }

    ~CelsiusThermometerAdaptor()
    {
        unregisterAdaptor();
    }

protected:
    virtual uint32_t getCurrentTemperature() override
    {
        return m_currentTemperature++;
    }

private:
    uint32_t m_currentTemperature{};
};

class CelsiusThermometerProxy : public sdbus::ProxyInterfaces<org::sdbuscpp::stresstests::celsius::thermometer_proxy>
{
public:
    CelsiusThermometerProxy(sdbus::IConnection& connection, std::string destination, std::string objectPath)
        : ProxyInterfaces(connection, std::move(destination), std::move(objectPath))
    {
        registerProxy();
    }

    ~CelsiusThermometerProxy()
    {
        unregisterProxy();
    }
};

class FahrenheitThermometerAdaptor final : public sdbus::AdaptorInterfaces< org::sdbuscpp::stresstests::fahrenheit::thermometer_adaptor
                                                                          , org::sdbuscpp::stresstests::fahrenheit::thermometer::factory_adaptor >
{
public:
    FahrenheitThermometerAdaptor(sdbus::IConnection& connection, std::string objectPath, bool isDelegate)
        : AdaptorInterfaces(connection, std::move(objectPath))
        , celsiusProxy_(connection, SERVICE_2_BUS_NAME, CELSIUS_THERMOMETER_OBJECT_PATH)
    {
        if (!isDelegate)
        {
            unsigned int workers = std::thread::hardware_concurrency();
            if (workers < 4)
                workers = 4;

            for (unsigned int i = 0; i < workers; ++i)
                workers_.emplace_back([this]()
                {
                    //std::cout << "Created FTA worker thread 0x" << std::hex << std::this_thread::get_id() << std::dec << std::endl;

                    while(!exit_)
                    {
                        // Pop a work item from the queue
                        std::unique_lock<std::mutex> lock(mutex_);
                        cond_.wait(lock, [this]{return !requests_.empty() || exit_;});
                        if (exit_)
                            break;
                        auto request = std::move(requests_.front());
                        requests_.pop();
                        lock.unlock();

                        // Either create or destroy a delegate object
                        if (request.delegateObjectPath.empty())
                        {
                            // Create new delegate object
                            auto& connection = getObject().getConnection();
                            sdbus::ObjectPath newObjectPath = FAHRENHEIT_THERMOMETER_OBJECT_PATH + "/" + std::to_string(request.objectNr);

                            // Here we are testing dynamic creation of a D-Bus object in an async way
                            auto adaptor = std::make_unique<FahrenheitThermometerAdaptor>(connection, newObjectPath, true);

                            std::unique_lock<std::mutex> lock{childrenMutex_};
                            children_.emplace(newObjectPath, std::move(adaptor));
                            lock.unlock();

                            request.result.returnResults(newObjectPath);
                        }
                        else
                        {
                            // Destroy existing delegate object
                            // Here we are testing dynamic removal of a D-Bus object in an async way
                            std::lock_guard<std::mutex> lock{childrenMutex_};
                            children_.erase(request.delegateObjectPath);
                        }
                    }
                });
        }

        registerAdaptor();
    }

    ~FahrenheitThermometerAdaptor()
    {
        exit_ = true;
        cond_.notify_all();
        for (auto& worker : workers_)
            worker.join();

        unregisterAdaptor();
    }

protected:
    virtual uint32_t getCurrentTemperature() override
    {
        // In this D-Bus call, make yet another D-Bus call to another service over the same connection
        return static_cast<uint32_t>(celsiusProxy_.getCurrentTemperature() * 1.8 + 32.);
    }

    virtual void createDelegateObject(sdbus::Result<sdbus::ObjectPath>&& result) override
    {
        static size_t objectCounter{};
        objectCounter++;

        std::unique_lock<std::mutex> lock(mutex_);
        requests_.push(WorkItem{objectCounter, std::string{}, std::move(result)});
        lock.unlock();
        cond_.notify_one();
    }

    virtual void destroyDelegateObject(sdbus::Result<>&& /*result*/, sdbus::ObjectPath delegate) override
    {
        std::unique_lock<std::mutex> lock(mutex_);
        requests_.push(WorkItem{0, std::move(delegate), {}});
        lock.unlock();
        cond_.notify_one();
    }

private:
    CelsiusThermometerProxy celsiusProxy_;
    std::map<std::string, std::unique_ptr<FahrenheitThermometerAdaptor>> children_;
    std::mutex childrenMutex_;

    struct WorkItem
    {
        size_t objectNr;
        sdbus::ObjectPath delegateObjectPath;
        sdbus::Result<sdbus::ObjectPath> result;
    };
    std::mutex mutex_;
    std::condition_variable cond_;
    std::queue<WorkItem> requests_;
    std::vector<std::thread> workers_;
    std::atomic<bool> exit_{};
};

class FahrenheitThermometerProxy : public sdbus::ProxyInterfaces< org::sdbuscpp::stresstests::fahrenheit::thermometer_proxy
                                                                , org::sdbuscpp::stresstests::fahrenheit::thermometer::factory_proxy >
{
public:
    FahrenheitThermometerProxy(sdbus::IConnection& connection, std::string destination, std::string objectPath)
        : ProxyInterfaces(connection, std::move(destination), std::move(objectPath))
    {
        registerProxy();
    }

    ~FahrenheitThermometerProxy()
    {
        unregisterProxy();
    }
};

class ConcatenatorAdaptor final : public sdbus::AdaptorInterfaces<org::sdbuscpp::stresstests::concatenator_adaptor>
{
public:
    ConcatenatorAdaptor(sdbus::IConnection& connection, std::string objectPath)
        : AdaptorInterfaces(connection, std::move(objectPath))
    {
        unsigned int workers = std::thread::hardware_concurrency();
        if (workers < 4)
            workers = 4;

        for (unsigned int i = 0; i < workers; ++i)
            workers_.emplace_back([this]()
            {
                //std::cout << "Created CA worker thread 0x" << std::hex << std::this_thread::get_id() << std::dec << std::endl;

                while(!exit_)
                {
                    // Pop a work item from the queue
                    std::unique_lock<std::mutex> lock(mutex_);
                    cond_.wait(lock, [this]{return !requests_.empty() || exit_;});
                    if (exit_)
                        break;
                    auto request = std::move(requests_.front());
                    requests_.pop();
                    lock.unlock();

                    // Do concatenation work, return results and fire signal
                    auto aString = request.input.at("key1").get<std::string>();
                    auto aNumber = request.input.at("key2").get<uint32_t>();
                    auto resultString = aString + " " + std::to_string(aNumber);

                    request.result.returnResults(resultString);

                    emitConcatenatedSignal(resultString);
                }
            });

        registerAdaptor();
    }

    ~ConcatenatorAdaptor()
    {
        exit_ = true;
        cond_.notify_all();
        for (auto& worker : workers_)
            worker.join();

        unregisterAdaptor();
    }

protected:
    virtual void concatenate(sdbus::Result<std::string>&& result, std::map<std::string, sdbus::Variant> params) override
    {
        std::unique_lock<std::mutex> lock(mutex_);
        requests_.push(WorkItem{std::move(params), std::move(result)});
        lock.unlock();
        cond_.notify_one();
    }

private:
    struct WorkItem
    {
        std::map<std::string, sdbus::Variant> input;
        sdbus::Result<std::string> result;
    };
    std::mutex mutex_;
    std::condition_variable cond_;
    std::queue<WorkItem> requests_;
    std::vector<std::thread> workers_;
    std::atomic<bool> exit_{};
};

class ConcatenatorProxy final : public sdbus::ProxyInterfaces<org::sdbuscpp::stresstests::concatenator_proxy>
{
public:
    ConcatenatorProxy(sdbus::IConnection& connection, std::string destination, std::string objectPath)
        : ProxyInterfaces(connection, std::move(destination), std::move(objectPath))
    {
        registerProxy();
    }

    ~ConcatenatorProxy()
    {
        unregisterProxy();
    }

private:
    virtual void onConcatenateReply(const std::string& result, [[maybe_unused]] const sdbus::Error* error) override
    {
        assert(error == nullptr);

        std::stringstream str(result);
        std::string aString;
        str >> aString;
        assert(aString == "sdbus-c++-stress-tests");

        uint32_t aNumber;
        str >> aNumber;
        assert(aNumber > 0);

        ++repliesReceived_;
    }

    virtual void onConcatenatedSignal(const std::string& concatenatedString) override
    {
        std::stringstream str(concatenatedString);
        std::string aString;
        str >> aString;
        assert(aString == "sdbus-c++-stress-tests");

        uint32_t aNumber;
        str >> aNumber;
        assert(aNumber > 0);

        ++signalsReceived_;
    }

public:
    std::atomic<uint32_t> repliesReceived_{};
    std::atomic<uint32_t> signalsReceived_{};
};

//-----------------------------------------
int main(int argc, char *argv[])
{
    long loops;
    long loopDuration;

    if (argc == 1)
    {
        loops = 1;
        loopDuration = 30000;
    }
    else if (argc == 3)
    {
        loops = std::atol(argv[1]);
        loopDuration = std::atol(argv[2]);
    }
    else
        throw std::runtime_error("Wrong program options");

    std::cout << "Going on with " << loops << " loops and " << loopDuration << "ms loop duration" << std::endl;

    std::atomic<uint32_t> concatenationCallsMade{0};
    std::atomic<uint32_t> concatenationRepliesReceived{0};
    std::atomic<uint32_t> concatenationSignalsReceived{0};
    std::atomic<uint32_t> thermometerCallsMade{0};

    std::atomic<bool> exitLogger{};
    std::thread loggerThread([&]()
    {
        while (!exitLogger)
        {
            std::this_thread::sleep_for(1s);

            std::cout << "Made " << concatenationCallsMade << " concatenation calls, received " << concatenationRepliesReceived << " replies and " << concatenationSignalsReceived << " signals so far." << std::endl;
            std::cout << "Made " << thermometerCallsMade << " thermometer calls so far." << std::endl << std::endl;
        }
    });

    for (long loop = 0; loop < loops; ++loop)
    {
        std::cout << "Entering loop " << loop+1 << std::endl;

        auto service2Connection = sdbus::createSystemBusConnection(SERVICE_2_BUS_NAME);
        std::atomic<bool> service2ThreadReady{};
        std::thread service2Thread([&con = *service2Connection, &service2ThreadReady]()
        {
            CelsiusThermometerAdaptor thermometer(con, CELSIUS_THERMOMETER_OBJECT_PATH);
            service2ThreadReady = true;
            con.enterEventLoop();
        });

        auto service1Connection = sdbus::createSystemBusConnection(SERVICE_1_BUS_NAME);
        std::atomic<bool> service1ThreadReady{};
        std::thread service1Thread([&con = *service1Connection, &service1ThreadReady]()
        {
            ConcatenatorAdaptor concatenator(con, CONCATENATOR_OBJECT_PATH);
            FahrenheitThermometerAdaptor thermometer(con, FAHRENHEIT_THERMOMETER_OBJECT_PATH, false);
            service1ThreadReady = true;
            con.enterEventLoop();
        });

        // Wait for both services to export their D-Bus objects
        while (!service2ThreadReady || !service1ThreadReady)
            std::this_thread::sleep_for(1ms);

        auto clientConnection = sdbus::createSystemBusConnection();
        std::mutex clientThreadExitMutex;
        std::condition_variable clientThreadExitCond;
        bool clientThreadExit{};
        std::thread clientThread([&, &con = *clientConnection]()
        {
            std::atomic<bool> stopClients{false};

            std::thread concatenatorThread([&]()
            {
                ConcatenatorProxy concatenator(con, SERVICE_1_BUS_NAME, CONCATENATOR_OBJECT_PATH);

                uint32_t localCounter{};

                // Issue async concatenate calls densely one after another
                while (!stopClients)
                {
                    std::map<std::string, sdbus::Variant> param;
                    param["key1"] = "sdbus-c++-stress-tests";
                    param["key2"] = ++localCounter;

                    concatenator.concatenate(param);

                    if ((localCounter % 10) == 0)
                    {
                        // Make sure the system is catching up with our async requests,
                        // otherwise sleep a bit to slow down flooding the server.
                        assert(localCounter >= concatenator.repliesReceived_);
                        while ((localCounter - concatenator.repliesReceived_) > 40 && !stopClients)
                            std::this_thread::sleep_for(1ms);

                        // Update statistics
                        concatenationCallsMade = localCounter;
                        concatenationRepliesReceived = (uint32_t)concatenator.repliesReceived_;
                        concatenationSignalsReceived = (uint32_t)concatenator.signalsReceived_;
                    }
                }
            });

            std::thread thermometerThread([&]()
            {
                // Here we continuously remotely call getCurrentTemperature(). We have one proxy object,
                // first we use it's factory interface to create another proxy object, call getCurrentTemperature()
                // on that one, and then destroy that proxy object. All that continously in a loop.
                // This tests dynamic creation and destruction of remote D-Bus objects and local object proxies.

                FahrenheitThermometerProxy thermometer(con, SERVICE_1_BUS_NAME, FAHRENHEIT_THERMOMETER_OBJECT_PATH);
                uint32_t localCounter{};
                [[maybe_unused]] uint32_t previousTemperature{};

                while (!stopClients)
                {
                    localCounter++;

                    auto newObjectPath = thermometer.createDelegateObject();
                    FahrenheitThermometerProxy proxy{con, SERVICE_1_BUS_NAME, newObjectPath};

                    auto temperature = proxy.getCurrentTemperature();
                    assert(temperature >= previousTemperature); // The temperature shall rise continually
                    previousTemperature = temperature;
                    //std::this_thread::sleep_for(1ms);

                    if ((localCounter % 10) == 0)
                        thermometerCallsMade = localCounter;

                    thermometer.destroyDelegateObject(newObjectPath);
                }
            });

            // We could run the loop in a sync way, but we want it to run also when proxies are destroyed for better
            // coverage of multi-threaded scenarios, so we run it async and use condition variable for exit notification
            //con.enterEventLoop();
            con.enterEventLoopAsync();

            std::unique_lock<std::mutex> lock(clientThreadExitMutex);
            clientThreadExitCond.wait(lock, [&]{return clientThreadExit;});

            stopClients = true;
            thermometerThread.join();
            concatenatorThread.join();
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(loopDuration));

        //clientConnection->leaveEventLoop();
        std::unique_lock<std::mutex> lock(clientThreadExitMutex);
        clientThreadExit = true;
        lock.unlock();
        clientThreadExitCond.notify_one();
        clientThread.join();

        service1Connection->leaveEventLoop();
        service1Thread.join();

        service2Connection->leaveEventLoop();
        service2Thread.join();
    }

    exitLogger = true;
    loggerThread.join();
    
    return 0;
}
