Using sdbus-c++ library
=======================

**Table of contents**

1. [Introduction](#introduction)
2. [Integrating sdbus-c++ into your project](#integrating-sdbus-c-into-your-project)
3. [Solving libsystemd dependency](#solving-libsystemd-dependency)
4. [Distributing sdbus-c++](#distributing-sdbus-c)
5. [Header files and namespaces](#header-files-and-namespaces)
6. [Error signalling and propagation](#error-signalling-and-propagation)
7. [Design of sdbus-c++](#design-of-sdbus-c)
8. [Multiple layers of sdbus-c++ API](#multiple-layers-of-sdbus-c-api)
9. [An example: Number concatenator](#an-example-number-concatenator)
10. [Implementing the Concatenator example using basic sdbus-c++ API layer](#implementing-the-concatenator-example-using-basic-sdbus-c-api-layer)
11. [Implementing the Concatenator example using convenience sdbus-c++ API layer](#implementing-the-concatenator-example-using-convenience-sdbus-c-api-layer)
12. [Implementing the Concatenator example using sdbus-c++-generated stubs](#implementing-the-concatenator-example-using-sdbus-c-generated-stubs)
13. [Asynchronous server-side methods](#asynchronous-server-side-methods)
14. [Asynchronous client-side methods](#asynchronous-client-side-methods)
15. [Using D-Bus properties](#using-d-bus-properties)
16. [Standard D-Bus interfaces](#standard-d-bus-interfaces)
17. [Using D-Bus Types](#using-d-bus-types)
18. [Support for match rules](#support-for-match-rules)
19. [Conclusion](#conclusion)

Introduction
------------

sdbus-c++ is a C++ D-Bus library built on top of [sd-bus](http://0pointer.net/blog/the-new-sd-bus-api-of-systemd.html), a lightweight D-Bus client library implemented within [systemd](https://github.com/systemd/systemd) project. It provides D-Bus functionality on a higher level of abstraction, trying to employ C++ type system to shift as much work as possible from the developer to the compiler.

Although sdbus-c++ covers most of sd-bus API, it does not (yet) fully cover every sd-bus API detail. The focus is put on the most widely used functionality: D-Bus connections, object, proxies, synchronous and asynchronous method calls, signals, and properties. If you are missing a desired functionality, you are welcome to submit an issue, or, best, to contribute to sdbus-c++ by submitting a pull request.

Integrating sdbus-c++ into your project
---------------------------------------

The library build system is based on CMake. The library provides a config and an export file, so integrating it into your CMake project is sooo simple:

```cmake
# First, find sdbus-c++
find_package(sdbus-c++ REQUIRED)

# Use the sdbus-c++ target in SDBusCpp namespace
add_executable(exe exe.cpp)
target_link_libraries(exe PRIVATE SDBusCpp::sdbus-c++)
```

The library also supports `pkg-config`, so it easily be integrated into e.g. an Autotools project:

```bash
PKG_CHECK_MODULES(SDBUSCPP, [sdbus-c++ >= 0.6],,
    AC_MSG_ERROR([You need the sdbus-c++ library (version 0.6 or newer)]
    [http://www.kistler.com/])
)
```

> **_Note_:** sdbus-c++ library uses a number of modern C++17 features. Please make certain you have a recent compiler (gcc >= 7, clang >= 6).

If you intend to use stub generator (explained later) in your project to generate interface headers from XML, you can integrate that too with CMake or `pkg-config`:

```cmake
# First, find sdbus-c++-tools
find_package(sdbus-c++-tools REQUIRED)

# Use the sdbus-c++-xml2cpp in SDBusCpp namespace to generate the headers
add_custom_command(
    OUTPUT myproject-client-glue.h myproject-server-glue.h
    COMMAND SDBusCpp::sdbus-c++-xml2cpp ${PROJECT_SOURCE_DIR}/dbus/myproject-bindings.xml
        --proxy=myproject-client-glue.h --adaptor=myproject-server-glue.h
    DEPENDS dbus/myproject-bindings.xml
    COMMENT "Generating D-Bus interfaces for ${PROJECT_NAME}."
)
```

Solving libsystemd dependency
-----------------------------

sdbus-c++ depends on libsystemd, a C library that is part of [systemd](https://github.com/systemd/systemd) and that contains underlying sd-bus implementation.

Minimum required libsystemd shared library version is 0.20.0 (which corresponds to minimum systemd version 236).

If your target Linux distribution is already based on systemd ecosystem of version 236 and higher, then there is no additional effort, just make sure you have corresponding systemd header files available (provided by `libsystemd-dev` package on Debian/Ubuntu, for example), and you may go on building sdbus-c++ seamlessly.

However, sdbus-c++ can perfectly be used in non-systemd environments as well. There are two ways to approach this:

### Building and distributing libsystemd as a shared library yourself

Fortunately, libsystemd is rather self-contained and can be built and used independently of the rest of systemd ecosystem. To build libsystemd shared library for sdbus-c++:

```shell
$ git clone https://github.com/systemd/systemd
$ cd systemd
$ git checkout v242  # or any other recent stable version
$ mkdir build
$ cd build
$ meson --buildtype=release .. # solve systemd dependencies if any pop up, e.g. libmount-dev, libcap, librt...
$ ninja version.h # building version.h target is only necessary in systemd version >= 241
$ ninja libsystemd.so.0.26.0  # or another version number depending which systemd version you have
# finally, manually install the library, header files and libsystemd.pc pkgconfig file
```

### Building and distributing libsystemd as part of sdbus-c++

sdbus-c++ provides `BUILD_LIBSYSTEMD` configuration option. When turned on, sdbus-c++ will automatically download and build libsystemd as a static library and make it an opaque part of sdbus-c++ shared library for you. This is the most convenient and effective approach to build, distribute and use sdbus-c++ as a self-contained, systemd-independent library in non-systemd enviroments. Just make sure your build machine has all dependencies needed by libsystemd build process. That includes, among others, `meson`, `ninja`, `git`, `gperf`, and -- primarily -- libraries and library headers for `libmount`, `libcap` and `librt` (part of glibc). Be sure to check out the systemd documentation for the  Also, when distributing, make sure these dependency libraries are installed on the production machine.

You may additionally set the `LIBSYSTEMD_VERSION` configuration flag to fine-tune the version of systemd to be taken in. (The default value is 242).

Distributing sdbus-c++
----------------------

### Yocto

There are Yocto recipes for sdbus-c++ available in the [`meta-oe`](https://github.com/openembedded/meta-openembedded/tree/master/meta-oe/recipes-core/sdbus-c%2B%2B) layer of the `meta-openembedded` project. There are two recipes:

  * One for sdbus-c++ itself. It detects whether systemd feature is turned on in the poky linux configuration. If so, it simply depends on systemd and makes use of libsystemd shared library available in the target system. Otherwise it automatically downloads and builds libsystemd static library and links it into the sdbus-c++ shared library. The recipe also supports ptest.
  * One for sdbus-c++ native tools, namely sdbus-c++ code generator to generate C++ adaptor and proxy binding classes.

> **_Tip_:** If you get `ERROR: Program or command 'getent' not found or not executable` when building sdbus-c++ in Yocto, please make sure you've added `getent` to `HOSTTOOLS`. For example, you can add `HOSTTOOLS_NONFATAL += "getent"` into your local.conf file.

### Conan

sdbus-c++ recipe is available in ConanCenter repository as [`sdbus-cpp`](https://conan.io/center/sdbus-cpp).

### Buildroot

There is the Buildroot package [`sdbus-cpp`](https://git.buildroot.net/buildroot/tree/package/sdbus-cpp?h=2022.02) to build sdbus-c++ library itself without a code generation tool.

Contributors willing to help with bringing sdbus-c++ to other popular package systems are welcome.

Verifying sdbus-c++
-------------------

You can build and run sdbus-c++ unit and integration tests to verify sdbus-c++ build:

```
$ cd build
$ cmake .. -DBUILD_TESTS=ON
$ sudo cp ../tests/integrationtests/files/org.sdbuscpp.integrationtests.conf /etc/dbus-1/system.d/
$ cmake --build . --target test
```

Header files and namespaces
---------------------------

All sdbus-c++ header files reside in the `sdbus-c++` subdirectory within the standard include directory. Users can either include individual header files, like so:

```cpp
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/IProxy.h>
```

or just include the global header file that pulls in everything:

```cpp
#include <sdbus-c++/sdbus-c++.h>
```

All public types and functions of sdbus-c++ reside in the `sdbus` namespace.

Error signalling and propagation
--------------------------------

`sdbus::Error` exception is used to signal errors in sdbus-c++. There are two types of errors:

  * D-Bus related errors, like call timeouts, failed socket allocation, etc. These are raised by the D-Bus library or D-Bus daemon itself.
  * user-defined errors, i.e. errors signalled and propagated from remote methods back to the caller. So these are issued by sdbus-c++ users.

`sdbus::Error` is a carrier for both types of errors, carrying the error name and error message with it.

Design of sdbus-c++
-------------------

The following diagram illustrates the major entities in sdbus-c++.

![class](sdbus-c++-class-diagram.png)

`IConnection` represents the concept of a D-Bus connection. You can connect to either the system bus or a session bus. Services can assign unique service names to those connections. An I/O event loop should be run on the bus connection.

`IObject` represents the concept of an object that exposes its methods, signals and properties. Its responsibilities are:

  * registering (possibly multiple) interfaces and methods, signals, properties on those interfaces,
  * emitting signals.

`IProxy` represents the concept of the proxy, which is a view of the `Object` from the client side. Its responsibilities are:

  * invoking remote methods of the corresponding object, in both synchronous and asynchronous way,
  * registering handlers for signals,

`Message` class represents a message, which is the fundamental DBus concept. There are three distinctive types of message that are derived from the `Message` class:

  * `MethodCall` (be it synchronous or asynchronous method call, with serialized parameters),
  * `MethodReply` (with serialized return values),
  * `Signal` (with serialized parameters),
  * `PropertySetCall` (with serialized parameter value to be set)
  * `PropertyGetReply` (where property value shall be stored)
  * `PlainMessage` (for internal purposes).

### Thread safety in sdbus-c++

sdbus-c++ is completely thread-aware by design. Though sdbus-c++ is not thread-safe in general, there are situations where sdbus-c++ provides and guarantees API-level thread safety by design. It is safe to do these operations (operations within the bullet points, not across them) from multiple threads at the same time:

  * Making or destroying distinct `Object`/`Proxy` instances simultaneously (even on a shared connection that is running an event loop already, see below). Under *making* here is meant a complete sequence of construction, registration of method/signal/property callbacks and export of the `Object`/`Proxy` so it is ready to issue/receive messages. This sequence must be completely done within the context of one thread.
  * Creating and sending asynchronous method replies on an `Object` instance.
  * Creating and emitting signals on an `Object` instance.
  * Creating and sending method calls (both synchronously and asynchronously) on an `Proxy` instance. (But it's generally better that our threads use their own exclusive instances of proxy, to minimize shared state and contention.)

sdbus-c++ is designed such that all the above operations are thread-safe also on a connection that is running an event loop (usually in a separate thread) at that time. It's an internal thread safety. For example, a signal arrives and is processed by sdbus-c++ even loop at an appropriate `Proxy` instance, while the user is going to destroy that instance in their application thread. The user cannot explicitly control these situations (or they could, but that would be very limiting and cubersome on the API level).

However, other combinations, that the user invokes explicitly from within more threads are NOT thread-safe in sdbus-c++ by design, and the user should make sure by their design that these cases never occur. For example, destroying an `Object` instance in one thread while emitting a signal on it in another thread is not thread-safe. In this specific case, the user should make sure in their application that all threads stop working with a specific instance before a thread proceeds with deleting that instance.

Multiple layers of sdbus-c++ API
-------------------------------

sdbus-c++ API comes in two layers:

  * [the basic layer](#implementing-the-concatenator-example-using-basic-sdbus-c-api-layer), which is a simple wrapper layer on top of sd-bus, using mechanisms that are native to C++ (e.g. serialization/deserialization of data from messages),
  * [the convenience layer](#implementing-the-concatenator-example-using-convenience-sdbus-c-api-layer), building on top of the basic layer, which aims at alleviating users from unnecessary details and enables them to write shorter, safer, and more expressive code.

sdbus-c++ also ships with a stub generator tool that converts D-Bus IDL in XML format into stub code for the adaptor as well as the proxy part. Hierarchically, these stubs provide yet another layer of convenience (the "stubs layer"), making it possible for D-Bus RPC calls to completely look like native C++ calls on a local object.

An example: Number concatenator
-------------------------------

Let's have an object `/org/sdbuscpp/concatenator` that implements the `org.sdbuscpp.concatenator` interface. The interface exposes the following:

  * a `concatenate` method that takes a collection of integers and a separator string and returns a string that is the concatenation of all integers from the collection using given separator,
  * a `concatenated` signal that is emitted at the end of each successful concatenation.

In the following sections, we will elaborate on the ways of implementing such an object on both the server and the client side.

> **Before running Concatenator example in your system:** In order for your service to be allowed to provide a D-Bus API on system bus, a D-Bus security policy file has to be put in place for that service. Otherwise the service will fail to start (you'll get `[org.freedesktop.DBus.Error.AccessDenied] Failed to request bus name (Permission denied)`, for example). To make the Concatenator example work in your system, [look in this section of systemd configuration](systemd-dbus-config.md#dbus-configuration) for how to name the file, where to place it, how to populate it. For further information, consult [dbus-daemon documentation](https://dbus.freedesktop.org/doc/dbus-daemon.1.html), sections *INTEGRATING SYSTEM SERVICES* and *CONFIGURATION FILE*. As an example used for sdbus-c++ integration tests, you may look at the [policy file for sdbus-c++ integration tests](/tests/integrationtests/files/org.sdbuscpp.integrationtests.conf).

Implementing the Concatenator example using basic sdbus-c++ API layer
---------------------------------------------------------------------

In the basic API layer, we already have abstractions for D-Bus connections, objects and object proxies, with which we can interact via their interface classes (`IConnection`, `IObject`, `IProxy`), but, analogously to the underlying sd-bus C library, we still work on the level of D-Bus messages. We need to

  * create them,
  * serialize/deserialize arguments to/from them (thanks to many overloads of C++ insertion/extraction operators, this is very simple),
  * send them over to the other side.

This is how a simple Concatenator service implemented upon the basic sdbus-c++ API could look like:

### Server side

```c++
#include <sdbus-c++/sdbus-c++.h>
#include <vector>
#include <string>

// Yeah, global variable is ugly, but this is just an example and we want to access
// the concatenator instance from within the concatenate method handler to be able
// to emit signals.
sdbus::IObject* g_concatenator{};

void concatenate(sdbus::MethodCall call)
{
    // Deserialize the collection of numbers from the message
    std::vector<int> numbers;
    call >> numbers;

    // Deserialize separator from the message
    std::string separator;
    call >> separator;

    // Return error if there are no numbers in the collection
    if (numbers.empty())
        throw sdbus::Error("org.sdbuscpp.Concatenator.Error", "No numbers provided");

    std::string result;
    for (auto number : numbers)
    {
        result += (result.empty() ? std::string() : separator) + std::to_string(number);
    }

    // Serialize resulting string to the reply and send the reply to the caller
    auto reply = call.createReply();
    reply << result;
    reply.send();

    // Emit 'concatenated' signal
    const char* interfaceName = "org.sdbuscpp.Concatenator";
    auto signal = g_concatenator->createSignal(interfaceName, "concatenated");
    signal << result;
    g_concatenator->emitSignal(signal);
}

int main(int argc, char *argv[])
{
    // Create D-Bus connection to the system bus and requests name on it.
    const char* serviceName = "org.sdbuscpp.concatenator";
    auto connection = sdbus::createSystemBusConnection(serviceName);

    // Create concatenator D-Bus object.
    const char* objectPath = "/org/sdbuscpp/concatenator";
    auto concatenator = sdbus::createObject(*connection, objectPath);

    g_concatenator = concatenator.get();

    // Register D-Bus methods and signals on the concatenator object, and exports the object.
    const char* interfaceName = "org.sdbuscpp.Concatenator";
    concatenator->registerMethod(interfaceName, "concatenate", "ais", "s", &concatenate);
    concatenator->registerSignal(interfaceName, "concatenated", "s");
    concatenator->finishRegistration();

    // Run the I/O event loop on the bus connection.
    connection->enterEventLoop();
}
```

We establish a D-Bus sytem connection and request `org.sdbuscpp.concatenator` D-Bus name on it. This name will be used by D-Bus clients to find the service. We then create an object with path `/org/sdbuscpp/concatenator` on this connection. We register  interfaces, its methods, signals that the object provides, and, through `finishRegistration()`, export the object (i.e., make it visible to clients) on the bus. Then we need to make sure to run the event loop upon the connection, which handles all incoming, outgoing and other requests.

The callback for any D-Bus object method on this level is any callable of signature `void(sdbus::MethodCall call)`. The `call` parameter is the incoming method call message. We need to deserialize our method input arguments from it. Then we can invoke the logic of the method and get the results. Then for the given `call`, we create a `reply` message, pack results into it and send it back to the caller through `send()`. (If we had a void-returning method, we'd just send an empty `reply` back.) We also fire a signal with the results. To do this, we need to create a signal message via object's `createSignal()`, serialize the results into it, and then send it out to subscribers by invoking object's `emitSignal()`.

Please note that we can create and destroy D-Bus objects on a connection dynamically, at any time during runtime, even while there is an active event loop upon the connection. So managing D-Bus objects' lifecycle (creating, exporting and destroying D-Bus objects) is completely thread-safe.

### Client side

```c++
#include <sdbus-c++/sdbus-c++.h>
#include <vector>
#include <string>
#include <iostream>
#include <unistd.h>

void onConcatenated(sdbus::Signal& signal)
{
    std::string concatenatedString;
    signal >> concatenatedString;

    std::cout << "Received signal with concatenated string " << concatenatedString << std::endl;
}

int main(int argc, char *argv[])
{
    // Create proxy object for the concatenator object on the server side. Since here
    // we are creating the proxy instance without passing connection to it, the proxy
    // will create its own connection automatically, and it will be system bus connection.
    const char* destinationName = "org.sdbuscpp.concatenator";
    const char* objectPath = "/org/sdbuscpp/concatenator";
    auto concatenatorProxy = sdbus::createProxy(destinationName, objectPath);

    // Let's subscribe for the 'concatenated' signals
    const char* interfaceName = "org.sdbuscpp.Concatenator";
    concatenatorProxy->registerSignalHandler(interfaceName, "concatenated", &onConcatenated);
    concatenatorProxy->finishRegistration();

    std::vector<int> numbers = {1, 2, 3};
    std::string separator = ":";

    // Invoke concatenate on given interface of the object
    {
        auto method = concatenatorProxy->createMethodCall(interfaceName, "concatenate");
        method << numbers << separator;
        auto reply = concatenatorProxy->callMethod(method);
        std::string result;
        reply >> result;
        assert(result == "1:2:3");
    }

    // Invoke concatenate again, this time with no numbers and we shall get an error
    {
        auto method = concatenatorProxy->createMethodCall(interfaceName, "concatenate");
        method << std::vector<int>() << separator;
        try
        {
            auto reply = concatenatorProxy->callMethod(method);
            assert(false);
        }
        catch(const sdbus::Error& e)
        {
            std::cerr << "Got concatenate error " << e.getName() << " with message " << e.getMessage() << std::endl;
        }
    }

    // Give sufficient time to receive 'concatenated' signal from the first concatenate invocation
    sleep(1);

    return 0;
}
```

In simple cases, we don't need to create D-Bus connection explicitly for our proxies. Unless a connection is provided to a proxy object explicitly via factory parameter, the proxy will create a connection of his own, and it will be a system bus connection. This is the case in the example above. (This approach is not scalable and resource-saving if we have plenty of proxies; see section [Working with D-Bus connections](#working-with-d-bus-connections-in-sdbus-c) for elaboration.) So, in the example, we create a proxy for object `/org/sdbuscpp/concatenator` publicly available at bus `org.sdbuscpp.concatenator`. We register signal handlers, if any, and finish the registration, making the proxy ready for use.

The callback for a D-Bus signal handler on this level is any callable of signature `void(sdbus::Signal& signal)`. The one and only parameter `signal` is the incoming signal message. We need to deserialize arguments from it, and then we can do our business logic with it.

Subsequently, we invoke two RPC calls to object's `concatenate()` method. We create a method call message by invoking proxy's `createMethodCall()`. We serialize method input arguments into it, and make a synchronous call via proxy's `callMethod()`. As a return value we get the reply message as soon as it arrives. We deserialize return values from that message, and further use it in our program. The second `concatenate()` RPC call is done with invalid arguments, so we get a D-Bus error reply from the service, which as we can see is manifested via `sdbus::Error` exception being thrown.

Please note that we can create and destroy D-Bus object proxies dynamically, at any time during runtime, even when they share a common D-Bus connection and there is an active event loop upon the connection. So managing D-Bus object proxies' lifecycle (creating and destroying D-Bus object proxies) is completely thread-safe.

### Working with D-Bus connections in sdbus-c++

The design of D-Bus connections in sdbus-c++ allows for certain flexibility and enables users to choose simplicity over scalability or scalability (at a finer granularity of user's choice) at the cost of slightly decreased simplicity.

How shall we use connections in relation to D-Bus objects and object proxies?

A D-Bus connection is represented by a `IConnection` instance. Each connection needs an event loop being run upon it. So it needs a thread handling the event loop. This thread serves all incoming and outgoing messages and all communication towards D-Bus daemon. One process can have one but also multiple D-Bus connections (we just have to make certain that the connections with assigned bus names don't share a common name; the name must be unique).

A typical use case for most services is **one** D-Bus connection in the application. The application runs event loop on that connection. When creating objects or proxies, the application provides reference of that connection to those objects and proxies. This means all these objects and proxies share the same connection. This is nicely scalable, because with whatever number of objects or proxies, there is only one connection and one event loop thread. Yet, services that provide objects at various bus names have to create and maintain multiple D-Bus connections, each with the unique bus name.

The connection is thread-safe and objects and proxies can invoke operations on it from multiple threads simultaneously, but the operations are serialized. This means, for example, that if an object's callback for an incoming remote method call is going to be invoked in an event loop thread, and in another thread we use a proxy to call remote method in another process, the threads are contending and only one can go on while the other must wait and can only proceed after the first one has finished, because both are using a shared resource -- the connection.

We should bear that in mind when designing more complex, multi-threaded services with high parallelism. If we have undesired contention on a connection, creating a specific, dedicated connection for a hot spot helps to increase concurrency. sdbus-c++ provides us freedom to create as many connections as we want and assign objects and proxies to those connections at our will. We, as application developers, choose whatever approach is more suitable to us at quite a fine granularity.

So, more technically, how can we use connections from the server and the client perspective?

#### Using D-Bus connections on the server side

On the **server** side, we generally need to create D-Bus objects and publish their APIs. For that we first need a connection with a unique bus name. We need to create the D-Bus connection manually ourselves, request bus name on it, and manually launch its event loop:

  * either in a blocking way, through `enterEventLoop()`,
  * or in a non-blocking async way, through `enterEventLoopAsync()`,
  * or, when we have our own implementation of an event loop (e.g. we are using sd-event event loop), we can ask the connection for its underlying fd, I/O events and timeouts through `getEventLoopPollData()` and use that data in our event loop mechanism.

Of course, at any time before or after running the event loop on the connection, we can create and "hook", as well as remove, objects and proxies upon that connection.

#### Using D-Bus connections on the client side

On the **client** side we have more options when creating D-Bus proxies. That corresponds to three overloads of the `createProxy()` factory:

  * In case we (the application) already maintain a D-Bus connection, e.g. because we a D-Bus service anyway, the simple and typical approach is to create proxy upon that connection. The proxy will share the connection with others. So we pass connection reference to proxy factory. With this approach we must of course ensure that the connection exists as long as the proxy exists.

  * Or -- and this is typical when we have a simple D-Bus client application -- we have another option: we let proxy maintain its own connection (and an associated thread):

    * We either create the connection ourselves and `std::move` it to the proxy object factory. The proxy becomes an owner of this connection, and will run the event loop on that connection. This had the advantage that we may choose the type of connection (system, session, remote).

    * Or we don't bother about any connection at all when creating a proxy (the factory overload with no connection parameter). Under the hood, the proxy creates its own *system bus* connection, creates a separate thread and runs an event loop in it. This is **the simplest approach** for non-complex D-Bus clients. For more complex ones, with big number of proxies, this hurts scalability but may improve concurrency (see discussion higher above), so we should make a conscious choice.

#### Stopping I/O event loops graciously

A connection with an asynchronous event loop (i.e. one initiated through `enterEventLoopAsync()`) will stop and join its event loop thread automatically in its destructor. An event loop that blocks in the synchronous `enterEventLoop()` call can be unblocked through `leaveEventLoop()` call on the respective bus connection issued from a different thread or from an OS signal handler.

Implementing the Concatenator example using convenience sdbus-c++ API layer
---------------------------------------------------------------------------

One of the major sdbus-c++ design goals is to make the sdbus-c++ API easy to use correctly, and hard to use incorrectly.

The convenience API layer abstracts the concept of underlying D-Bus messages away completely. It abstracts away D-Bus signatures. The interface uses small, focused functions, with a few parameters only, to form a chained function statement that reads like a human language sentence. To achieve that, sdbus-c++ utilizes the power of the C++ type system, which deduces and resolves a lot of things at compile time, and the run-time performance cost compared to the basic layer is close to zero.

Thus, in the end of the day, the code written using the convenience API is:

  - more expressive,
  - at a higher level of abstraction (closer to the abstraction level of the problem being solved),
  - significantly shorter,
  - almost as fast as one written using the basic API layer.

The code written using this layer expresses in a declarative way *what* it does, rather than *how*. Let's look at code samples.

### Server side

```c++
#include <sdbus-c++/sdbus-c++.h>
#include <vector>
#include <string>

int main(int argc, char *argv[])
{
    // Create D-Bus connection to the system bus and requests name on it.
    const char* serviceName = "org.sdbuscpp.concatenator";
    auto connection = sdbus::createSystemBusConnection(serviceName);

    // Create concatenator D-Bus object.
    const char* objectPath = "/org/sdbuscpp/concatenator";
    auto concatenator = sdbus::createObject(*connection, objectPath);
    
    auto concatenate = [&concatenator](const std::vector<int> numbers, const std::string& separator)
    {
        // Return error if there are no numbers in the collection
        if (numbers.empty())
            throw sdbus::Error("org.sdbuscpp.Concatenator.Error", "No numbers provided");

        std::string result;
        for (auto number : numbers)
        {
            result += (result.empty() ? std::string() : separator) + std::to_string(number);
        }

        // Emit 'concatenated' signal
        const char* interfaceName = "org.sdbuscpp.Concatenator";
        concatenator->emitSignal("concatenated").onInterface(interfaceName).withArguments(result);

        return result;
    };

    // Register D-Bus methods and signals on the concatenator object, and exports the object.
    const char* interfaceName = "org.sdbuscpp.Concatenator";
    concatenator->registerMethod("concatenate").onInterface(interfaceName).implementedAs(std::move(concatenate));
    concatenator->registerSignal("concatenated").onInterface(interfaceName).withParameters<std::string>();
    concatenator->finishRegistration();

    // Run the loop on the connection.
    connection->enterEventLoop();
}
```

### Client side

```c++
#include <sdbus-c++/sdbus-c++.h>
#include <vector>
#include <string>
#include <iostream>
#include <unistd.h>

void onConcatenated(const std::string& concatenatedString)
{
    std::cout << "Received signal with concatenated string " << concatenatedString << std::endl;
}

int main(int argc, char *argv[])
{
    // Create proxy object for the concatenator object on the server side
    const char* destinationName = "org.sdbuscpp.concatenator";
    const char* objectPath = "/org/sdbuscpp/concatenator";
    auto concatenatorProxy = sdbus::createProxy(destinationName, objectPath);

    // Let's subscribe for the 'concatenated' signals
    const char* interfaceName = "org.sdbuscpp.Concatenator";
    concatenatorProxy->uponSignal("concatenated").onInterface(interfaceName).call([](const std::string& str){ onConcatenated(str); });
    concatenatorProxy->finishRegistration();

    std::vector<int> numbers = {1, 2, 3};
    std::string separator = ":";

    // Invoke concatenate on given interface of the object
    {
        std::string concatenatedString;
        concatenatorProxy->callMethod("concatenate").onInterface(interfaceName).withArguments(numbers, separator).storeResultsTo(concatenatedString);
        assert(concatenatedString == "1:2:3");
    }

    // Invoke concatenate again, this time with no numbers and we shall get an error
    {
        try
        {
            concatenatorProxy->callMethod("concatenate").onInterface(interfaceName).withArguments(std::vector<int>(), separator);
            assert(false);
        }
        catch(const sdbus::Error& e)
        {
            std::cerr << "Got concatenate error " << e.getName() << " with message " << e.getMessage() << std::endl;
        }
    }

    // Give sufficient time to receive 'concatenated' signal from the first concatenate invocation
    sleep(1);

    return 0;
}
```

When registering methods, calling methods or emitting signals, multiple lines of code have shrunk into simple one-liners. Signatures of provided callbacks are introspected and types of provided arguments are deduced at compile time, so the D-Bus signatures as well as serialization and deserialization of arguments to and from D-Bus messages are generated for us completely by the compiler.

sdbus-c++ users shall prefer the convenience API to the lower level, basic API. When feasible, using generated adaptor and proxy stubs is even better. These stubs provide yet another, higher API level built on top of the convenience API. They are described in the following section.

> **_Note_:** By default, signal callback handlers are not invoked (i.e., the signal is silently dropped) if there is a signal signature mismatch. If clients want to be informed of such situations, they can prepend `const sdbus::Error*` parameter to their signal callback handler's parameter list. This argument will be `nullptr` in normal cases, and will provide access to the corresponding `sdbus::Error` object in case of deserialization failures. An example of a handler with the signature (`int`) different from the real signal contents (`string`):
> ```c++
>     void onConcatenated(const sdbus::Error* e, int wrongParameter)
>     {
>         assert(e);
>         assert(e->getMessage() == "Failed to deserialize a int32 value");
>     }
> ```

> **_Tip_:** When registering a D-Bus object, we can additionally provide names of input and output parameters of its methods and names of parameters of its signals. When the object is introspected, these names are listed in the resulting introspection XML, which improves the description of object's interfaces:
> ```c++
>    concatenator->registerMethod("concatenate")
>                 .onInterface(interfaceName)
>                 .withInputParamNames("numbers", "separator")
>                 .withOutputParamNames("concatenatedString")
>                 .implementedAs(&concatenate);
>    concatenator->registerSignal("concatenated")
>                 .onInterface(interfaceName)
>                 .withParameters<std::string>("concatenatedString");
> ```

### Accessing a corresponding D-Bus message

The convenience API hides away the level of D-Bus messages. But the messages carry with them additional information that may need in some implementations. For example, a name of a method call sender; or info on credentials. Is there a way to access a corresponding D-Bus message in a high-level callback handler?

Yes, there is -- we can access the corresponding D-Bus message in:

* method implementation callback handlers (server side),
* property set implementation callback handlers (server side),
* signal callback handlers (client side).

Both `IObject` and `IProxy` provide the `getCurrentlyProcessedMessage()` method. This method is meant to be called from within a callback handler. It returns a pointer to the corresponding D-Bus message that caused invocation of the handler. The pointer is only valid (dereferencable) as long as the flow of execution does not leave the callback handler. When called from other contexts/threads, the pointer may be both zero or non-zero, and its dereferencing is undefined behavior.

An excerpt of the above example of concatenator modified to print out a name of the sender of method call:

```c++
    auto concatenate = [&concatenator](const std::vector<int> numbers, const std::string& separator)
    {
        const auto* methodCallMsg = concatenator->getCurrentlyProcessedMessage();
        std::cout << "Sender of this method call: " << methodCallMsg.getSender() << std::endl;

        /*...*/
    };
```

Implementing the Concatenator example using sdbus-c++-generated stubs
---------------------------------------------------------------------

sdbus-c++ ships with the native stub generator tool called `sdbus-c++-xml2cpp`. The tool is very similar to `dbusxx-xml2cpp` tool that comes with the dbus-c++ library.

The generator tool takes D-Bus XML IDL description of D-Bus interfaces on its input, and can be instructed to generate one or both of these: an adaptor header file for use on the server side, and a proxy header file for use on the client side. Like this:

```bash
sdbus-c++-xml2cpp database-bindings.xml --adaptor=database-server-glue.h --proxy=database-client-glue.h
```

The adaptor header file contains classes that can be used to implement interfaces described in the IDL (these classes represent object interfaces). The proxy header file contains classes that can be used to make calls to remote objects (these classes represent remote object interfaces).

### XML description of the Concatenator interface

As an example, let's look at an XML description of our Concatenator's interfaces.

```xml
<?xml version="1.0" encoding="UTF-8"?>

<node name="/org/sdbuscpp/concatenator">
    <interface name="org.sdbuscpp.Concatenator">
        <method name="concatenate">
            <arg type="ai" name="numbers" direction="in" />
            <arg type="s" name="separator" direction="in" />
            <arg type="s" name="concatenatedString" direction="out" />
        </method>
        <signal name="concatenated">
            <arg type="s" name="concatenatedString" />
        </signal>
    </interface>
</node>
```

After running this through the stubs generator, we get the stub code that is described in the following two subsections.

### concatenator-server-glue.h

For each interface in the XML IDL file the generator creates one class that represents it. The class is de facto an interface which shall be implemented by the class inheriting it. The class' constructor takes care of registering all methods, signals and properties. For each D-Bus method there is a pure virtual member function. These pure virtual functions must be implemented in the child class. For each signal, there is a public function member that emits this signal.

```cpp
/*
 * This file was automatically generated by sdbus-c++-xml2cpp; DO NOT EDIT!
 */

#ifndef __sdbuscpp__concatenator_server_glue_h__adaptor__H__
#define __sdbuscpp__concatenator_server_glue_h__adaptor__H__

#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <tuple>

namespace org {
namespace sdbuscpp {

class Concatenator_adaptor
{
public:
    static constexpr const char* INTERFACE_NAME = "org.sdbuscpp.Concatenator";

protected:
    Concatenator_adaptor(sdbus::IObject& object)
        : object_(object)
    {
        object_.registerMethod("concatenate").onInterface(INTERFACE_NAME).withInputParamNames("numbers", "separator").withOutputParamNames("concatenatedString").implementedAs([this](const std::vector<int32_t>& numbers, const std::string& separator){ return this->concatenate(numbers, separator); });
        object_.registerSignal("concatenated").onInterface(INTERFACE_NAME).withParameters<std::string>("concatenatedString");
    }

    ~Concatenator_adaptor() = default;

public:
    void emitConcatenated(const std::string& concatenatedString)
    {
        object_.emitSignal("concatenated").onInterface(INTERFACE_NAME).withArguments(concatenatedString);
    }

private:
    virtual std::string concatenate(const std::vector<int32_t>& numbers, const std::string& separator) = 0;

private:
    sdbus::IObject& object_;
};

}} // namespaces

#endif
```

### concatenator-client-glue.h

Analogously to the adaptor classes described above, there is one class generated for one interface in the XML IDL file. The class is de facto a proxy to the concrete single interface of a remote object. For each D-Bus signal there is a pure virtual member function whose body must be provided in a child class. For each method, there is a public function member that calls the method remotely.

```cpp
/*
 * This file was automatically generated by sdbus-c++-xml2cpp; DO NOT EDIT!
 */

#ifndef __sdbuscpp__concatenator_client_glue_h__proxy__H__
#define __sdbuscpp__concatenator_client_glue_h__proxy__H__

#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <tuple>

namespace org {
namespace sdbuscpp {

class Concatenator_proxy
{
public:
    static constexpr const char* INTERFACE_NAME = "org.sdbuscpp.Concatenator";

protected:
    Concatenator_proxy(sdbus::IProxy& proxy)
        : proxy_(proxy)
    {
        proxy_.uponSignal("concatenated").onInterface(INTERFACE_NAME).call([this](const std::string& concatenatedString){ this->onConcatenated(concatenatedString); });
    }

    ~Concatenator_proxy() = default;

    virtual void onConcatenated(const std::string& concatenatedString) = 0;

public:
    std::string concatenate(const std::vector<int32_t>& numbers, const std::string& separator)
    {
        std::string result;
        proxy_.callMethod("concatenate").onInterface(INTERFACE_NAME).withArguments(numbers, separator).storeResultsTo(result);
        return result;
    }

private:
    sdbus::IProxy& proxy_;
};

}} // namespaces

#endif
```

### Providing server implementation based on generated adaptors

To implement a D-Bus object that implements all its D-Bus interfaces, we now need to create a class representing the D-Bus object. This class must inherit from all corresponding `*_adaptor` classes (a-ka object interfaces, because these classes are as-if interfaces) and implement all pure virtual member functions.

How do we do that technically? Simply, our object class just needs to inherit from `AdaptorInterfaces` variadic template class. We fill its template arguments with a list of all generated interface classes. The `AdaptorInterfaces` is a convenience class that hides a few boiler-plate details. For example, in its constructor, it creates an `Object` instance, and it takes care of proper initialization of all adaptor superclasses.

In our object class we need to:

  * Give an implementation to the D-Bus object's methods by overriding corresponding virtual functions,
  * call `registerAdaptor()` in the constructor, which makes the adaptor (the D-Bus object underneath it) available for remote calls,
  * call `unregisterAdaptor()`, which, conversely, unregisters the adaptor from the bus.

Calling `registerAdaptor()` and `unregisterAdaptor()` was not necessary in previous sdbus-c++ versions, as it was handled by the parent class. This was convenient, but suffered from a potential pure virtual function call issue. Only the class that implements virtual functions can do the registration, hence this slight inconvenience on user's shoulders.

```cpp
#include <sdbus-c++/sdbus-c++.h>
#include "concatenator-server-glue.h"

class Concatenator : public sdbus::AdaptorInterfaces<org::sdbuscpp::Concatenator_adaptor /*, more adaptor classes if there are more interfaces*/>
{
public:
    Concatenator(sdbus::IConnection& connection, std::string objectPath)
        : AdaptorInterfaces(connection, std::move(objectPath))
    {
        registerAdaptor();
    }

    ~Concatenator()
    {
        unregisterAdaptor();
    }

protected:
    std::string concatenate(const std::vector<int32_t>& numbers, const std::string& separator) override
    {
        // Return error if there are no numbers in the collection
        if (numbers.empty())
            throw sdbus::Error("org.sdbuscpp.Concatenator.Error", "No numbers provided");

        // Concatenate the numbers
        std::string result;
        for (auto number : numbers)
        {
            result += (result.empty() ? std::string() : separator) + std::to_string(number);
        }

        // Emit the 'concatenated' signal with the resulting string
        emitConcatenated(result);

        // Return the resulting string
        return result;
    }
};
```

> **_Tip_:** By inheriting from `sdbus::AdaptorInterfaces`, we get access to the protected `getObject()` method. We can call this method inside our adaptor implementation class to access the underlying `IObject` object.

That's it. We now have an implementation of a D-Bus object implementing `org.sdbuscpp.Concatenator` interface. Let's now create a service publishing the object.

```cpp
#include "Concatenator.h"

int main(int argc, char *argv[])
{
    // Create D-Bus connection to the system bus and requests name on it.
    const char* serviceName = "org.sdbuscpp.concatenator";
    auto connection = sdbus::createSystemBusConnection(serviceName);

    // Create concatenator D-Bus object.
    const char* objectPath = "/org/sdbuscpp/concatenator";
    Concatenator concatenator(*connection, objectPath);

    // Run the loop on the connection.
    connection->enterEventLoop();
}
```

Now we have a service with a unique bus name and a D-Bus object available on it. Let's write a client.

### Providing client implementation based on generated proxies

To implement a proxy for a remote D-Bus object, we shall create a class representing the proxy object. This class must inherit from all corresponding `*_proxy` classes (a-ka remote object interfaces, because these classes are as-if interfaces) and -- if applicable -- implement all pure virtual member functions.

How do we do that technically? Simply, our proxy class just needs to inherit from `ProxyInterfaces` variadic template class. We fill its template arguments with a list of all generated interface classes. The `ProxyInterfaces` is a convenience class that hides a few boiler-plate details. For example, in its constructor, it can create a `Proxy` instance for us, and it takes care of proper initialization of all generated interface superclasses.

In our proxy class we need to:

  * Give an implementation to signal handlers and asynchronous method reply handlers (if any) by overriding corresponding virtual functions,
  * call `registerProxy()` in the constructor, which makes the proxy (the D-Bus proxy object underneath it) ready to receive signals and async call replies,
  * call `unregisterProxy()`, which, conversely, unregisters the proxy from the bus.

Calling `registerProxy()` and `unregisterProxy()` was not necessary in previous versions of sdbus-c++, as it was handled by the parent class. This was convenient, but suffered from a potential pure virtual function call issue. Only the class that implements virtual functions can do the registration, hence this slight inconvenience on user's shoulders.

```cpp
#include <sdbus-c++/sdbus-c++.h>
#include "concatenator-client-glue.h"

class ConcatenatorProxy : public sdbus::ProxyInterfaces<org::sdbuscpp::Concatenator_proxy /*, more proxy classes if there are more interfaces*/>
{
public:
    ConcatenatorProxy(std::string destination, std::string objectPath)
        : ProxyInterfaces(std::move(destination), std::move(objectPath))
    {
        registerProxy();
    }

    ~ConcatenatorProxy()
    {
        unregisterProxy();
    }

protected:
    void onConcatenated(const std::string& concatenatedString) override
    {
        std::cout << "Received signal with concatenated string " << concatenatedString << std::endl;
    }
};
```

> **_Tip_:** By inheriting from `sdbus::ProxyInterfaces`, we get access to the protected `getProxy()` method. We can call this method inside our proxy implementation class to access the underlying `IProxy` object.

In the above example, a proxy is created that creates and maintains its own system bus connection. However, there are `ProxyInterfaces` class template constructor overloads that also take the connection from the user as the first parameter, and pass that connection over to the underlying proxy. The connection instance is used by all interfaces listed in the `ProxyInterfaces` template parameter list.

Note however that there are multiple `ProxyInterfaces` constructor overloads, and they differ in how the proxy behaves towards the D-Bus connection. These overloads precisely map the `sdbus::createProxy` overloads, as they are actually implemented on top of them. See [Proxy and D-Bus connection](#Proxy-and-D-Bus-connection) for more info. We can even create a `IProxy` instance on our own, and inject it into our proxy class -- there is a constructor overload for it in `ProxyInterfaces`. This can help if we need to provide mocked implementations in our unit tests.

Now let's use this proxy to make remote calls and listen to signals in a real application.

```cpp
#include "ConcatenatorProxy.h"
#include <unistd.h>

int main(int argc, char *argv[])
{
    // Create proxy object for the concatenator object on the server side
    const char* destinationName = "org.sdbuscpp.concatenator";
    const char* objectPath = "/org/sdbuscpp/concatenator";
    ConcatenatorProxy concatenatorProxy(destinationName, objectPath);

    std::vector<int> numbers = {1, 2, 3};
    std::string separator = ":";

    // Invoke concatenate with some numbers
    auto concatenatedString = concatenatorProxy.concatenate(numbers, separator);
    assert(concatenatedString == "1:2:3");

    // Invoke concatenate again, this time with no numbers and we shall get an error
    try
    {
        auto concatenatedString = concatenatorProxy.concatenate(std::vector<int>(), separator);
        assert(false);
    }
    catch(const sdbus::Error& e)
    {
        std::cerr << "Got concatenate error " << e.getName() << " with message " << e.getMessage() << std::endl;
    }

    // Give sufficient time to receive 'concatenated' signal from the first concatenate invocation
    sleep(1);

    return 0;
}
```

### Accessing a corresponding D-Bus message

Simply combine `getObject()`/`getProxy()` and `getCurrentlyProcessedMessage()` methods. Both were already discussed above. An example:

```c++
class Concatenator : public sdbus::AdaptorInterfaces</*...*/>
{
public:
    /*...*/

protected:
    std::string concatenate(const std::vector<int32_t>& numbers, const std::string& separator) override
    {
        const auto* methodCallMsg = getObject().getCurrentlyProcessedMessage();
        std::cout << "Sender of this method call: " << methodCallMsg.getSender() << std::endl;

        /*...*/
    }
};
```

Asynchronous server-side methods
--------------------------------

So far in our tutorial, we have only considered simple server methods that are executed in a synchronous way. Sometimes the method call may take longer, however, and we don't want to block (potentially starve) other clients (whose requests may take relative short time). The solution is to execute the D-Bus methods asynchronously, and return the control quickly back to the D-Bus dispatching thread. sdbus-c++ provides API supporting async methods, and gives users the freedom to come up with their own concrete implementation mechanics (one worker thread? thread pool? ...).

### Using basic sdbus-c++ API

This is how the concatenate method would look like if wrote it as an asynchronous D-Bus method using the basic, lower-level API of sdbus-c++:

```c++
void concatenate(sdbus::MethodCall call)
{
    // Deserialize the collection of numbers from the message
    std::vector<int> numbers;
    call >> numbers;

    // Deserialize separator from the message
    std::string separator;
    call >> separator;

    // Launch a thread for async execution...
    std::thread([numbers = std::move(numbers), separator = std::move(separator), call = std::move(call)]()
    {
        // Return error if there are no numbers in the collection
        if (numbers.empty())
        {
            // Let's send the error reply message back to the client
            auto reply = call.createErrorReply({"org.sdbuscpp.Concatenator.Error", "No numbers provided"});
            reply.send();
            return;
        }

        std::string result;
        for (auto number : numbers)
        {
            result += (result.empty() ? std::string() : separator) + std::to_string(number);
        }

        // Let's send the reply message back to the client
        auto reply = call.createReply();
        reply << result;
        reply.send();

        // Emit 'concatenated' signal (creating and emitting signals is thread-safe)
        const char* interfaceName = "org.sdbuscpp.Concatenator";
        auto signal = g_concatenator->createSignal(interfaceName, "concatenated");
        signal << result;
        g_concatenator->emitSignal(signal);
    }).detach();
}
```

There are a few slight differences compared to the synchronous version. Notice that we `std::move` the `call` message to the worker thread (btw we might also do input arguments deserialization in the worker thread, we don't have to do it in the current thread and then move input arguments to the worker thread...). We need the `call` message there to create the reply message once we have the (normal or error) result. Creating and sending replies, as well as creating and emitting signals is thread-safe by design. Also notice that, unlike in sync methods, sending back errors cannot be done by throwing `Error`, since we are now in the context of the worker thread, not that of the D-Bus dispatcher thread. Instead, we pass the `Error` object to the `createErrorReply()` method of the call message (this way of sending back errors, in addition to throwing, we can actually use also in classic synchronous D-Bus methods).

Method callback signature is the same in sync and async version. That means sdbus-c++ doesn't care how we execute our D-Bus method. We might very well in run-time decide whether we execute it synchronously, or whether (perhaps in case of longer, more complex calculations) we move the execution to a worker thread.

### Convenience API

Callbacks of async methods based on convenience sdbus-c++ API have slightly different signature. They take a result object parameter in addition to other input parameters. The requirements are:

  * The result holder is of type `Result<Types...>&&`, where `Types...` is a list of method output argument types.
  * The result object must be the first physical parameter of the callback taken by r-value ref. `Result` class template is move-only.
  * The callback itself is physically a void-returning function.
  * Method input arguments are taken by value rathern than by const ref, because we usually want to `std::move` them to the worker thread. Moving is usually a lot cheaper than copying, and it's idiomatic. For non-movable types, this falls back to copying.

So the concatenate callback signature would change from `std::string concatenate(const std::vector<int32_t>& numbers, const std::string& separator)` to `void concatenate(sdbus::Result<std::string>&& result, std::vector<int32_t> numbers, std::string separator)`:

```c++
void concatenate(sdbus::Result<std::string>&& result, std::vector<int32_t> numbers, std::string separator) override
{
    // Launch a thread for async execution...
    std::thread([this, methodResult = std::move(result), numbers = std::move(numbers), separator = std::move(separator)]()
    {
        // Return error if there are no numbers in the collection
        if (numbers.empty())
        {
            // Let's send the error reply message back to the client
            methodResult.returnError({"org.sdbuscpp.Concatenator.Error", "No numbers provided"});
            return;
        }

        std::string result;
        for (auto number : numbers)
        {
            result += (result.empty() ? std::string() : separator) + std::to_string(number);
        }

        // Let's send the reply message back to the client
        methodResult.returnResults(result);

        // Emit the 'concatenated' signal with the resulting string
        this->emitConcatenated(result);
    }).detach();
}
```

The `Result` is a convenience class that represents a future method result, and it is where we write the results (`returnResults()`) or an error (`returnError()`) which we want to send back to the client.

Registraion (`implementedAs()`) doesn't change. Nothing else needs to change.

### Marking server-side async methods in the IDL

sdbus-c++ stub generator can generate stub code for server-side async methods. We just need to annotate the method with `org.freedesktop.DBus.Method.Async`. The annotation element value must be either `server` (async method on server-side only) or `clientserver` (async method on both client- and server-side):

```xml
<?xml version="1.0" encoding="UTF-8"?>

<node name="/org/sdbuscpp/concatenator">
    <interface name="org.sdbuscpp.Concatenator">
        <method name="concatenate">
            <annotation name="org.freedesktop.DBus.Method.Async" value="server" />
            <arg type="ai" name="numbers" direction="in" />
            <arg type="s" name="separator" direction="in" />
            <arg type="s" name="concatenatedString" direction="out" />
        </method>
        <signal name="concatenated">
            <arg type="s" name="concatenatedString" />
        </signal>
    </interface>
</node>
```

For a real example of a server-side asynchronous D-Bus method, please look at sdbus-c++ [stress tests](/tests/stresstests).

Asynchronous client-side methods
--------------------------------

sdbus-c++ also supports asynchronous approach at the client (the proxy) side. With this approach, we can issue a D-Bus method call without blocking current thread's execution while waiting for the reply. We go on doing other things, and when the reply comes, a given callback is invoked within the context of the D-Bus dispatcher thread.

### Lower-level API

Considering the Concatenator example based on lower-level API, if we wanted to call `concatenate` in an async way, we'd have to pass a callback to the proxy when issuing the call, and that callback gets invoked when the reply arrives:

```c++
int main(int argc, char *argv[])
{
    /* ...  */

    auto callback = [](MethodReply& reply, const sdbus::Error* error)
    {
        if (error == nullptr) // No error
        {
            std::string result;
            reply >> result;
            std::cout << "Got concatenate result: " << result << std::endl;
        }
        else // We got a D-Bus error...
        {
            std::cerr << "Got concatenate error " << error->getName() << " with message " << error->getMessage() << std::endl;
        }
    }

    // Invoke concatenate on given interface of the object
    {
        auto method = concatenatorProxy->createMethodCall(interfaceName, "concatenate");
        method << numbers << separator;
        concatenatorProxy->callMethod(method, callback);
        // When the reply comes, we shall get "Got concatenate result 1:2:3" on the standard output
    }

    // Invoke concatenate again, this time with no numbers and we shall get an error
    {
        auto method = concatenatorProxy->createMethodCall(interfaceName, "concatenate");
        method << std::vector<int>() << separator;
        concatenatorProxy->callMethod(method, callback);
        // When the reply comes, we shall get concatenation error message on the standard error output
    }

    /* ... */

    return 0;
}
```

The callback is a void-returning function taking two arguments: a reference to the reply message, and a pointer to the prospective `sdbus::Error` instance. Zero `Error` pointer means that no D-Bus error occurred while making the call, and the reply message contains valid reply. Non-zero `Error` pointer, however, points to the valid `Error` instance, meaning that an error occurred. Error name and message can then be read out by the client from that instance.

### Convenience API

On the convenience API level, the call statement starts with `callMethodAsync()`, and ends with `uponReplyInvoke()` that takes a callback handler. The callback is a void-returning function that takes at least one argument: pointer to the `sdbus::Error` instance. All subsequent arguments shall exactly reflect the D-Bus method output arguments. A concatenator example:

```c++
int main(int argc, char *argv[])
{
    /* ...  */

    auto callback = [](const sdbus::Error* error, const std::string& concatenatedString)
    {
        if (error == nullptr) // No error
            std::cout << "Got concatenate result: " << concatenatedString << std::endl;
        else // We got a D-Bus error...
            std::cerr << "Got concatenate error " << error->getName() << " with message " << error->getMessage() << std::endl;
    }

    // Invoke concatenate on given interface of the object
    {
        concatenatorProxy->callMethodAsync("concatenate").onInterface(interfaceName).withArguments(numbers, separator).uponReplyInvoke(callback);
        // When the reply comes, we shall get "Got concatenate result 1:2:3" on the standard output
    }

    // Invoke concatenate again, this time with no numbers and we shall get an error
    {
        concatenatorProxy->callMethodAsync("concatenate").onInterface(interfaceName).withArguments(std::vector<int>{}, separator).uponReplyInvoke(callback);
        // When the reply comes, we shall get concatenation error message on the standard error output
    }

    /* ... */

    return 0;
}
```

When the `Error` pointer is zero, it means that no D-Bus error occurred while making the call, and subsequent arguments are valid D-Bus method return values. Non-zero `Error` pointer, however, points to the valid `Error` instance, meaning that an error occurred during the call (and subsequent arguments are simply default-constructed). Error name and message can then be read out by the client from `Error` instance.

### Marking client-side async methods in the IDL

sdbus-c++ stub generator can generate stub code for client-side async methods. We just need to annotate the method with `org.freedesktop.DBus.Method.Async`. The annotation element value must be either `client` (async on the client-side only) or `clientserver` (async method on both client- and server-side):

```xml
<?xml version="1.0" encoding="UTF-8"?>

<node name="/org/sdbuscpp/concatenator">
    <interface name="org.sdbuscpp.Concatenator">
        <method name="concatenate">
            <annotation name="org.freedesktop.DBus.Method.Async" value="client" />
            <arg type="ai" name="numbers" direction="in" />
            <arg type="s" name="separator" direction="in" />
            <arg type="s" name="concatenatedString" direction="out" />
        </method>
        <signal name="concatenated">
            <arg type="s" name="concatenatedString" />
        </signal>
    </interface>
</node>
```

For each client-side async method, a corresponding `on<MethodName>Reply` pure virtual function, where `<MethodName>` is the capitalized D-Bus method name, is generated in the generated proxy class. This function is the callback invoked when the D-Bus method reply arrives, and must be provided a body by overriding it in the implementation class.

So in the specific example above, the stub generator will generate a `Concatenator_proxy` class similar to one shown in a [dedicated section above](#concatenator-client-glueh), with the difference that it will also generate an additional `virtual void onConcatenateReply(const sdbus::Error* error, const std::string& concatenatedString);` method, which we shall override in derived `ConcatenatorProxy`.

For a real example of a client-side asynchronous D-Bus method, please look at sdbus-c++ [stress tests](/tests/stresstests).

## Method call timeout

Annotate the element with `org.freedesktop.DBus.Method.Timeout` in order to specify the timeout value for the method call. The value should be a number of microseconds or number with duration literal (`us`/`ms`/`s`/`min`). Optionally combine it with `org.freedesktop.DBus.Method.Async`.

```xml
<?xml version="1.0" encoding="UTF-8"?>
<node>
  <interface name="org.bluez.Device1">
    <method name="Connect">
      <annotation name="org.freedesktop.DBus.Method.Async" value="client"/>
      <annotation name="org.freedesktop.DBus.Method.Timeout" value="3000ms"/>
    </method>
    <method name="Disconnect">
      <annotation name="org.freedesktop.DBus.Method.Async" value="client"/>
      <annotation name="org.freedesktop.DBus.Method.Timeout" value="2000000"/> <!-- 2000000us -->
    </method>
  </interface>
</node>

```

Using D-Bus properties
----------------------

Defining and working with D-Bus properties using XML description is quite easy.

### Defining a property in the IDL

A property element has no arg child element. It just has the attributes name, type and access, which are all mandatory. The access attribute allows the values ‘readwrite’, ‘read’, and ‘write’.

An example of a read-write property `status`:

```xml
<?xml version="1.0" encoding="UTF-8"?>

<node name="/org/sdbuscpp/propertyprovider">
    <interface name="org.sdbuscpp.PropertyProvider">
        <!--...-->
        <property name="status" type="u" access="readwrite"/>
        <!--...-->
    </interface>
</node>
```

### Generated stubs

This is how generated adaptor and proxy classes would look like with the read-write `status` property. The adaptor:

```cpp
class PropertyProvider_adaptor
{
    /*...*/

public:
    PropertyProvider_adaptor(sdbus::IObject& object)
        : object_(object)
    {
        object_.registerProperty("status").onInterface(INTERFACE_NAME).withGetter([this](){ return this->status(); }).withSetter([this](const uint32_t& value){ this->status(value); });
    }

    ~PropertyProvider_adaptor() = default;

private:
    // property getter
    virtual uint32_t status() = 0;
    // property setter
    virtual void status(const uint32_t& value) = 0;

    /*...*/
};
#endif
```

The proxy:

```cpp
class PropertyProvider_proxy
{
    /*...*/

public:
    // getting the property value
    uint32_t status()
    {
        return object_.getProperty("status").onInterface(INTERFACE_NAME);
    }

    // setting the property value
    void status(const uint32_t& value)
    {
        object_.setProperty("status").onInterface(INTERFACE_NAME).toValue(value);
    }

    /*...*/
};
```

When implementing the adaptor, we simply need to provide the body for `status` getter and setter method by overriding them. Then in the proxy, we just call them.

Standard D-Bus interfaces
-------------------------

sdbus-c++ provides support for standard D-Bus interfaces. These are:

* `org.freedesktop.DBus.Peer`
* `org.freedesktop.DBus.Introspectable`
* `org.freedesktop.DBus.Properties`
* `org.freedesktop.DBus.ObjectManager`

The implementation of methods that these interfaces define is provided by the library. `Peer`, `Introspectable` and `Properties` are automatically part of interfaces of every D-Bus object. `ObjectManager` is not automatically present and has to be enabled by the client when using `IObject` API. When using generated `ObjectManager_adaptor`, `ObjectManager` is enabled automatically in its constructor.

Pre-generated `*_proxy` and `*_adaptor` convenience classes for these standard interfaces are located in `sdbus-c++/StandardInterfaces.h`. To use them, we simply have to add them as additional parameters of `sdbus::ProxyInterfaces` or `sdbus::AdaptorInterfaces` class template, and our proxy or adaptor class inherits convenience functions from those interface classes.

For example, for our `Concatenator` example above in this tutorial, we may want to conveniently emit a `PropertyChanged` signal under `org.freedesktop.DBus.Properties` interface. First, we must augment our `Concatenator` class to also inherit from `org.freedesktop.DBus.Properties` interface: `class Concatenator : public sdbus::AdaptorInterfaces<org::sdbuscpp::Concatenator_adaptor, sdbus::Properties_adaptor> {...};`, and then we just issue `emitPropertiesChangedSignal` function of our adaptor object.

Note that signals of afore-mentioned standard D-Bus interfaces are not emitted by the library automatically. It's clients who are supposed to emit them.

Working examples of using standard D-Bus interfaces can be found in [sdbus-c++ integration tests](/tests/integrationtests/DBusStandardInterfacesTests.cpp) or the [examples](/examples) directory.

Using D-Bus Types
-------------------------

For many D-Bus interactions dealing with D-Bus types is necessary. For that, sdbus-c++ provides many predefined D-Bus
types. The table below shows which C++ type corresponds to which D-Bus type.

Example:
The D-Bus signature of a method is GetManagedObjects(o,a{sa{sv}}).
This means the corresponding C++ return type is:<br>
std::map<sdbus::ObjectPath, std::map<std::string, std::map<std::string, sdbus::Variant>>>


| Category            | Code        | Code ASCII | Conventional Name	 | C++ Type                        |
|---------------------|-------------|------------|--------------------|---------------------------------|
| reserved            | 0           | NUL        | INVALID            | -                               |
| fixed, basic	       | 121         | y          | BYTE               | uint8_t                         |
| fixed, basic	       | 98          | b          | BOOLEAN            | bool                            |
| fixed, basic	       | 110         | n          | INT16              | int16_t                         |
| fixed, basic	       | 113         | q          | UINT16             | uint16_t                        |
| fixed, basic	       | 105         | i          | INT32              | int32_t                         |
| fixed, basic	       | 117         | u          | UINT32             | uint32_t                        |
| fixed, basic	       | 120         | x          | INT64              | int64_t                         |
| fixed, basic	       | 116         | t          | UINT64             | uint64_t                        |
| fixed, basic	       | 100         | d          | DOUBLE             | double                          |
| string-like, basic	 | 115         | s          | STRING             | const char*, std::string        |
| string-like, basic	 | 111         | o          | OBJECT_PATH        | sdbus::ObjectPath               |
| string-like, basic	 | 103         | g          | SIGNATURE          | sdbus::Signature                |
| container           | 97          | a          | ARRAY              | std::vector<T>, std::map<T1,T2> |
| container           | 114,40,41   | r()        | STRUCT             | -                               |
| container           | 118         | v          | VARIANT            | sdbus::Variant                  |
| container           | 101,123,125 | e{}        | DICT_ENTRY         | -                               |
| fixed, basic        | 104         | h          | UNIX_FD            | sdbus::UnixFd                   |
| reserved            | 109         | m          | (reserved)         | -                               |
| reserved            | 42          | *          | (reserved)         | -                               |
| reserved            | 63          | ?          | (reserved)         | -                               |
| reserved            | 64,38,94    | @&^        | (reserved)         | -                               |
    
Support for match rules
-----------------------

`IConnection` class provides `addMatch` method that you can use to install match rules. An associated callback handler will be called upon an incoming message matching given match rule. There is support for both client-owned and floating (library-owned) match rules. Consult `IConnection` header or sdbus-c++ doxygen documentation for more information.

Conclusion
----------

There is no conclusion. Happy journeys by D-Bus with sdbus-c++!
