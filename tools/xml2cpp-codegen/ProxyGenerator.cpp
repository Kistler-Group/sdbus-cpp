/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
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
            << tab << tab << ": proxy_(proxy)" << endl;

    Nodes methods = interface["method"];
    Nodes signals = interface["signal"];
    Nodes properties = interface["property"];

    std::string registration, declaration;
    std::tie(registration, declaration) = processSignals(signals);

    body << tab << "{" << endl
            << registration
            << tab << "}" << endl << endl;

    body << tab << "~" << className << "() = default;" << endl << endl;

    if (!declaration.empty())
        body << declaration << endl;

    std::string methodDefinitions, asyncDeclarations;
    std::tie(methodDefinitions, asyncDeclarations) = processMethods(methods);

    if (!asyncDeclarations.empty())
    {
        body << asyncDeclarations << endl;
    }

    if (!methodDefinitions.empty())
    {
        body << "public:" << endl << methodDefinitions;
    }

    std::string propertyDefinitions = processProperties(properties);
    if (!propertyDefinitions.empty())
    {
        body << "public:" << endl << propertyDefinitions;
    }

    body << "private:" << endl
            << tab << "sdbus::IProxy& proxy_;" << endl
            << "};" << endl << endl
            << std::string(namespacesCount, '}') << " // namespaces" << endl << endl;

    return body.str();
}

std::tuple<std::string, std::string> ProxyGenerator::processMethods(const Nodes& methods) const
{
    std::ostringstream definitionSS, asyncDeclarationSS;

    for (const auto& method : methods)
    {
        auto name = method->get("name");
        Nodes args = (*method)["arg"];
        Nodes inArgs = args.select("direction" , "in");
        Nodes outArgs = args.select("direction" , "out");

        bool dontExpectReply{false};
        bool async{false};
        Nodes annotations = (*method)["annotation"];
        for (const auto& annotation : annotations)
        {
            if (annotation->get("name") == "org.freedesktop.DBus.Method.NoReply" && annotation->get("value") == "true")
                dontExpectReply = true;
            else if (annotation->get("name") == "org.freedesktop.DBus.Method.Async"
                     && (annotation->get("value") == "client" || annotation->get("value") == "clientserver"))
                async = true;
        }
        if (dontExpectReply && outArgs.size() > 0)
        {
            std::cerr << "Function: " << name << ": ";
            std::cerr << "Option 'org.freedesktop.DBus.Method.NoReply' not allowed for methods with 'out' variables! Option ignored..." << std::endl;
            dontExpectReply = false;
        }

        auto retType = outArgsToType(outArgs);
        std::string inArgStr, inArgTypeStr;
        std::tie(inArgStr, inArgTypeStr, std::ignore) = argsToNamesAndTypes(inArgs);
        std::string outArgStr, outArgTypeStr;
        std::tie(outArgStr, outArgTypeStr, std::ignore) = argsToNamesAndTypes(outArgs);

        definitionSS << tab << (async ? "void" : retType) << " " << name << "(" << inArgTypeStr << ")" << endl
                << tab << "{" << endl;

        if (outArgs.size() > 0 && !async)
        {
            definitionSS << tab << tab << retType << " result;" << endl;
        }

        definitionSS << tab << tab << "proxy_.callMethod" << (async ? "Async" : "") << "(\"" << name << "\")"
                        ".onInterface(INTERFACE_NAME)";

        if (inArgs.size() > 0)
        {
            definitionSS << ".withArguments(" << inArgStr << ")";
        }

        if (async && !dontExpectReply)
        {
            auto nameBigFirst = name;
            nameBigFirst[0] = islower(nameBigFirst[0]) ? nameBigFirst[0] + 'A' - 'a' : nameBigFirst[0];

            definitionSS << ".uponReplyInvoke([this](const sdbus::Error* error" << (outArgTypeStr.empty() ? "" : ", ") << outArgTypeStr << ")"
                                             "{ this->on" << nameBigFirst << "Reply(" << outArgStr << (outArgStr.empty() ? "" : ", ") << "error); })";

            asyncDeclarationSS << tab << "virtual void on" << nameBigFirst << "Reply("
                               << outArgTypeStr << (outArgTypeStr.empty() ? "" : ", ")  << "const sdbus::Error* error) = 0;" << endl;
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
        std::tie(argStr, argTypeStr, std::ignore) = argsToNamesAndTypes(args);

        registrationSS << tab << tab << "proxy_"
                ".uponSignal(\"" << name << "\")"
                ".onInterface(INTERFACE_NAME)"
                ".call([this](" << argTypeStr << ")"
                "{ this->on" << nameBigFirst << "(" << argStr << "); });" << endl;

        declarationSS << tab << "virtual void on" << nameBigFirst << "(" << argTypeStr << ") = 0;" << endl;
    }

    return std::make_tuple(registrationSS.str(), declarationSS.str());
}

std::string ProxyGenerator::processProperties(const Nodes& properties) const
{
    std::ostringstream propertySS;
    for (const auto& property : properties)
    {
        auto propertyName = property->get("name");
        auto propertyAccess = property->get("access");
        auto propertySignature = property->get("type");

        auto propertyType = signature_to_type(propertySignature);
        auto propertyArg = std::string("value");
        auto propertyTypeArg = std::string("const ") + propertyType + "& " + propertyArg;

        if (propertyAccess == "read" || propertyAccess == "readwrite")
        {
            propertySS << tab << propertyType << " " << propertyName << "()" << endl
                    << tab << "{" << endl;
            propertySS << tab << tab << "return proxy_.getProperty(\"" << propertyName << "\")"
                            ".onInterface(INTERFACE_NAME)";
            propertySS << ";" << endl << tab << "}" << endl << endl;
        }

        if (propertyAccess == "readwrite" || propertyAccess == "write")
        {
            propertySS << tab << "void " << propertyName << "(" << propertyTypeArg << ")" << endl
                    << tab << "{" << endl;
            propertySS << tab << tab << "proxy_.setProperty(\"" << propertyName << "\")"
                            ".onInterface(INTERFACE_NAME)"
                            ".toValue(" << propertyArg << ")";
            propertySS << ";" << endl << tab << "}" << endl << endl;
        }
    }

    return propertySS.str();
}
