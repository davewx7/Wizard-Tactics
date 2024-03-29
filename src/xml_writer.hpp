#ifndef XML_WRITER_HPP_INCLUDED
#define XML_WRITER_HPP_INCLUDED

#include <string>

#include "wml_node_fwd.hpp"

namespace wml
{
void write_xml(const wml::const_node_ptr& node, std::string& res);
void write_xml(const wml::const_node_ptr& node, std::string& res,
           std::string& indent, wml::const_node_ptr base=wml::const_node_ptr());
std::string output_xml(const wml::const_node_ptr& node);
}

#endif
