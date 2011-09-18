
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef XML_PARSER_HPP_INCLUDED
#define XML_PARSER_HPP_INCLUDED

#include <string>

#include "wml_node.hpp"

namespace wml
{

node_ptr parse_xml(const std::string& doc);

}

#endif
