/*
   Copyright (C) 2008 by David White <dave@whitevine.net>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2
   or at your option any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef FONT_HPP_INCLUDED
#define FONT_HPP_INCLUDED

#include <string>

#include "SDL.h"

#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_HARMATTAN && !TARGET_OS_IPHONE
#include "SDL_ttf.h"
#endif

#include "texture.hpp"

namespace font {

struct manager {
	manager();
	~manager();
};

struct error {
};

graphics::texture render_text(const std::string& text,
                              const SDL_Color& color, int size);

int char_width(int size);
int char_height(int size);

}

#endif
