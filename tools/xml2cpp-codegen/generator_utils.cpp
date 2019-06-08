/**
 * Inspired by: http://dbus-cplusplus.sourceforge.net/
 */

#include <iostream>
#include <cstdlib>
#include <map>

#include "generator_utils.h"


std::string underscorize(const std::string& str)
{
    std::string res = str;
    for (unsigned int i = 0; i < res.length(); ++i)
    {
        if (!isalpha(res[i]) && !isdigit(res[i]))
        {
            res[i] = '_';
        }
    }
    return res;
}

std::string stub_name(const std::string& name)
{
    return "_" + underscorize(name) + "_stub";
}

const char *atomic_type_to_string(char t)
{
    static std::map<char, const char*> atos
    {
            { 'y', "uint8_t" },
            { 'b', "bool" },
            { 'n', "int16_t" },
            { 'q', "uint16_t" },
            { 'i', "int32_t" },
            { 'u', "uint32_t" },
            { 'x', "int64_t" },
            { 't', "uint64_t" },
            { 'd', "double" },
            { 's', "std::string" },
            { 'o', "sdbus::ObjectPath" },
            { 'g', "sdbus::Signature" },
            { 'v', "sdbus::Variant" },
            { 'h', "sdbus::UnixFd" },
            { '\0', "" }
    };

    if (atos.count(t))
    {
        return atos[t];
    }

    return nullptr;
}

static void _parse_signature(const std::string &signature, std::string &type, unsigned int &i, bool only_once = false)
{
    for (; i < signature.length(); ++i)
    {
        switch (signature[i])
        {
            case 'a':
            {
                switch (signature[++i])
                {
                    case '{':
                    {
                        type += "std::map<";
                        ++i;
                        _parse_signature(signature, type, i);
                        type += ">";

                        break;
                    }
                    case '(':
                    {
                        type += "std::vector<sdbus::Struct<";
                        ++i;
                        _parse_signature(signature, type, i);
                        type += ">>";

                        break;
                    }
                    default:
                    {
                        type += "std::vector<";
                        _parse_signature(signature, type, i, true);

                        type += ">";

                        break;
                    }
                }
                break;
            }
            case '(':
            {
                type += "sdbus::Struct<";
                ++i;

                _parse_signature(signature, type, i);

                type += ">";
                break;
            }
            case ')':
            case '}':
            {
                return;
            }
            default:
            {
                const char *atom = atomic_type_to_string(signature[i]);
                if (!atom)
                {
                    std::cerr << "Invalid signature: " << signature << std::endl;
                    exit(-1);
                }
                type += atom;

                break;
            }
        }

        if (only_once)
            return;

        if (i + 1 < signature.length() && signature[i + 1] != ')' && signature[i + 1] != '}')
        {
            type += ", ";
        }
    }
}

std::string signature_to_type(const std::string& signature)
{
    std::string type;
    unsigned int i = 0;
    _parse_signature(signature, type, i);
    return type;
}
