/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 *
 * @file BaseGenerator.cpp
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
#include "BaseGenerator.h"

// STL
#include <algorithm>
#include <fstream>
#include <iterator>
#include <iostream>
#include <map>
#include <sstream>


using std::endl;

using sdbuscpp::xml::Document;
using sdbuscpp::xml::Node;
using sdbuscpp::xml::Nodes;


int BaseGenerator::transformXmlToFile(const Document& doc, const char* filename) const
{
    return transformXmlToFileImpl(doc, filename);
}


int BaseGenerator::writeToFile(const char* filename, const std::string& data) const
{
    std::ofstream file(filename);
    if (file.bad())
    {
        std::cerr << "Unable to write file " << filename << endl;
        return 1;
    }

    file << data;
    file.close();
    return 0;
}

std::string BaseGenerator::createHeader(const char* filename, const StubType& stubType) const
{
    std::ostringstream head;
    head << getHeaderComment();

    std::string specialization = stubType == StubType::ADAPTOR ? "adaptor" : "proxy";

    std::string cond_comp{"__sdbuscpp__" + underscorize(filename)
        + "__" + specialization + "__H__"};

    head << "#ifndef " << cond_comp << endl
            << "#define " << cond_comp << endl << endl;

    head << "#include <sdbus-c++/sdbus-c++.h>" << endl
            << "#include <string>" << endl
            << "#include <tuple>" << endl
            << endl;

    return head.str();
}

std::tuple<unsigned, std::string> BaseGenerator::generateNamespaces(const std::string& ifaceName) const
{
    std::stringstream ss{ifaceName};
    std::ostringstream body;
    unsigned count{0};

    // prints the namespaces X and Y defined with <interface name="X.Y.Z">
    while (ss.str().find('.', ss.tellg()) != std::string::npos)
    {
        std::string nspace;
        getline(ss, nspace, '.');
        body << "namespace " << nspace << " {" << endl;
        ++count;
    }
    body << endl;

    return std::make_tuple(count, body.str());
}


std::tuple<std::string, std::string, std::string> BaseGenerator::argsToNamesAndTypes(const Nodes& args) const
{
    std::ostringstream argSS, argTypeSS, typeSS;

    for (size_t i = 0; i < args.size(); ++i)
    {
        auto arg = args.at(i);
        if (i > 0)
        {
            argSS << ", ";
            argTypeSS << ", ";
            typeSS << ", ";
        }

        auto argName = arg->get("name");
        if (argName.empty())
        {
            argName = "arg" + std::to_string(i);
        }
        auto type = signature_to_type(arg->get("type"));
        argSS << argName;
        argTypeSS << "const " << type << "& " << argName;
        typeSS << type;
    }

    return std::make_tuple(argSS.str(), argTypeSS.str(), typeSS.str());
}

/**
 *
 */
std::string BaseGenerator::outArgsToType(const Nodes& args, bool bareList) const
{
    std::ostringstream retTypeSS;

    if (args.size() == 0)
    {
        if (bareList)
            return "";

        retTypeSS << "void";
    }
    else if (args.size() == 1)
    {
        const auto& arg = *args.begin();
        retTypeSS << signature_to_type(arg->get("type"));
    }
    else if (args.size() >= 2)
    {
        if (!bareList)
            retTypeSS << "std::tuple<";

        bool firstArg = true;
        for (const auto& arg : args)
        {
            if (firstArg) firstArg = false; else retTypeSS << ", ";
            retTypeSS << signature_to_type(arg->get("type"));
        }

        if (!bareList)
            retTypeSS << ">";
    }

    return retTypeSS.str();
}

