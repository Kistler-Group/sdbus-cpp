/**
 * Example of a D-Bus server which tracks its clients via sdbus::Track
 *
 * The server registers "org.sdbuscpp.lock" service name and an object "/org/sdbuscpp/lock"
 * which implements "org.sdbuscpp.Lock" interface with methods Acquire() and Release().
 * Only one client can hold the lock at a time. If the client holding the lock disconnects
 * from the bus without calling Release(), the lock is released automatically.
 *
 * Try it with busctl, which disconnects from the bus right after the call:
 * busctl --user call org.sdbuscpp.lock /org/sdbuscpp/lock org.sdbuscpp.Lock Acquire
 *
 * The server prints info like this:
 * Lock acquired by :1.123
 * Lock released, :1.123 has disconnected
 */

#include <sdbus-c++/sdbus-c++.h>
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <utility>

int main()
try
{
    const sdbus::ServiceName serviceName{"org.sdbuscpp.lock"};
    auto connection = sdbus::createSessionBusConnection(serviceName);

    const sdbus::ObjectPath objectPath{"/org/sdbuscpp/lock"};
    auto object = sdbus::createObject(*connection, objectPath);

    // Tracks the client holding the lock. All callbacks run in the event loop thread, so no locking is needed.
    std::optional<sdbus::Track> owner;

    auto acquire = [&]()
    {
        if (owner)
            throw sdbus::Error(sdbus::Error::Name{"org.sdbuscpp.Lock.Error.Busy"}, "Lock is held by another client");

        auto call = object->getCurrentlyProcessedMessage();
        const std::string sender = call.getSender();

        // The handler is invoked once the tracked client disappears from the bus
        owner.emplace(connection->createTrack([&owner, sender](sdbus::Track&)
        {
            std::cout << "Lock released, " << sender << " has disconnected" << '\n';
            owner.reset();
        }));
        owner->addSender(call);

        std::cout << "Lock acquired by " << sender << '\n';
    };

    auto release = [&]()
    {
        auto call = object->getCurrentlyProcessedMessage();
        const sdbus::BusName sender{call.getSender()};

        if (!owner || !owner->contains(sender))
            throw sdbus::Error(sdbus::Error::Name{"org.sdbuscpp.Lock.Error.NotOwner"}, "Lock is not held by the caller");

        // Destroying a non-empty track does not invoke its handler
        owner.reset();

        std::cout << "Lock released by " << sender << '\n';
    };

    object->addVTable( sdbus::registerMethod("Acquire").implementedAs(std::move(acquire))
                     , sdbus::registerMethod("Release").implementedAs(std::move(release)) )
                     .forInterface(sdbus::InterfaceName{"org.sdbuscpp.Lock"});

    connection->enterEventLoop();
}
catch (const std::exception& e)
{
    std::cerr << "Error: " << e.what() << '\n';
    return 1;
}
