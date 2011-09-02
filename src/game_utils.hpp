#ifndef GAME_UTILS_HPP_INCLUDED
#define GAME_UTILS_HPP_INCLUDED

#include <set>

class game;

int terrain_area(const game& g, const hex::location& loc, std::set<hex::location>* locs=0);

int tower_resource_production(const game& g, const hex::location& tower, int resource, std::set<hex::location>* locs);

#endif
