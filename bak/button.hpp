
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef BUTTON_HPP_INCLUDED
#define BUTTON_HPP_INCLUDED

#include <boost/function.hpp>

#include "texture.hpp"
#include "widget.hpp"
#include "framed_gui_element.hpp"


namespace gui {

enum BUTTON_RESOLUTION { BUTTON_SIZE_NORMAL_RESOLUTION, BUTTON_SIZE_DOUBLE_RESOLUTION };
enum BUTTON_STYLE { BUTTON_STYLE_NORMAL, BUTTON_STYLE_DEFAULT };	//"default" means a visually fat-edged button - the one that gets pressed by hitting enter.  This is standard gui lingo, it's what the dialogue "defaults" to doing when you press return.

//a button widget. Forwards to a given function whenever it is clicked.
class button : public widget
{
public:
	button(widget_ptr label, boost::function<void ()> onclick, BUTTON_STYLE button_style = BUTTON_STYLE_NORMAL, BUTTON_RESOLUTION button_resolution = BUTTON_SIZE_NORMAL_RESOLUTION);

protected:
	void set_label(widget_ptr label);

private:
	bool in_button(int x, int y) const;
	void handle_draw() const;
	bool handle_event(const SDL_Event& event, bool claimed);

	BUTTON_RESOLUTION button_resolution_;
	BUTTON_STYLE button_style_;
	widget_ptr label_;
	boost::function<void ()> onclick_;
	bool down_;
	
	const_framed_gui_element_ptr normal_button_image_set_,depressed_button_image_set_,focus_button_image_set_,current_button_image_set_;
};

typedef boost::shared_ptr<button> button_ptr;

}

#endif
