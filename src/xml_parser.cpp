
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include <cassert>
#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <stack>
#include <string>
#include <vector>

#include <string.h>

#include <boost/shared_ptr.hpp>

#include "asserts.hpp"
#include "concurrent_cache.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "preprocessor.hpp"
#include "string_utils.hpp"
#include "unit_test.hpp"
#include "wml_parser.hpp"
#include "wml_utils.hpp"
#include "xml_parser.hpp"
#include "xml_writer.hpp"
#include "tinyxml.h"

namespace wml
{

namespace {
wml::node_ptr xml_to_wml(const TiXmlElement& el)
{
	wml::node_ptr res(new wml::node(el.ValueStr()));

	for(const TiXmlAttribute* attr = el.FirstAttribute(); attr != NULL; attr = attr->Next()) {
		res->set_attr(attr->Name(), attr->Value());
	}

	for(const TiXmlElement* child = el.FirstChildElement(); child != NULL; child = child->NextSiblingElement()) {
		res->add_child(xml_to_wml(*child));
	}

	return res;
}
}

node_ptr parse_xml(const std::string& str)
{
	TiXmlDocument doc;
	doc.Parse(str.c_str());

	const TiXmlElement* el = doc.RootElement();
	return xml_to_wml(*el);
}

}

UTILITY(wml_to_xml)
{
	foreach(const std::string& arg, args) {
		wml::const_node_ptr node = wml::parse_wml(sys::read_file(arg));
		sys::write_file(arg, wml::output_xml(node));
	}
}
