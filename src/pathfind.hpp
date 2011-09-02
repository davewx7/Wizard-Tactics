#ifndef PATHFIND_HPP_INCLUDED
#define PATHFIND_HPP_INCLUDED

#include <map>
#include <vector>

#include "tile_logic.hpp"

namespace hex
{

class path_cost_calculator {
public:
	virtual ~path_cost_calculator();
	virtual int movement_cost(const location& a, const location& b) const;
	virtual int estimated_cost(const location& a, const location& b) const;
	virtual bool allowed_to_move(const location& a) const;
	virtual bool legal_move_endpoint(const location& a) const;
};

int find_path(const location& src, const location& dst, const path_cost_calculator& calc, std::vector<location>* result, int max_cost=10000, bool adjacent_only=false, bool find_partial_result=false);

struct route {
	std::vector<hex::location> steps;
	int cost;
};

typedef std::map<hex::location, route> route_map;
void find_possible_moves(const hex::location& loc, int max_cost, const path_cost_calculator& calc, route_map& routes);

}

#endif
