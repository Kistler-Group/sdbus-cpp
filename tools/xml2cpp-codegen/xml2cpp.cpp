/**
 * (C) 2016 - 2021 KISTLER INSTRUMENTE AG, Winterthur, Switzerland
 * (C) 2016 - 2026 Stanislav Angelovic <stanislav.angelovic@protonmail.com>
 *
 * @file xml2cpp.cpp
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
#include "xml.h"
#include "AdaptorGenerator.h"
#include "ProxyGenerator.h"

// STL
#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>

using std::endl;
using namespace sdbuscpp;

void usage(std::ostream& output, const char* programName)
{
    output << "Usage: " << programName << " [OPTION]... [FILE]" << endl <<
            "Creates C++ stubs for DBus API for adaptor and/or client" << endl <<
            endl <<
            "Available options:" << endl <<
            "      --proxy=FILE     Generate header file FILE with proxy class (client)" << endl <<
            "      --adaptor=FILE   Generate header file FILE with stub class (server)" << endl <<
            "  -h, --help           " << endl <<
            "      --verbose        Explain what is being done" << endl <<
            "  -v, --version        Prints out sdbus-c++ version used by the tool" << endl <<
            endl <<
            "The stub generator takes an XML file describing DBus interface and creates" << endl <<
            "C++ header files to be used by C++ code wanting to cumminicate through that" << endl <<
            "interface. Clients of the interface (those making the calls) need header" << endl <<
            "created with the --proxy option as this header forwards the calls via DBus" << endl <<
            "to provider of the service and the returns the result to the caller. Server" << endl <<
            "implementing the service should derive from interface classes in header" << endl <<
            "generated for --adaptor option and implement their methods." << endl <<
            endl <<
            "When FILE is not specified, standard input is read. Exit status is 0 when" << endl <<
            "no error was encountered and all requested headers were sucessfully generated." << endl <<
            "Otherwise 1 is returned." << endl;
}


int main(int argc, char **argv)
{
    const char* programName = argv[0];
    argv++;
    argc--;

    const char* proxy = nullptr;
    const char* adaptor = nullptr;
    const char* xmlFile = nullptr;
    bool verbose = false;

    while (argc > 0)
    {
        if (!strncmp(*argv, "--proxy=", 8))
        {
            if (proxy != nullptr)
            {
                std::cerr << "Multiple occurrencies of --proxy is not allowed" << endl;
                usage(std::cerr, programName);
                return 1;
            }
            proxy = *argv + 8;
        }
        else if (!strncmp(*argv, "--adaptor=", 10) || !strncmp(*argv, "--adapter=", 10))
        {
            if (adaptor != nullptr)
            {
                std::cerr << "Multiple occurrencies of --adaptor is not allowed" << endl;
                usage(std::cerr, programName);
                return 1;
            }
            adaptor = *argv + 10;
        }
        else if (!strcmp(*argv, "--help") || !strcmp(*argv, "-h"))
        {
            usage(std::cout, programName);
            return 0;
        }
        else if (!strcmp(*argv, "--version") || !strcmp(*argv, "-v"))
        {
            std::cout << "Version: " << SDBUS_XML2CPP_VERSION << std::endl;
            return 0;
        }
        else if (!strcmp(*argv, "--verbose"))
        {
            verbose = true;
        }
        else if (**argv == '-')
        {
            std::cerr << "Unknown option " << *argv << endl;
            usage(std::cerr, programName);
            return 1;
        }
        else
        {
            if (xmlFile != nullptr)
            {
                std::cerr << "More than one input file specified: " << *argv << endl;
                usage(std::cerr, programName);
                return 1;
            }
            xmlFile = *argv;
        }

        argc--;
        argv++;
    }

    if (!proxy && !adaptor)
    {
        std::cerr << "Either --proxy or --adapter need to be specified" << endl;
        usage(std::cerr, programName);
        return 1;
    }

    xml::Document doc;

    try
    {
        if (xmlFile != nullptr)
        {
            if (verbose)
            {
                std::cerr << "Reading DBus interface from " << xmlFile << endl;
            }

            std::ifstream input(xmlFile);

            if (input.fail())
            {
                std::cerr << "Unable to open file " << xmlFile << endl;
                return 1;
            }

            input >> doc;
        }
        else
        {
            if (verbose)
            {
                std::cerr << "Reading DBus interface from standard input" << endl;
            }

            std::cin >> doc;
        }
    }
    catch (const xml::Error& e)
    {
        std::cerr << "Parsing error: " << e.what() << endl;
        return 1;
    }

    if (!doc.root)
    {
        std::cerr << "Empty document" << endl;
        return 1;
    }

    if (proxy)
    {
        if (verbose)
        {
            std::cerr << "Generating proxy header " << proxy << endl;
        }
        ProxyGenerator pg;
        if(pg.transformXmlToFile(doc, proxy)) {
            std::cerr << "Failed to generate proxy header" << endl;
            return 1;
        }
    }

    if (adaptor)
    {
        if (verbose)
        {
            std::cerr << "Generating adaptor header " << adaptor << endl;
        }
        AdaptorGenerator ag;
        if(ag.transformXmlToFile(doc, adaptor)) {
            std::cerr << "Failed to generate adaptor header" << endl;
            return 1;
        }

    }

    return 0;
}
