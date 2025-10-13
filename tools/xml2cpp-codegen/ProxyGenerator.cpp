/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2024 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file ProxyGenerator.cpp
 *
 * Created on: Feb 1, 2017
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

// Own
#include "generator_utils.h"
#include "ProxyGenerator.h"

// STL
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <iterator>
#include <regex>
#include <unordered_map>

using std::endl;

using sdbuscpp::xml::Document;
using sdbuscpp::xml::Node;
using sdbuscpp::xml::Nodes;

/**
 * Generate proxy code - client glue
 */
int ProxyGenerator::transformXmlToFileImpl(const Document &doc, const char *filename) const
{
    Node &root = *(doc.root);
    Nodes interfaces = root["interface"];

    std::ostringstream code;
    code << createHeader(filename, StubType::PROXY);

    for (const auto& interface : interfaces)
    {
        code << processInterface(*interface);
    }

    code << "#endif" << endl;

    return writeToFile(filename, code.str());
}

std::string prepareDefaultTimeout(const std::string& val, const std::string& unit)
{
    const std::unordered_map<std::string, std::string> chronoTypes = {
        {"us", "std::chrono::microseconds"},
        {"ms", "std::chrono::milliseconds"},
        {"s", "std::chrono::seconds"},
        {"min", "std::chrono::minutes"}  
    };
    const auto iter = chronoTypes.find(unit);

    const std::string type = (iter != chronoTypes.end()) ? iter->second : "std::chrono::microseconds";
    const std::string defaultValue = type + "(" + val + ")";
    return defaultValue;
}

std::string ProxyGenerator::processInterface(Node& interface) const
{
    std::string ifaceName = interface.get("name");
    std::cout << "Generating proxy code for interface " << ifaceName << endl;

    unsigned int namespacesCount = 0;
    std::string namespacesStr;
    std::tie(namespacesCount, namespacesStr) = generateNamespaces(ifaceName);

    std::ostringstream body;
    body << namespacesStr;

    std::string className = ifaceName.substr(ifaceName.find_last_of(".") + 1)
            + "_proxy";

    body << "class " << className << endl
            << "{" << endl
            << "public:" << endl
            << tab << "static constexpr const char* INTERFACE_NAME = \"" << ifaceName << "\";" << endl << endl
            << "protected:" << endl
            << tab << className << "(sdbus::IProxy& proxy)" << endl
            << tab << tab << ": m_proxy(proxy)" << endl
            << tab << "{" << endl
            << tab << "}" << endl << endl;

    // Rule of Five
    body << tab << className << "(const " << className << "&) = delete;" << endl;
    body << tab << className << "& operator=(const " << className << "&) = delete;" << endl;
    body << tab << className << "(" << className << "&&) = delete;" << endl;
    body << tab << className << "& operator=(" << className << "&&) = delete;" << endl << endl;

    body << tab << "~" << className << "() = default;" << endl << endl;

    Nodes methods = interface["method"];
    Nodes signals = interface["signal"];
    Nodes properties = interface["property"];

    std::string registration, declaration;
    std::tie(registration, declaration) = processSignals(signals);

    body << tab << "void registerProxy()" << endl
         << tab << "{" << endl
         << registration
         << tab << "}" << endl << endl;

    if (!declaration.empty())
        body << declaration << endl;

    std::string methodDefinitions, asyncDeclarationsMethods;
    std::tie(methodDefinitions, asyncDeclarationsMethods) = processMethods(methods);
    std::string propertyDefinitions, asyncDeclarationsProperties;
    std::tie(propertyDefinitions, asyncDeclarationsProperties) = processProperties(properties);

    if (!asyncDeclarationsMethods.empty())
    {
        body << asyncDeclarationsMethods << endl;
    }
    if (!asyncDeclarationsProperties.empty())
    {
        body << asyncDeclarationsProperties << endl;
    }

    if (!methodDefinitions.empty())
    {
        body << "public:" << endl << methodDefinitions;
    }

    if (!propertyDefinitions.empty())
    {
        body << "public:" << endl << propertyDefinitions;
    }

    body << "private:" << endl
            << tab << "sdbus::IProxy& m_proxy;" << endl
            << "};" << endl << endl
            << std::string(namespacesCount, '}') << " // namespaces" << endl << endl;

    return body.str();
}

std::tuple<std::string, std::string> ProxyGenerator::processMethods(const Nodes& methods) const
{
    const std::regex patternTimeout{R"(^(\d+)(min|s|ms|us)?$)"};

    std::ostringstream definitionSS, asyncDeclarationSS;

    for (const auto& method : methods)
    {
        auto name = method->get("name");
        auto nameSafe = mangle_name(name);
        Nodes args = (*method)["arg"];
        Nodes inArgs = args.select("direction" , "in");
        Nodes outArgs = args.select("direction" , "out");

        bool dontExpectReply{false};
        bool async{false};
        bool future{false}; // Async methods implemented by means of either std::future or callbacks
        std::string timeoutValue;
        std::smatch smTimeout;

        Nodes annotations = (*method)["annotation"];
        for (const auto& annotation : annotations)
        {
            const auto annotationName = annotation->get("name");
            const auto annotationValue = annotation->get("value");

            if (annotationName == "org.freedesktop.DBus.Method.NoReply" && annotationValue == "true")
                dontExpectReply = true;
            else
            {
                if (annotationName == "org.freedesktop.DBus.Method.Async"
                     && (annotationValue == "client" || annotationValue == "clientserver" || annotationValue == "client-server"))
                    async = true;
                else if (annotationName == "org.freedesktop.DBus.Method.Async.ClientImpl" && annotationValue == "callback")
                    future = false;
                else if (annotationName == "org.freedesktop.DBus.Method.Async.ClientImpl" && (annotationValue == "future" || annotationValue == "std::future"))
                    future = true;
            }
            if (annotationName == "org.freedesktop.DBus.Method.Timeout")
                timeoutValue = annotationValue;
        }
        if (dontExpectReply && outArgs.size() > 0)
        {
            std::cerr << "Function: " << name << ": ";
            std::cerr << "Option 'org.freedesktop.DBus.Method.NoReply' not allowed for methods with 'out' variables! Option ignored..." << std::endl;
            dontExpectReply = false;
        }
        if (!timeoutValue.empty() && dontExpectReply)
        {
            std::cerr << "Function: " << name << ": ";
            std::cerr << "Option 'org.freedesktop.DBus.Method.Timeout' not allowed for 'NoReply' methods! Option ignored..." << std::endl;
            timeoutValue.clear();
        }

        if (!timeoutValue.empty() && !std::regex_match(timeoutValue, smTimeout, patternTimeout))
        {
            std::cerr << "Function: " << name << ": ";
            std::cerr << "Option 'org.freedesktop.DBus.Method.Timeout' has unsupported timeout value! Option ignored..." << std::endl;
            timeoutValue.clear();
        }

        auto retType = outArgsToType(outArgs);
        auto retTypeBare = outArgsToType(outArgs, true);
        std::string inArgStr, inArgTypeStr;
        std::tie(inArgStr, inArgTypeStr, std::ignore, std::ignore) = argsToNamesAndTypes(inArgs);
        std::string outArgStr, outArgTypeStr;
        std::tie(outArgStr, outArgTypeStr, std::ignore, std::ignore) = argsToNamesAndTypes(outArgs);

        const std::string realRetType = (async && !dontExpectReply ? (future ? "std::future<" + retType + ">" : "sdbus::PendingAsyncCall") : async ? "void" : retType);

        std::string timeoutDefaultValue;
        if (!timeoutValue.empty())
        {
            const auto val = smTimeout.str(1);
            const auto unit = smTimeout.str(2);
            timeoutDefaultValue = prepareDefaultTimeout(val, unit);
        }
        definitionSS << tab << realRetType << " " << nameSafe << "(" << inArgTypeStr  << (!timeoutDefaultValue.empty() ? ", const std::chrono::microseconds& timeout = " + timeoutDefaultValue : "")  << ")" << endl
                << tab << "{" << endl;

        if (outArgs.size() > 0 && !async)
        {
            definitionSS << tab << tab << retType << " result;" << endl;
        }

        definitionSS << tab << tab << (async && !dontExpectReply ? "return " : "")
                     << "m_proxy.callMethod" << (async ? "Async" : "") << "(\"" << name << "\").onInterface(INTERFACE_NAME)";

        if (!timeoutValue.empty())
        {
            definitionSS << ".withTimeout(" << "timeout" << ")";
        }

        if (inArgs.size() > 0)
        {
            definitionSS << ".withArguments(" << inArgStr << ")";
        }

        if (async && !dontExpectReply)
        {
            auto nameBigFirst = name;
            nameBigFirst[0] = islower(nameBigFirst[0]) ? nameBigFirst[0] + 'A' - 'a' : nameBigFirst[0];

            if (future) // Async methods implemented through future
            {
                definitionSS << ".getResultAsFuture<" << retTypeBare << ">()";
            }
            else // Async methods implemented through callbacks
            {
                definitionSS << ".uponReplyInvoke([this](std::optional<sdbus::Error> error" << (outArgTypeStr.empty() ? "" : ", ") << outArgTypeStr << ")"
                                                 "{ this->on" << nameBigFirst << "Reply(" << outArgStr << (outArgStr.empty() ? "" : ", ") << "std::move(error)); })";

                asyncDeclarationSS << tab << "virtual void on" << nameBigFirst << "Reply("
                                   << outArgTypeStr << (outArgTypeStr.empty() ? "" : ", ")  << "std::optional<sdbus::Error> error) = 0;" << endl;
            }
        }
        else if (outArgs.size() > 0)
        {
            definitionSS << ".storeResultsTo(result);" << endl
                         << tab << tab << "return result";
        }
        else if (dontExpectReply)
        {
            definitionSS << ".dontExpectReply()";
        }

        definitionSS << ";" << endl << tab << "}" << endl << endl;
    }

    return std::make_tuple(definitionSS.str(), asyncDeclarationSS.str());
}

std::tuple<std::string, std::string> ProxyGenerator::processSignals(const Nodes& signals) const
{
    std::ostringstream registrationSS, declarationSS;

    for (const auto& signal : signals)
    {
        auto name = signal->get("name");
        Nodes args = (*signal)["arg"];

        auto nameBigFirst = name;
        nameBigFirst[0] = islower(nameBigFirst[0]) ? nameBigFirst[0] + 'A' - 'a' : nameBigFirst[0];

        std::string argStr, argTypeStr;
        std::tie(argStr, argTypeStr, std::ignore, std::ignore) = argsToNamesAndTypes(args);

        registrationSS << tab << tab << "m_proxy"
                ".uponSignal(\"" << name << "\")"
                ".onInterface(INTERFACE_NAME)"
                ".call([this](" << argTypeStr << ")"
                "{ this->on" << nameBigFirst << "(" << argStr << "); });" << endl;

        declarationSS << tab << "virtual void on" << nameBigFirst << "(" << argTypeStr << ") = 0;" << endl;
    }

    return std::make_tuple(registrationSS.str(), declarationSS.str());
}

std::tuple<std::string, std::string> ProxyGenerator::processProperties(const Nodes& properties) const
{
    std::ostringstream propertySS, asyncDeclarationSS;
    for (const auto& property : properties)
    {
        auto propertyName = property->get("name");
        auto propertyNameSafe = mangle_name(propertyName);
        auto propertyAccess = property->get("access");
        auto propertySignature = property->get("type");

        auto propertyType = signature_to_type(propertySignature);
        auto propertyArg = std::string("value");
        auto propertyTypeArg = std::string("const ") + propertyType + "& " + propertyArg;

        bool asyncGet{false};
        bool futureGet{false}; // Async property getter implemented by means of either std::future or callbacks
        bool asyncSet{false};
        bool futureSet{false}; // Async property setter implemented by means of either std::future or callbacks

        Nodes annotations = (*property)["annotation"];
        for (const auto& annotation : annotations)
        {
            const auto annotationName = annotation->get("name");
            const auto annotationValue = annotation->get("value");

            if (annotationName == "org.freedesktop.DBus.Property.Get.Async" && annotationValue == "client") // Server-side not supported (may be in the future)
                asyncGet = true;
            else if (annotationName == "org.freedesktop.DBus.Property.Get.Async.ClientImpl" && annotationValue == "callback")
                futureGet = false;
            else if (annotationName == "org.freedesktop.DBus.Property.Get.Async.ClientImpl" && (annotationValue == "future" || annotationValue == "std::future"))
                futureGet = true;
            else if (annotationName == "org.freedesktop.DBus.Property.Set.Async" && annotationValue == "client") // Server-side not supported (may be in the future)
                asyncSet = true;
            else if (annotationName == "org.freedesktop.DBus.Property.Set.Async.ClientImpl" && annotationValue == "callback")
                futureSet = false;
            else if (annotationName == "org.freedesktop.DBus.Property.Set.Async.ClientImpl" && (annotationValue == "future" || annotationValue == "std::future"))
                futureSet = true;
        }

        if (propertyAccess == "read" || propertyAccess == "readwrite")
        {
            const std::string realRetType = (asyncGet ? (futureGet ? "std::future<sdbus::Variant>" : "sdbus::PendingAsyncCall") : propertyType);

            propertySS << tab << realRetType << " " << propertyNameSafe << "()" << endl
                    << tab << "{" << endl;
            propertySS << tab << tab << "return m_proxy.getProperty" << (asyncGet ? "Async" : "") << "(\"" << propertyName << "\")"
                            ".onInterface(INTERFACE_NAME)";
            if (!asyncGet)
            {
                propertySS << ".get<" << realRetType << ">()";
            }
            else
            {
                auto nameBigFirst = propertyName;
                nameBigFirst[0] = islower(nameBigFirst[0]) ? nameBigFirst[0] + 'A' - 'a' : nameBigFirst[0];

                if (futureGet) // Async methods implemented through future
                {
                    propertySS << ".getResultAsFuture()";
                }
                else // Async methods implemented through callbacks
                {
                    propertySS << ".uponReplyInvoke([this](std::optional<sdbus::Error> error, const sdbus::Variant& value)"
                                                   "{ this->on" << nameBigFirst << "PropertyGetReply(value.get<" << propertyType << ">(), std::move(error)); })";

                    asyncDeclarationSS << tab << "virtual void on" << nameBigFirst << "PropertyGetReply("
                                       << "const " << propertyType << "& value, std::optional<sdbus::Error> error) = 0;" << endl;
                }
            }
            propertySS << ";" << endl << tab << "}" << endl << endl;
        }

        if (propertyAccess == "readwrite" || propertyAccess == "write")
        {
            if (propertySignature == "v")
                propertyArg = "{" + propertyArg + ", sdbus::embed_variant}";

            const std::string realRetType = (asyncSet ? (futureSet ? "std::future<void>" : "sdbus::PendingAsyncCall") : "void");

            propertySS << tab << realRetType << " " << propertyNameSafe << "(" << propertyTypeArg << ")" << endl
                       << tab << "{" << endl;
            propertySS << tab << tab << (asyncSet ? "return " : "") << "m_proxy.setProperty" << (asyncSet ? "Async" : "")
                       << "(\"" << propertyName << "\")"
                            ".onInterface(INTERFACE_NAME)"
                            ".toValue(" << propertyArg << ")";

            if (asyncSet)
            {
                auto nameBigFirst = propertyName;
                nameBigFirst[0] = islower(nameBigFirst[0]) ? nameBigFirst[0] + 'A' - 'a' : nameBigFirst[0];

                if (futureSet) // Async methods implemented through future
                {
                    propertySS << ".getResultAsFuture()";
                }
                else // Async methods implemented through callbacks
                {
                    propertySS << ".uponReplyInvoke([this](std::optional<sdbus::Error> error)"
                                  "{ this->on" << nameBigFirst << "PropertySetReply(std::move(error)); })";

                    asyncDeclarationSS << tab << "virtual void on" << nameBigFirst << "PropertySetReply("
                                       << "std::optional<sdbus::Error> error) = 0;" << endl;
                }
            }

            propertySS << ";" << endl << tab << "}" << endl << endl;
        }
    }

    return std::make_tuple(propertySS.str(), asyncDeclarationSS.str());
}
