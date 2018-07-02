/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 *
 * @file AdaptorGenerator.cpp
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
#include "AdaptorGenerator.h"

// STL
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <algorithm>
#include <iterator>

using std::endl;

using sdbuscpp::xml::Document;
using sdbuscpp::xml::Node;
using sdbuscpp::xml::Nodes;

/**
 * Generate adaptor code - server glue
 */
int AdaptorGenerator::transformXmlToFileImpl(const Document& doc, const char* filename) const
{
    Node &root = *(doc.root);
    Nodes interfaces = root["interface"];

    std::ostringstream code;
    code << createHeader(filename, StubType::ADAPTOR);

    for (const auto& interface : interfaces)
    {
        code << processInterface(*interface);
    }

    code << "#endif" << endl;

    return writeToFile(filename, code.str());
}


std::string AdaptorGenerator::processInterface(Node& interface) const
{
    std::string ifaceName = interface.get("name");
    std::cout << "Generating adaptor code for interface " << ifaceName << endl;

    unsigned int namespacesCount = 0;
    std::string namespacesStr;
    std::tie(namespacesCount, namespacesStr) = generateNamespaces(ifaceName);

    std::ostringstream body;
    body << namespacesStr;

    std::string className = ifaceName.substr(ifaceName.find_last_of(".") + 1)
            + "_adaptor";

    body << "class " << className << endl
            << "{" << endl
            << "public:" << endl
            << tab << "static constexpr const char* interfaceName = \"" << ifaceName << "\";" << endl << endl
            << "protected:" << endl
            << tab << className << "(sdbus::IObject& object)" << endl
            << tab << tab << ": object_(object)" << endl;

    Nodes methods = interface["method"];
    Nodes signals = interface["signal"];
    Nodes properties = interface["property"];

    std::string methodRegistration, methodDeclaration;
    std::tie(methodRegistration, methodDeclaration) = processMethods(methods);

    std::string signalRegistration, signalMethods;
    std::tie(signalRegistration, signalMethods) = processSignals(signals);

    std::string propertyRegistration, propertyAccessorDeclaration;
    std::tie(propertyRegistration, propertyAccessorDeclaration) = processProperties(properties);

    body << tab << "{" << endl
            << methodRegistration
            << signalRegistration
            << propertyRegistration
            << tab << "}" << endl << endl;

    if (!signalMethods.empty())
    {
        body << "public:" << endl << signalMethods;
    }

    if (!methodDeclaration.empty())
    {
        body << "private:" << endl << methodDeclaration << endl;
    }

    if (!propertyAccessorDeclaration.empty())
    {
        body << "private:" << endl << propertyAccessorDeclaration << endl;
    }

    body << "private:" << endl
            << tab << "sdbus::IObject& object_;" << endl
            << "};" << endl << endl
            << std::string(namespacesCount, '}') << " // namespaces" << endl << endl;

    return body.str();
}


std::tuple<std::string, std::string> AdaptorGenerator::processMethods(const Nodes& methods) const
{
    std::ostringstream registrationSS, declarationSS;

    for (const auto& method : methods)
    {
        auto methodName = method->get("name");

        bool async{false};
        Nodes annotations = (*method)["annotation"];

        for (const auto& annotation : annotations)
        {
            if (annotation->get("name") == "org.freedesktop.DBus.Method.Async"
                && (annotation->get("value") == "server" || annotation->get("value") == "clientserver"))
            {
                async = true;
                break;
            }
        }

        Nodes args = (*method)["arg"];
        Nodes inArgs = args.select("direction" , "in");
        Nodes outArgs = args.select("direction" , "out");

        std::string argStr, argTypeStr;
        std::tie(argStr, argTypeStr, std::ignore) = argsToNamesAndTypes(inArgs);
        
        using namespace std::string_literals;

        registrationSS << tab << tab << "object_.registerMethod(\""
                << methodName << "\")"
                << ".onInterface(interfaceName)"
                << ".implementedAs("
                << "[this]("
                << (async ? "sdbus::Result<" + outArgsToType(outArgs, true) + "> result" + (argTypeStr.empty() ? "" : ", ") : "")
                << argTypeStr
                << "){ " << (async ? "" : "return ") << "this->" << methodName << "("
                << (async ? "std::move(result)"s + (argTypeStr.empty() ? "" : ", ") : "")
                << argStr << "); });" << endl;

        declarationSS << tab
                << "virtual "
                << (async ? "void" : outArgsToType(outArgs))
                << " " << methodName
                << "("
                << (async ? "sdbus::Result<" + outArgsToType(outArgs, true) + "> result" + (argTypeStr.empty() ? "" : ", ") : "")
                << argTypeStr
                << ") = 0;" << endl;
    }

    return std::make_tuple(registrationSS.str(), declarationSS.str());
}


std::tuple<std::string, std::string> AdaptorGenerator::processSignals(const Nodes& signals) const
{
    std::ostringstream signalRegistrationSS, signalMethodSS;

    for (const auto& signal : signals)
    {
        auto name = signal->get("name");
        Nodes args = (*signal)["arg"];

        std::string argStr, argTypeStr, typeStr;;
        std::tie(argStr, argTypeStr, typeStr) = argsToNamesAndTypes(args);

        signalRegistrationSS << tab << tab
                << "object_.registerSignal(\"" << name << "\")"
                        ".onInterface(interfaceName)";

        if (args.size() > 0)
        {
            signalRegistrationSS << ".withParameters<" << typeStr << ">()";
        }

        signalRegistrationSS << ";" << endl;

        signalMethodSS << tab << "void " << name << "(" << argTypeStr << ")" << endl
                << tab << "{" << endl
                << tab << tab << "object_.emitSignal(\"" << name << "\")"
                        ".onInterface(interfaceName)";

        if (!argStr.empty())
        {
            signalMethodSS << ".withArguments(" << argStr << ")";
        }

        signalMethodSS << ";" << endl
                << tab << "}" << endl << endl;
    }

    return std::make_tuple(signalRegistrationSS.str(), signalMethodSS.str());
}


std::tuple<std::string, std::string> AdaptorGenerator::processProperties(const Nodes& properties) const
{
    std::ostringstream registrationSS, declarationSS;

    for (const auto& property : properties)
    {
        auto propertyName = property->get("name");
        auto propertyAccess = property->get("access");
        auto propertySignature = property->get("type");

        auto propertyType = signature_to_type(propertySignature);
        auto propertyArg = std::string("value");
        auto propertyTypeArg = std::string("const ") + propertyType + "& " + propertyArg;

        registrationSS << tab << tab << "object_.registerProperty(\""
                << propertyName << "\")"
                << ".onInterface(interfaceName)";

        if (propertyAccess == "read" || propertyAccess == "readwrite")
        {
            registrationSS << ".withGetter([this](){ return this->" << propertyName << "(); })";
        }

        if (propertyAccess == "readwrite" || propertyAccess == "write")
        {
            registrationSS
                << ".withSetter([this](" << propertyTypeArg << ")"
                   "{ this->" << propertyName << "(" << propertyArg << "); })";
        }

        registrationSS << ";" << endl;

        if (propertyAccess == "read" || propertyAccess == "readwrite")
            declarationSS << tab << "virtual " << propertyType << " " << propertyName << "() = 0;" << endl;
        if (propertyAccess == "readwrite" || propertyAccess == "write")
            declarationSS << tab << "virtual void " << propertyName << "(" << propertyTypeArg << ") = 0;" << endl;
    }

    return std::make_tuple(registrationSS.str(), declarationSS.str());
}
