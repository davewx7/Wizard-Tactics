#include <assert.h>
#include <iostream>

#include "hex_geometry.hpp"

hex::location pixel_pos_to_loc(int pixel_x, int pixel_y)
{
	const int ypos = pixel_y/HexHeight;
	const int xpos = (pixel_x - ((ypos%2 == 1) ? HexWidth/2 : 0))/HexWidth;
	return hex::location(xpos, ypos);
}
