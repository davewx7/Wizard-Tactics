#ifndef HEX_GEOMETRY_HPP_INCLUDED
#define HEX_GEOMETRY_HPP_INCLUDED

#include <GL/gl.h>

#include "tile_logic.hpp"

static const int HexHeight = 38;
static const int HexWidth = 60;
static const int HexSize = HexHeight;

inline int tile_pixel_x(const hex::location& loc)
{
	return loc.x()*HexWidth + ((loc.y()%2 == 1) ? HexWidth/2 : 0);
}

inline int tile_pixel_y(const hex::location& loc)
{
	return loc.y()*HexHeight;
}

hex::location pixel_pos_to_loc(int pixel_x, int pixel_y);

inline int tile_center_x(const hex::location& loc)
{
	return tile_pixel_x(loc) + HexWidth/2;
}

inline int tile_center_y(const hex::location& loc)
{
	return tile_pixel_y(loc) + HexHeight/2;
}


#endif
