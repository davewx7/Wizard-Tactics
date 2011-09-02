#include <vector>

#include "pathfind.hpp"
#include "tile_logic.hpp"
#include "unit.hpp"

class game;

class unit_movement_cost_calculator : public hex::path_cost_calculator {
	const game* game_;
	const_unit_ptr unit_;
public:
	unit_movement_cost_calculator(const game* g, const_unit_ptr u);

	int movement_cost(const hex::location& a, const hex::location& b) const;
	bool allowed_to_move(const hex::location& a) const;
	bool legal_move_endpoint(const hex::location& a) const;
};

int unit_protection(const game& g, const_unit_ptr u);
