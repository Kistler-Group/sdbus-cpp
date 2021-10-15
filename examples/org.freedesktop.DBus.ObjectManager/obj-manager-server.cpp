/**
 * Example of a D-Bus server which implements org.freedesktop.DBus.ObjectManager
 *
 * The example uses the generated stub API layer to register an object manager under "org.sdbuscpp.examplemanager"
 * and add objects underneath which implement "org.sdbuscpp.ExampleManager.Planet1".
 *
 * We add and remove objects after a few seconds and print info like this:
 * Creating PlanetAdaptor in 5 4 3 2 1
 * Creating PlanetAdaptor in 5 4 3 2 1
 * Creating PlanetAdaptor in 5 4 3 2 1
 * Removing PlanetAdaptor in 5 4 3 2 1
 * Removing PlanetAdaptor in 5 4 3 2 1
 */

#include "examplemanager-planet1-server-glue.h"
#include <sdbus-c++/sdbus-c++.h>
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

class ManagerAdaptor : public sdbus::AdaptorInterfaces< sdbus::ObjectManager_adaptor >
{
public:
    ManagerAdaptor(sdbus::IConnection& connection, std::string path)
    : AdaptorInterfaces(connection, std::move(path))
    {
        registerAdaptor();
    }

    ~ManagerAdaptor()
    {
        unregisterAdaptor();
    }
};

class PlanetAdaptor final : public sdbus::AdaptorInterfaces< org::sdbuscpp::ExampleManager::Planet1_adaptor,
                                                sdbus::ManagedObject_adaptor,
                                                sdbus::Properties_adaptor >
{
public:
    PlanetAdaptor(sdbus::IConnection& connection, std::string path, std::string name, uint64_t poulation)
    : AdaptorInterfaces(connection, std::move(path))
    , m_name(std::move(name))
    , m_population(poulation)
    {
        registerAdaptor();
        emitInterfacesAddedSignal({org::sdbuscpp::ExampleManager::Planet1_adaptor::INTERFACE_NAME});
    }

    ~PlanetAdaptor()
    {
        emitInterfacesRemovedSignal({org::sdbuscpp::ExampleManager::Planet1_adaptor::INTERFACE_NAME});
        unregisterAdaptor();
    }

    uint64_t GetPopulation() override
    {
        return m_population;
    }

    std::string Name() override
    {
        return m_name;
    }

private:
    std::string m_name;
    uint64_t m_population;
};

void printCountDown(const std::string& message, int seconds)
{
    std::cout << message << std::flush;
    for (int i = seconds; i > 0; i--) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << i << " " << std::flush;
    }
    std::cout << std::endl;
}

int main()
{
    auto connection = sdbus::createSessionBusConnection();
    connection->requestName("org.sdbuscpp.examplemanager");
    connection->enterEventLoopAsync();

    auto manager = std::make_unique<ManagerAdaptor>(*connection, "/org/sdbuscpp/examplemanager");
    while (true)
    {
        printCountDown("Creating PlanetAdaptor in ", 5);
        auto earth = std::make_unique<PlanetAdaptor>(*connection, "/org/sdbuscpp/examplemanager/Planet1/Earth", "Earth", 7'874'965'825);
        printCountDown("Creating PlanetAdaptor in ", 5);
        auto trantor = std::make_unique<PlanetAdaptor>(*connection, "/org/sdbuscpp/examplemanager/Planet1/Trantor", "Trantor", 40'000'000'000);
        printCountDown("Creating PlanetAdaptor in ", 5);
        auto laconia = std::make_unique<PlanetAdaptor>(*connection, "/org/sdbuscpp/examplemanager/Planet1/Laconia", "Laconia", 231'721);
        printCountDown("Removing PlanetAdaptor in ", 5);
        earth.reset();
        printCountDown("Removing PlanetAdaptor in ", 5);
        trantor.reset();
        printCountDown("Removing PlanetAdaptor in ", 5);
        laconia.reset();
    }

    connection->releaseName("org.sdbuscpp.examplemanager");
    connection->leaveEventLoop();
    return 0;
}