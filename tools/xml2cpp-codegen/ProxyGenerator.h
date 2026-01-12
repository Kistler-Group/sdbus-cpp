/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2026 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file ProxyGenerator.h
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

#ifndef __SDBUSCPP_TOOLS_PROXY_GENERATOR_H
#define __SDBUSCPP_TOOLS_PROXY_GENERATOR_H

// Own headers
#include "xml.h"
#include "BaseGenerator.h"

// STL
#include <tuple>

class ProxyGenerator : public BaseGenerator
{
protected:
    /**
     * Transform xml to proxy code
     * @param doc
     * @param filename
     * @return 0 if ok
     */
    int transformXmlToFileImpl(const sdbuscpp::xml::Document& doc, const char* filename) const override;

private:

    /**
     * Generate source code for interface
     * @param interface
     * @return source code
     */
    std::string processInterface(sdbuscpp::xml::Node& interface) const;

    /**
     * Generate method calls
     * @param methods
     * @return tuple: definition of methods, declaration of virtual async reply handlers
     */
    std::tuple<std::string, std::string> processMethods(const sdbuscpp::xml::Nodes& methods) const;

    /**
     * Generate code for handling signals
     * @param signals
     * @return tuple: registration, declaration of virtual methods
     */
    std::tuple<std::string, std::string> processSignals(const sdbuscpp::xml::Nodes& signals) const;

    /**
     * Generate calls for properties
     * @param properties
     * @return source code
     */
    std::tuple<std::string, std::string> processProperties(const sdbuscpp::xml::Nodes& properties) const;

};



#endif //__SDBUSCPP_TOOLS_PROXY_GENERATOR_H
