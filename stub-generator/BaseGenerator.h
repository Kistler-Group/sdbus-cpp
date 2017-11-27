/**
 * (C) 2017 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 *
 * @file BaseGenerator.h
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

#ifndef __SDBUSCPP_TOOLS_BASE_GENERATOR_H
#define __SDBUSCPP_TOOLS_BASE_GENERATOR_H

// Own headers
#include "xml.h"

// STL
#include <string>
#include <tuple>

class BaseGenerator
{

public:
    int transformXmlToFile(const sdbuscpp::xml::Document& doc, const char* filename) const;

protected:
    enum class StubType
    {
        ADAPTOR,
        PROXY
    };

    constexpr static const char *tab = "    ";

    virtual ~BaseGenerator() {}

    /**
     * Implementation of public function that is provided by inherited class
     * @param doc
     * @param filename
     * @return
     */
    virtual int transformXmlToFileImpl(const sdbuscpp::xml::Document& doc, const char* filename) const = 0;


    /**
     * Write data to file
     * @param filename Written file
     * @param data Data to write
     * @return 0 if ok
     */
    int writeToFile(const char* filename, const std::string& data) const;

    /**
     * Crete header of file - include guard, includes
     * @param filename
     * @param stubType
     * @return
     */
    std::string createHeader(const char* filename, const StubType& stubType) const;

    /**
     * Namespaces according to the interface name
     * @param ifaceName
     * @return tuple: count of namespaces, string with code
     */
    std::tuple<unsigned, std::string> generateNamespaces(const std::string& ifaceName) const;

    /**
     * Transform arguments into source code
     * @param args
     * @return tuple: argument names, argument types and names, argument types
     */
    std::tuple<std::string, std::string, std::string> argsToNamesAndTypes(const sdbuscpp::xml::Nodes& args) const;

    /**
     * Output arguments to return type
     * @param args
     * @return return type
     */
    std::string outArgsToType(const sdbuscpp::xml::Nodes& args) const;

};



#endif //__SDBUSCPP_TOOLS_BASE_GENERATOR_H
