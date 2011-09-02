
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef SURFACE_CACHE_HPP_INCLUDED
#define SURFACE_CACHE_HPP_INCLUDED

#include <string>

#include "surface.hpp"

namespace graphics
{

namespace surface_cache
{

surface get(const std::string& key);
surface get_no_cache(const std::string& key);
void clear_unused();
void clear();

}

}

#endif
