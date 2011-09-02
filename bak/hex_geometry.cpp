#include <assert.h>
#include <iostream>

#include "hex_geometry.hpp"

void tile_corner_pixel_xy(const hex::location& loc, hex::DIRECTION dir, int* x, int* y)
{
	using namespace hex;
	hex::location adj;
	bool on_left;
	switch(dir) {
	case NORTH:
		adj = tile_in_direction(loc, NORTH_WEST);
		on_left = false;
		break;
	case NORTH_EAST:
		adj = tile_in_direction(loc, NORTH_EAST);
		on_left = true;
		break;
	case SOUTH_EAST:
		adj = loc;
		on_left = false;
		break;
	case SOUTH:
		adj = tile_in_direction(loc, SOUTH_EAST);
		on_left = true;
		break;
	case SOUTH_WEST:
		adj = tile_in_direction(loc, SOUTH_WEST);
		on_left = false;
		break;
	case NORTH_WEST:
		adj = loc;
		on_left = true;
		break;
	default:
		assert(false);
	}

	*x = tile_pixel_x(adj) + (on_left ? 0 : HexSize);
	*y = tile_pixel_y(adj) + HexHeight/2;
}

hex::location pixel_pos_to_loc(int pixel_x, int pixel_y)
{
	const int xpos = pixel_x/HexWidth;
	const int ypos = (pixel_y - ((xpos%2 == 1) ? HexHeight/2 : 0))/HexHeight;
	return hex::location(xpos, ypos);
}
