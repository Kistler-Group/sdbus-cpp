/**
 * Inspired by: http://dbus-cplusplus.sourceforge.net/
 */

#ifndef __SDBUSCPP_TOOLS_XML_H
#define __SDBUSCPP_TOOLS_XML_H


#include <exception>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <iostream>
#include <sstream>

namespace sdbuscpp
{
namespace xml
{

class Error : public std::exception
{
public:

    Error(const char *error, int line, int column);

    ~Error() {}

    const char *what() const noexcept { return m_error.c_str(); }

private:
    std::string m_error;
};


class Node;

class Nodes : public std::vector<Node *>
{
public:
    Nodes operator[](const std::string &key) const;
    Nodes select(const std::string &attr, const std::string &value) const;
};

class Node
{
public:

        using Attributes = std::map<std::string, std::string>;
        using Children = std::vector<Node>;

        std::string name;
        std::string cdata;
        Children children;

        Node(std::string n, Attributes a) :
            name(n),
            m_attrs(a)
        {}

        Node(const char* n, const char** a = nullptr);

        Nodes operator[](const std::string& key);

        std::string get(const std::string& attribute) const;

        void set(const std::string &attribute, std::string value);

        std::string to_xml() const;

        Node& add(Node child)
        {
            children.push_back(child);
            return children.back();
        }

private:

        void _raw_xml(std::string& xml, int& depth) const;

        Attributes m_attrs;
};

class Document
{
public:
        struct Expat;

        Node* root;

        Document();
        ~Document();

        Document(const std::string& xml);

        void from_xml(const std::string& xml);

        std::string to_xml() const;

private:

        int m_depth;
};

} // namespace xml

} // namespace sdbuscpp

std::istream &operator >> (std::istream &, sdbuscpp::xml::Document &);
std::ostream &operator << (std::ostream &, sdbuscpp::xml::Document &);

#endif//__SDBUSCPP_TOOLS_XML_H
