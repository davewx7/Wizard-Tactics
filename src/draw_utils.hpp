#ifndef DRAW_UTILS_HPP_INCLUDED
#define DRAW_UTILS_HPP_INCLUDED

#include <vector>

#include "tile_logic.hpp"

void draw_arrow(const std::vector<hex::location>& path);
void draw_arrow(int xfrom, int yfrom, int xto, int yto);

#endif
