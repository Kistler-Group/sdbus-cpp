Using sdbus-c++ library
=======================

**Table of contents**

1. [Introduction](#introduction)
2. [Integrating sdbus-c++ into your project](#integrating-sdbus-c-into-your-project)
3. [Header files and namespaces](#header-files-and-namespaces)
4. [Error signalling and propagation](#error-signalling-and-propagation)
5. [Multiple layers of sdbus-c++ API](#multiple-layers-of-sdbus-c-api)
6. [An example: Number concatenator](#an-example-number-concatenator)
7. [Implementing the Concatenator example using basic sdbus-c++ API layer](#implementing-the-concatenator-example-using-basic-sdbus-c-api-layer)
8. [Implementing the Concatenator example using convenience sdbus-c++ API layer](#implementing-the-concatenator-example-using-convenience-sdbus-c-api-layer)
9. [Implementing the Concatenator example using sdbus-c++-generated stubs](#implementing-the-concatenator-example-using-sdbus-c-generated-stubs)
10. [Using D-Bus properties](#using-d-bus-properties)
11. [Conclusion](#conclusion)

Introduction
------------

sdbus-c++ is a C++ wrapper library built on top of [sd-bus](http://0pointer.net/blog/the-new-sd-bus-api-of-systemd.html), a lightweight D-Bus client
library implemented within systemd project. It provides D-Bus functionality on a higher level of abstraction, trying to employ C++ type system 
to shift as much work as possible from the developer to the compiler.

sdbus-c++ does not cover the entire sd-bus API, but provides tools for implementing the most common functionality - RPC 
method calls, signals and properties. There is room for additions and improvements, as needed and when needed.

Integrating sdbus-c++ into your project
---------------------------------------

The library build system is based on Autotools. The library supports `pkg-config`, so integrating it into your autotools project 
is a two step process.

1. Add `PKG_CHECK_MODULES` macro into your `configure.ac`:
```bash
PKG_CHECK_MODULES(SDBUSCPP, [sdbus-c++ >= 0.1],,
    AC_MSG_ERROR([You need the sdbus-c++ library (version 0.1 or better)]
    [http://www.kistler.com/])
)
```

2. Update `*_CFLAGS` and `*_LDFLAGS` in Makefiles of modules that use *sdbus-c++*, for example like this:
```bash
AM_CXXFLAGS = -std=c++17 -pedantic -W -Wall @SDBUSCPP_CFLAGS@ ...
AM_LDFLAGS = @SDBUSCPP_LIBS@ ...
```

Note: sdbus-c++ library depends on C++17, since it uses C++17 `std::uncaught_exceptions()` feature. When building sdbus-c++ manually, make sure you use a compiler that supports that feature. To use the library, make sure you have a C++ standard library that supports the feature. The feature is supported by e.g. gcc >= 6, and clang >= 3.7.

Header files and namespaces
---------------------------

All sdbus-c++ header files reside in the `sdbus-c++` subdirectory within the standard include directory. Users can either include 
individual header files, like so:

```cpp
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/IObjectProxy.h>
```

or just include the global header file that pulls in everything:

```cpp
#include <sdbus-c++/sdbus-c++.h>
```

All public types and functions of sdbus-c++ reside in the `sdbus` namespace.

Error signalling and propagation
--------------------------------

`sdbus::Error` exception is used to signal errors in sdbus-c++. There are two types of errors:
  * D-Bus related errors, like call timeouts, failed socket allocation, etc.
  * user errors, i.e. errors signalled and propagated from remote methods back to the caller.

The exception object carries the error name and error message with it.

sdbus-c++ design
----------------

The following diagram illustrates the major entities in sdbus-c++.

![class](sdbus-c++-class-diagram.png)

`IConnection` represents the concept of a D-Bus connection. You can connect to either the system bus or a session bus. Services can assign unique service names to those connections. A processing loop can be run on the connection.

`IObject` represents the concept of an object that exposes its methods, signals and properties. Its responsibilities are:
* registering (possibly multiple) interfaces and methods, signals, properties on those interfaces,
* emitting signals.
    
`IObjectProxy` represents the concept of the proxy, which is a view of the `Object` from the client side. Its responsibilities are:
* invoking remote methods of the corresponding object,
* registering handlers for signals.

`Message` class represents a message, which is the fundamental DBus concept. The message can be
* a method call (with serialized parameters),
* a method reply (with serialized return values),
* or a signal (with serialized parameters).

Multiple layers of sdbus-c++ API
-------------------------------

sdbus-c++ API comes in two layers:
  * [the basic layer](#implementing-the-concatenator-example-using-basic-sdbus-c-api-layer), which is a simple wrapper layer on top of sd-bus, using mechanisms that are native to C++ (e.g. serialization/deserialization of data from messages),
  * [the convenience layer](#implementing-the-concatenator-example-using-convenience-sdbus-c-api-layer), building on top of the basic layer, which aims at alleviating users from unnecessary details and enables them to write shorter, safer, and more expressive code.

sdbus-c++ also ships with a stub generator tool that converts D-Bus IDL in XML format into stub code for the adaptor as well as proxy part. Hierarchically, these stubs provide yet another layer of convenience (the "stubs layer"), making it possible for D-Bus RPC calls to completely look like native C++ calls on a local object.

An example: Number concatenator
-------------------------------

Let's have an object `/org/sdbuscpp/concatenator` that implements the `org.sdbuscpp.concatenator` interface. The interface exposes the following:
* a `concatenate` method that takes a collection of integers and a separator string and returns a string that is the concatenation of all 
  integers from the collection using given separator,
* a `concatenated` signal that is emitted at the end of each successful concatenation.

In the following sections, we will elaborate on the ways of implementing such an object on both the server and the client side.

Implementing the Concatenator example using basic sdbus-c++ API layer
---------------------------------------------------------------------

In the basic API layer, we already have abstractions for D-Bus connections, objects and object proxies, with which we can interact via 
interfaces. However, we still work with the concept of messages. To issue a method call for example, we have to go through several steps: 
we have to create a method call message first, serialize method arguments into the message, and send the message at last. We get the reply 
message (if applicable) in return, so we have to deserialize the return values from it manually.

Overloaded versions of C++ insertion/extraction operators are used for serialization/deserialization. That makes the client code much simpler.

### Server side

```c++
#include <sdbus-c++/sdbus-c++.h>
#include <vector>
#include <string>

// Yeah, global variable is ugly, but this is just an example and we want to access 
// the concatenator instance from within the concatenate method handler to be able
// to emit signals.
sdbus::IObject* g_concatenator{};

void concatenate(sdbus::Message& msg, sdbus::Message& reply)
{
    // Deserialize the collection of numbers from the message
    std::vector<int> numbers;
    msg >> numbers;
    
    // Deserialize separator from the message
    std::string separator;
    msg >> separator;
    
    // Return error if there are no numbers in the collection
    if (numbers.empty())
        throw sdbus::Error("org.sdbuscpp.Concatenator.Error", "No numbers provided");
    
    std::string result;
    for (auto number : numbers)
    {
        result += (result.empty() ? std::string() : separator) + std::to_string(number);
    }
    
    // Serialize resulting string to the reply
    reply << result;
    
    // Emit 'concatenated' signal
    const char* interfaceName = "org.sdbuscpp.Concatenator";
    auto signalMsg = g_concatenator->createSignal(interfaceName, "concatenated");
    signalMsg << result;
    g_concatenator->emitSignal(signalMsg);
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

    // Run the loop on the connection.
    connection->enterProcessingLoop();
}
```

### Client side

```c++
#include <sdbus-c++/sdbus-c++.h>
#include <vector>
#include <string>
#include <iostream>
#include <unistd.h>

void onConcatenated(sdbus::Message& signalMsg)
{
    std::string concatenatedString;
    msg >> concatenatedString;
    
    std::cout << "Received signal with concatenated string " << concatenatedString << std::endl;
}

int main(int argc, char *argv[])
{
    // Create proxy object for the concatenator object on the server side. Since we don't pass
    // the D-Bus connection object to the proxy constructor, the proxy will internally create
    // its own connection to the system bus.
    const char* destinationName = "org.sdbuscpp.concatenator";
    const char* objectPath = "/org/sdbuscpp/concatenator";
    auto concatenatorProxy = sdbus::createObjectProxy(destinationName, objectPath);
    
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

The object proxy is created without explicitly providing a D-Bus connection as an argument in its factory function. In that case, the proxy
will create its own connection to the *system* bus and listen to signals on it in a separate thread. That means the `onConcatenated` method is invoked always
in the context of a thread different from the main thread.

Implementing the Concatenator example using convenience sdbus-c++ API layer
---------------------------------------------------------------------------

One of the major sdbus-c++ design goals is to make the sdbus-c++ API easy to use correctly, and hard to use incorrectly.

The convenience API layer abstracts the concept of underlying D-Bus messages away completely. It abstracts away D-Bus signatures. And it tries 
to provide an interface that uses small, focused functions, with one or zero parameters, to form a chained function statement that reads like
a sentence to a human reading the code. To achieve that, sdbus-c++ utilizes the power of the C++ type system, so many issues are resolved at 
compile time, and the run-time performance cost compared to the basic layer is close to zero.

Thus, in the end of the day, the code written using the convenience API is:
- more expressive,
- closer to the abstraction level of the problem being solved,
- shorter,
- almost as fast (if not equally fast) as one written using the basic API layer.

Rather than *how*, the code written using this layer expresses *what* it does. Let's look at code samples to see if you agree :)

### Server side

```c++
#include <sdbus-c++/sdbus-c++.h>
#include <vector>
#include <string>

// Yeah, global variable is ugly, but this is just an example and we want to access 
// the concatenator instance from within the concatenate method handler to be able
// to emit signals.
sdbus::IObject* g_concatenator{};

std::string concatenate(const std::vector<int> numbers, const std::string& separator)
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
    g_concatenator->emitSignal("concatenated").onInterface(interfaceName).withArguments(result);
    
    return result;
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
    concatenator->registerMethod("concatenate").onInterface(interfaceName).implementedAs(&concatenate);
    concatenator->registerSignal("concatenated").onInterface(interfaceName).withParameters<std::string>();
    concatenator->finishRegistration();

    // Run the loop on the connection.
    connection->enterProcessingLoop();
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
    auto concatenatorProxy = sdbus::createObjectProxy(destinationName, objectPath);
    
    // Let's subscribe for the 'concatenated' signals
    const char* interfaceName = "org.sdbuscpp.Concatenator";
    concatenatorProxy->uponSignal("concatenated").onInterface(interfaceName).call([this](const std::string& str){ onConcatenated(str); });
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

Several lines of code have shrunk into one-liners when registering/calling methods or signals. D-Bus signatures and the serialization/deserialization 
of arguments from the messages is generated at compile time, by introspecting signatures of provided callbacks or deducing types of provided arguments.

Implementing the Concatenator example using sdbus-c++-generated stubs
---------------------------------------------------------------------

sdbus-c++ ships with the native stub generator tool called sdbuscpp-xml2cpp. The tool is very similar to dbusxx-xml2cpp tool that comes from 
dbus-c++ project.

The generator tool takes D-Bus XML IDL description of D-Bus interfaces on its input, and can be instructed to generate one or both of these: 
an adaptor header file for use at server side, and a proxy header file for use at client side. Like this:

```bash
sdbuscpp-xml2cpp database-bindings.xml --adaptor=database-server-glue.h --proxy=database-client-glue.h
```

The adaptor header file contains classes that can be used to implement described interfaces. The proxy header file contains classes that can be used 
to make calls to remote objects.

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

There is specific class for each interface in the XML IDL file. The class is de facto an interface which shall be implemented by inheriting from it. 
The class' constructor takes care of registering all methods, signals and properties. For each D-Bus method there is a pure virtual member function.
These pure virtual functions must be implemented in the child class. For each signal, there is a public function member that emits this signal.

```cpp
/*
 * This file was automatically generated by sdbuscpp-xml2cpp; DO NOT EDIT!
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
    static constexpr const char* interfaceName = "org.sdbuscpp.Concatenator";

protected:
    Concatenator_adaptor(sdbus::IObject& object)
        : object_(object)
    {
        object_.registerMethod("concatenate").onInterface(interfaceName).implementedAs([this](const std::vector<int32_t>& numbers, const std::string& separator){ return this->concatenate(numbers, separator); });
        object_.registerSignal("concatenated").onInterface(interfaceName).withParameters<std::string>();
    }

public:
    void concatenated(const std::string& concatenatedString)
    {
        object_.emitSignal("concatenated").onInterface(interfaceName).withArguments(concatenatedString);
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

Analogously to the adaptor classes described above, there is specific class for each interface in the XML IDL file. The class is de facto a proxy 
to the concrete interface of a remote object. For each D-Bus signal there is a pure virtual member function whose body must be provided in a child
class. For each method, there is a public function member that calls the method remotely.

```cpp
/*
 * This file was automatically generated by sdbuscpp-xml2cpp; DO NOT EDIT!
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
    static constexpr const char* interfaceName = "org.sdbuscpp.Concatenator";

protected:
    Concatenator_proxy(sdbus::IObjectProxy& object)
        : object_(object)
    {
        object_.uponSignal("concatenated").onInterface(interfaceName).call([this](const std::string& concatenatedString){ this->onConcatenated(concatenatedString); });
    }

    virtual void onConcatenated(const std::string& concatenatedString) = 0;

public:
    std::string concatenate(const std::vector<int32_t>& numbers, const std::string& separator)
    {
        std::string result;
        object_.callMethod("concatenate").onInterface(interfaceName).withArguments(numbers, separator).storeResultsTo(result);
        return result;
    }

private:
    sdbus::IObjectProxy& object_;
};

}} // namespaces

#endif
```

### Providing server implementation based on generated adaptors

To implement a D-Bus object that implements all its D-Bus interfaces, we shall create a class representing the object that inherits from all 
corresponding `*_adaptor` classes and implements all pure virtual member functions. Specifically, the object class shall inherit from the 
`Interfaces` template class, the template arguments of which are individual adaptor classes. The `Interfaces` is just a convenience class that 
hides a few boiler-plate details. For example, in its constructor, it creates an `Object` instance, it takes care of proper initialization of 
all adaptor superclasses, and exports the object finally.

```cpp
#include <sdbus-c++/sdbus-c++.h>
#include "concatenator-server-glue.h"

class Concatenator : public sdbus::Interfaces<org::sdbuscpp::Concatenator_adaptor /*, more adaptor classes if there are more interfaces*/>
{
public:
    Concatenator(sdbus::IConnection& connection, std::string objectPath)
        : sdbus::Interfaces<org::sdbuscpp::Concatenator_adaptor>(connection, std::move(objectPath))
    {
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
        concatenated(result);
        
        // Return the resulting string
        return result;
    }
};
```

That's it. We now have an implementation of a D-Bus object implementing `org.sdbuscpp.Concatenator` interface. Let's now create a service 
publishing the object.

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
    connection->enterProcessingLoop();
}
```

It's that simple!

### Providing client implementation based on generated proxies

To implement a proxy for a remote D-Bus object, we shall create a class representing the object proxy that inherits from all corresponding 
`*_proxy` classes and -- if applicable -- implements all pure virtual member functions. Specifically, the object proxy class shall inherit 
from the `ProxyInterfaces` template class. As its template arguments we shall provide all proxy classes. The `ProxyInterfaces` is just a 
convenience class that hides a few boiler-plate details. For example, in its constructor, it creates an `ObjectProxy` instance, and it takes 
care of proper initialization of all proxy superclasses.

```cpp
#include <sdbus-c++/sdbus-c++.h>
#include "concatenator-client-glue.h"

class ConcatenatorProxy : public sdbus::ProxyInterfaces<org::sdbuscpp::Concatenator_proxy /*, more proxy classes if there are more interfaces*/>
{
public:
    ConcatenatorProxy(std::string destination, std::string objectPath)
        : sdbus::ProxyInterfaces<org::sdbuscpp::Concatenator_proxy>(std::move(destination), std::move(objectPath))
    {
    }

protected:
    void onConcatenated(const std::string& concatenatedString) override
    {
        std::cout << "Received signal with concatenated string " << concatenatedString << std::endl;
    }
};
```

Now let's use this proxy to make remote calls and listen to signals in a real application.

```cpp
#include "ConcatenatorProxy.h"

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

Using D-Bus properties
----------------------

Defining and working with D-Bus properties using XML description is quite easy.

### Defining a property in the XML

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

Conclusion
----------

There is no conclusion. Happy journeys by D-Bus with sdbus-c++!
