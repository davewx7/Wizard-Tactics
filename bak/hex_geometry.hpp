#ifndef HEX_GEOMETRY_HPP_INCLUDED
#define HEX_GEOMETRY_HPP_INCLUDED

#include <GL/gl.h>

#include "tile_logic.hpp"

static const int HexHeight = 72;
static const int HexWidth = 54;
static const int HexSize = HexHeight;

static const GLfloat XRatio = 0.75;

inline GLfloat tile_translate_x(GLfloat x) {
	return x*XRatio;
}

inline GLfloat tile_translate_x(const hex::location& loc) {
	return tile_translate_x(static_cast<GLfloat>(loc.x()));
}

inline GLfloat tile_translate_y(GLfloat y) {
	return y + ((static_cast<int>(y)%2) == 0 ? 0.0 : 0.5);
}

inline GLfloat tile_translate_y(const hex::location& loc) {
	return static_cast<GLfloat>(loc.y()) +
	                ((loc.x()%2) == 0 ? 0.0 : 0.5);
}


inline int tile_pixel_x(const hex::location& loc)
{
	return loc.x()*HexWidth;
}

inline int tile_pixel_y(const hex::location& loc)
{
	return loc.y()*HexHeight + ((loc.x()%2 == 1) ? HexHeight/2 : 0);
}

void tile_corner_pixel_xy(const hex::location& loc, hex::DIRECTION dir, int* x, int* y);

hex::location pixel_pos_to_loc(int pixel_x, int pixel_y);

#endif
