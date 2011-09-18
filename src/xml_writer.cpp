#include <iostream>
#include <set>

#include "foreach.hpp"
#include "string_utils.hpp"
#include "wml_node.hpp"
#include "xml_writer.hpp"

namespace wml
{

void write_xml(const wml::const_node_ptr& node, std::string& res)
{
	std::string indent;
	write_xml(node,res,indent);
}

void write_xml(const wml::const_node_ptr& node, std::string& res,
               std::string& indent, wml::const_node_ptr base)
{
	res += indent + "<";
	if(node->prefix().empty() == false) {
		res += node->prefix() + ":";
	}
	
	res += node->name() + " ";

	const std::vector<std::string>& attr_order = node->attr_order();
	foreach(const std::string& attr, attr_order) {
		if(base && base->attr(attr).str() == node->attr(attr).str()) {
			continue;
		}

		res += indent + attr + "=\"" + node->attr(attr).str() + "\"\n";
	}

	for(wml::node::const_attr_iterator i = node->begin_attr();
	    i != node->end_attr(); ++i) {
		if(std::find(attr_order.begin(), attr_order.end(), i->first) != attr_order.end()) {
			continue;
		}

		if(base && base->attr(i->first).str() == i->second.str()) {
			continue;
		}

		res += indent + i->first + "=\"" + i->second.str() + "\"\n";
	}

	res += ">\n";

	indent.push_back('\t');

	std::set<std::string> base_written;
	for(wml::node::const_all_child_iterator i = node->begin_children();
	    i != node->end_children(); ++i) {
		wml::const_node_ptr base_node = node->get_base_element((*i)->name());
		if(base_node && base_written.count((*i)->name()) == 0) {
			base_written.insert((*i)->name());
			if(base_node) {
				write_xml(base_node, res, indent);
			}
		}

		write_xml(*i, res, indent, base_node);
	}
	indent.resize(indent.size()-1);
	res += indent + "</" + node->name() + ">\n\n";
}

std::string output_xml(const wml::const_node_ptr& node)
{
	std::string res;
	write_xml(node, res);
	return res;
}

}
