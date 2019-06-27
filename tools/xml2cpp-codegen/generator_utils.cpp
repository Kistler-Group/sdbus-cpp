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

const char *atomic_type_to_string(char t, bool incoming)
{
    switch (t)
    {
            case 'y': return "uint8_t";
            case 'b': return "bool";
            case 'n': return "int16_t";
            case 'q': return "uint16_t";
            case 'i': return "int32_t";
            case 'u': return "uint32_t";
            case 'x': return "int64_t";
            case 't': return "uint64_t";
            case 'd': return "double";
            case 's': return "std::string";
            case 'o': return "sdbus::ObjectPath";
            case 'g': return "sdbus::Signature";
            case 'v': return "sdbus::Variant";
            case 'h': return incoming ? "sdbus::UnixFdRef" : "sdbus::AnyUnixFd";
            case '\0': return "";
            default: return nullptr;
    };
}

static void _parse_signature(const std::string &signature, std::string &type, unsigned int &i, bool incoming, bool only_once = false)
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
                        _parse_signature(signature, type, i, incoming);
                        type += ">";

                        break;
                    }
                    case '(':
                    {
                        type += "std::vector<sdbus::Struct<";
                        ++i;
                        _parse_signature(signature, type, i, incoming);
                        type += ">>";

                        break;
                    }
                    default:
                    {
                        type += "std::vector<";
                        _parse_signature(signature, type, i, incoming, true);

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

                _parse_signature(signature, type, i, incoming);

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
                const char *atom = atomic_type_to_string(signature[i], incoming);
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

std::string signature_to_type(const std::string& signature, bool incoming)
{
    std::string type;
    unsigned int i = 0;
    _parse_signature(signature, type, i, incoming);
    return type;
}
