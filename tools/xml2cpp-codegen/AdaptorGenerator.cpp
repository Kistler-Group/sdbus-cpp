/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2024 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
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
#include <cctype>

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
            << tab << "static constexpr const char* INTERFACE_NAME = \"" << ifaceName << "\";" << endl << endl
            << "protected:" << endl
            << tab << className << "(sdbus::IObject& object)" << endl
            << tab << tab << ": m_object(object)" << endl
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

    auto annotations = getAnnotations(interface);
    std::string annotationRegistration;
    for (const auto& annotation : annotations)
    {
        const auto& annotationName = annotation.first;
        const auto& annotationValue = annotation.second;

        if (annotationName == "org.freedesktop.DBus.Deprecated" && annotationValue == "true")
            annotationRegistration += ".markAsDeprecated()";
        else if (annotationName == "org.freedesktop.systemd1.Privileged" && annotationValue == "true")
            annotationRegistration += ".markAsPrivileged()";
        else if (annotationName == "org.freedesktop.DBus.Property.EmitsChangedSignal")
            annotationRegistration += ".withPropertyUpdateBehavior(" + propertyAnnotationToFlag(annotationValue) + ")";
        else
            std::cerr << "Node: " << ifaceName << ": "
                      << "Option '" << annotationName << "' not allowed or supported in this context! Option ignored..." << std::endl;
    }
    if(!annotationRegistration.empty())
    {
        std::stringstream str;
        str << "sdbus::setInterfaceFlags()" << annotationRegistration << ";";
        annotationRegistration = str.str();
    }

    std::vector<std::string> methodRegistrations;
    std::string methodDeclaration;
    std::tie(methodRegistrations, methodDeclaration) = processMethods(methods);

    std::vector<std::string> signalRegistrations;
    std::string signalMethods;
    std::tie(signalRegistrations, signalMethods) = processSignals(signals);

    std::vector<std::string> propertyRegistrations;
    std::string propertyAccessorDeclaration;
    std::tie(propertyRegistrations, propertyAccessorDeclaration) = processProperties(properties);

    auto vtableRegistration = createVTableRegistration(annotationRegistration, methodRegistrations, signalRegistrations, propertyRegistrations);

    body << tab << "void registerAdaptor()" << endl
         << tab << "{" << endl
                       << vtableRegistration << endl
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
            << tab << "sdbus::IObject& m_object;" << endl
            << "};" << endl << endl
            << std::string(namespacesCount, '}') << " // namespaces" << endl << endl;

    return body.str();
}


std::tuple<std::vector<std::string>, std::string> AdaptorGenerator::processMethods(const Nodes& methods) const
{
    std::ostringstream declarationSS;

    std::vector<std::string> methodRegistrations;

    for (const auto& method : methods)
    {
        std::ostringstream registrationSS;

        auto methodName = method->get("name");
        auto methodNameSafe = mangle_name(methodName);

        auto annotations = getAnnotations(*method);
        bool async{false};
        std::string annotationRegistration;
        for (const auto& annotation : annotations)
        {
            const auto& annotationName = annotation.first;
            const auto& annotationValue = annotation.second;

            if (annotationName == "org.freedesktop.DBus.Deprecated")
            {
                if (annotationValue == "true")
                    annotationRegistration += ".markAsDeprecated()";
            }
            else if (annotationName == "org.freedesktop.DBus.Method.NoReply")
            {
                if (annotationValue == "true")
                    annotationRegistration += ".withNoReply()";
            }
            else if (annotationName == "org.freedesktop.DBus.Method.Async")
            {
                if (annotationValue == "server" || annotationValue == "clientserver" || annotationValue == "client-server")
                    async = true;
            }
            else if (annotationName == "org.freedesktop.systemd1.Privileged")
            {
                if (annotationValue == "true")
                    annotationRegistration += ".markAsPrivileged()";
            }
            else if (annotationName != "org.freedesktop.DBus.Method.Timeout") // Whatever else...
            {
                std::cerr << "Node: " << methodName << ": "
                          << "Option '" << annotationName << "' not allowed or supported in this context! Option ignored..." << std::endl;
            }
        }

        Nodes args = (*method)["arg"];
        Nodes inArgs = args.select("direction" , "in");
        Nodes outArgs = args.select("direction" , "out");

        std::string argStr, argTypeStr, argStringsStr, outArgStringsStr;
        std::tie(argStr, argTypeStr, std::ignore, argStringsStr) = argsToNamesAndTypes(inArgs, async);
        std::tie(std::ignore, std::ignore, std::ignore, outArgStringsStr) = argsToNamesAndTypes(outArgs);

        using namespace std::string_literals;

        registrationSS << "sdbus::registerMethod(\""
                << methodName << "\")"
                << (!argStringsStr.empty() ? (".withInputParamNames(" + argStringsStr + ")") : "")
                << (!outArgStringsStr.empty() ? (".withOutputParamNames(" + outArgStringsStr + ")") : "")
                << ".implementedAs("
                << "[this]("
                << (async ? "sdbus::Result<" + outArgsToType(outArgs, true) + ">&& result" + (argTypeStr.empty() ? "" : ", ") : "")
                << argTypeStr
                << "){ " << (async ? "" : "return ") << "this->" << methodNameSafe << "("
                << (async ? "std::move(result)"s + (argTypeStr.empty() ? "" : ", ") : "")
                << argStr << "); })"
                << annotationRegistration;

        methodRegistrations.push_back(registrationSS.str());

        declarationSS << tab
                << "virtual "
                << (async ? "void" : outArgsToType(outArgs))
                << " " << methodNameSafe
                << "("
                << (async ? "sdbus::Result<" + outArgsToType(outArgs, true) + ">&& result" + (argTypeStr.empty() ? "" : ", ") : "")
                << argTypeStr
                << ") = 0;" << endl;
    }

    return std::make_tuple(methodRegistrations, declarationSS.str());
}


std::tuple<std::vector<std::string>, std::string> AdaptorGenerator::processSignals(const Nodes& signals) const
{
    std::ostringstream signalMethodSS;

    std::vector<std::string> signalRegistrations;

    for (const auto& signal : signals)
    {
        std::stringstream signalRegistrationSS;

        auto name = signal->get("name");

        auto annotations = getAnnotations(*signal);
        std::string annotationRegistration;
        for (const auto& annotation : annotations)
        {
            const auto& annotationName = annotation.first;
            const auto& annotationValue = annotation.second;

            if (annotationName == "org.freedesktop.DBus.Deprecated" && annotationValue == "true")
                annotationRegistration += ".markAsDeprecated()";
            else
                std::cerr << "Node: " << name << ": "
                          << "Option '" << annotationName << "' not allowed or supported in this context! Option ignored..." << std::endl;
        }

        Nodes args = (*signal)["arg"];

        std::string argStr, argTypeStr, typeStr, argStringsStr;
        std::tie(argStr, argTypeStr, typeStr, argStringsStr) = argsToNamesAndTypes(args);

        signalRegistrationSS << "sdbus::registerSignal(\"" << name << "\")";

        if (args.size() > 0)
        {
            signalRegistrationSS << ".withParameters<" << typeStr << ">(" << argStringsStr << ")";
        }

        signalRegistrationSS << annotationRegistration;

        signalRegistrations.push_back(signalRegistrationSS.str());

        auto nameWithCapFirstLetter = name;
        nameWithCapFirstLetter[0] = std::toupper(nameWithCapFirstLetter[0]);
        nameWithCapFirstLetter = mangle_name(nameWithCapFirstLetter);

        signalMethodSS << tab << "void emit" << nameWithCapFirstLetter << "(" << argTypeStr << ")" << endl
                << tab << "{" << endl
                << tab << tab << "m_object.emitSignal(\"" << name << "\")"
                        ".onInterface(INTERFACE_NAME)";

        if (!argStr.empty())
        {
            signalMethodSS << ".withArguments(" << argStr << ")";
        }

        signalMethodSS << ";" << endl
                << tab << "}" << endl << endl;
    }

    return std::make_tuple(signalRegistrations, signalMethodSS.str());
}


std::tuple<std::vector<std::string>, std::string> AdaptorGenerator::processProperties(const Nodes& properties) const
{
    std::ostringstream declarationSS;

    std::vector<std::string> propertyRegistrations;

    for (const auto& property : properties)
    {
        std::ostringstream registrationSS;

        auto propertyName = property->get("name");
        auto propertyNameSafe = mangle_name(propertyName);
        auto propertyAccess = property->get("access");
        auto propertySignature = property->get("type");

        auto propertyType = signature_to_type(propertySignature);
        auto propertyArg = std::string("value");
        auto propertyTypeArg = std::string("const ") + propertyType + "& " + propertyArg;

        auto annotations = getAnnotations(*property);
        std::string annotationRegistration;
        for (const auto& annotation : annotations)
        {
            const auto& annotationName = annotation.first;
            const auto& annotationValue = annotation.second;

            if (annotationName == "org.freedesktop.DBus.Deprecated" && annotationValue == "true")
                annotationRegistration += ".markAsDeprecated()";
            else if (annotationName == "org.freedesktop.DBus.Property.EmitsChangedSignal")
                annotationRegistration += ".withUpdateBehavior(" + propertyAnnotationToFlag(annotationValue) + ")";
            else if (annotationName == "org.freedesktop.systemd1.Privileged" && annotationValue == "true")
                annotationRegistration += ".markAsPrivileged()";
            else
                std::cerr << "Node: " << propertyName << ": "
                          << "Option '" << annotationName << "' not allowed or supported in this context! Option ignored..." << std::endl;
        }

        registrationSS << "sdbus::registerProperty(\""
                << propertyName << "\")";

        if (propertyAccess == "read" || propertyAccess == "readwrite")
        {
            registrationSS << ".withGetter([this](){ return this->" << propertyNameSafe << "(); })";
        }

        if (propertyAccess == "readwrite" || propertyAccess == "write")
        {
            registrationSS
                << ".withSetter([this](" << propertyTypeArg << ")"
                   "{ this->" << propertyNameSafe << "(" << propertyArg << "); })";
        }

        registrationSS << annotationRegistration;

        propertyRegistrations.push_back(registrationSS.str());

        if (propertyAccess == "read" || propertyAccess == "readwrite")
            declarationSS << tab << "virtual " << propertyType << " " << propertyNameSafe << "() = 0;" << endl;
        if (propertyAccess == "readwrite" || propertyAccess == "write")
            declarationSS << tab << "virtual void " << propertyNameSafe << "(" << propertyTypeArg << ") = 0;" << endl;
    }

    return std::make_tuple(propertyRegistrations, declarationSS.str());
}

std::string AdaptorGenerator::createVTableRegistration(const std::string& annotationRegistration,
                                                       const std::vector<std::string>& methodRegistrations,
                                                       const std::vector<std::string>& signalRegistrations,
                                                       const std::vector<std::string>& propertyRegistrations) const
{
    std::vector<std::string> allRegistrations;
    if (!annotationRegistration.empty())
        allRegistrations.push_back(annotationRegistration);
    allRegistrations.insert(allRegistrations.end(), methodRegistrations.begin(), methodRegistrations.end());
    allRegistrations.insert(allRegistrations.end(), signalRegistrations.begin(), signalRegistrations.end());
    allRegistrations.insert(allRegistrations.end(), propertyRegistrations.begin(), propertyRegistrations.end());

    if (allRegistrations.empty())
        return {};

    std::ostringstream registrationSS;
    if (allRegistrations.size() == 1)
    {
        registrationSS << tab << tab << "m_object.addVTable(" << allRegistrations[0] << ").forInterface(INTERFACE_NAME);";
    }
    else
    {
        registrationSS     << tab << tab << "m_object.addVTable( " << allRegistrations[0] << endl;
        for (size_t i = 1; i < allRegistrations.size(); ++i)
            registrationSS << tab << tab << "                  , " << allRegistrations[i] << endl;
        registrationSS     << tab << tab << "                  ).forInterface(INTERFACE_NAME);";
    }

    return registrationSS.str();
}

std::map<std::string, std::string> AdaptorGenerator::getAnnotations( sdbuscpp::xml::Node& node) const
{
    std::map<std::string, std::string> result;

    Nodes annotations = (node)["annotation"];
    for (const auto& annotation : annotations)
    {
        result[annotation->get("name")] = annotation->get("value");
    }

    return result;
}

std::string AdaptorGenerator::propertyAnnotationToFlag(const std::string& annotationValue) const
{
    return annotationValue == "true" ? "sdbus::Flags::EMITS_CHANGE_SIGNAL"
         : annotationValue == "invalidates" ? "sdbus::Flags::EMITS_INVALIDATION_SIGNAL"
         : annotationValue == "const" ? "sdbus::Flags::CONST_PROPERTY_VALUE"
         : annotationValue == "false" ? "sdbus::Flags::EMITS_NO_SIGNAL"
         : "EMITS_CHANGE_SIGNAL"; // Default
}
