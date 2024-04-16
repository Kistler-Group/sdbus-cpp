/**
 * Example of a D-Bus client which implements org.freedesktop.DBus.ObjectManager
 *
 * The example uses the generated stub API layer to listen to interfaces added to new objects under
 * "org.sdbuscpp.examplemanager". If added, we access "org.sdbuscpp.ExampleManager.Planet1" to print
 * info like this:
 * /org/sdbuscpp/examplemanager/Planet1/Earth added:	org.sdbuscpp.ExampleManager.Planet1
 * Earth has a population of 7874965825.
 *
 */

#include "examplemanager-planet1-client-glue.h"
#include <sdbus-c++/sdbus-c++.h>
#include <iostream>
#include <thread>

class PlanetProxy final : public sdbus::ProxyInterfaces< org::sdbuscpp::ExampleManager::Planet1_proxy >
{
public:
    PlanetProxy(sdbus::IConnection& connection, sdbus::ServiceName destination, sdbus::ObjectPath path)
    : ProxyInterfaces(connection, std::move(destination), std::move(path))
    {
        registerProxy();
    }

    ~PlanetProxy()
    {
        unregisterProxy();
    }
};

class ManagerProxy final : public sdbus::ProxyInterfaces<sdbus::ObjectManager_proxy>
{
public:
    ManagerProxy(sdbus::IConnection& connection, sdbus::ServiceName destination, sdbus::ObjectPath path)
        : ProxyInterfaces(connection, destination, std::move(path))
        , m_connection(connection)
        , m_destination(destination)
    {
        registerProxy();
    }

    ~ManagerProxy()
    {
        unregisterProxy();
    }

    void handleExistingObjects()
    {
        auto objectsInterfacesAndProperties = GetManagedObjects();
        for (const auto& [object, interfacesAndProperties] : objectsInterfacesAndProperties) {
            onInterfacesAdded(object, interfacesAndProperties);
        }
    }

private:
    void onInterfacesAdded( const sdbus::ObjectPath& objectPath
                          , const std::map<sdbus::InterfaceName, std::map<sdbus::PropertyName, sdbus::Variant>>& interfacesAndProperties) override
    {
        std::cout << objectPath << " added:\t";
        for (const auto& [interface, _] : interfacesAndProperties) {
            std::cout << interface << " ";
        }
        std::cout << std::endl;

        // Parse and print some more info
        auto planetInterface = interfacesAndProperties.find(sdbus::InterfaceName{org::sdbuscpp::ExampleManager::Planet1_proxy::INTERFACE_NAME});
        if (planetInterface == interfacesAndProperties.end()) {
            return;
        }
        const auto& properties = planetInterface->second;
        // get a property which was passed as part of the signal.
        const auto& name = properties.at(sdbus::PropertyName{"Name"}).get<std::string>();
        // or create a proxy instance to the newly added object.
        PlanetProxy planet(m_connection, m_destination, objectPath);
        std::cout << name << " has a population of " << planet.GetPopulation() << ".\n" << std::endl;
    }

    void onInterfacesRemoved( const sdbus::ObjectPath& objectPath
                            , const std::vector<sdbus::InterfaceName>& interfaces) override
    {
        std::cout << objectPath << " removed:\t";
        for (const auto& interface : interfaces) {
            std::cout << interface << " ";
        }
        std::cout << std::endl;
    }

    sdbus::IConnection& m_connection;
    sdbus::ServiceName m_destination;
};

int main()
{
    auto connection = sdbus::createSessionBusConnection();

    sdbus::ServiceName destination{"org.sdbuscpp.examplemanager"};
    sdbus::ObjectPath objectPath{"/org/sdbuscpp/examplemanager"};
    auto managerProxy = std::make_unique<ManagerProxy>(*connection, std::move(destination), std::move(objectPath));
    try {
        managerProxy->handleExistingObjects();
    }
    catch (const sdbus::Error& e) {
        if (e.getName() == "org.freedesktop.DBus.Error.ServiceUnknown") {
            std::cout << "Waiting for server to start ..." << std::endl;
        }
    }

    connection->enterEventLoop();
    return 0;
}
